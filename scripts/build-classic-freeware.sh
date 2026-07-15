#!/usr/bin/env bash
set -euo pipefail

readonly repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
readonly source_url=${CNCWEB_FREEWARE_SOURCE_URL:-https://openra.ppmsite.com/cnc-packages.zip}
readonly source_bytes=7911636
readonly source_sha256=a55b2c160b534f6d1b865ad6120e1f4fde8c418d47bb2fb1a9c72c586a5e1603
readonly package_name=classic-freeware-gdi-v1.cncweb
readonly descriptor_name=classic-freeware-v1.json
readonly output_dir=${1:-"$repo_root/.cache/classic-freeware"}
readonly download_dir="$repo_root/.cache/freeware-downloads"
readonly source_zip="$download_dir/cnc-packages-$source_sha256.zip"

if [[ $# -gt 1 || ${1:-} == --help || ${1:-} == -h ]]; then
    echo "Usage: $0 [OUTPUT_DIRECTORY]" >&2
    echo "Builds a deterministic, music-free GDI campaign sidecar; default output is .cache/classic-freeware." >&2
    exit $([[ $# -gt 1 ]] && echo 2 || echo 0)
fi

for command in cargo curl sha256sum stat unzip; do
    command -v "$command" >/dev/null || {
        echo "Missing required command: $command" >&2
        exit 1
    }
done

mkdir -p "$download_dir" "$output_dir"

if [[ ! -f "$source_zip" ]]; then
    curl --fail --location --retry 3 --output "$source_zip.partial" "$source_url"
    mv -- "$source_zip.partial" "$source_zip"
fi

actual_bytes=$(stat -c '%s' "$source_zip")
actual_sha256=$(sha256sum "$source_zip" | awk '{print $1}')
if [[ $actual_bytes != "$source_bytes" || $actual_sha256 != "$source_sha256" ]]; then
    echo "Freeware source failed its pinned size/SHA-256 check" >&2
    echo "Expected $source_bytes bytes and $source_sha256" >&2
    echo "Received $actual_bytes bytes and $actual_sha256" >&2
    exit 1
fi

work=$(mktemp -d "${TMPDIR:-/tmp}/theater-freeware.XXXXXXXX")
trap 'rm -rf -- "$work"' EXIT
unzip -q "$source_zip" -d "$work/source"
mkdir -p "$work/output"

cargo run --quiet --locked --manifest-path "$repo_root/tools/content-packer/Cargo.toml" -- \
    convert-mission "$work/source" "$work/output/$package_name" \
    --profile td-gdi-campaign \
    --package-id classic-freeware-gdi-v1 \
    --source-product tiberian-dawn-freeware \
    --provider ea-freeware \
    --created-at-unix-ms 0 \
    --quiet

cargo run --quiet --locked --manifest-path "$repo_root/tools/content-packer/Cargo.toml" -- \
    emit-freeware-bootstrap "$work/output/$package_name" "$work/output/$descriptor_name" \
    --archive-url "./$package_name"

mv -f -- "$work/output/$package_name" "$output_dir/$package_name"
mv -f -- "$work/output/$descriptor_name" "$output_dir/$descriptor_name"

echo "Classic freeware sidecar ready: $output_dir"
