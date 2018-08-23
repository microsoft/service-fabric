// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "userenv.h"
#include "util/pal_hosting_util.h"
#include "util/pal_string_util.h"
#include <dirent.h>
#include <pwd.h>
#include <string>

using namespace std;
using namespace Pal;

USERENVAPI
BOOL
WINAPI
DeleteProfileW (
        IN LPCWSTR lpSidString,
        IN LPCWSTR lpProfilePath,
        IN LPCWSTR lpComputerName)
{
    DIR *dir;
    if(lpProfilePath)
    {
        string path = utf16to8(lpProfilePath);
        dir = opendir(path.c_str());
        if (dir) {
            closedir(dir);
            string cmd = string("rm -r ") + path + (" > /dev/null 2>&1");
            system(cmd.c_str());
        }
    }

    string sidString = utf16to8(lpSidString);
    auto last = sidString.rfind("-");
    int uid = atoi(sidString.substr(last + 1, sidString.length() - last - 1).c_str());
    string uname, udir;
    if (0 == GetPwUid(uid, uname, udir))
    {
        string cmd("/usr/sbin/userdel");
        cmd += string(" ") + uname;
        cmd += " > /dev/null 2>&1";
        system(cmd.c_str());

        if (!udir.empty() && (dir = opendir(udir.c_str())))
        {
            closedir(dir);
            string cmd = string("rm -r ") + udir + (" > /dev/null 2>&1");
            system(cmd.c_str());
        }
    }

    return TRUE;
}
