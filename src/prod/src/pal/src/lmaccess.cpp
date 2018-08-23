// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "lmaccess.h"
#include "lmerr.h"
#include "util/pal_hosting_util.h"
#include "util/pal_string_util.h"
#include "rpc.h"
#include "winperf.h"
#include <grp.h>
#include <string>
#include <unistd.h>
#include <vector>

using namespace std;
using namespace Pal;

NET_API_STATUS NET_API_FUNCTION
NetUserAdd (
    IN  LPCWSTR    ServiceName OPTIONAL,
    IN  DWORD      Level,
    IN  LPBYTE     Buffer,
    OUT LPDWORD    ParmError OPTIONAL
    )
{
    USER_INFO_4* pui = (USER_INFO_4*)Buffer;
    string uname = utf16to8(pui->usri4_name).substr(0, ACCT_NAME_MAX);
    string password = utf16to8(pui->usri4_password);
    string comment = utf16to8(pui->usri4_comment);

    if (0 == GetPwUname(uname.c_str()))
    {
        return NERR_UserExists;
    }

    string cmd("/usr/sbin/useradd");

    if(uname.compare("P_FSSUserffffffff") == 0 || uname.compare("S_FSSUserffffffff") == 0)
    {
        cmd += string(" -s /usr/bin/rssh");
    }
    else
    {
        cmd += string(" -s /usr/sbin/nologin");
    }

    cmd += string(" -r -N -M -c '") + comment + "' -G ServiceFabricAllowedUsers " + uname;
    cmd += " > /dev/null 2>&1";
    int status = AccountOperationWithRetry(cmd, ACCOUNT_OP_ADD);
    if(status == 0)
    {
        cmd = string("/usr/sbin/usermod -c ") + "'" + comment + "'" + " " + uname;
        cmd += " > /dev/null 2>&1";
        status = AccountOperationWithRetry(cmd, ACCOUNT_OP_ADD);
        if (status == 0)
        {
            string pipein = uname + ":" + password + "\n";
            status = AccountOperationWithRetry("chpasswd > /dev/null 2>&1", ACCOUNT_OP_MOD, ACCOUNT_OP_PIPEMODE_IN, pipein);
        }
        if (status == 0 && (uname.compare("P_FSSUserffffffff") == 0 || uname.compare("S_FSSUserffffffff") == 0))
        {
            const string rsshug = "rsshusers";
            if (0 == GetGrName(rsshug))
            {
                string cmd("/usr/sbin/usermod -a -G ");
                cmd += rsshug + " " + uname;
                cmd += " > /dev/null 2>&1";
                status = AccountOperationWithRetry(cmd, ACCOUNT_OP_MOD);
            }
        }
    }
    return status;
}

NET_API_STATUS NET_API_FUNCTION
NetUserGetInfo (
    IN  LPCWSTR    ServerName OPTIONAL,
    IN  LPCWSTR    UserName,
    IN  DWORD      Level,
    OUT LPBYTE     *Buffer
    )
{
    // we only need comment field
    char* buf = new char[NETAPI_BUF_MAX];
    LPUSER_INFO_1 uinfo = (LPUSER_INFO_1)buf;
    string unameA = utf16to8(UserName).substr(0, ACCT_NAME_MAX);
    string gecos;
    if(0 != GetPwUname(unameA, 0, 0, &gecos))
    {
        return NERR_UserNotFound;
    }

    wstring ucommentW = utf8to16(gecos.c_str());
    uinfo->usri1_comment = (LPWSTR)(uinfo + 1);
    memcpy(uinfo->usri1_comment, ucommentW.c_str(), (ucommentW.length() + 1) * sizeof(wchar_t));
    *Buffer = (LPBYTE)uinfo;
    return NERR_Success;
}

