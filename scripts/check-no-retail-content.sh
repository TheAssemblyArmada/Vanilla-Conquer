#!/usr/bin/env bash
set -euo pipefail

readonly forbidden_pattern='\.(meg|mix|pgm|bk2|vqa|aud|wav|tga|dds|loc|iso|cue|cncweb|cncsave|cncreplay)$'
readonly scenario_pattern='(^|/)sc[gb][0-9]{2,3}[ew][a-dl]\.(ini|bin)$'

mapfile -d '' candidates < <(git ls-files --cached --others --exclude-standard -z)
violations=()

for path in "${candidates[@]}"; do
    normalized=${path,,}
    basename=${normalized##*/}
    if [[ $normalized =~ $forbidden_pattern \
        || $normalized =~ $scenario_pattern \
        || $basename == setup.z \
        || $basename == cnc-packages.zip ]]; then
        violations+=("$path")
    fi
done

if ((${#violations[@]} > 0)); then
    echo "EA game-data or user-generated content must not be committed:" >&2
    printf '  %s\n' "${violations[@]}" >&2
    echo "See docs/content-policy.md." >&2
    exit 1
fi

echo "No forbidden retail-content file types found."
