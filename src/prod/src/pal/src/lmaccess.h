// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"
#include "winnt.h"
#include "lmcons.h"

#ifdef __cplusplus
extern "C" {
#endif

// 46
NET_API_STATUS NET_API_FUNCTION
NetUserAdd (
    IN  LPCWSTR     servername OPTIONAL,
    IN  DWORD      level,
    IN  LPBYTE     buf,
    OUT LPDWORD    parm_err OPTIONAL
    );

NET_API_STATUS NET_API_FUNCTION
NetUserGetInfo (
    IN  LPCWSTR     servername OPTIONAL,
    IN  LPCWSTR     username,
    IN  DWORD      level,
    OUT LPBYTE     *bufptr
    );

NET_API_STATUS NET_API_FUNCTION
NetUserSetInfo (
    IN  LPCWSTR    servername OPTIONAL,
    IN  LPCWSTR    username,
    IN  DWORD     level,
    IN  LPBYTE    buf,
    OUT LPDWORD   parm_err OPTIONAL
    );

NET_API_STATUS NET_API_FUNCTION
NetUserDel (
    IN  LPCWSTR    servername OPTIONAL,
    IN  LPCWSTR    username
    );

NET_API_STATUS NET_API_FUNCTION
NetUserGetLocalGroups (
    IN  LPCWSTR    servername OPTIONAL,
    IN  LPCWSTR    username,
    IN  DWORD     level,
    IN  DWORD     flags,
    OUT LPBYTE    *bufptr,
    IN  DWORD     prefmaxlen,
    OUT LPDWORD   entriesread,
    OUT LPDWORD   totalentries
    );

typedef struct _LOCALGROUP_INFO_1 {
    LPWSTR   lgrpi1_name;
    LPWSTR   lgrpi1_comment;
}LOCALGROUP_INFO_1, *PLOCALGROUP_INFO_1, *LPLOCALGROUP_INFO_1;

typedef struct _LOCALGROUP_INFO_1002 {
     LPWSTR  lgrpi1002_comment;
}LOCALGROUP_INFO_1002, *PLOCALGROUP_INFO_1002, *LPLOCALGROUP_INFO_1002;

typedef struct _LOCALGROUP_MEMBERS_INFO_0 {
     PSID    lgrmi0_sid;
} LOCALGROUP_MEMBERS_INFO_0, *PLOCALGROUP_MEMBERS_INFO_0,
  *LPLOCALGROUP_MEMBERS_INFO_0;

typedef struct _LOCALGROUP_MEMBERS_INFO_1 {
     PSID         lgrmi1_sid;
     SID_NAME_USE lgrmi1_sidusage;
     LPWSTR       lgrmi1_name;
} LOCALGROUP_MEMBERS_INFO_1, *PLOCALGROUP_MEMBERS_INFO_1,
  *LPLOCALGROUP_MEMBERS_INFO_1;

typedef struct _LOCALGROUP_MEMBERS_INFO_3 {
     LPWSTR       lgrmi3_domainandname;
} LOCALGROUP_MEMBERS_INFO_3, *PLOCALGROUP_MEMBERS_INFO_3,
  *LPLOCALGROUP_MEMBERS_INFO_3;

typedef struct _LOCALGROUP_USERS_INFO_0 {
     LPWSTR  lgrui0_name;
} LOCALGROUP_USERS_INFO_0, *PLOCALGROUP_USERS_INFO_0,
  *LPLOCALGROUP_USERS_INFO_0;

typedef struct _USER_INFO_1 {
    LPWSTR   usri1_name;
    LPWSTR   usri1_password;
    DWORD    usri1_password_age;
    DWORD    usri1_priv;
    LPWSTR   usri1_home_dir;
    LPWSTR   usri1_comment;
    DWORD    usri1_flags;
    LPWSTR   usri1_script_path;
}USER_INFO_1, *PUSER_INFO_1, *LPUSER_INFO_1;

typedef struct _USER_INFO_4 {
    LPWSTR   usri4_name;
    LPWSTR   usri4_password;
    DWORD    usri4_password_age;
    DWORD    usri4_priv;
    LPWSTR   usri4_home_dir;
    LPWSTR   usri4_comment;
    DWORD    usri4_flags;
    LPWSTR   usri4_script_path;
    DWORD    usri4_auth_flags;
    LPWSTR   usri4_full_name;
    LPWSTR   usri4_usr_comment;
    LPWSTR   usri4_parms;
    LPWSTR   usri4_workstations;
    DWORD    usri4_last_logon;
    DWORD    usri4_last_logoff;
    DWORD    usri4_acct_expires;
    DWORD    usri4_max_storage;
    DWORD    usri4_units_per_week;
    PBYTE    usri4_logon_hours;
    DWORD    usri4_bad_pw_count;
    DWORD    usri4_num_logons;
    LPWSTR   usri4_logon_server;
    DWORD    usri4_country_code;
    DWORD    usri4_code_page;
    PSID     usri4_user_sid;
    DWORD    usri4_primary_group_id;
    LPWSTR   usri4_profile;
    LPWSTR   usri4_home_dir_drive;
    DWORD    usri4_password_expired;
}USER_INFO_4, *PUSER_INFO_4, *LPUSER_INFO_4;

typedef struct _USER_INFO_1007 {
     LPWSTR  usri1007_comment;
} USER_INFO_1007, *PUSER_INFO_1007, *LPUSER_INFO_1007;

// 479
#define UF_SCRIPT                          0x0001
#define UF_ACCOUNTDISABLE                  0x0002
#define UF_HOMEDIR_REQUIRED                0x0008
#define UF_LOCKOUT                         0x0010
#define UF_PASSWD_NOTREQD                  0x0020
#define UF_PASSWD_CANT_CHANGE              0x0040
#define UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED 0x0080

#define UF_TEMP_DUPLICATE_ACCOUNT       0x0100
#define UF_NORMAL_ACCOUNT               0x0200
#define UF_INTERDOMAIN_TRUST_ACCOUNT    0x0800
#define UF_WORKSTATION_TRUST_ACCOUNT    0x1000
#define UF_SERVER_TRUST_ACCOUNT         0x2000

// 510
#define UF_DONT_EXPIRE_PASSWD                         0x10000
#define UF_MNS_LOGON_ACCOUNT                          0x20000
#define UF_SMARTCARD_REQUIRED                         0x40000
#define UF_TRUSTED_FOR_DELEGATION                     0x80000
#define UF_NOT_DELEGATED                             0x100000
#define UF_USE_DES_KEY_ONLY                          0x200000
#define UF_DONT_REQUIRE_PREAUTH                      0x400000
#define UF_PASSWORD_EXPIRED                          0x800000
#define UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION   0x1000000
#define UF_NO_AUTH_DATA_REQUIRED                    0x2000000
#define UF_PARTIAL_SECRETS_ACCOUNT                  0x4000000
#define UF_USE_AES_KEYS                             0x8000000

#define LG_INCLUDE_INDIRECT         (0x0001)

// 678
#define NULL_USERSETINFO_PASSWD     "              "

#define TIMEQ_FOREVER               ((unsigned long) -1L)
#define USER_MAXSTORAGE_UNLIMITED   ((unsigned long) -1L)
#define USER_NO_LOGOFF              ((unsigned long) -1L)
#define UNITS_PER_DAY               24
#define UNITS_PER_WEEK              UNITS_PER_DAY * 7

// 949
NET_API_STATUS NET_API_FUNCTION
NetLocalGroupAdd (
    IN  LPCWSTR   servername OPTIONAL,
    IN  DWORD    level,
    IN  LPBYTE   buf,
    OUT LPDWORD  parm_err OPTIONAL
    );

// 975
NET_API_STATUS NET_API_FUNCTION
NetLocalGroupGetInfo (
    IN  LPCWSTR   servername OPTIONAL,
    IN  LPCWSTR   groupname,
    IN  DWORD    level,
    OUT LPBYTE   *bufptr
    );

NET_API_STATUS NET_API_FUNCTION
NetLocalGroupSetInfo (
    IN  LPCWSTR   servername OPTIONAL,
    IN  LPCWSTR   groupname,
    IN  DWORD    level,
    IN  LPBYTE   buf,
    OUT LPDWORD  parm_err OPTIONAL
    );

// 993
NET_API_STATUS NET_API_FUNCTION
NetLocalGroupDel (
    IN  LPCWSTR   servername OPTIONAL,
    IN  LPCWSTR   groupname
    );

NET_API_STATUS NET_API_FUNCTION
NetLocalGroupSetMembers (
    IN  LPCWSTR     servername OPTIONAL,
    IN  LPCWSTR     groupname,
    IN  DWORD      level,
    IN  LPBYTE     buf,
    IN  DWORD      totalentries
    );

NET_API_STATUS NET_API_FUNCTION
NetLocalGroupGetMembers (
    IN  LPCWSTR     servername OPTIONAL,
    IN  LPCWSTR     localgroupname,
    IN  DWORD      level,
    OUT LPBYTE     *bufptr,
    IN  DWORD      prefmaxlen,
    OUT LPDWORD    entriesread,
    OUT LPDWORD    totalentries,
    IN OUT PDWORD_PTR resumehandle
    );

// 1026
NET_API_STATUS NET_API_FUNCTION
NetLocalGroupAddMembers (
    IN  LPCWSTR     servername OPTIONAL,
    IN  LPCWSTR     groupname,
    IN  DWORD      level,
    IN  LPBYTE     buf,
    IN  DWORD      totalentries
    );

#define SERVICE_ACCOUNT_PASSWORD TEXT("_SA_{262E99C9-6160-4871-ACEC-4E61736B6F21}")

#ifdef __cplusplus
}
#endif
