use std::fs::{self, OpenOptions};
use std::path::PathBuf;
use std::process::ExitCode;
use std::time::{SystemTime, UNIX_EPOCH};

use clap::{Parser, Subcommand, ValueEnum};
use cncweb_content_packer::conversion::{
    convert_owned_content, inspect_conversion_source_for_product, ConversionOptions,
    ConversionProfile,
};
use cncweb_content_packer::error::{Error, Result};
use cncweb_content_packer::install::{InstallProfile, ValidationOptions};
use cncweb_content_packer::manifest::{
    ContentDescriptorV1, GameId, SourceProduct, SourceProvider, SourceV1,
};
use cncweb_content_packer::meg::{MegArchive, MegFormat};
use cncweb_content_packer::package::{CreateOptions, PackageLimits};
use cncweb_content_packer::{
    create_package, emit_classic_freeware_bootstrap, extract_package, inspect_package,
    validate_install, verify_package, Sha256Digest,
};

#[derive(Debug, Parser)]
#[command(name = "cncweb-content", version, about)]
struct Cli {
    #[command(subcommand)]
    command: Command,
}

#[derive(Debug, Subcommand)]
enum Command {
    /// Validate a user-selected Remastered Collection installation.
    ValidateInstall {
        root: PathBuf,
        #[arg(long, value_enum, default_value_t = CliProfile::Collection)]
        profile: CliProfile,
        /// Hash complete required archives and emit an installation fingerprint.
        #[arg(long)]
        hash: bool,
    },
    /// List safe logical entries in an EA MEG archive.
    MegList { archive: PathBuf },
    /// Extract one named MEG entry to an explicitly selected local path.
    MegExtract {
        archive: PathBuf,
        entry: String,
        output: PathBuf,
    },
    /// Diagnose whether an owned installation can build a mission-scoped browser pack.
    PlanMission {
        root: PathBuf,
        #[arg(long, value_enum, default_value_t = CliConversionProfile::TdGdi01EastA)]
        profile: CliConversionProfile,
        #[arg(long, value_enum, default_value_t = CliSourceProduct::CncRemasteredCollection)]
        source_product: CliSourceProduct,
    },
    /// Convert an owned install directly into a verified browser-v1 mission package.
    ConvertMission {
        root: PathBuf,
        output: PathBuf,
        #[arg(long, value_enum, default_value_t = CliConversionProfile::TdGdi01EastA)]
        profile: CliConversionProfile,
        /// Package identity; defaults to the selected conversion profile ID.
        #[arg(long)]
        package_id: Option<String>,
        #[arg(long, value_enum, default_value_t = CliProvider::Unknown)]
        provider: CliProvider,
        #[arg(long, value_enum, default_value_t = CliSourceProduct::CncRemasteredCollection)]
        source_product: CliSourceProduct,
        #[arg(long = "locale", default_value = "en-US")]
        locales: Vec<String>,
        /// Override creation time for reproducible builds.
        #[arg(long)]
        created_at_unix_ms: Option<u64>,
        #[arg(long, default_value_t = 6)]
        compression_level: i64,
        /// Emit newline-delimited JSON progress objects to stderr.
        #[arg(long)]
        json_progress: bool,
        /// Suppress progress output; the final report is still written to stdout.
        #[arg(long, conflicts_with = "json_progress")]
        quiet: bool,
    },
    /// Package a staging directory using the browser-v1 import limits.
    Pack {
        input: PathBuf,
        output: PathBuf,
        #[arg(long)]
        package_id: String,
        #[arg(long)]
        source_fingerprint: Sha256Digest,
        #[arg(long, value_enum, default_value_t = CliProvider::Unknown)]
        provider: CliProvider,
        #[arg(long, value_enum, default_value_t = CliSourceProduct::CncRemasteredCollection)]
        source_product: CliSourceProduct,
        #[arg(long = "game", value_enum, required = true)]
        games: Vec<CliGame>,
        #[arg(long = "locale", default_value = "en-US")]
        locales: Vec<String>,
        /// Override creation time for reproducible builds.
        #[arg(long)]
        created_at_unix_ms: Option<u64>,
        #[arg(long, default_value_t = 6)]
        compression_level: i64,
    },
    /// Inspect manifest and ZIP safety limits without hashing every content file.
    Inspect { package: PathBuf },
    /// Stream and SHA-256 verify every package file.
    Verify { package: PathBuf },
    /// Transactionally extract and verify a package into a new directory.
    Extract { package: PathBuf, output: PathBuf },
    /// Verify a music-free freeware package and emit its same-origin web descriptor.
    EmitFreewareBootstrap {
        package: PathBuf,
        output: PathBuf,
        /// Same-directory relative URL used by the deployed descriptor.
        #[arg(long)]
        archive_url: String,
    },
}

