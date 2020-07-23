// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

StringLiteral TraceType_SecurityDescriptor = "SecurityDescriptor";

SecurityDescriptor::SecurityDescriptor(const PSECURITY_DESCRIPTOR pSecurityDescriptor)
    : pSecurityDescriptor_(pSecurityDescriptor)
{
    ASSERT_IFNOT(IsValid(pSecurityDescriptor_), "The SecurityDescriptor must be valid.");
}

SecurityDescriptor::~SecurityDescriptor()
{
}

bool SecurityDescriptor::IsEquals(const PSECURITY_DESCRIPTOR pDescriptor) const
{
    if (!IsValid(pDescriptor)) return false;

    DWORD length = ::GetSecurityDescriptorLength(pDescriptor);
    if (this->Length != length)
    {
        return false;
    }

    // The following assumes self-relative security descriptor, it does not work for absolute ones.
    return memcmp(pDescriptor, this->PSecurityDescriptor, length) == 0;
}

bool SecurityDescriptor::IsValid(PSECURITY_DESCRIPTOR const pSecurityDescriptor)
{
    if (pSecurityDescriptor == NULL) { return false; }
    return ::IsValidSecurityDescriptor(pSecurityDescriptor) ? true : false;
}

void SecurityDescriptor::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    PSID ownerSid;
    BOOL ownerDefaulted;
    wstring domain, account;
    if (GetSecurityDescriptorOwner(pSecurityDescriptor_, &ownerSid, &ownerDefaulted))
    {
        Sid::LookupAccount(ownerSid, domain, account);
    }

    if (ownerDefaulted)
    {
        w.Write("[owner(defaulted)='{0}\\{1}', ", domain, account);
    }
    else
    {
        w.Write("[owner='{0}\\{1}', ", domain, account);
    }

    PSID groupSid;
    BOOL groupDefaulted;
    wstring groupSidString;
    if(GetSecurityDescriptorGroup(pSecurityDescriptor_, &groupSid, &groupDefaulted))
    {
        Sid::LookupAccount(groupSid, domain, account);
    }

    if (groupDefaulted)
    {
        w.Write("group(defaulted)='{0}\\{1}', ", domain, account);
    }
    else
    {
        w.Write("group='{0}\\{1}', ", domain, account);
    }

    BOOL daclPresent;
    PACL dacl;
    BOOL daclDefaulted;
    if (GetSecurityDescriptorDacl(pSecurityDescriptor_, &daclPresent, &dacl, &daclDefaulted) == FALSE)
    {
        w.Write("dacl=FailedToRetrieve({0})]", ::GetLastError());
        return;
    }

    if (daclDefaulted)
    {
        w.Write("dacl(defaulted)=");
    }
    else
    {
        w.Write("dacl=");
    }

    if (dacl == nullptr)
    {
        w.Write("null]");
        return;
    }

    ACL_SIZE_INFORMATION aclSizeInformation = {};
    if (GetAclInformation(dacl, &aclSizeInformation, sizeof(aclSizeInformation), AclSizeInformation) == FALSE)
    {
        w.Write("FailedToRetrieve({0})]", ::GetLastError());
        return;
    }

    if (aclSizeInformation.AceCount == 0)
    {
        w.Write("empty]");
        return;
    }

    for (ULONG i = 0; i < aclSizeInformation.AceCount; ++ i)
    {
        ACCESS_ALLOWED_ACE* ace;
        if (GetAce(dacl, i, (PVOID*)&ace))
        {
            PSID sid = (PSID)(&(ace->SidStart));
            wstring sidString;
            auto error = Sid::ToString(sid, sidString);
            if (!error.IsSuccess())
            {
                w.Write(
                    "(type={0},flags={1:x},access={2:x},FailedToConvertSidToString:{3}",
                    ace->Header.AceType,
                    ace->Header.AceFlags,
                    ace->Mask,
                    error);
                continue;
            }

            Sid::LookupAccount(sid, domain, account);
            w.Write(
                "(type={0},flags={1:x},access={2:x},sid={3},account='{4}\\{5}')",
                ace->Header.AceType,
                ace->Header.AceFlags,
                ace->Mask,
                sidString,
                domain,
                account);
        }
    }

    w.Write(']');
}

BufferedSecurityDescriptor::BufferedSecurityDescriptor(ByteBuffer && buffer)
    : SecurityDescriptor(GetPSECURITY_DESCRIPTOR(buffer)),
    buffer_(move(buffer))
{
}

BufferedSecurityDescriptor::~BufferedSecurityDescriptor()
{
}

