# Runtime catalog v1

Every mission-ready browser package contains `runtime/catalog-v1.json`. The
main-thread runtime reads it from the selected immutable OPFS revision before
launching a mission worker. It is generated metadata and contains no bundled
retail text or assets.

The authoritative synthetic example is
[`fixtures/runtime-catalog-v1.example.json`](../fixtures/runtime-catalog-v1.example.json)
and the machine-readable contract is
[`schema/runtime-catalog-v1.schema.json`](../schema/runtime-catalog-v1.schema.json).
Unknown fields are rejected.

## Catalog identity and ordering

The fixed catalog identity remains:

- `format`: `cncweb-runtime`
- `version`: `1`
- `engine`: `tiberian-dawn`
- `engineRoot`: `engine/td`

`missions` contains 1 through 256 entries. Array order is the producer's stable
campaign-selection order and is preserved by consumers. Mission IDs and
scenario roots must each be unique. The Rust validator additionally checks
those two uniqueness constraints because JSON Schema's `uniqueItems` can only
compare complete objects. The compact serialized catalog, including its final
newline, must not exceed the browser's 64 KiB metadata limit.

## Generalized mission descriptor

Every newly produced mission contains:

| Field | Contract |
| --- | --- |
| `id` | 1–128 lowercase-safe characters matching `[a-z0-9][a-z0-9._-]*` |
| `scenarioRoot` | `SCG` or `SCB`, a zero-padded 1–999 scenario, `E`/`W`, and `A`–`D`/`L` |
| `scenario` | Integer 1–999 |
| `variation` | `0`–`3` for A–D or `5` for the legacy lose variation |
| `direction` | `0` east or `1` west |
| `buildLevel` | Integer 0–255 |
| `sabotagedStructure` | `-1` for none or integer 0–255 |
| `faction` | `gdi` or `nod` |
| `title` | Non-empty UTF-8 display title, at most 128 bytes in runtime validation |
| `briefing` | Locally derived normalized UTF-8, 1–4096 bytes |
| `theater` | `temperate`, `desert`, or `winter` |

`scenarioRoot` must exactly match the faction, scenario, direction, and
variation launch fields. For example, GDI scenario 4, west, variation B is
`SCG04WB`; Nod scenario 13, east, variation C is `SCB13EC`.

The converter derives `briefing` and `theater` from the resolved mission INI.
Numbered briefing entries retain source order, `@` becomes a paragraph break,
and repeated whitespace is collapsed. UTF-8 BOMs and native `;`/`#` comments
are accepted. `[Map] Theater` must occur exactly once.

The runtime derives the two loose mission files and matching theater archives
from `scenarioRoot` and `theater`:

| Theater | Required engine archives |
| --- | --- |
| `temperate` | `TEMPERAT.MIX`, `TEMPICNH.MIX` |
| `desert` | `DESERT.MIX`, `DESEICNH.MIX` |
| `winter` | `WINTER.MIX`, `WINTICNH.MIX` |

The catalog deliberately has no redundant `requiredFiles` array. The
conversion report's required asset records and package manifest are the
concrete file declaration, while `scenarioRoot` plus `theater` is the runtime
preflight contract.

## Campaign profiles

The canonical profile order is derived from `CountryArray` in
`tiberiandawn/mapsel.cpp`, with duplicate roots removed without reordering.
`Set_Scenario_Name` supplies the `SCG`/`SCB`, direction, and variation naming
rules. Normal campaign loading sets `buildLevel` to `scenario`.

- `td-gdi-campaign`: 25 unique GDI descriptors, scenarios 1–15, sourced from
  CD1.
- `td-nod-campaign`: 25 unique Nod descriptors, scenarios 1–13, sourced from
  CD2.
- `td-gdi-01-east-a`: the compatibility profile, still exactly one GDI mission
  from CD1.

Every selected INI and BIN is resolved independently with patch archives
before base archives and extracted loose under `engine/td`.

## Narrow legacy compatibility

Schema v1 retains one narrow alternative for packages installed before
`scenarioRoot` and `theater` existed: the catalog must contain exactly the
canonical `gdi-01-east-a` object with all original launch fields and no new
fields. Any multi-mission or non-GDI1 catalog must use the generalized shape.
Current producers, including the compatibility conversion profile, always emit
`scenarioRoot` and `theater`.

Catalog v1 has no atlas or movie fields. Browser-native sound and speech use
the independent [`runtime/audio-v1.json` contract](audio-v1.md).