#[derive(Debug, Clone, Copy, ValueEnum)]
enum CliProfile {
    MapEditor,
    TiberianDawn,
    RedAlert,
    Collection,
}

#[derive(Debug, Clone, Copy, ValueEnum)]
enum CliConversionProfile {
    #[value(name = "td-gdi-01-east-a")]
    TdGdi01EastA,
    #[value(name = "td-gdi-campaign")]
    TdGdiCampaign,
    #[value(name = "td-nod-campaign")]
    TdNodCampaign,
}

impl From<CliConversionProfile> for ConversionProfile {
    fn from(value: CliConversionProfile) -> Self {
        match value {
            CliConversionProfile::TdGdi01EastA => Self::TdGdi01EastA,
            CliConversionProfile::TdGdiCampaign => Self::TdGdiCampaign,
            CliConversionProfile::TdNodCampaign => Self::TdNodCampaign,
        }
    }
}

impl From<CliProfile> for InstallProfile {
    fn from(value: CliProfile) -> Self {
        match value {
            CliProfile::MapEditor => Self::MapEditor,
            CliProfile::TiberianDawn => Self::TiberianDawn,
            CliProfile::RedAlert => Self::RedAlert,
            CliProfile::Collection => Self::Collection,
        }
    }
}

#[derive(Debug, Clone, Copy, ValueEnum)]
enum CliProvider {
    Steam,
    EaApp,
    CopiedInstallation,
    EaFreeware,
    Unknown,
}

impl From<CliProvider> for SourceProvider {
    fn from(value: CliProvider) -> Self {
        match value {
            CliProvider::Steam => Self::Steam,
            CliProvider::EaApp => Self::EaApp,
            CliProvider::CopiedInstallation => Self::CopiedInstallation,
            CliProvider::EaFreeware => Self::EaFreeware,
            CliProvider::Unknown => Self::Unknown,
        }
    }
}

#[derive(Debug, Clone, Copy, ValueEnum)]
enum CliSourceProduct {
    CncRemasteredCollection,
    TiberianDawnFreeware,
}

impl From<CliSourceProduct> for SourceProduct {
    fn from(value: CliSourceProduct) -> Self {
        match value {
            CliSourceProduct::CncRemasteredCollection => Self::CncRemasteredCollection,
            CliSourceProduct::TiberianDawnFreeware => Self::TiberianDawnFreeware,
        }
    }
}

#[derive(Debug, Clone, Copy, ValueEnum)]
enum CliGame {
    TiberianDawn,
    RedAlert,
}

impl From<CliGame> for GameId {
    fn from(value: CliGame) -> Self {
        match value {
            CliGame::TiberianDawn => Self::TiberianDawn,
            CliGame::RedAlert => Self::RedAlert,
        }
    }
}

fn main() -> ExitCode {
    match run(Cli::parse()) {
        Ok(()) => ExitCode::SUCCESS,
        Err(error) => {
            eprintln!("error: {error}");
            ExitCode::FAILURE
        }
    }
}

