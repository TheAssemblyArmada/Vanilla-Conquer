/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CNC_WEB_CONTENT_PREFLIGHT_H
#define CNC_WEB_CONTENT_PREFLIGHT_H

#include "protocol.h"

#include <stdint.h>

#include <string>
#include <vector>

namespace cnc {
namespace web {

enum ContentIssueKind
{
    CONTENT_ISSUE_INVALID_START = 1,
    CONTENT_ISSUE_INVALID_ROOT = 2,
    CONTENT_ISSUE_ROOT_NOT_FOUND = 3,
    CONTENT_ISSUE_ROOT_NOT_DIRECTORY = 4,
    CONTENT_ISSUE_REQUIRED_FILE_MISSING = 5,
    CONTENT_ISSUE_REQUIRED_FILE_INVALID = 6,
    CONTENT_ISSUE_OPTIONAL_FILE_MISSING = 7
};

struct ContentIssue
{
    ContentIssue();

    uint32_t kind;
    bool required;
    std::string name;
    std::string detail;
};

struct ContentPreflight
{
    ContentPreflight();

    cnc_web_status_t status;
    std::string scenario_root;
    std::vector<ContentIssue> issues;
};

/*
 * Checks the browser mount contract and reads only the bounded, loose
 * scenario INI needed to select the theater archives. MIX contents remain
 * opaque; the packager remains responsible for archive/hash validation.
 */
ContentPreflight ValidateContentMount(const StartConfig& config);

/* Returns false when a random legacy direction/variation prevents prediction. */
bool ExpectedScenarioRoot(const StartConfig& config, std::string& scenario_root);

/* Builds the case-preserving legacy command line used during one-time init. */
bool BuildLegacyStartupCommand(const std::string& content_directory, std::string& command_line);

} // namespace web
} // namespace cnc

#endif /* CNC_WEB_CONTENT_PREFLIGHT_H */
