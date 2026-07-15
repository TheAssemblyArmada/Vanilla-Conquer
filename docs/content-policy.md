# Game-content policy

The source repository and public CI are code-only. EA game content is neither
source code nor a project fixture. A free, noncommercial deployment may attach
a separately generated classic-freeware sidecar under EA's current franchise
modding guidelines.

## Never commit or transmit

- MEG, MIX, PGM, or other retail archives
- textures, sprites, fonts, logos, maps, XML/LOC data, audio, music, or movies
- extracted or transcoded retail files
- `.cncweb` packages created by the companion
- freeware ISO/CUE images, `INSTALL/SETUP.Z`, or `cnc-packages.zip`
- loose `SCG*`/`SCB*` mission INI/BIN payloads
- Steam or EA credentials, entitlement tokens, or depot downloads

Personal Remastered-derived packs are for the owning user's local devices and
must not be uploaded. The release-only freeware pack is different: it is built
from a hash-pinned, music-free mirror of EA's freeware data, verified by the
packer, and published only as a same-origin deployment sidecar. It must remain
free and noncommercial, must carry the required EA disclaimer, and must never
contain C&amp;C music or movies.

## Tests

Repository and public CI tests use small synthetic archives created by the test
suite. Tests against a real installation or freeware sidecar run only on a
user-controlled machine or release environment. Golden image or media output
derived from EA data must not be uploaded as a public CI artifact.

Diagnostics may contain engine versions, normalized logical identifiers,
checksums, sizes, timings, and error codes. Diagnostics must not include retail
bytes, extracted text, absolute user paths, account identifiers, or credentials.

## Browser storage

Before manual import, show the package size, available quota, the local-only
data promise, and the consequences of clearing site data. Hosted freeware
bootstrap data is same-origin, byte-count and SHA-256 pinned, and independently
checked against its package manifest before commit. Both paths write into a
staging directory, verify every manifest hash, and commit by atomically writing
the completed package marker. Cancellation or failure removes only staging
data. Saves remain exportable independently of content packs.
