// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "pal_hosting_util.h"
#include <grp.h>
#include <stdlib.h>
#include <unistd.h>
#include "Common/Common.h"

using namespace std;
using namespace Pal;

Common::StringLiteral const AccountTraceSource("PAL.AccountMgmt");

int Pal::AccountOperationWithRetry(const string &cmd, int op, int mode, string &pipeio)
{
    int status = 0, retries = ACCOUNT_OP_RETRIES, interval = ACCOUNT_OP_RETRIES_INTERVAL;
    Trace.WriteInfo(AccountTraceSource, "{0}", cmd);

    do
    {
        if (mode == ACCOUNT_OP_PIPEMODE_NONE)
        {
            status = system(cmd.c_str());
        }
        else if (mode == ACCOUNT_OP_PIPEMODE_IN)
        {
            FILE *pipe = popen(cmd.c_str(), "w");
            fprintf(pipe, "%s", pipeio.c_str());
            status = pclose(pipe);
        }

        // For add, Already-Existing (9) is success; For del, Doesnot-Exist (6) is success, we also ignore primary group issue.
        if ((op == ACCOUNT_OP_ADD) && (WEXITSTATUS(status) == 9)
            ||(op == ACCOUNT_OP_DEL) && (WEXITSTATUS(status) == 6)
            ||(op == ACCOUNT_OP_DEL) && ((WEXITSTATUS(status) == 3) || (WEXITSTATUS(status) == 7) || (WEXITSTATUS(status) == 8)))
        {
            status = 0;
        }

        if (status != 0)
        {
            Sleep(ACCOUNT_OP_RETRIES_INTERVAL);
        }
    } while (status != 0 && retries-- > 0);

    return status;
}

int Pal::AccountOperationWithRetry(const string &cmd, int op)
{
    string pipeio;
    return AccountOperationWithRetry(cmd, op, ACCOUNT_OP_PIPEMODE_NONE, pipeio);
}

int Pal::GetPwUid(uid_t uid, std::string& uname, std::string &udir)
{
    long buflen = GETGR_PW_R_SIZE_MAX;
    unique_ptr<char[]> buf(new char[buflen]);
    struct passwd pwbuf, *pwbufp;
    getpwuid_r(uid, &pwbuf, buf.get(), buflen, &pwbufp);
    if (pwbufp) 
    {
        uname = pwbufp->pw_name;
        udir = (pwbufp->pw_dir ? pwbufp->pw_dir : "");
        return 0;
    }
    return -1;
}

int Pal::GetPwUname(const std::string &uname, uid_t *puid, gid_t *pgid, std::string *gecos)
{
    long buflen = GETGR_PW_R_SIZE_MAX;
    unique_ptr<char[]> buf(new char[buflen]);
    struct passwd pwbuf, *pwbufp;
    getpwnam_r(uname.c_str(), &pwbuf, buf.get(), buflen, &pwbufp);
    if (pwbufp)
    {
        if (puid) *puid = pwbufp->pw_uid; 
        if (pgid) *pgid = pwbufp->pw_gid; 
        if (gecos) *gecos = pwbufp->pw_gecos;
        return 0;
    }
    return -1;
}

int Pal::GetGrName(const string &gname)
{
    long buflen = GETGR_PW_R_SIZE_MAX;
    unique_ptr<char[]> buf(new char[buflen]);
    struct group grbuf, *grbufp;
    getgrnam_r(gname.substr(0, ACCT_NAME_MAX).c_str(), &grbuf, buf.get(), buflen, &grbufp);
    return (grbufp ? 0 : -1);
}

