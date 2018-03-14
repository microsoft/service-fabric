// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

using namespace std;
using namespace Common;

StringLiteral TraceType_Sid = "Sid";

Sid::Sid(PSID const pSid)
    : pSid_(pSid)
{
    ASSERT_IFNOT(IsValid(pSid_), "The SID must be valid.");
}

Sid::~Sid()
{
}

bool Sid::IsLocalSystem() const
{
    return ((PLSID)pSid_)->SubAuthority[SID_SUBAUTH_ARRAY_UID] == 0;
}

bool Sid::IsNetworkService() const
{
    return false;
}

bool Sid::IsAnonymous() const
{
    return false;
}

SID_IDENTIFIER_AUTHORITY Sid::GetAuthority() const
{
    return ((PLSID)(pSid_))->IdentifierAuthority;
}

UCHAR Sid::GetSubAuthorityCount() const
{
    return SID_SUBAUTH_ARRAY_SIZE;
}

DWORD Sid::GetSubAuthority(DWORD subAuthority) const
{
    return ((PLSID)pSid_)->SubAuthority[subAuthority];
}

ErrorCode Sid::ToString(__out wstring & stringSid) const
{
    return ToString(pSid_, stringSid);
}

ErrorCode Sid::GetAllowDacl(__out wstring & allowDacl) const
{
    wstring stringSid;
    auto error = ToString(stringSid);
    if (!error.IsSuccess()) { return error; }

    wstringstream sddlstream;
    sddlstream << L"D:(A;;GX;;;" << stringSid << L")";
    allowDacl = sddlstream.str();

    return ErrorCode(ErrorCodeValue::Success);
}

bool Sid::IsValid(PSID pSid)
{
    if (pSid == NULL) { return false; }

    PLSID plSID = (PLSID)pSid;
    return (plSID->Revision == SID_REVISION);
}

ErrorCode Sid::LookupAccount(PSID pSid, __out std::wstring & domainName, __out std::wstring & userName)
{
    ErrorCode err = ErrorCodeValue::InvalidArgument;

    if(!pSid) return err;

    uid_t uid = ((PLSID)pSid)->SubAuthority[1];
    struct passwd *pw = getpwuid(uid);
    if(pw)
    {
        string domainNameA = ".", userNameA = pw->pw_name;
        StringUtility::Utf8ToUtf16(domainNameA, domainName);
        StringUtility::Utf8ToUtf16(userNameA, userName);
        err = ErrorCodeValue::Success;
    }
    return err;
}

ErrorCode Sid::ToString(PSID pSid, __out wstring & stringSid)
{
    PLSID plSid = (PLSID)pSid;
    stringSid = wformatString("S-{0}-{1}-{2}-{3}",
                        plSid->Revision,
                        plSid->IdentifierAuthority.Value[5],
                        plSid->SubAuthority[SID_SUBAUTH_ARRAY_DOMAIN],
                        plSid->SubAuthority[SID_SUBAUTH_ARRAY_UID]);
    return ErrorCode(ErrorCodeValue::Success);
}


BufferedSid::BufferedSid(ByteBuffer && buffer)
    : Sid(GetPSID(buffer)),
    buffer_(move(buffer))
{
}

BufferedSid::~BufferedSid()
{
}