PSECURITY_DESCRIPTOR BufferedSecurityDescriptor::GetPSECURITY_DESCRIPTOR(ByteBuffer const & buffer)
{
    if (buffer.size() == 0)
    {
        return NULL;
    }
    else
    {
        auto pByte = const_cast<BYTE*>(buffer.data());
        return reinterpret_cast<PSECURITY_DESCRIPTOR>(pByte);
    }
}

ErrorCode BufferedSecurityDescriptor::CreateSPtr(wstring const & accountName, __out SecurityDescriptorSPtr & sd)
{
    ASSERT_IF(accountName.empty(), "AccountName must not be empty.");

    EXPLICIT_ACCESS_W access = {0};
    ::BuildExplicitAccessWithNameW(&access, (LPWSTR)&accountName[0], FWP_ACTRL_MATCH_FILTER, GRANT_ACCESS, 0);

    PSECURITY_DESCRIPTOR descriptor = NULL;
    ULONG length = 0;
    if (!::BuildSecurityDescriptorW(NULL, NULL, 1, &access, 0, NULL, NULL, &length, &descriptor) == ERROR_SUCCESS)
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(),
            TraceTaskCodes::Common,
            TraceType_SecurityDescriptor,
            "BuildSecurityDescriptorW");
    }

    try
    {
        return CreateSPtr(descriptor, sd);
    }
    catch(...)
    {
        ::LocalFree(descriptor);
        throw;
    }
}

ErrorCode BufferedSecurityDescriptor::CreateSPtr(PSECURITY_DESCRIPTOR const pSecurityDescriptor, __out SecurityDescriptorSPtr & sd)
{
    ASSERT_IFNOT(SecurityDescriptor::IsValid(pSecurityDescriptor), "The specified SecurityDescriptor must be valid.");

    DWORD revision;
    SECURITY_DESCRIPTOR_CONTROL sdc = { 0 };
    if ( !::GetSecurityDescriptorControl(pSecurityDescriptor, &sdc, &revision))
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(),
            TraceTaskCodes::Common,
            TraceType_SecurityDescriptor,
            "GetSecurityDescriptorControl");
    }

    DWORD length = ::GetSecurityDescriptorLength(pSecurityDescriptor);
    ByteBuffer buffer;
    buffer.resize(length, 0);
    PSECURITY_DESCRIPTOR pNewDescriptor = BufferedSecurityDescriptor::GetPSECURITY_DESCRIPTOR(buffer);

    if (sdc & SE_SELF_RELATIVE)
    {
        auto err = memcpy_s(pNewDescriptor, length, pSecurityDescriptor, length);
        if (err != ERROR_SUCCESS)
        {
            TraceWarning(
                TraceTaskCodes::Common,
                TraceType_SecurityDescriptor,
                "memcpy_s failed with {0} in copying SecurityDescriptor of length {1}",
                err,
                length);

            return ErrorCode(ErrorCodeValue::OperationFailed);
        }
    }
    else
    {
        if (!::MakeSelfRelativeSD(pSecurityDescriptor, pNewDescriptor, &length))
        {
            return ErrorCode::TraceReturn(
                ErrorCode::FromWin32Error(),
                TraceTaskCodes::Common,
                TraceType_SecurityDescriptor,
                "MakeSelfRelativeSD"); 
        }
    }

    sd = make_shared<BufferedSecurityDescriptor>(move(buffer));
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode BufferedSecurityDescriptor::CreateSPtr(__out SecurityDescriptorSPtr & sd)
{
    ByteBuffer buffer;
    buffer.resize(SECURITY_DESCRIPTOR_MIN_LENGTH, 0);
    PSECURITY_DESCRIPTOR pSecurityDescriptor = BufferedSecurityDescriptor::GetPSECURITY_DESCRIPTOR(buffer);

    if (!::InitializeSecurityDescriptor(pSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION)) 
    {  
         return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(),
            TraceTaskCodes::Common,
            TraceType_SecurityDescriptor,
            "InitializeSecurityDescriptor"); 
    }

    sd = make_shared<BufferedSecurityDescriptor>(move(buffer));
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode BufferedSecurityDescriptor::CreateSPtrFromKernelObject(HANDLE object, __out SecurityDescriptorSPtr & sd)
{
     ULONG length = 0;
    ::GetKernelObjectSecurity (object, DACL_SECURITY_INFORMATION, NULL, 0, &length);
    DWORD win32Error = ::GetLastError();
    if (win32Error != ERROR_INSUFFICIENT_BUFFER)
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(win32Error),
            TraceTaskCodes::Common,
            TraceType_SecurityDescriptor,
            "GetKernelObjectSecurity-Size");
    }

    ByteBuffer buffer;
    buffer.resize(length, 0);
    PSECURITY_DESCRIPTOR pSecurityDescriptor = BufferedSecurityDescriptor::GetPSECURITY_DESCRIPTOR(buffer);
    if (!::GetKernelObjectSecurity (object, DACL_SECURITY_INFORMATION, pSecurityDescriptor, length, &length))
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(),
            TraceTaskCodes::Common,
            TraceType_SecurityDescriptor,
            "GetKernelObjectSecurity");
    }

    sd = make_shared<BufferedSecurityDescriptor>(move(buffer));
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode BufferedSecurityDescriptor::ConvertSecurityDescriptorStringToSecurityDescriptor(
    wstring const & stringSecurityDescriptor,
    __out SecurityDescriptorSPtr & sd)
{
    ErrorCode errorCode(ErrorCodeValue::Success);
    ULONG sdSize;
    PSECURITY_DESCRIPTOR sdPtr;
    BOOL retval = ::ConvertStringSecurityDescriptorToSecurityDescriptor(stringSecurityDescriptor.c_str(),
            SDDL_REVISION_1,
            &sdPtr,
            &sdSize);
    if(!retval)
    {
        errorCode = ErrorCode::FromWin32Error(::GetLastError());
        return errorCode;
    }
    ByteBuffer sdBuffer(sdSize, 0);
    KMemCpySafe(sdBuffer.data(), sdBuffer.size(), sdPtr, sdSize);
    LocalFree(sdPtr);

    sd = make_shared<BufferedSecurityDescriptor>(move(sdBuffer));
    return errorCode;
}

