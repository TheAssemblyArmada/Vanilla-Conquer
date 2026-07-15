import { expect, type Locator, type Page, type TestInfo } from "@playwright/test";

const CENTRAL_CROP_WIDTH = 0.8;
const CENTRAL_CROP_HEIGHT = 0.4;
const NON_DARK_CHANNEL = 32;
const MINIMUM_NON_DARK_FRACTION = 0.08;
const MINIMUM_QUANTIZED_COLORS = 16;

export interface CompositedPixelReport {
  imageWidth: number;
  imageHeight: number;
  cropX: number;
  cropY: number;
  cropWidth: number;
  cropHeight: number;
  sampledPixels: number;
  nonDarkPixels: number;
  nonDarkFraction: number;
  quantizedNonDarkColors: number;
  meanLuminance: number;
}

/** Keeps first-run chrome from influencing image acceptance or pointer targets. */
export async function dismissBattlefieldGuide(page: Page): Promise<void> {
  const dismiss = page.getByRole("button", { name: "Dismiss battlefield controls guide", exact: true });
  if (await dismiss.isVisible().catch(() => false)) await dismiss.click();
}

/**
 * Validates browser-composited output instead of WebGL's transient drawing
 * buffer. The central crop avoids the focus outline and the worst of either
 * portrait or landscape letterboxing.
 */
export async function expectCompositedBattlefield(
  page: Page,
  battlefield: Locator,
  testInfo: TestInfo,
  imageAttachmentName: string,
): Promise<CompositedPixelReport> {
  await dismissBattlefieldGuide(page);
  const png = await battlefield.screenshot({
    // Element screenshots include overlapping siblings in the compositor. Hide
    // both first-run states for this capture so UI chrome cannot satisfy the
    // battlefield color/diversity floor.
    style: ".battlefield-guide, .battlefield-guide-launcher { visibility: hidden !important; }",
  });
  const report = await page.evaluate(async ({ encoded, cropWidthRatio, cropHeightRatio, nonDarkChannel }) => {
    const image = new Image();
    image.src = `data:image/png;base64,${encoded}`;
    await image.decode();

    const canvas = document.createElement("canvas");
    canvas.width = image.naturalWidth;
    canvas.height = image.naturalHeight;
    const context = canvas.getContext("2d", { willReadFrequently: true });
    if (!context) throw new Error("Could not decode the composited battlefield screenshot");
    context.drawImage(image, 0, 0);

    const cropWidth = Math.max(1, Math.floor(canvas.width * cropWidthRatio));
    const cropHeight = Math.max(1, Math.floor(canvas.height * cropHeightRatio));
    const cropX = Math.floor((canvas.width - cropWidth) / 2);
    const cropY = Math.floor((canvas.height - cropHeight) / 2);
    const pixels = context.getImageData(cropX, cropY, cropWidth, cropHeight).data;
    const quantizedColors = new Set<number>();
    let nonDarkPixels = 0;
    let luminanceTotal = 0;
    for (let offset = 0; offset < pixels.length; offset += 4) {
      const red = pixels[offset];
      const green = pixels[offset + 1];
      const blue = pixels[offset + 2];
      luminanceTotal += 0.2126 * red + 0.7152 * green + 0.0722 * blue;
      if (Math.max(red, green, blue) < nonDarkChannel) continue;
      nonDarkPixels += 1;
      quantizedColors.add(((red >> 4) << 8) | ((green >> 4) << 4) | (blue >> 4));
    }
    const sampledPixels = pixels.length / 4;
    return {
      imageWidth: canvas.width,
      imageHeight: canvas.height,
      cropX,
      cropY,
      cropWidth,
      cropHeight,
      sampledPixels,
      nonDarkPixels,
      nonDarkFraction: nonDarkPixels / sampledPixels,
      quantizedNonDarkColors: quantizedColors.size,
      meanLuminance: luminanceTotal / sampledPixels,
    };
  }, {
    encoded: png.toString("base64"),
    cropWidthRatio: CENTRAL_CROP_WIDTH,
    cropHeightRatio: CENTRAL_CROP_HEIGHT,
    nonDarkChannel: NON_DARK_CHANNEL,
  });

  await testInfo.attach(imageAttachmentName, { body: png, contentType: "image/png" });
  await testInfo.attach(imageAttachmentName.replace(/\.png$/i, "-pixels.json"), {
    body: JSON.stringify({
      thresholds: {
        centralCropWidth: CENTRAL_CROP_WIDTH,
        centralCropHeight: CENTRAL_CROP_HEIGHT,
        nonDarkChannel: NON_DARK_CHANNEL,
        minimumNonDarkFraction: MINIMUM_NON_DARK_FRACTION,
        minimumQuantizedColors: MINIMUM_QUANTIZED_COLORS,
      },
      report,
    }, null, 2),
    contentType: "application/json",
  });

  expect(
    report.nonDarkFraction,
    `Composited battlefield non-dark fraction ${report.nonDarkFraction.toFixed(4)} is below the release floor`,
  ).toBeGreaterThanOrEqual(MINIMUM_NON_DARK_FRACTION);
  expect(
    report.quantizedNonDarkColors,
    `Composited battlefield exposes only ${report.quantizedNonDarkColors} non-dark quantized colors`,
  ).toBeGreaterThanOrEqual(MINIMUM_QUANTIZED_COLORS);
  return report;
}
