use std::fs::{self, File};
use std::io::{Cursor, Write};

use cncweb_content_packer::hash::hash_reader;
use cncweb_content_packer::install::{InstallProfile, ValidationOptions};
use cncweb_content_packer::manifest::{
    ContentDescriptorV1, ContentRoleV1, GameId, ManifestFileV1, ManifestV1, SourceProduct,
    SourceProvider, SourceV1,
};
use cncweb_content_packer::package::{CreateOptions, PackageLimits};
use cncweb_content_packer::{
    create_package, extract_package, inspect_package, validate_install, verify_package,
    AudioIndexV1, Error, RuntimeCatalogV1, Sha256Digest, BROWSER_PACKAGE_LIMITS_V1,
};
use tempfile::tempdir;
use zip::write::SimpleFileOptions;
use zip::{CompressionMethod, ZipWriter};

fn digest(byte: u8) -> Sha256Digest {
    Sha256Digest::from_bytes([byte; 32])
}

#[test]
fn published_runtime_catalog_matches_the_rust_contract() {
    let example = include_str!("../fixtures/runtime-catalog-v1.example.json");
    let catalog: RuntimeCatalogV1 = serde_json::from_str(example).unwrap();
    catalog.validate().unwrap();
    assert_eq!(
        catalog,
        RuntimeCatalogV1::td_gdi_01("Synthetic mission briefing.")
    );

    let schema: serde_json::Value =
        serde_json::from_str(include_str!("../schema/runtime-catalog-v1.schema.json")).unwrap();
    assert_eq!(schema["properties"]["format"]["const"], "cncweb-runtime");
    assert_eq!(schema["properties"]["engineRoot"]["const"], "engine/td");
    assert_eq!(schema["oneOf"][0]["properties"]["missions"]["minItems"], 1);
    assert_eq!(
        schema["oneOf"][0]["properties"]["missions"]["maxItems"],
        256
    );
    assert_eq!(
        schema["oneOf"][0]["properties"]["missions"]["uniqueItems"],
        true
    );
    assert!(schema["$defs"]["mission"]["required"]
        .as_array()
        .unwrap()
        .iter()
        .any(|field| field == "scenarioRoot"));
    assert!(schema["$defs"]["mission"]["required"]
        .as_array()
        .unwrap()
        .iter()
        .any(|field| field == "theater"));
    assert_eq!(schema["oneOf"][1]["properties"]["missions"]["maxItems"], 1);
}

#[test]
fn published_audio_index_matches_the_rust_contract() {
    let example = include_str!("../fixtures/audio-v1.example.json");
    let index: AudioIndexV1 = serde_json::from_str(example).unwrap();
    index.validate().unwrap();

    let schema: serde_json::Value =
        serde_json::from_str(include_str!("../schema/audio-v1.schema.json")).unwrap();
    assert_eq!(schema["properties"]["format"]["const"], "cncweb-audio");
    assert_eq!(schema["properties"]["encoding"]["const"], "wav-pcm");
}

#[test]
fn published_manifest_example_matches_the_rust_contract() {
    let example = include_str!("../fixtures/manifest-v1.example.json");
    let manifest: ManifestV1 = serde_json::from_str(example).unwrap();
    manifest.validate().unwrap();

    let schema: serde_json::Value =
        serde_json::from_str(include_str!("../schema/manifest-v1.schema.json")).unwrap();
    assert_eq!(schema["properties"]["format"]["const"], "cncweb-content");
    assert_eq!(schema["properties"]["version"]["const"], 1);
}

#[test]
fn browser_package_profile_fixture_matches_the_rust_defaults() {
    let fixture: serde_json::Value =
        serde_json::from_str(include_str!("../fixtures/browser-package-profile-v1.json")).unwrap();
    assert_eq!(fixture["profile"], "cncweb-browser-import");
    assert_eq!(fixture["version"], 1);

    let limits = &fixture["limits"];
    let profile = PackageLimits::browser_v1();
    assert_eq!(PackageLimits::default(), profile);
    assert_eq!(limits["maxEntries"], profile.max_entries);
    assert_eq!(limits["maxManifestBytes"], profile.max_manifest_bytes);
    assert_eq!(limits["maxFileBytes"], profile.max_file_bytes);
    assert_eq!(limits["maxTotalBytes"], profile.max_total_bytes);
    assert_eq!(limits["maxCompressionRatio"], profile.max_compression_ratio);
    assert_eq!(profile, BROWSER_PACKAGE_LIMITS_V1);
}

