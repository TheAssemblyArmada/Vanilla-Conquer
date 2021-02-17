// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
// software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.

// TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed
// in the hope that it will be useful, but with permitted additional restrictions
// under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT
// distributed with this program. You should have received a copy of the
// GNU General Public License along with permitted additional restrictions
// with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection
#include "crashpad.h"
#include "ccfile.h"
#include "gitinfo.h"
#include "ini.h"
#include "paths.h"
#include "utf.h"
#ifdef BUILD_WITH_CRASHPAD
#include <client/crash_report_database.h>
#include <client/settings.h>
#include <client/crashpad_client.h>
#endif

bool Init_Crash_Handler(const char* config_file)
{
    bool success = false;
#ifdef BUILD_WITH_CRASHPAD
    // Minichromiums api for paths is retarded and requires utf16 for windows, utf8 everywhere else.
#ifdef _WIN32
    std::wstring homedir = UTF8To16(Paths.User_Path());
    homedir += L"crashlogs";
    std::wstring handler_path = L"vc_crashhandler.exe";
#else
    std::string homedir = Paths.User_Path();
    homedir += "crashlogs";
    std::string handler_path = "vc_crashhandler";
#endif

    // Cache directory that will store crashpad information and minidumps
    base::FilePath database(homedir);
    // Path to the out-of-process handler executable
    base::FilePath handler(handler_path);
    // URL used to submit minidumps to
    std::string url = "";
    // Optional annotations passed via --annotations to the handler
    std::map<std::string, std::string> annotations = {{"commit", GitSHA1}};
    // Optional arguments to pass to the handler
    std::vector<std::string> arguments;
    // Optional attachments to upload with crash reports
    std::vector<base::FilePath> attachments;
    // Do we allow crash logs to be uploaded if we have a url to upload to?
    bool allow_upload = false;

    if (config_file != nullptr) {
        const char section[] = "CrashReporting";
        CCFileClass fc(config_file);
        INIClass ini;
        ini.Load(fc);

        allow_upload = ini.Get_Bool(section, "AllowUpload", allow_upload);
        url = ini.Get_String(section, "UploadURL", url);
    }

    std::unique_ptr<crashpad::CrashReportDatabase> db = crashpad::CrashReportDatabase::Initialize(database);

    if (db != nullptr && db->GetSettings() != nullptr) {
        db->GetSettings()->SetUploadsEnabled(allow_upload);
    }

    crashpad::CrashpadClient client;

    success = client.StartHandler(handler,
                                  database,
                                  database,
                                  url,
                                  annotations,
                                  arguments,
                                  /* restartable */ true,
                                  /* asynchronous_start */ false,
                                  attachments);
#endif
    return success;
}