fn run(cli: Cli) -> Result<()> {
    match cli.command {
        Command::ValidateInstall {
            root,
            profile,
            hash,
        } => {
            let report = validate_install(
                root,
                profile.into(),
                ValidationOptions {
                    compute_hashes: hash,
                    ..ValidationOptions::default()
                },
            )?;
            print_json(&report)?;
            if !report.valid {
                return Err(Error::Install(
                    "one or more required archives are missing or invalid".into(),
                ));
            }
        }
        Command::MegList { archive } => {
            let archive = MegArchive::open(archive)?;
            let format = match &archive.header().format {
                MegFormat::Legacy { prefix } => serde_json::json!({
                    "kind": "legacy",
                    "prefix": prefix,
                }),
                MegFormat::Remastered {
                    magic,
                    version,
                    declared_header_size,
                } => serde_json::json!({
                    "kind": "remastered",
                    "magic": magic,
                    "version": version,
                    "declared_header_size": declared_header_size,
                }),
            };
            let entries: Vec<_> = archive
                .entries()
                .iter()
                .map(|entry| {
                    serde_json::json!({
                        "name": entry.name,
                        "size": entry.size,
                        "crc32": entry.crc32,
                        "flags": entry.flags,
                    })
                })
                .collect();
            print_json(&serde_json::json!({
                "format": format,
                "archive_bytes": archive.archive_len(),
                "entries": entries,
            }))?;
        }
        Command::MegExtract {
            archive,
            entry,
            output,
        } => {
            if output.exists() {
                return Err(Error::OutputExists(output));
            }
            if let Some(parent) = output.parent() {
                fs::create_dir_all(parent)?;
            }
            let mut archive = MegArchive::open(archive)?;
            let mut output_file = OpenOptions::new()
                .write(true)
                .create_new(true)
                .open(output)?;
            archive.copy_entry(&entry, &mut output_file)?;
            output_file.sync_all()?;
        }
        Command::PlanMission {
            root,
            profile,
            source_product,
        } => {
            let report = inspect_conversion_source_for_product(
                root,
                profile.into(),
                source_product.into(),
                Default::default(),
            )?;
            print_json(&report)?;
            if !report.valid {
                return Err(Error::Conversion(format!(
                    "installation does not satisfy mission profile `{}`",
                    ConversionProfile::from(profile).id()
                )));
            }
        }
        Command::ConvertMission {
            root,
            output,
            profile,
            package_id,
            provider,
            source_product,
            locales,
            created_at_unix_ms,
            compression_level,
            json_progress,
            quiet,
        } => {
            let conversion_profile: ConversionProfile = profile.into();
            let package_id = package_id.unwrap_or_else(|| conversion_profile.id().into());
            let mut options = ConversionOptions::for_profile(
                conversion_profile,
                package_id,
                created_at_unix_ms.unwrap_or(now_unix_ms()?),
                provider.into(),
                locales,
            );
            options.compression_level = compression_level;
            options.source_product = source_product.into();
            let report = convert_owned_content(root, output, options, |progress| {
                if quiet {
                    return;
                }
                if json_progress {
                    if let Ok(line) = serde_json::to_string(progress) {
                        eprintln!("{line}");
                    }
                } else {
                    eprintln!(
                        "[{:?}] {}/{} {}: {}",
                        progress.phase,
                        progress.current,
                        progress.total,
                        progress.item,
                        progress.message
                    );
                }
            })?;
            print_json(&report)?;
        }
        Command::Pack {
            input,
            output,
            package_id,
            source_fingerprint,
            provider,
            source_product,
            games,
            locales,
            created_at_unix_ms,
            compression_level,
        } => {
            if !(0..=9).contains(&compression_level) {
                return Err(Error::InvalidManifest(
                    "compression level must be between 0 and 9".into(),
                ));
            }
            let manifest = create_package(
                input,
                output,
                CreateOptions {
                    package_id,
                    created_at_unix_ms: created_at_unix_ms.unwrap_or(now_unix_ms()?),
                    source: SourceV1 {
                        product: source_product.into(),
                        provider: provider.into(),
                        install_fingerprint_sha256: source_fingerprint,
                    },
                    content: ContentDescriptorV1 {
                        games: games.into_iter().map(Into::into).collect(),
                        locales,
                    },
                    compression_level,
                    // `pack` always emits archives within the browser import
                    // profile; incompatible staging trees fail before hashing.
                    limits: PackageLimits::browser_v1(),
                },
            )?;
            print_json(&manifest)?;
        }
        Command::Inspect { package } => {
            let manifest = inspect_package(package, PackageLimits::browser_v1())?;
            print_json(&manifest)?;
        }
        Command::Verify { package } => {
            let manifest = verify_package(package, PackageLimits::browser_v1())?;
            print_json(&manifest)?;
        }
        Command::Extract { package, output } => {
            let report = extract_package(package, output, PackageLimits::browser_v1())?;
            print_json(&report)?;
        }
        Command::EmitFreewareBootstrap {
            package,
            output,
            archive_url,
        } => {
            let descriptor = emit_classic_freeware_bootstrap(
                package,
                output,
                archive_url,
                PackageLimits::browser_v1(),
            )?;
            print_json(&descriptor)?;
        }
    }
    Ok(())
}