fn create_options() -> CreateOptions {
    CreateOptions {
        package_id: "synthetic-td-slice".into(),
        created_at_unix_ms: 1_700_000_000_000,
        source: SourceV1 {
            product: SourceProduct::CncRemasteredCollection,
            provider: SourceProvider::CopiedInstallation,
            install_fingerprint_sha256: digest(9),
        },
        content: ContentDescriptorV1 {
            games: vec![GameId::TiberianDawn],
            locales: vec!["en-US".into()],
        },
        compression_level: 6,
        limits: PackageLimits::default(),
    }
}

#[test]
fn creates_verifies_and_transactionally_extracts_synthetic_content() {
    let temp = tempdir().unwrap();
    let staging = temp.path().join("staging");
    fs::create_dir_all(staging.join("audio")).unwrap();
    fs::create_dir_all(staging.join("config")).unwrap();
    fs::write(staging.join("audio/cue.raw"), b"synthetic audio bytes").unwrap();
    fs::write(staging.join("config/runtime.json"), b"{\"tickRate\":15}\n").unwrap();

    let package = temp.path().join("slice.cncweb");
    let created = create_package(&staging, &package, create_options()).unwrap();
    assert_eq!(created.files.len(), 2);
    assert_eq!(
        inspect_package(&package, PackageLimits::default()).unwrap(),
        created
    );
    assert_eq!(
        verify_package(&package, PackageLimits::default()).unwrap(),
        created
    );

    let extracted = temp.path().join("extracted");
    let report = extract_package(&package, &extracted, PackageLimits::default()).unwrap();
    assert_eq!(report.files_extracted, 2);
    assert_eq!(
        fs::read(extracted.join("audio/cue.raw")).unwrap(),
        b"synthetic audio bytes"
    );
    assert!(extracted.join("manifest.json").is_file());
}

#[test]
fn rejects_zip_traversal_before_creating_output() {
    let temp = tempdir().unwrap();
    let package = temp.path().join("traversal.cncweb");
    let mut zip = ZipWriter::new(File::create(&package).unwrap());
    zip.start_file(
        "../escape",
        SimpleFileOptions::default().compression_method(CompressionMethod::Stored),
    )
    .unwrap();
    zip.write_all(b"nope").unwrap();
    zip.finish().unwrap();

    let output = temp.path().join("output");
    assert!(extract_package(&package, &output, PackageLimits::default()).is_err());
    assert!(!output.exists());
    assert!(!temp.path().join("escape").exists());
}

#[test]
fn rejects_case_variant_of_the_reserved_manifest_name() {
    let temp = tempdir().unwrap();
    let staging = temp.path().join("staging");
    fs::create_dir(&staging).unwrap();
    fs::write(staging.join("Manifest.JSON"), b"not a package manifest").unwrap();
    assert!(create_package(
        &staging,
        temp.path().join("invalid.cncweb"),
        create_options()
    )
    .is_err());
}

#[test]
fn browser_profile_rejects_a_file_one_byte_over_the_import_limit() {
    let temp = tempdir().unwrap();
    let staging = temp.path().join("staging");
    fs::create_dir(&staging).unwrap();
    File::create(staging.join("oversized.bin"))
        .unwrap()
        .set_len(BROWSER_PACKAGE_LIMITS_V1.max_file_bytes + 1)
        .unwrap();

    let output = temp.path().join("oversized.cncweb");
    let error = create_package(&staging, &output, create_options()).unwrap_err();
    assert!(
        matches!(&error, Error::PackageLimit(message) if message.contains("per-file limit")),
        "unexpected error: {error:?}"
    );
    assert!(!output.exists());
}

