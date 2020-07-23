// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

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

#ifndef PLATFORM_UNIX

bool Sid::IsLocalSystem() const
{
    return ::IsWellKnownSid(pSid_, WinLocalSystemSid) == TRUE;
}

bool Sid::IsNetworkService() const
{
    return ::IsWellKnownSid(pSid_, WinNetworkServiceSid) == TRUE;
}

bool Sid::IsAnonymous() const
{
    return ::IsWellKnownSid(pSid_, WinAnonymousSid) == TRUE;
}

#endif

SID_IDENTIFIER_AUTHORITY Sid::GetAuthority() const
{
    return *::GetSidIdentifierAuthority(pSid_);
}

UCHAR Sid::GetSubAuthorityCount() const
{
    return *::GetSidSubAuthorityCount(pSid_);
}

DWORD Sid::GetSubAuthority(DWORD subAuthority) const
{
    return *::GetSidSubAuthority(pSid_, subAuthority);
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
    return ::IsValidSid(pSid) ? true : false;
}

ErrorCode Sid::LookupAccount(PSID pSid, __out std::wstring & domainName, __out std::wstring & userName)
{
    SID_NAME_USE SidType;
    wchar_t lpUserName[MAX_PATH];
    wchar_t lpDomainName[MAX_PATH];
    DWORD dwSize = MAX_PATH;

    if (!::LookupAccountSidW(NULL, pSid, lpUserName, &dwSize, lpDomainName, &dwSize, &SidType))
    {
        DWORD const nStatus = ::GetLastError();
        auto error = ErrorCode::FromWin32Error(nStatus);
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_Sid,
            "LookupAccountSidW failed. Result={0}",
            error);
        if (nStatus == ERROR_NONE_MAPPED)
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        return error;
    }

    domainName = wstring(lpDomainName);
    userName = wstring(lpUserName);
    return ErrorCode(ErrorCodeValue::Success);  
}

ErrorCode Sid::ToString(PSID pSid, __out wstring & stringSid)
{
    LPWSTR strOut;
    if (::ConvertSidToStringSid(pSid, &strOut))
    {
        stringSid = strOut;
        ::LocalFree(strOut);

        return ErrorCode(ErrorCodeValue::Success);
    }
    else
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(),
            TraceTaskCodes::Common,
            TraceType_Sid,
            "ConvertSidToStringSid");
    }
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

ErrorCode BufferedSid::CreateUPtrFromStringSid(wstring const & stringSid, __out SidUPtr & sid)
{
    PSID pSid = NULL;
    try
    {
        if (::ConvertStringSidToSid(stringSid.c_str(), & pSid))
        {
            auto error = BufferedSid::CreateUPtr(pSid, sid);
            ::LocalFree(pSid); pSid = NULL;

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
        if (pSid != NULL) { ::LocalFree(pSid); }
        throw;
    }
}

ErrorCode BufferedSid::CreateSPtrFromStringSid(wstring const & stringSid, __out SidSPtr & sid)
{
    PSID pSid = NULL;
    try
    {
        if (::ConvertStringSidToSid(stringSid.c_str(), & pSid))
        {
            auto error = BufferedSid::CreateSPtr(pSid, sid);
            ::LocalFree(pSid); pSid = NULL;

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
        if (pSid != NULL) { ::LocalFree(pSid); }
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

    UINT sidLength = ::GetLengthSid(pSid);
    buffer.resize(sidLength, 0);
    PSID pNewSid = GetPSID(buffer);

    if (!::CopySid(sidLength, pNewSid, pSid))
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(),
            TraceTaskCodes::Common,
            TraceType_Sid,
            "CopySid");
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode BufferedSid::Create(wstring const & accountName,  __inout ByteBuffer & buffer)
{
    ASSERT_IF(accountName.empty(), "Account name cannot be empty"); 

    SID_NAME_USE accountType;
    DWORD sidLength = SECURITY_MAX_SID_SIZE;
    buffer.resize(sidLength, 0);
    PSID pSid = GetPSID(buffer);
    DWORD domainLength = MAX_PATH;
    wstring domainName;
    domainName.resize(domainLength, '\0');

    if (!::LookupAccountName(NULL, accountName.c_str(), pSid, &sidLength, &domainName[0], &domainLength, &accountType))
    {
        DWORD const nStatus = ::GetLastError();
        auto error = ErrorCode::FromWin32Error(nStatus);
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_Sid,
            "LookupAccountName failed. Result={0}, AccountName={1}",
            error,
            accountName);
        if (nStatus == ERROR_NONE_MAPPED)
        {
            // The account was not found
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        return error;
    }

    return ErrorCode(ErrorCodeValue::Success);
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

    auto subAuthoritySize = static_cast<BYTE>(subAuthority.size());
    auto length = ::GetSidLengthRequired(subAuthoritySize);
    if (length > SECURITY_MAX_SID_SIZE)
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_Sid,
            "GetSidLengthRequired returned {0}. SubAuthority.size() = {1}, SECURITY_MAX_SID_SIZE={2}",
            length,
            SECURITY_MAX_SID_SIZE,
            subAuthoritySize);
        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

    if (!::InitializeSid(pSid, const_cast<SID_IDENTIFIER_AUTHORITY*>(&identifierAuthority), subAuthoritySize))
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(),
            TraceTaskCodes::Common,
            TraceType_Sid,
            "InitializeSid");
    }

    for (UINT index = 0; index < subAuthority.size(); index++)
    {
        *::GetSidSubAuthority(pSid, index) = subAuthority[index];
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode BufferedSid::GetCurrentUserSid(__out SidUPtr & sid)
{
    HANDLE token;
    ByteBuffer userToken;

    BOOL retval = ::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token);
    if(!retval)
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(::GetLastError()),
            TraceTaskCodes::Common,
            TraceType_Sid,
            "OpenProcessToken");
    }

    HandleUPtr processToken = make_unique<Handle>(token);
    DWORD length = 0;

    ::GetTokenInformation(processToken->Value, TokenUser, nullptr, 0, &length);
  
    userToken.resize(length, 0);

    retval = ::GetTokenInformation(processToken->Value, TokenUser, &userToken.front(), length, &length);
    if(!retval)
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(::GetLastError()),
            TraceTaskCodes::Common,
            TraceType_Sid,
            "GetTokenInformation");
    }

    TOKEN_USER & tokenUser = reinterpret_cast<TOKEN_USER&>(userToken.front());
    return BufferedSid::CreateUPtr(tokenUser.User.Sid, sid);
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
