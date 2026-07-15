/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "content_preflight.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>

namespace {

cnc::web::StartConfig FirstGdiMission(const std::string& root)
{
    cnc::web::StartConfig config;
    config.seed = 1u;
    config.scenario = 1;
    config.variation = 0;
    config.direction = 0;
    config.build_level = 1;
    config.sabotaged_structure = -1;
    config.faction = CNC_WEB_FACTION_GDI;
    config.game_mode = CNC_WEB_GAME_CAMPAIGN;
    config.player_id = 42u;
    config.content_id_hash = 1u;
    config.content_directory = root;
    return config;
}

void WriteFixture(const std::string& root, const char* name, off_t size = 6)
{
    const std::string path = root + "/" + name;
    const int file = open(path.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0600);
    assert(file >= 0);
    assert(size > 0);
    const unsigned char first = 0u;
    assert(write(file, &first, sizeof(first)) == static_cast<ssize_t>(sizeof(first)));
    assert(ftruncate(file, size) == 0);
    assert(close(file) == 0);
}

void WriteTextFixture(const std::string& root, const char* name, const std::string& contents)
{
    const std::string path = root + "/" + name;
    const int file = open(path.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0600);
    assert(file >= 0);
    size_t written = 0u;
    while (written < contents.size()) {
        const ssize_t result = write(file, contents.data() + written, contents.size() - written);
        assert(result > 0);
        written += static_cast<size_t>(result);
    }
    assert(close(file) == 0);
}

bool HasIssue(const cnc::web::ContentPreflight& result, uint32_t kind, const char* name)
{
    for (std::vector<cnc::web::ContentIssue>::const_iterator issue = result.issues.begin();
         issue != result.issues.end();
         ++issue) {
        if (issue->kind == kind && issue->name == name) {
            return true;
        }
    }
    return false;
}

bool HasIssueDetail(const cnc::web::ContentPreflight& result,
                    uint32_t kind,
                    const char* name,
                    const char* detail)
{
    for (std::vector<cnc::web::ContentIssue>::const_iterator issue = result.issues.begin();
         issue != result.issues.end();
         ++issue) {
        if (issue->kind == kind && issue->name == name && issue->detail == detail) {
            return true;
        }
    }
    return false;
}

} // namespace