ErrorCode BufferedSid::CreateUPtr(__in PSID pSid, __out SidUPtr & sid)
{
    ByteBuffer buffer;
    auto error = Create(pSid, buffer);
    if (!error.IsSuccess()) { return error; }

    sid = make_unique<BufferedSid>(move(buffer));
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode BufferedSid::CreateSPtr(__in PSID pSid, __out SidSPtr & sid)
{
    ByteBuffer buffer;
    auto error = Create(pSid, buffer);
    if (!error.IsSuccess()) { return error; }

    sid = make_shared<BufferedSid>(move(buffer));
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode BufferedSid::CreateGroupSPtr(std::wstring const & accountName, __out SidSPtr & sid)
{
    ByteBuffer sidbuf;
    sidbuf.resize(SECURITY_MAX_SID_SIZE, 0);
    PLSID plSid = (PLSID)GetPSID(sidbuf);

    string accountNameA;
    StringUtility::Utf16ToUtf8(accountName, accountNameA);
    long buflen = 16384; // large enough
    unique_ptr<char[]> buf(new char[buflen]);
    struct group grbuf, *grbufp;
    if ((0 == getgrnam_r(accountNameA.c_str(), &grbuf, buf.get(), buflen, &grbufp)) && grbufp) {
        gid_t gid = grbuf.gr_gid;
        plSid->Revision = SID_REVISION;
        plSid->SubAuthorityCount = 2;
        plSid->IdentifierAuthority = SECURITY_LOCAL_SID_AUTHORITY;
        plSid->SubAuthority[0] = SECURITY_LINUX_DOMAIN_GROUP_RID;
        plSid->SubAuthority[1] = gid;

        sid = make_shared<BufferedSid>(move(sidbuf));
        return ErrorCodeValue::Success;
    }
    TraceError(
        TraceTaskCodes::Common,
        TraceType_Sid,
        "CreateAppRunAsGroupSPtr failed. errno={0}. buflen: {1}. account: {2}",
        errno, buflen, accountNameA);

    return ErrorCodeValue::NotFound;
}

ErrorCode BufferedSid::CreateUPtrFromStringSid(wstring const & stringSid, __out SidUPtr & sid)
{
    PSID pSid = NULL;
    try
    {
        vector<wstring> tokens;
        StringUtility::Split<wstring>(stringSid, tokens, L"-");
        int uid;
        if(StringUtility::TryFromWString<int>(tokens.back(), uid))
        {
            // TODO: memory reclamation..
            pSid = new LSID {SID_REVISION, 2, SECURITY_LOCAL_SID_AUTHORITY, {SECURITY_LINUX_DOMAIN_RID, uid}};
            auto error = BufferedSid::CreateUPtr(pSid, sid);
            delete pSid; pSid = NULL;
            return error;
        }
        else
        {
            return ErrorCode::TraceReturn(
                ErrorCode::FromWin32Error(),
                TraceTaskCodes::Common,
                TraceType_Sid,
                "ConvertStringSidToSid"); 
        }
    }
    catch(...)
    {
        if (pSid != NULL) { delete pSid; }
        throw;
    }
}

ErrorCode BufferedSid::CreateSPtrFromStringSid(wstring const & stringSid, __out SidSPtr & sid)
{
    PSID pSid = NULL;
    try
    {
        vector<wstring> tokens;
        StringUtility::Split<wstring>(stringSid, tokens, L"-");
        int utype, uid;
        if(StringUtility::TryFromWString<int>(tokens[tokens.size()-2], utype)
           && StringUtility::TryFromWString<int>(tokens[tokens.size()-1], uid))
        {
            pSid = new LSID {SID_REVISION, 2, SECURITY_LOCAL_SID_AUTHORITY, {utype, uid}};
            auto error = BufferedSid::CreateSPtr(pSid, sid);
            delete pSid; pSid = NULL;

            return ErrorCodeValue::Success;
        }
        else
        {
            return ErrorCode::TraceReturn(
                ErrorCode::FromWin32Error(),
                TraceTaskCodes::Common,
                TraceType_Sid,
                "ConvertStringSidToSid"); 
        }
    }
    catch(...)
    {
        if (pSid != NULL) { delete pSid; }
        throw;
    }
}

ErrorCode BufferedSid::CreateUPtr(wstring const & accountName, __out SidUPtr & sid)
{
    ByteBuffer buffer;
    auto error = Create(accountName, buffer);
    if (!error.IsSuccess()) { return error; }

    sid = make_unique<BufferedSid>(move(buffer));
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode BufferedSid::CreateSPtr(wstring const & accountName, __out SidSPtr & sid)
{
    ByteBuffer buffer;
    auto error = Create(accountName, buffer);
    if (!error.IsSuccess()) { return error; }

    sid = make_shared<BufferedSid>(move(buffer));
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode BufferedSid::CreateUPtr(WELL_KNOWN_SID_TYPE wellKnownSid, __out SidUPtr & sid)
{
    ByteBuffer buffer;
    auto error = Create(wellKnownSid, buffer);
    if (!error.IsSuccess()) { return error; }

    sid = make_unique<BufferedSid>(move(buffer));
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode BufferedSid::CreateSPtr(WELL_KNOWN_SID_TYPE wellKnownSid, __out SidSPtr & sid)
{
    ByteBuffer buffer;
    auto error = Create(wellKnownSid, buffer);
    if (!error.IsSuccess()) { return error; }

    sid = make_shared<BufferedSid>(move(buffer));
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode BufferedSid::CreateUPtr(SID_IDENTIFIER_AUTHORITY const & identifierAuthority, vector<DWORD> const & subAuthority, __out SidUPtr & sid)
{
    ByteBuffer buffer;
    auto error = Create(identifierAuthority, subAuthority, buffer);
    if (!error.IsSuccess()) { return error; }

    sid = make_unique<BufferedSid>(move(buffer));
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode BufferedSid::CreateSPtr(SID_IDENTIFIER_AUTHORITY const & identifierAuthority, vector<DWORD> const & subAuthority, __out SidSPtr & sid)
{
    ByteBuffer buffer;
    auto error = Create(identifierAuthority, subAuthority, buffer);
    if (!error.IsSuccess()) { return error; }

    sid = make_shared<BufferedSid>(move(buffer));
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode BufferedSid::Create(__in PSID pSid, __inout ByteBuffer & buffer)
{
    ASSERT_IFNOT(Sid::IsValid(pSid), "The specified Sid must be valid.");

    PLSID lpSid = (PLSID)pSid;
    UINT sidLength = sizeof(LSID);
    buffer.resize(sidLength, 0);
    PSID pNewSid = GetPSID(buffer);

    memcpy(pNewSid, pSid, sidLength);
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode BufferedSid::Create(wstring const & accountName,  __inout ByteBuffer & buffer)
{
    ASSERT_IF(accountName.empty(), "Account name cannot be empty"); 

    SID_NAME_USE accountType;
    DWORD sidLength = SECURITY_MAX_SID_SIZE;
    buffer.resize(sidLength, 0);
    PLSID plSid = (PLSID)GetPSID(buffer);

    string accountNameA;
    StringUtility::Utf16ToUtf8(accountName, accountNameA);

    long buflen = max(sysconf(_SC_GETPW_R_SIZE_MAX), sysconf(_SC_GETGR_R_SIZE_MAX));

    while(true)
    {
        int err = 0;
        unique_ptr<char[]> buf(new char[buflen]);
        struct passwd pwbuf, *pwbufp;
        struct group grbuf, *grbufp;
        if (0 == (err = getpwnam_r(accountNameA.c_str(), &pwbuf, buf.get(), buflen, &pwbufp)) && pwbufp) {
            uid_t uid = pwbuf.pw_uid;
            plSid->Revision = SID_REVISION;
            plSid->SubAuthorityCount = 2;
            plSid->IdentifierAuthority = SECURITY_LOCAL_SID_AUTHORITY;
            plSid->SubAuthority[0] = SECURITY_LINUX_DOMAIN_RID;
            plSid->SubAuthority[1] = uid;

            return ErrorCodeValue::Success;
        }
        else if (0 == (err = getgrnam_r(accountNameA.substr(0, 32).c_str(), &grbuf, buf.get(), buflen, &grbufp)) && grbufp) {
            gid_t gid = grbuf.gr_gid;
            plSid->Revision = SID_REVISION;
            plSid->SubAuthorityCount = 2;
            plSid->IdentifierAuthority = SECURITY_LOCAL_SID_AUTHORITY;
            plSid->SubAuthority[0] = SECURITY_LINUX_DOMAIN_GROUP_RID;
            plSid->SubAuthority[1] = gid;

            return ErrorCodeValue::Success;
        }
        if(err == ERANGE)
        {
            buflen *= 2;
            continue;
        }
        else
        {
            return ErrorCodeValue::NotFound;
        }
    }
}

ErrorCode BufferedSid::Create(WELL_KNOWN_SID_TYPE wellKnownSid,  __inout Common::ByteBuffer & buffer)
{
    DWORD sidLength = SECURITY_MAX_SID_SIZE;
    buffer.resize(sidLength, 0);
    PSID pSid = GetPSID(buffer);

    if (!::CreateWellKnownSid (wellKnownSid, NULL, pSid, &sidLength))
    {
        auto error = ErrorCode::FromWin32Error();
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_Sid,
            "CreateWellKnownSid failed. Result={0}, WellKnownSid={1}",
            error,
            (int)wellKnownSid);
        return error;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode BufferedSid::Create(SID_IDENTIFIER_AUTHORITY const & identifierAuthority, vector<DWORD> const & subAuthority,  __inout ByteBuffer & buffer)
{
    ASSERT_IF(subAuthority.empty(), "SubAuthorities cannot be empty");

    buffer.resize(SECURITY_MAX_SID_SIZE, 0);
    PSID pSid = GetPSID(buffer);

    PLSID plSid = (PLSID)pSid;
    plSid->Revision = SID_REVISION;
    plSid->SubAuthorityCount = subAuthority.size();
    for (UINT index = 0; index < subAuthority.size(); index++)
    {
        plSid->SubAuthority[index] = subAuthority[index];
    }
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode BufferedSid::GetCurrentUserSid(__out SidUPtr & sid)
{
    uid_t uid = geteuid();
    PSID pSid = new LSID {SID_REVISION, 2, SECURITY_LOCAL_SID_AUTHORITY, {SECURITY_LINUX_DOMAIN_RID, uid}};
    return BufferedSid::CreateUPtr(pSid, sid);
}

PSID BufferedSid::GetPSID(ByteBuffer const & buffer)
{
    if (buffer.size() == 0)
    {
        return NULL;
    }
    else
    {
        auto pByte = const_cast<BYTE*>(buffer.data());
        return reinterpret_cast<PSID>(pByte);
    }
}