NET_API_STATUS NET_API_FUNCTION
NetUserSetInfo (
    IN  LPCWSTR   ServerName OPTIONAL,
    IN  LPCWSTR   UserName,
    IN  DWORD     Level,
    IN  LPBYTE    Buffer,
    OUT LPDWORD   ParmError OPTIONAL
    )
{
    string unameA = utf16to8(UserName).substr(0, ACCT_NAME_MAX);

    PUSER_INFO_1007 commentUserInfo = (PUSER_INFO_1007)Buffer;
    string commentA = utf16to8(commentUserInfo->usri1007_comment);

    string cmd("/usr/sbin/usermod");
    cmd += string(" -c ") + "'" + commentA + "'" + " " + unameA;
    cmd += " > /dev/null 2>&1";
    int status = AccountOperationWithRetry(cmd, ACCOUNT_OP_MOD);
    return status;
}

NET_API_STATUS NET_API_FUNCTION
NetUserDel (
    IN  LPCWSTR    ServerName OPTIONAL,
    IN  LPCWSTR    UserName
    )
{
    string uname = utf16to8(UserName).substr(0, ACCT_NAME_MAX);
    string cmd("/usr/sbin/userdel");
    cmd += string(" ") + uname;
    cmd += " > /dev/null 2>&1";
    int status = AccountOperationWithRetry(cmd, ACCOUNT_OP_DEL);
    return status;
}

NET_API_STATUS NET_API_FUNCTION
NetUserGetLocalGroups (
    IN  LPCWSTR   ServerName OPTIONAL,
    IN  LPCWSTR   UserName,
    IN  DWORD     Level,
    IN  DWORD     Flags,
    OUT LPBYTE    *Buffer,
    IN  DWORD     PrefMaxLen,
    OUT LPDWORD   EntriesRead,
    OUT LPDWORD   EntriesLeft
    )
{
    string unameA = utf16to8(UserName).substr(0, ACCT_NAME_MAX);
    int ngroups = 50;
    gid_t *groups = new gid_t[ngroups];
    gid_t gid;
    if (0 != GetPwUname(unameA, 0, &gid))
    {
        return ERROR_INVALID_PARAMETER;
    }
    getgrouplist(unameA.c_str(), gid, groups, &ngroups);

    char* buf = new char[NETAPI_BUF_MAX];
    LPLOCALGROUP_USERS_INFO_0 localGroupInfo = (LPLOCALGROUP_USERS_INFO_0)buf;
    LPWSTR strbuf = (LPWSTR)(localGroupInfo + ngroups);

    for(int i = 0; i < ngroups; i++)
    {
        struct group *gr = getgrgid(groups[i]);
        wstring s = utf8to16(gr->gr_name);
        int sz = sizeof(wchar_t) * (s.length() + 1);
        memcpy(strbuf, s.c_str(), sz);
        localGroupInfo[i].lgrui0_name = strbuf;
        strbuf += sz;
    }
    *Buffer = (LPBYTE)localGroupInfo;
    *EntriesRead = ngroups;
    *EntriesLeft = 0;
    delete groups;
    return NERR_Success;
}

NET_API_STATUS NET_API_FUNCTION
NetLocalGroupAdd (
    IN  LPCWSTR  ServerName OPTIONAL,
    IN  DWORD    Level,
    IN  LPBYTE   Buffer,
    OUT LPDWORD  ParmError OPTIONAL
    )
{
    LOCALGROUP_INFO_1 *groupInfo = (LOCALGROUP_INFO_1 *)Buffer;
    string gname = utf16to8(groupInfo->lgrpi1_name).substr(0, ACCT_NAME_MAX);
    string gcomment = utf16to8(groupInfo->lgrpi1_comment);

    if (0 == GetGrName(gname.substr(0, ACCT_NAME_MAX)))
    {
        return NERR_GroupExists;
    }

    string cmd("/usr/sbin/groupadd");
    cmd += string(" -r ") + gname;
    cmd += " > /dev/null 2>&1";
    int status = AccountOperationWithRetry(cmd, ACCOUNT_OP_ADD);
    if (status != 0) return status;

    const string metagroupName = "WF-Int-MetaGroup";
    if (0 != GetGrName(metagroupName))
    {
        string gacmd("/usr/sbin/groupadd");
        gacmd += string(" -r ") + metagroupName;
        gacmd += " > /dev/null 2>&1";
        status = AccountOperationWithRetry(gacmd, ACCOUNT_OP_ADD);
        if (status != 0) return status;
    }

    string uname = string(GROUP_SHADOW_USER_PREFIX) + gname.substr(0,ACCT_NAME_MAX - strlen(GROUP_SHADOW_USER_PREFIX));
    string cmd2("/usr/sbin/useradd");
    cmd2 += string(" -s /usr/sbin/nologin -r -c '") + gcomment + "' -g " + metagroupName + " " + uname;
    cmd2 += " > /dev/null 2>&1";
    status = AccountOperationWithRetry(cmd2, ACCOUNT_OP_ADD);

    return status;
}

