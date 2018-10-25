// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma

#include <string>
#include <pwd.h>

#define ACCT_NAME_MAX 32
#define GROUP_SHADOW_USER_PREFIX "WF-Int-"
#define NETAPI_BUF_MAX 2048
#define GETGR_PW_R_SIZE_MAX 16384

namespace Pal {

const int ACCOUNT_OP_ADD              = 0x01;
const int ACCOUNT_OP_MOD              = 0x02;
const int ACCOUNT_OP_DEL              = 0x03;

const int ACCOUNT_OP_PIPEMODE_NONE    = 0x01;
const int ACCOUNT_OP_PIPEMODE_IN      = 0x02;

const int ACCOUNT_OP_RETRIES          = 5;
const int ACCOUNT_OP_RETRIES_INTERVAL = 100;

int AccountOperationWithRetry(const std::string &cmd, int op, int mode, std::string &pipeio);

int AccountOperationWithRetry(const std::string &cmd, int op);

int GetPwUid(uid_t uid, std::string& uname, std::string &udir);

int GetPwUname(const std::string &uname, uid_t *puid = 0, gid_t *pgid = 0, std::string *gecos = 0);

int GetGrName(const std::string &gname);

}
