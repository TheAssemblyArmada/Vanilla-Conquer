/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "content_preflight.h"

#include <sys/stat.h>

#include <stdio.h>

namespace cnc {
namespace web {

namespace {

const size_t kMaximumLegacyContentPath = 480u;
const uint64_t kMinimumMixFileSize = 6u;
const uint64_t kMaximumScenarioIniSize = 1024u * 1024u;

struct RequiredFile
{
    const char* name;
    bool required;
};

const RequiredFile kRuntimeFiles[] = {
    {"CCLOCAL.MIX", true},
    {"CONQUER.MIX", true},
    {"GENERAL.MIX", true},
    {"LOCAL.MIX", false},
    {"SCORES.MIX", false},
    {"SOUNDS.MIX", true},
    {"SPEECH.MIX", true},
    {"TRANSIT.MIX", true},
    {"UPDATA.MIX", false},
    {"UPDATE.MIX", false},
    {"UPDATEC.MIX", true},
    {"MOVIES.MIX", false},
};

struct TheaterFiles
{
    const char* theater;
    const char* archive;
    const char* icon_archive;
    const char* loose_palette;
};

/* Never derive paths from the INI value: only these fixed names may be opened. */
const TheaterFiles kTheaterFiles[] = {
    {"TEMPERATE", "TEMPERAT.MIX", "TEMPICNH.MIX", "TEMPERAT.PAL"},
    {"DESERT", "DESERT.MIX", "DESEICNH.MIX", "DESERT.PAL"},
    {"WINTER", "WINTER.MIX", "WINTICNH.MIX", "WINTER.PAL"},
};

bool IsIniWhitespace(char value)
{
    return value == ' ' || value == '\t' || value == '\r' || value == '\f' || value == '\v';
}

std::string TrimIniValue(const std::string& value)
{
    size_t first = 0u;
    while (first < value.size() && IsIniWhitespace(value[first])) {
        ++first;
    }
    size_t last = value.size();
    while (last > first && IsIniWhitespace(value[last - 1u])) {
        --last;
    }
    return value.substr(first, last - first);
}

bool EqualsAsciiInsensitive(const std::string& left, const char* right)
{
    size_t index = 0u;
    while (index < left.size() && right[index] != '\0') {
        unsigned char left_character = static_cast<unsigned char>(left[index]);
        unsigned char right_character = static_cast<unsigned char>(right[index]);
        if (left_character >= 'a' && left_character <= 'z') {
            left_character = static_cast<unsigned char>(left_character - 'a' + 'A');
        }
        if (right_character >= 'a' && right_character <= 'z') {
            right_character = static_cast<unsigned char>(right_character - 'a' + 'A');
        }
        if (left_character != right_character) {
            return false;
        }
        ++index;
    }
    return index == left.size() && right[index] == '\0';
}

const TheaterFiles* FindTheaterFiles(const std::string& value)
{
    for (size_t index = 0u; index < sizeof(kTheaterFiles) / sizeof(kTheaterFiles[0]); ++index) {
        if (EqualsAsciiInsensitive(value, kTheaterFiles[index].theater)) {
            return &kTheaterFiles[index];
        }
    }
    return NULL;
}

const TheaterFiles* ParseScenarioTheater(const std::string& contents, std::string& detail)
{
    detail.clear();
    bool in_map_section = false;
    size_t theater_declarations = 0u;
    std::string theater_value;
    size_t cursor = 0u;
    while (cursor < contents.size()) {
        const size_t newline = contents.find('\n', cursor);
        const size_t line_end = newline == std::string::npos ? contents.size() : newline;
        std::string line = contents.substr(cursor, line_end - cursor);
        if (cursor == 0u && line.size() >= 3u && static_cast<unsigned char>(line[0]) == 0xefu
            && static_cast<unsigned char>(line[1]) == 0xbbu
            && static_cast<unsigned char>(line[2]) == 0xbfu) {
            line.erase(0u, 3u);
        }
        const size_t comment = line.find_first_of(";#");
        if (comment != std::string::npos) {
            line.erase(comment);
        }
        line = TrimIniValue(line);
        if (!line.empty() && line[0] == '[') {
            in_map_section = false;
            if (line.size() >= 2u && line[line.size() - 1u] == ']') {
                const std::string section = TrimIniValue(line.substr(1u, line.size() - 2u));
                in_map_section = EqualsAsciiInsensitive(section, "MAP");
            }
        } else if (in_map_section && !line.empty()) {
            const size_t separator = line.find('=');
            if (separator != std::string::npos
                && EqualsAsciiInsensitive(TrimIniValue(line.substr(0u, separator)), "THEATER")) {
                ++theater_declarations;
                if (theater_declarations == 1u) {
                    theater_value = TrimIniValue(line.substr(separator + 1u));
                }
            }
        }
        if (newline == std::string::npos) {
            break;
        }
        cursor = newline + 1u;
    }

    if (theater_declarations == 0u) {
        detail = "scenario INI [Map] section must declare Theater exactly once";
        return NULL;
    }
    if (theater_declarations != 1u) {
        detail = "scenario INI [Map] Theater must not be declared more than once";
        return NULL;
    }
    const TheaterFiles* files = FindTheaterFiles(theater_value);
    if (files == NULL) {
        detail = "scenario INI [Map] Theater must be TEMPERATE, DESERT, or WINTER";
    }
    return files;
}

bool ReadBoundedScenarioIni(const std::string& path, std::string& contents)
{
    contents.clear();
    FILE* file = fopen(path.c_str(), "rb");
    if (file == NULL) {
        return false;
    }
    struct stat file_status;
    const int descriptor = fileno(file);
    if (descriptor < 0 || fstat(descriptor, &file_status) != 0 || !S_ISREG(file_status.st_mode)
        || file_status.st_size <= 0 || static_cast<uint64_t>(file_status.st_size) > kMaximumScenarioIniSize) {
        fclose(file);
        return false;
    }

    const size_t size = static_cast<size_t>(file_status.st_size);
    contents.resize(size);
    const size_t read = fread(&contents[0], 1u, size, file);
    const int extra = read == size ? fgetc(file) : EOF;
    const bool valid = read == size && extra == EOF && ferror(file) == 0;
    const bool closed = fclose(file) == 0;
    if (!valid || !closed) {
        contents.clear();
        return false;
    }
    return true;
}

bool IsSafeRoot(const std::string& value)
{
    if (value.empty() || value.size() > kMaximumLegacyContentPath || value[0] != '/') {
        return false;
    }
    if (value.size() > 1u && value[value.size() - 1u] == '/') {
        return false;
    }

    size_t segment_start = 1u;
    for (size_t index = 1u; index <= value.size(); ++index) {
        const bool boundary = index == value.size() || value[index] == '/';
        if (!boundary) {
            const unsigned char character = static_cast<unsigned char>(value[index]);
            const bool ascii_alphanumeric = (character >= '0' && character <= '9')
                || (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z');
            if (!ascii_alphanumeric && character != '_' && character != '-' && character != '.') {
                return false;
            }
            continue;
        }

        const size_t length = index - segment_start;
        if (index != value.size() && length == 0u) {
            return false;
        }
        if ((length == 1u && value[segment_start] == '.')
            || (length == 2u && value[segment_start] == '.' && value[segment_start + 1u] == '.')) {
            return false;
        }
        segment_start = index + 1u;
    }
    return true;
}

bool IsSafeOverrideName(const std::string& value)
{
    if (value.empty()) {
        return true;
    }
    if (value.size() > 255u || value == "." || value == "..") {
        return false;
    }
    for (size_t index = 0u; index < value.size(); ++index) {
        const unsigned char character = static_cast<unsigned char>(value[index]);
        const bool ascii_alphanumeric = (character >= '0' && character <= '9')
            || (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z');
        if (!ascii_alphanumeric && character != '_' && character != '-') {
            return false;
        }
    }
    return true;
}

void AddIssue(ContentPreflight& result,
              uint32_t kind,
              bool required,
              const std::string& name,
              const std::string& detail)
{
    ContentIssue issue;
    issue.kind = kind;
    issue.required = required;
    issue.name = name;
    issue.detail = detail;
    result.issues.push_back(issue);
}

std::string JoinPath(const std::string& root, const char* name)
{
    return root == "/" ? root + name : root + "/" + name;
}

bool CanReadFile(const std::string& path)
{
    FILE* file = fopen(path.c_str(), "rb");
    if (file == NULL) {
        return false;
    }
    const bool readable = fgetc(file) != EOF;
    fclose(file);
    return readable;
}

void ValidateMixFile(ContentPreflight& result, const std::string& root, const char* name, bool required)
{
    const std::string path = JoinPath(root, name);
    struct stat file_status;
    if (stat(path.c_str(), &file_status) != 0) {
        AddIssue(result,
                 required ? CONTENT_ISSUE_REQUIRED_FILE_MISSING : CONTENT_ISSUE_OPTIONAL_FILE_MISSING,
                 required,
                 name,
                 path);
        if (required) {
            result.status = CNC_WEB_CONTENT_MISMATCH;
        }
        return;
    }
    if (!S_ISREG(file_status.st_mode) || static_cast<uint64_t>(file_status.st_size) < kMinimumMixFileSize
        || !CanReadFile(path)) {
        AddIssue(result,
                 CONTENT_ISSUE_REQUIRED_FILE_INVALID,
                 required,
                 name,
                 "runtime file is not a regular, non-empty MIX archive");
        if (required) {
            result.status = CNC_WEB_CONTENT_MISMATCH;
        }
    }
}

void ValidateOptionalPalette(ContentPreflight& result, const std::string& root, const char* name)
{
    const std::string path = JoinPath(root, name);
    struct stat file_status;
    if (stat(path.c_str(), &file_status) != 0) {
        AddIssue(result, CONTENT_ISSUE_OPTIONAL_FILE_MISSING, false, name, path);
    } else if (!S_ISREG(file_status.st_mode) || file_status.st_size != 768 || !CanReadFile(path)) {
        AddIssue(result,
                 CONTENT_ISSUE_REQUIRED_FILE_INVALID,
                 false,
                 name,
                 "optional loose theater palette must contain exactly 768 bytes");
    }
}

bool IsKnownStart(const StartConfig& config)
{
    const bool known_variation = (config.variation >= -1 && config.variation <= 3) || config.variation == 5;
    const bool known_scenario = config.scenario >= 0
        && (config.game_mode != CNC_WEB_GAME_CAMPAIGN || config.scenario > 0);
    return known_scenario && config.build_level >= 0 && config.direction >= -1 && config.direction <= 1
        && known_variation && IsSafeOverrideName(config.override_map_name);
}

} // namespace

ContentIssue::ContentIssue()
    : kind(0u)
    , required(false)
{
}

ContentPreflight::ContentPreflight()
    : status(CNC_WEB_OK)
{
}

bool ExpectedScenarioRoot(const StartConfig& config, std::string& scenario_root)
{
    scenario_root.clear();
    if (!config.override_map_name.empty()) {
        if (!IsSafeOverrideName(config.override_map_name)) {
            return false;
        }
        scenario_root = config.override_map_name;
        return true;
    }
    if (config.direction < 0 || config.variation < 0 || config.direction > 1
        || (config.variation > 3 && config.variation != 5)) {
        return false;
    }

    char player = 'M';
    if (config.game_mode == CNC_WEB_GAME_CAMPAIGN) {
        player = config.faction == CNC_WEB_FACTION_GDI
            ? 'G'
            : (config.faction == CNC_WEB_FACTION_NOD ? 'B' : 'J');
    }
    const char direction = config.direction == 0 ? 'E' : 'W';
    const char variation = config.variation == 5 ? 'L' : static_cast<char>('A' + config.variation);
    char value[32];
    const int written = snprintf(value, sizeof(value), "SC%c%02d%c%c", player, config.scenario, direction, variation);
    if (written <= 0 || static_cast<size_t>(written) >= sizeof(value)) {
        return false;
    }
    scenario_root.assign(value, static_cast<size_t>(written));
    return true;
}

bool BuildLegacyStartupCommand(const std::string& content_directory, std::string& command_line)
{
    command_line.clear();
    if (!IsSafeRoot(content_directory)) {
        return false;
    }
    command_line = "-CD" + content_directory + " -STEALTH";
    return command_line.size() < 1024u;
}

ContentPreflight ValidateContentMount(const StartConfig& config)
{
    ContentPreflight result;
    ExpectedScenarioRoot(config, result.scenario_root);

    if (!IsKnownStart(config)) {
        result.status = CNC_WEB_INVALID_ARGUMENT;
        AddIssue(result,
                 CONTENT_ISSUE_INVALID_START,
                 true,
                 "StartV1",
                 "scenario, direction, variation, build level, or override map name is invalid");
        return result;
    }
    if (!IsSafeRoot(config.content_directory)) {
        result.status = CNC_WEB_INVALID_ARGUMENT;
        AddIssue(result,
                 CONTENT_ISSUE_INVALID_ROOT,
                 true,
                 config.content_directory,
                 "content root must be a canonical absolute POSIX path without separators or traversal segments");
        return result;
    }

    struct stat root_status;
    if (stat(config.content_directory.c_str(), &root_status) != 0) {
        result.status = CNC_WEB_CONTENT_MISMATCH;
        AddIssue(result,
                 CONTENT_ISSUE_ROOT_NOT_FOUND,
                 true,
                 config.content_directory,
                 "mounted content root does not exist");
        return result;
    }
    if (!S_ISDIR(root_status.st_mode)) {
        result.status = CNC_WEB_CONTENT_MISMATCH;
        AddIssue(result,
                 CONTENT_ISSUE_ROOT_NOT_DIRECTORY,
                 true,
                 config.content_directory,
                 "mounted content root is not a directory");
        return result;
    }

    for (size_t index = 0u; index < sizeof(kRuntimeFiles) / sizeof(kRuntimeFiles[0]); ++index) {
        const RequiredFile& requirement = kRuntimeFiles[index];
        ValidateMixFile(result, config.content_directory, requirement.name, requirement.required);
    }

    /* Mission-scoped runtime packs expose the selected map as loose files. */
    if (!result.scenario_root.empty()) {
        const std::string ini_name = result.scenario_root + ".INI";
        const std::string ini_path = JoinPath(config.content_directory, ini_name.c_str());
        const TheaterFiles* theater = NULL;
        struct stat ini_status;
        if (stat(ini_path.c_str(), &ini_status) != 0) {
            AddIssue(result, CONTENT_ISSUE_REQUIRED_FILE_MISSING, true, ini_name, ini_path);
            result.status = CNC_WEB_CONTENT_MISMATCH;
        } else if (!S_ISREG(ini_status.st_mode) || ini_status.st_size <= 0
                   || static_cast<uint64_t>(ini_status.st_size) > kMaximumScenarioIniSize) {
            AddIssue(result,
                     CONTENT_ISSUE_REQUIRED_FILE_INVALID,
                     true,
                     ini_name,
                     "scenario INI must contain 1 byte to 1 MiB");
            result.status = CNC_WEB_CONTENT_MISMATCH;
        } else {
            std::string contents;
            if (!ReadBoundedScenarioIni(ini_path, contents)) {
                AddIssue(result,
                         CONTENT_ISSUE_REQUIRED_FILE_INVALID,
                         true,
                         ini_name,
                         "scenario INI could not be read safely within the 1 MiB limit");
                result.status = CNC_WEB_CONTENT_MISMATCH;
            } else {
                std::string theater_detail;
                theater = ParseScenarioTheater(contents, theater_detail);
                if (theater == NULL) {
                    AddIssue(result,
                             CONTENT_ISSUE_REQUIRED_FILE_INVALID,
                             true,
                             ini_name,
                             theater_detail);
                    result.status = CNC_WEB_CONTENT_MISMATCH;
                }
            }
        }

        const std::string bin_name = result.scenario_root + ".BIN";
        const std::string bin_path = JoinPath(config.content_directory, bin_name.c_str());
        struct stat bin_status;
        if (stat(bin_path.c_str(), &bin_status) != 0) {
            AddIssue(result, CONTENT_ISSUE_REQUIRED_FILE_MISSING, true, bin_name, bin_path);
            result.status = CNC_WEB_CONTENT_MISMATCH;
        } else if (!S_ISREG(bin_status.st_mode) || bin_status.st_size != 8 * 1024 || !CanReadFile(bin_path)) {
            AddIssue(result,
                     CONTENT_ISSUE_REQUIRED_FILE_INVALID,
                     true,
                     bin_name,
                     "classic scenario BIN must contain exactly 8192 bytes");
            result.status = CNC_WEB_CONTENT_MISMATCH;
        }

        if (theater != NULL) {
            ValidateMixFile(result, config.content_directory, theater->archive, true);
            ValidateMixFile(result, config.content_directory, theater->icon_archive, true);
            ValidateOptionalPalette(result, config.content_directory, theater->loose_palette);
        }
    }
    return result;
}

} // namespace web
} // namespace cnc