NET_API_STATUS NET_API_FUNCTION
NetLocalGroupGetInfo (
    IN  LPCWSTR  ServerName OPTIONAL,
    IN  LPCWSTR  LocalGroupName,
    IN  DWORD    Level,
    OUT LPBYTE   *Buffer
    )
{
    string gnameA = utf16to8(LocalGroupName).substr(0, ACCT_NAME_MAX);
    if(0 != GetGrName(gnameA))
    {
        return NERR_GroupNotFound;
    }
    string unameA = string(GROUP_SHADOW_USER_PREFIX) + gnameA.substr(0, ACCT_NAME_MAX - strlen(GROUP_SHADOW_USER_PREFIX));
    string gecos;
    if(0 != GetPwUname(unameA, 0, 0, &gecos))
    {
        string cmd("/usr/sbin/groupdel");
        cmd += string(" ") + gnameA;
        cmd += " > /dev/null 2>&1";
        AccountOperationWithRetry(cmd, ACCOUNT_OP_DEL);
        return NERR_GroupNotFound;
    }
    wstring gpassW = utf8to16(gecos.c_str());

    char* buf = new char[NETAPI_BUF_MAX];
    PLOCALGROUP_INFO_1 ginfo = (PLOCALGROUP_INFO_1)buf;
    LPWSTR gname = (LPWSTR)(ginfo + 1);
    memcpy(gname, LocalGroupName, (wcslen(LocalGroupName) + 1)* sizeof(wchar_t));
    LPWSTR gcomment = gname + wcslen(gname) + 1;
    ginfo->lgrpi1_name = gname;
    ginfo->lgrpi1_comment = gcomment;

    memcpy(ginfo->lgrpi1_comment, gpassW.c_str(), (gpassW.length()+1)*sizeof(wchar_t));
    *Buffer = (LPBYTE)ginfo;
    return NERR_Success;
}

NET_API_STATUS NET_API_FUNCTION
NetLocalGroupSetInfo (
    IN  LPCWSTR  ServerName OPTIONAL,
    IN  LPCWSTR  LocalGroupName,
    IN  DWORD    Level,
    IN  LPBYTE   Buffer,
    OUT LPDWORD  ParmError OPTIONAL
    )
{
    string gnameA = utf16to8(LocalGroupName).substr(0, ACCT_NAME_MAX);
    if(0 != GetGrName(gnameA))
    {
        return NERR_GroupNotFound;
    }
    string unameA = string(GROUP_SHADOW_USER_PREFIX) + gnameA.substr(0, ACCT_NAME_MAX - strlen(GROUP_SHADOW_USER_PREFIX));

    PLOCALGROUP_INFO_1002 commentGroupInfo = (PLOCALGROUP_INFO_1002)Buffer;
    string commentA = utf16to8(commentGroupInfo->lgrpi1002_comment);

    string cmd("/usr/sbin/usermod");
    cmd += string(" -c ") + "'" + commentA + "'";
    cmd += string(" ") + unameA;
    cmd += " > /dev/null 2>&1";
    int status = AccountOperationWithRetry(cmd, ACCOUNT_OP_MOD);

    return status;
}