#[test]
fn browser_profile_rejects_total_content_one_byte_over_the_import_limit() {
    let temp = tempdir().unwrap();
    let staging = temp.path().join("staging");
    fs::create_dir(&staging).unwrap();
    let limits = PackageLimits::browser_v1();
    assert_eq!(limits.max_total_bytes % limits.max_file_bytes, 0);
    let full_files = limits.max_total_bytes / limits.max_file_bytes;
    for index in 0..full_files {
        File::create(staging.join(format!("chunk-{index:03}.bin")))
            .unwrap()
            .set_len(limits.max_file_bytes)
            .unwrap();
    }
    File::create(staging.join("one-byte-over.bin"))
        .unwrap()
        .set_len(1)
        .unwrap();

    let output = temp.path().join("oversized-total.cncweb");
    let error = create_package(&staging, &output, create_options()).unwrap_err();
    assert!(
        matches!(&error, Error::PackageLimit(message) if message.contains("staged content exceeds")),
        "unexpected error: {error:?}"
    );
    assert!(!output.exists());
}

#[test]
fn detects_content_tampering_and_removes_partial_extraction() {
    let temp = tempdir().unwrap();
    let package = temp.path().join("tampered.cncweb");
    let (expected_hash, _) = hash_reader(&mut Cursor::new(b"good")).unwrap();
    let manifest = ManifestV1::new(
        "tamper-test".into(),
        1,
        create_options().source,
        create_options().content,
        vec![ManifestFileV1 {
            path: "audio/test.raw".into(),
            size: 4,
            sha256: expected_hash,
            role: ContentRoleV1::Audio,
        }],
    );
    let mut zip = ZipWriter::new(File::create(&package).unwrap());
    let options = SimpleFileOptions::default().compression_method(CompressionMethod::Stored);
    zip.start_file("manifest.json", options).unwrap();
    zip.write_all(&serde_json::to_vec(&manifest).unwrap())
        .unwrap();
    zip.start_file("audio/test.raw", options).unwrap();
    zip.write_all(b"evil").unwrap();
    zip.finish().unwrap();

    inspect_package(&package, PackageLimits::default()).unwrap();
    assert!(verify_package(&package, PackageLimits::default()).is_err());
    let output = temp.path().join("tampered-output");
    assert!(extract_package(&package, &output, PackageLimits::default()).is_err());
    assert!(!output.exists());
}

#[test]
fn structurally_validates_a_runtime_generated_install_fixture() {
    let temp = tempdir().unwrap();
    let data = temp.path().join("DATA");
    fs::create_dir(&data).unwrap();
    for archive_name in InstallProfile::MapEditor.required_archives() {
        fs::write(data.join(archive_name), synthetic_meg()).unwrap();
    }

    let report = validate_install(
        temp.path(),
        InstallProfile::MapEditor,
        ValidationOptions {
            compute_hashes: true,
            ..ValidationOptions::default()
        },
    )
    .unwrap();
    assert!(report.valid, "{:?}", report.issues);
    assert_eq!(report.archives.len(), 5);
    assert!(report.install_fingerprint_sha256.is_some());

    fs::remove_file(data.join("CONFIG.MEG")).unwrap();
    let missing = validate_install(
        temp.path(),
        InstallProfile::MapEditor,
        ValidationOptions::default(),
    )
    .unwrap();
    assert!(!missing.valid);
}

fn synthetic_meg() -> Vec<u8> {
    const NAME: &str = "SYNTHETIC/PROBE.BIN";
    const PAYLOAD: &[u8] = b"fixture";
    let string_table_size = 2 + NAME.len();
    let header_size = 24 + string_table_size + 20;
    let mut bytes = Vec::new();
    bytes.extend(0xffff_ffff_u32.to_le_bytes());
    bytes.extend(0.99_f32.to_le_bytes());
    bytes.extend((header_size as u32).to_le_bytes());
    bytes.extend(1_u32.to_le_bytes());
    bytes.extend(1_u32.to_le_bytes());
    bytes.extend((string_table_size as u32).to_le_bytes());
    bytes.extend((NAME.len() as u16).to_le_bytes());
    bytes.extend(NAME.as_bytes());
    bytes.extend(0_u16.to_le_bytes());
    bytes.extend(0_u32.to_le_bytes());
    bytes.extend(0_i32.to_le_bytes());
    bytes.extend((PAYLOAD.len() as u32).to_le_bytes());
    bytes.extend((header_size as u32).to_le_bytes());
    bytes.extend(0_u16.to_le_bytes());
    bytes.extend(PAYLOAD);
    bytes
}