int main()
{
    std::string scenario;
    cnc::web::StartConfig config = FirstGdiMission("/content/revision");
    assert(cnc::web::ExpectedScenarioRoot(config, scenario));
    assert(scenario == "SCG01EA");
    config.faction = CNC_WEB_FACTION_NOD;
    config.direction = 1;
    config.variation = 5;
    assert(cnc::web::ExpectedScenarioRoot(config, scenario));
    assert(scenario == "SCB01WL");
    config = FirstGdiMission("/content/revision");
    config.game_mode = CNC_WEB_GAME_SKIRMISH;
    config.scenario = 7;
    config.variation = 1;
    assert(cnc::web::ExpectedScenarioRoot(config, scenario));
    assert(scenario == "SCM07EB");
    config = FirstGdiMission("/content/revision");
    config.override_map_name = "CUSTOM_MAP";
    assert(cnc::web::ExpectedScenarioRoot(config, scenario));
    assert(scenario == "CUSTOM_MAP");

    std::string command;
    assert(cnc::web::BuildLegacyStartupCommand("/content/revision", command));
    assert(command == "-CD/content/revision -STEALTH");
    assert(!cnc::web::BuildLegacyStartupCommand("/content/revision with space", command));
    assert(!cnc::web::BuildLegacyStartupCommand("relative/content", command));
    assert(!cnc::web::BuildLegacyStartupCommand("/content/../escape", command));
    assert(!cnc::web::BuildLegacyStartupCommand("/content;other", command));
    assert(!cnc::web::BuildLegacyStartupCommand("/content/$revision", command));

    config = FirstGdiMission("/content/revision");
    config.direction = -1;
    assert(!cnc::web::ExpectedScenarioRoot(config, scenario));
    config = FirstGdiMission("relative/content");
    cnc::web::ContentPreflight result = cnc::web::ValidateContentMount(config);
    assert(result.status == CNC_WEB_INVALID_ARGUMENT);
    assert(HasIssue(result, cnc::web::CONTENT_ISSUE_INVALID_ROOT, "relative/content"));

    char temporary[] = "/tmp/cnc-web-content-XXXXXX";
    char* root = mkdtemp(temporary);
    assert(root != NULL);
    config = FirstGdiMission(root);
    result = cnc::web::ValidateContentMount(config);
    assert(result.status == CNC_WEB_CONTENT_MISMATCH);
    assert(HasIssue(result, cnc::web::CONTENT_ISSUE_REQUIRED_FILE_MISSING, "CCLOCAL.MIX"));
    assert(HasIssue(result, cnc::web::CONTENT_ISSUE_REQUIRED_FILE_MISSING, "SCG01EA.INI"));
    assert(HasIssue(result, cnc::web::CONTENT_ISSUE_OPTIONAL_FILE_MISSING, "MOVIES.MIX"));

    const char* runtime_files[] = {"CCLOCAL.MIX",
                                   "CONQUER.MIX",
                                   "GENERAL.MIX",
                                   "SOUNDS.MIX",
                                   "SPEECH.MIX",
                                   "TRANSIT.MIX",
                                   "UPDATEC.MIX"};
    for (size_t index = 0u; index < sizeof(runtime_files) / sizeof(runtime_files[0]); ++index) {
        WriteFixture(root, runtime_files[index]);
    }
    WriteFixture(root, "SCG01EA.BIN", 8 * 1024);

    /* Theater and key names are ASCII case-insensitive; surrounding whitespace
     * and both conventional INI comment forms are ignored. */
    WriteTextFixture(root,
                     "SCG01EA.INI",
                     "; mission comment\r\n  [ mAp ] ; section comment\r\n  tHeAtEr = tEmPeRaTe ; value comment\r\n");
    result = cnc::web::ValidateContentMount(config);
    assert(result.status == CNC_WEB_CONTENT_MISMATCH);
    assert(HasIssue(result, cnc::web::CONTENT_ISSUE_REQUIRED_FILE_MISSING, "TEMPERAT.MIX"));
    assert(HasIssue(result, cnc::web::CONTENT_ISSUE_REQUIRED_FILE_MISSING, "TEMPICNH.MIX"));
    WriteFixture(root, "TEMPERAT.MIX");
    WriteFixture(root, "TEMPICNH.MIX");
    result = cnc::web::ValidateContentMount(config);
    assert(result.status == CNC_WEB_OK);
    assert(result.scenario_root == "SCG01EA");
    assert(HasIssue(result, cnc::web::CONTENT_ISSUE_OPTIONAL_FILE_MISSING, "MOVIES.MIX"));
    assert(HasIssue(result, cnc::web::CONTENT_ISSUE_OPTIONAL_FILE_MISSING, "LOCAL.MIX"));
    assert(HasIssue(result, cnc::web::CONTENT_ISSUE_OPTIONAL_FILE_MISSING, "SCORES.MIX"));
    assert(HasIssue(result, cnc::web::CONTENT_ISSUE_OPTIONAL_FILE_MISSING, "UPDATA.MIX"));
    assert(HasIssue(result, cnc::web::CONTENT_ISSUE_OPTIONAL_FILE_MISSING, "UPDATE.MIX"));
    assert(HasIssue(result, cnc::web::CONTENT_ISSUE_OPTIONAL_FILE_MISSING, "TEMPERAT.PAL"));

    /* Existing MIX checks remain strict for the theater-selected pair. */
    assert(truncate((std::string(root) + "/TEMPICNH.MIX").c_str(), 5) == 0);
    result = cnc::web::ValidateContentMount(config);
    assert(result.status == CNC_WEB_CONTENT_MISMATCH);
    assert(HasIssue(result, cnc::web::CONTENT_ISSUE_REQUIRED_FILE_INVALID, "TEMPICNH.MIX"));
    assert(truncate((std::string(root) + "/TEMPICNH.MIX").c_str(), 6) == 0);

    /* A desert declaration requires the desert pair even when the complete
     * temperate pair is present. */
    WriteTextFixture(root,
                     "SCG01EA.INI",
                     "# leading comment\n[MAP]\nTHEATER = desert # trailing comment\n");
    result = cnc::web::ValidateContentMount(config);
    assert(result.status == CNC_WEB_CONTENT_MISMATCH);
    assert(HasIssue(result, cnc::web::CONTENT_ISSUE_REQUIRED_FILE_MISSING, "DESERT.MIX"));
    assert(HasIssue(result, cnc::web::CONTENT_ISSUE_REQUIRED_FILE_MISSING, "DESEICNH.MIX"));
    WriteFixture(root, "DESERT.MIX");
    result = cnc::web::ValidateContentMount(config);
    assert(result.status == CNC_WEB_CONTENT_MISMATCH);
    assert(HasIssue(result, cnc::web::CONTENT_ISSUE_REQUIRED_FILE_MISSING, "DESEICNH.MIX"));
    WriteFixture(root, "DESEICNH.MIX");
    result = cnc::web::ValidateContentMount(config);
    assert(result.status == CNC_WEB_OK);
    assert(HasIssue(result, cnc::web::CONTENT_ISSUE_OPTIONAL_FILE_MISSING, "DESERT.PAL"));

    /* A UTF-8 BOM is tolerated, but a winter mission still requires only the
     * exact winter archive names. */
    WriteTextFixture(root,
                     "SCG01EA.INI",
                     std::string("\xef\xbb\xbf") + "[Map]\r\n Theater = WINTER\r\n");
    result = cnc::web::ValidateContentMount(config);
    assert(result.status == CNC_WEB_CONTENT_MISMATCH);
    assert(HasIssue(result, cnc::web::CONTENT_ISSUE_REQUIRED_FILE_MISSING, "WINTER.MIX"));
    assert(HasIssue(result, cnc::web::CONTENT_ISSUE_REQUIRED_FILE_MISSING, "WINTICNH.MIX"));
    WriteFixture(root, "WINTER.MIX");
    WriteFixture(root, "WINTICNH.MIX");
    result = cnc::web::ValidateContentMount(config);
    assert(result.status == CNC_WEB_OK);
    assert(HasIssue(result, cnc::web::CONTENT_ISSUE_OPTIONAL_FILE_MISSING, "WINTER.PAL"));

    WriteTextFixture(root, "SCG01EA.INI", "[Map]\nTheater=../../DESERT\n");
    result = cnc::web::ValidateContentMount(config);
    assert(result.status == CNC_WEB_CONTENT_MISMATCH);
    assert(HasIssueDetail(result,
                          cnc::web::CONTENT_ISSUE_REQUIRED_FILE_INVALID,
                          "SCG01EA.INI",
                          "scenario INI [Map] Theater must be TEMPERATE, DESERT, or WINTER"));

    WriteTextFixture(root, "SCG01EA.INI", "[Map]\nTheater=TEMPERATE\nTHEATER=DESERT\n");
    result = cnc::web::ValidateContentMount(config);
    assert(result.status == CNC_WEB_CONTENT_MISMATCH);
    assert(HasIssueDetail(result,
                          cnc::web::CONTENT_ISSUE_REQUIRED_FILE_INVALID,
                          "SCG01EA.INI",
                          "scenario INI [Map] Theater must not be declared more than once"));

    WriteTextFixture(root, "SCG01EA.INI", "[Map]\nWidth=64\n");
    result = cnc::web::ValidateContentMount(config);
    assert(result.status == CNC_WEB_CONTENT_MISMATCH);
    assert(HasIssueDetail(result,
                          cnc::web::CONTENT_ISSUE_REQUIRED_FILE_INVALID,
                          "SCG01EA.INI",
                          "scenario INI [Map] section must declare Theater exactly once"));

    WriteTextFixture(root, "SCG01EA.INI", "[Map\nTheater=TEMPERATE\n");
    result = cnc::web::ValidateContentMount(config);
    assert(result.status == CNC_WEB_CONTENT_MISMATCH);
    assert(HasIssueDetail(result,
                          cnc::web::CONTENT_ISSUE_REQUIRED_FILE_INVALID,
                          "SCG01EA.INI",
                          "scenario INI [Map] section must declare Theater exactly once"));

    assert(unlink((std::string(root) + "/SCG01EA.INI").c_str()) == 0);
    WriteFixture(root, "SCG01EA.INI", 1024 * 1024 + 1);
    result = cnc::web::ValidateContentMount(config);
    assert(result.status == CNC_WEB_CONTENT_MISMATCH);
    assert(HasIssueDetail(result,
                          cnc::web::CONTENT_ISSUE_REQUIRED_FILE_INVALID,
                          "SCG01EA.INI",
                          "scenario INI must contain 1 byte to 1 MiB"));
    assert(unlink((std::string(root) + "/SCG01EA.INI").c_str()) == 0);
    result = cnc::web::ValidateContentMount(config);
    assert(result.status == CNC_WEB_CONTENT_MISMATCH);
    assert(HasIssue(result, cnc::web::CONTENT_ISSUE_REQUIRED_FILE_MISSING, "SCG01EA.INI"));

    config.override_map_name = "../escape";
    result = cnc::web::ValidateContentMount(config);
    assert(result.status == CNC_WEB_INVALID_ARGUMENT);
    assert(HasIssue(result, cnc::web::CONTENT_ISSUE_INVALID_START, "StartV1"));

    for (size_t index = 0u; index < sizeof(runtime_files) / sizeof(runtime_files[0]); ++index) {
        assert(unlink((std::string(root) + "/" + runtime_files[index]).c_str()) == 0);
    }
    const char* theater_archives[] = {
        "TEMPERAT.MIX", "TEMPICNH.MIX", "DESERT.MIX", "DESEICNH.MIX", "WINTER.MIX", "WINTICNH.MIX"};
    for (size_t index = 0u; index < sizeof(theater_archives) / sizeof(theater_archives[0]); ++index) {
        assert(unlink((std::string(root) + "/" + theater_archives[index]).c_str()) == 0);
    }
    assert(unlink((std::string(root) + "/SCG01EA.BIN").c_str()) == 0);
    assert(rmdir(root) == 0);
    return 0;
}