NET_API_STATUS NET_API_FUNCTION
NetLocalGroupDel (
    IN  LPCWSTR   ServerName OPTIONAL,
    IN  LPCWSTR   LocalGroupName
    )
{
    string gname = utf16to8(LocalGroupName).substr(0, ACCT_NAME_MAX);
    string cmd("/usr/sbin/groupdel");
    cmd += string(" ") + gname;
    cmd += " > /dev/null 2>&1";
    int status = AccountOperationWithRetry(cmd, ACCOUNT_OP_DEL);
    if (status != 0) return status;

    string cmd2("/usr/sbin/userdel");
    cmd2 += string(" ") + string(GROUP_SHADOW_USER_PREFIX) + gname.substr(0, ACCT_NAME_MAX - strlen(GROUP_SHADOW_USER_PREFIX));
    cmd2 += " > /dev/null 2>&1";
    status = AccountOperationWithRetry(cmd2, ACCOUNT_OP_DEL);

    return status;
}

NET_API_STATUS NET_API_FUNCTION
NetLocalGroupSetMembers (
    IN  LPCWSTR     servername OPTIONAL,
    IN  LPCWSTR     groupname,
    IN  DWORD      level,
    IN  LPBYTE     buf,
    IN  DWORD      totalentries
    )
{
    if (totalentries == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    vector<string> nvec(totalentries);
    PLOCALGROUP_MEMBERS_INFO_0 memberInfo = (PLOCALGROUP_MEMBERS_INFO_0)buf;
    for (int i = 0; i < totalentries; i++)
    {
        PSID sid = memberInfo[i].lgrmi0_sid;
        uid_t uid = ((PLSID)sid)->SubAuthority[SID_SUBAUTH_ARRAY_UID];
        string uname, udir;
        if (0 != GetPwUid(uid, uname, udir)) {
            return ERROR_INVALID_PARAMETER;
        }
        nvec[i] = uname;
    }

    string gname = utf16to8(groupname).substr(0, ACCT_NAME_MAX);
    string cmd;
    if (access("/usr/bin/members", F_OK) != -1)
    {
        cmd = string("/usr/bin/members ") + gname;
    }
    else
    {
        cmd = string("/usr/sbin/lid -ng ") + gname;
    }
    FILE *of = popen(cmd.c_str(), "r");
    string obuf;
    obuf.resize(32*100);
    fgets((char*)obuf.data(), obuf.length(), of);
    pclose(of);

    int status;
    vector<string> ovec;
    char* pch = strtok ((char*)obuf.c_str()," \n");
    while (pch != NULL)
    {
        ovec.push_back(string(pch));
        pch = strtok (NULL, " \n");
    }
    for(string omem : ovec)
    {
        if (find(nvec.begin(), nvec.end(), omem) == nvec.end())
        {
            string cmd("/usr/bin/gpasswd -d ");
            cmd += omem + " " + gname;
            //cmd += " > /dev/null 2>&1";
            status = AccountOperationWithRetry(cmd, ACCOUNT_OP_DEL);
            if (status != 0) return status;
        }
    }

    for(int i = 0; i < totalentries; i++)
    {
        string uname = nvec[i];
        if (find(ovec.begin(), ovec.end(), uname) == ovec.end())
        {
            string cmd("/usr/sbin/usermod -a -G ");
            cmd += gname + " " + uname;
            cmd += " > /dev/null 2>&1";
            status = AccountOperationWithRetry(cmd, ACCOUNT_OP_MOD);  // this is a MOD w.r.t. return value
            if (status != 0) return status;

            // special handling for P_FSSUserffffffff and S_FSSUserffffffff
            if((uname.compare("P_FSSUserffffffff") == 0 || uname.compare("S_FSSUserffffffff") == 0) && gname.compare("FSSGroupffffffff") == 0
              || (uname.compare("P_FSSUserffffffff") != 0 && uname.compare("S_FSSUserffffffff") != 0 && uname.compare("sfuser") != 0))
            {
                cmd = string("/usr/sbin/usermod -g ") + gname + " " + uname;
                cmd += " > /dev/null 2>&1";
                status = AccountOperationWithRetry(cmd, ACCOUNT_OP_MOD);
                if (status != 0) return status;
            }
        }
    }
    return NERR_Success;
}

NET_API_STATUS NET_API_FUNCTION
NetLocalGroupGetMembers (
    IN  LPCWSTR    ServerName OPTIONAL,
    IN  LPCWSTR    LocalGroupName,
    IN  DWORD      Level,
    OUT LPBYTE     *Buffer,
    IN  DWORD      PrefMaxLen,
    OUT LPDWORD    EntriesRead,
    OUT LPDWORD    EntriesLeft,
    IN OUT PDWORD_PTR ResumeHandle
    )
{
    string gnameA = utf16to8(LocalGroupName).substr(0, ACCT_NAME_MAX);
    long buflen = GETGR_PW_R_SIZE_MAX;
    unique_ptr<char[]> buf(new char[buflen]);
    struct group grbuf, *grbufp;
    getgrnam_r(gnameA.c_str(), &grbuf, buf.get(), buflen, &grbufp);
    if(!grbufp)
    {
        return NERR_GroupNotFound;
    }
    int len = 0;
    while(grbufp->gr_mem[len]) len++;
    *EntriesRead = len;
    *EntriesLeft = 0;
    *Buffer = 0;
    if(len)
    {
        char* buf = new char[NETAPI_BUF_MAX];
        PLOCALGROUP_MEMBERS_INFO_1 info = (PLOCALGROUP_MEMBERS_INFO_1)buf;
        LPWSTR strbuf = (LPWSTR)(info + len);

        int index = 0;
        char *uname = grbufp->gr_mem[index];
        while(uname)
        {
            wstring unameW = wstring(utf8to16(uname));
            int sz = sizeof(wchar_t) * (unameW.length() + 1);
            memcpy(strbuf, unameW.c_str(), sz);
            info[index].lgrmi1_name = strbuf;
            uname = grbufp->gr_mem[++index];
            strbuf += sz;
        };
        *Buffer = (LPBYTE)info;
    }
    return NERR_Success;
}

NET_API_STATUS NET_API_FUNCTION
NetLocalGroupAddMembers (
    IN  LPCWSTR    ServerName OPTIONAL,
    IN  LPCWSTR    LocalGroupName,
    IN  DWORD      Level,
    IN  LPBYTE     Buffer,
    IN  DWORD      NewMemberCount
    )
{
    int status = 0;
    string gname = utf16to8(LocalGroupName).substr(0, ACCT_NAME_MAX);
    if((0 != GetGrName(gname)) && (gname.compare("ServiceFabricAllowedUsers")==0 || gname.compare("ServiceFabricAdministrators")==0))
    {
        string gacmd("/usr/sbin/groupadd");
        gacmd += string(" -r ") + gname;
        gacmd += " > /dev/null 2>&1";
        status = AccountOperationWithRetry(gacmd, ACCOUNT_OP_ADD);
        if (status != 0) return status;
    }

    LOCALGROUP_MEMBERS_INFO_3 *groupMembership = (LOCALGROUP_MEMBERS_INFO_3*)Buffer;
    string uname = utf16to8(groupMembership->lgrmi3_domainandname).substr(0, ACCT_NAME_MAX);

    string cmd("/usr/sbin/usermod -a -G ");
    cmd += gname + " " + uname;
    cmd += " > /dev/null 2>&1";
    status = AccountOperationWithRetry(cmd, ACCOUNT_OP_MOD);  // this is a MOD w.r.t. return value
    if (status != 0) return status;

    // special handling for P_FSSUserffffffff and S_FSSUserffffffff
    if((uname.compare("P_FSSUserffffffff")==0 || uname.compare("S_FSSUserffffffff")==0) && gname.compare("FSSGroupffffffff")==0)
    {
        cmd = string("/usr/sbin/usermod -g ") + gname + " " + uname;
        cmd += " > /dev/null 2>&1";
        status = AccountOperationWithRetry(cmd, ACCOUNT_OP_MOD);
    }

    return status;
}
