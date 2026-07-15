//! Safe, local-only content packaging primitives for the browser port.
//!
//! This crate does not contain, download, or distribute retail game assets. It
//! validates a user-selected installation and packages files that another
//! local extraction/conversion stage has placed in a staging directory.

pub mod aud;
pub mod audio;
pub mod bootstrap;
pub mod conversion;
pub mod error;
pub mod hash;
pub mod install;
pub mod manifest;
pub mod meg;
pub mod mix;
pub mod package;
pub mod path;
pub mod resolution;

pub use aud::{decode_aud, AudLimits, DecodedAud};
pub use audio::{
    build_audio_index, AudioAssetV1, AudioBuildOptions, AudioBuildReport, AudioDecodeFailureV1,
    AudioDiagnosticsV1, AudioEventKind, AudioIndexV1, AUDIO_INDEX_PATH,
};
pub use bootstrap::{
    emit_classic_freeware_bootstrap, ClassicFreewareArchiveV1, ClassicFreewareBootstrapV1,
    ClassicFreewarePackageV1, ClassicFreewareSourceV1, CLASSIC_FREEWARE_BOOTSTRAP_FORMAT,
    CLASSIC_FREEWARE_BOOTSTRAP_VERSION,
};
pub use conversion::{
    convert_owned_content, inspect_conversion_source, inspect_conversion_source_for_product,
    ConversionMetadataV1, ConversionOptions, ConversionPhase, ConversionProfile,
    ConversionProgress, ConversionReport, ConvertedMissionSourceV1, MissionSourceProbe,
    MissionTheater, RuntimeCatalogV1, RuntimeMissionV1, SourceInspection, CONVERSION_REPORT_PATH,
    ENGINE_ROOT, MAX_BRIEFING_BYTES, MAX_RUNTIME_CATALOG_BYTES, MISSION_ID, RUNTIME_CATALOG_PATH,
    SCENARIO_ROOT,
};
pub use error::{Error, Result};
pub use hash::{hash_file, hash_reader, Sha256Digest};
pub use install::{
    validate_install, ArchiveCheck, InstallProfile, InstallReport, ValidationOptions,
};
pub use manifest::{
    ContentDescriptorV1, ContentRoleV1, GameId, ManifestFileV1, ManifestV1, SourceProduct,
    SourceProvider, SourceV1, MANIFEST_FORMAT, MANIFEST_VERSION,
};
pub use meg::{MegArchive, MegEntry, MegFormat, MegHeader, MegLimits};
pub use mix::{mix_name_hash, MixArchive, MixEntry, MixFormat, MixHeader, MixLimits};
pub use package::{
    create_package, extract_package, inspect_package, verify_package, CreateOptions, ExtractReport,
    PackageLimits, BROWSER_PACKAGE_LIMITS_V1,
};
pub use resolution::{
    resolve_meg_entry, resolve_mix_entry, MegLayer, MixLayer, ResolvedMegEntry, ResolvedMixEntry,
};