fn print_json(value: &impl serde::Serialize) -> Result<()> {
    println!("{}", serde_json::to_string_pretty(value)?);
    Ok(())
}

fn now_unix_ms() -> Result<u64> {
    let milliseconds = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map_err(|error| Error::Install(format!("system clock is before UNIX epoch: {error}")))?
        .as_millis();
    u64::try_from(milliseconds)
        .map_err(|_| Error::Install("current timestamp does not fit in u64".into()))
}

#[cfg(test)]
mod tests {
    use super::*;

    fn plan_profile(arguments: &[&str]) -> ConversionProfile {
        let cli = Cli::try_parse_from(arguments).unwrap();
        let Command::PlanMission { profile, .. } = cli.command else {
            panic!("expected plan-mission command");
        };
        profile.into()
    }

    #[test]
    fn campaign_profiles_are_explicit_and_legacy_profile_remains_default() {
        assert_eq!(
            plan_profile(&["cncweb-content", "plan-mission", "."]),
            ConversionProfile::TdGdi01EastA
        );
        assert_eq!(
            plan_profile(&[
                "cncweb-content",
                "plan-mission",
                ".",
                "--profile",
                "td-gdi-campaign",
            ]),
            ConversionProfile::TdGdiCampaign
        );
        assert_eq!(
            plan_profile(&[
                "cncweb-content",
                "plan-mission",
                ".",
                "--profile",
                "td-nod-campaign",
            ]),
            ConversionProfile::TdNodCampaign
        );
        assert_eq!(
            plan_profile(&[
                "cncweb-content",
                "plan-mission",
                ".",
                "--profile",
                "td-gdi-01-east-a",
            ]),
            ConversionProfile::TdGdi01EastA
        );

        for (profile_name, expected_id) in [
            ("td-gdi-01-east-a", "td-gdi-01-east-a"),
            ("td-gdi-campaign", "td-gdi-campaign"),
            ("td-nod-campaign", "td-nod-campaign"),
        ] {
            let cli = Cli::try_parse_from([
                "cncweb-content",
                "convert-mission",
                ".",
                "output.cncweb",
                "--profile",
                profile_name,
            ])
            .unwrap();
            let Command::ConvertMission {
                profile,
                package_id,
                ..
            } = cli.command
            else {
                panic!("expected convert-mission command");
            };
            let profile: ConversionProfile = profile.into();
            assert_eq!(
                package_id.unwrap_or_else(|| profile.id().into()),
                expected_id
            );
        }
    }
}