_Use_decl_annotations_
ErrorCode BufferedSecurityDescriptor::CreateAllowed(
    HANDLE ownerToken,
    bool allowOwner,
    std::vector<Common::SidSPtr> const & allowedSIDs,
    DWORD accessMask,
    SecurityDescriptorSPtr & sd)
{
    ByteBuffer userTokenBuffer;
    DWORD length = 0;
    GetTokenInformation(ownerToken, TokenUser, nullptr, 0, &length);
    userTokenBuffer.resize(length, 0);

    ErrorCode errorCode;
    if (GetTokenInformation(ownerToken, TokenUser, &userTokenBuffer.front(), length, &length) == FALSE)
    {
        errorCode = ErrorCode::FromWin32Error();
        return errorCode;
    }

    TOKEN_USER & tokenUser = reinterpret_cast<TOKEN_USER&>(userTokenBuffer.front());
    TRUSTEE owner = {nullptr, NO_MULTIPLE_TRUSTEE, TRUSTEE_IS_SID, TRUSTEE_IS_USER, (LPWSTR)tokenUser.User.Sid};

    ByteBuffer primaryGroupTokenBuffer;
    length = 0;
    GetTokenInformation(ownerToken, TokenPrimaryGroup, nullptr, 0, &length);
    primaryGroupTokenBuffer.resize(length, 0);
    if (GetTokenInformation(ownerToken, TokenPrimaryGroup, &primaryGroupTokenBuffer.front(), length, &length) == FALSE)
    {
        errorCode = ErrorCode::FromWin32Error();
        return errorCode;
    }

    TOKEN_PRIMARY_GROUP & tokenPrimaryGroup = reinterpret_cast<TOKEN_PRIMARY_GROUP&>(primaryGroupTokenBuffer.front());
    TRUSTEE group = {nullptr, NO_MULTIPLE_TRUSTEE, TRUSTEE_IS_SID, TRUSTEE_IS_GROUP, (LPWSTR)tokenPrimaryGroup.PrimaryGroup};

    vector<EXPLICIT_ACCESS> allowEntries;
    allowEntries.reserve(allowedSIDs.size() + 1);

    // Add ACE for owner
    EXPLICIT_ACCESS accessInfo = {accessMask, SET_ACCESS, NO_INHERITANCE, owner};
    if (allowOwner)
    {
        allowEntries.push_back(accessInfo);
    }

    // Add ACE for allowed SIDs
    accessInfo.Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN; // It could be either user or group
    for (auto toAdd = allowedSIDs.cbegin(); toAdd != allowedSIDs.cend(); ++ toAdd)
    {
        if (!Sid::IsValid((*toAdd)->PSid))
        {
            TraceWarning(TraceTaskCodes::Common, TraceType_SecurityDescriptor, "CreateSPtr: invalid SID found in allow list");
            return ErrorCodeValue::InvalidArgument;
        }

        bool shouldAdd = true;
        for (auto added = allowEntries.cbegin(); added != allowEntries.cend(); ++ added)
        {
            if (::EqualSid(added->Trustee.ptstrName, (*toAdd)->PSid))
            {
                wstring sidString;
                if (Sid::ToString((*toAdd)->PSid, sidString).IsSuccess())
                {
                    shouldAdd = false;
                    TraceInfo(
                        TraceTaskCodes::Common,
                        TraceType_SecurityDescriptor,
                        "CreateSPtr: duplicate SID found in allow list: {0}",
                        sidString);

                    break;
                }
            }
        }

        if (!shouldAdd)
        {
            continue;
        }

        accessInfo.Trustee.ptstrName = (LPWSTR)(*toAdd)->PSid;
        allowEntries.push_back(accessInfo);
    }

    if (allowEntries.empty())
    {
        // Add a dummy 0 access entry to avoid a null dacl, null dacl will grant access to all
        accessInfo.grfAccessPermissions = 0;
        accessInfo.Trustee = owner;
        allowEntries.push_back(accessInfo);
        TraceInfo(
            TraceTaskCodes::Common,
            TraceType_SecurityDescriptor,
            "CreateSPtr: add dummy 0 access entry to avoid null dacl");
    }

    ULONG sdSize;
    PSECURITY_DESCRIPTOR sdPtr;
    DWORD retval = BuildSecurityDescriptor(
        &owner,
        &group,
        (ULONG)allowEntries.size(),
        allowEntries.data(),
        0,
        nullptr,
        nullptr,
        &sdSize,
        &sdPtr);

    if (retval != ERROR_SUCCESS)
    {
        errorCode = ErrorCode::FromWin32Error(retval);
        return errorCode;
    }

    ByteBuffer sdBuffer(sdSize, 0);
    KMemCpySafe(sdBuffer.data(), sdBuffer.size(), sdPtr, sdSize);
    LocalFree(sdPtr);

    sd = make_shared<BufferedSecurityDescriptor>(move(sdBuffer));
    return errorCode;
}

_Use_decl_annotations_
ErrorCode BufferedSecurityDescriptor::CreateAllowed(    
    std::vector<Common::SidSPtr> const & allowedSIDs,
    DWORD accessMask,
    SecurityDescriptorSPtr & sd)
{    
    ASSERT_IF(allowedSIDs.empty(), "allowedSids cannot be empty");

    vector<EXPLICIT_ACCESS> allowEntries;
    allowEntries.reserve(allowedSIDs.size());

    // Add ACE for allowed SIDs
    for (auto toAdd = allowedSIDs.cbegin(); toAdd != allowedSIDs.cend(); ++ toAdd)
    {
        if (!Sid::IsValid((*toAdd)->PSid))
        {
            TraceWarning(TraceTaskCodes::Common, TraceType_SecurityDescriptor, "CreateSPtr: invalid SID found in allow list");
            return ErrorCodeValue::InvalidArgument;
        }

        bool shouldAdd = true;
        for (auto added = allowEntries.cbegin(); added != allowEntries.cend(); ++ added)
        {
            if (::EqualSid(added->Trustee.ptstrName, (*toAdd)->PSid))
            {
                wstring sidString;
                if (Sid::ToString((*toAdd)->PSid, sidString).IsSuccess())
                {
                    shouldAdd = false;
                    TraceInfo(
                        TraceTaskCodes::Common,
                        TraceType_SecurityDescriptor,
                        "CreateSPtr: duplicate SID found in allow list: {0}",
                        sidString);

                    break;
                }
            }
        }

        if (!shouldAdd)
        {
            continue;
        }

        TRUSTEE trustee = {nullptr, NO_MULTIPLE_TRUSTEE, TRUSTEE_IS_SID, TRUSTEE_IS_UNKNOWN, (LPWSTR)(*toAdd)->PSid};
        EXPLICIT_ACCESS accessInfo = {accessMask, SET_ACCESS, NO_INHERITANCE, trustee};
        
        allowEntries.push_back(accessInfo);
    }

    ULONG sdSize;
    PSECURITY_DESCRIPTOR sdPtr;
    DWORD retval = BuildSecurityDescriptor(
        nullptr,
        nullptr,
        (ULONG)allowEntries.size(),
        allowEntries.data(),
        0,
        nullptr,
        nullptr,
        &sdSize,
        &sdPtr);

    if (retval != ERROR_SUCCESS)
    {
        return ErrorCode::FromWin32Error(retval);
        
    }

    ByteBuffer sdBuffer(sdSize, 0);
    KMemCpySafe(sdBuffer.data(), sdBuffer.size(), sdPtr, sdSize);
    LocalFree(sdPtr);

    sd = make_shared<BufferedSecurityDescriptor>(move(sdBuffer));

    return ErrorCodeValue::Success;
}

ULONG SecurityDescriptor::get_Length() const
{
    return ::GetSecurityDescriptorLength(this->PSecurityDescriptor);
}

