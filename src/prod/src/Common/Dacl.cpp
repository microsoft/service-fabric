// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

StringLiteral TraceType_Dacl = "Dacl";

Dacl::ACEInfo::ACEInfo(BYTE aceType, BYTE aceFlags, ACCESS_MASK accessMask, PSID pSid)
    : aceType_(aceType),
    aceFlags_(aceFlags),
    accessMask_(accessMask),
    pSid_(pSid)

{
    ASSERT_IFNOT(Sid::IsValid(PSid), "SID must be valid.");
    ASSERT_IFNOT(
        ((aceType == ACCESS_ALLOWED_ACE_TYPE) || (aceType == ACCESS_DENIED_ACE_TYPE)),
        "Unsupported ACE TYPE {0}",
        aceType);
}

Dacl::ACEInfo::ACEInfo(ACEInfo const & other)
    : aceType_(other.aceType_),
    aceFlags_(other.aceFlags_),
    accessMask_(other.accessMask_),
    pSid_(other.pSid_)
{
}

Dacl::ACEInfo::ACEInfo(ACEInfo && other)
    : aceType_(other.aceType_),
    aceFlags_(other.aceFlags_),
    accessMask_(other.accessMask_),
    pSid_(other.pSid_)
{
}

Dacl::ACEInfo const & Dacl::ACEInfo::operator = (ACEInfo const & other)
{
    if (this != &other)
    {
        this->aceType_ = other.aceType_;
        this->aceFlags_ = other.aceFlags_;
        this->accessMask_ = other.accessMask_;
        this->pSid_ = other.pSid_;
    }

    return *this;
}

Dacl::ACEInfo const & Dacl::ACEInfo::operator = (ACEInfo && other)
{
    if (this != &other)
    {
        this->aceType_ = other.aceType_;
        this->aceFlags_ = other.aceFlags_;
        this->accessMask_ = other.accessMask_;
        this->pSid_ = other.pSid_;
    }

    return *this;
}

DWORD Dacl::ACEInfo::GetLength() const
{
    DWORD length = offsetof(ACCESS_ALLOWED_ACE, SidStart);
    length += ::GetLengthSid(PSid);
    return length;
}

bool Dacl::ACEInfo::IsValid() const
{
    return Sid::IsValid(PSid);
}

Dacl::Dacl(PACL const pAcl)
    : pAcl_(pAcl)
{
    ASSERT_IFNOT(IsValid(pAcl_), "The PACL must be valid.");
}

Dacl::~Dacl()
{
}

bool Dacl::IsValid(PACL const pAcl)
{
    if (pAcl == NULL) { return false; }
    return ::IsValidAcl(pAcl) ? true : false;
}

BufferedDacl::BufferedDacl(ByteBuffer && buffer)
    : Dacl(GetPACL(buffer)),
    buffer_(move(buffer))
{
}

BufferedDacl::~BufferedDacl()
{
}

ErrorCode BufferedDacl::CreateSPtr(vector<Dacl::ACEInfo> entries, __out DaclSPtr & dacl)
{
    return CreateSPtr(NULL, entries, dacl);
}

ErrorCode BufferedDacl::CreateSPtr(PACL const existingAcl, __out DaclSPtr & dacl)
{
    return CreateSPtr(existingAcl, false, dacl);
}

ErrorCode BufferedDacl::CreateSPtr(PACL const existingAcl, bool removeInvalid, __out DaclSPtr & dacl)
{
    vector<Dacl::ACEInfo> additionalEntries;
    if (removeInvalid)
    {
        return CreateSPtr_ValidateExistingAce(
            existingAcl,
            additionalEntries,
            [](void * pAce, __out bool & isValid) -> ErrorCode { return BufferedDacl::ValidateSid(pAce, isValid); },
            dacl);
    }
    else
    {
        return CreateSPtr(existingAcl, additionalEntries, dacl);
    }
}

ErrorCode BufferedDacl::CreateSPtr(PACL const existingAcl, vector<SidSPtr> const & removeACLsOn, __out DaclSPtr & dacl)
{
    vector<wstring> sidStrings_RemoveACLsOn;
    for(auto iter = removeACLsOn.begin(); iter != removeACLsOn.end(); ++iter)
    {
        wstring stringSid;
        auto error = (*iter)->ToString(stringSid);
        if (!error.IsSuccess())
        {
            TraceWarning(
                TraceTaskCodes::Common,
                TraceType_Dacl,
                "The supplied Sid is not valid.");
            return ErrorCode::FromHResult(E_INVALIDARG);
        }

        sidStrings_RemoveACLsOn.push_back(stringSid);
    }
    vector<Dacl::ACEInfo> additionalEntries;

    return CreateSPtr_ValidateExistingAce(
        existingAcl,
        additionalEntries,
        [&sidStrings_RemoveACLsOn](void * pAce, __out bool & isValid) -> ErrorCode 
        { 
            bool isContains;
            auto error = BufferedDacl::ContainsSid(pAce, sidStrings_RemoveACLsOn, isContains);
            if(error.IsSuccess()) { isValid = !isContains; };
            return error;
        },
        dacl);
}

ErrorCode BufferedDacl::CreateSPtr(
    PACL const existingAcl,
    vector<Dacl::ACEInfo> const & additionalEntries,
    bool removeInvalid,
    __out DaclSPtr & dacl)
{
    if (removeInvalid)
    {
        return CreateSPtr_ValidateExistingAce(
            existingAcl,
            additionalEntries,
            [](void * pAce, __out bool & isValid) -> ErrorCode { return BufferedDacl::ValidateSid(pAce, isValid); },
            dacl);
    }
    else
    {
        return CreateSPtr(existingAcl, additionalEntries, dacl);
    }
}

ErrorCode BufferedDacl::CreateSPtr_ValidateExistingAce(
    PACL const existingAcl, 
    vector<Dacl::ACEInfo> const & additionalEntries,
    AceValidationCallback const & validationCallback, 
    __out DaclSPtr & dacl)
{
    if (existingAcl == NULL)
    {
        return ErrorCode::FromHResult(E_POINTER);
    }

    if (!Dacl::IsValid(existingAcl))
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_Dacl,
            "The supplied Dacl is not valid.");
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    // get information on the current ACLs
    ACL_SIZE_INFORMATION aclSizeInfo = { 0 };
    aclSizeInfo.AclBytesInUse = sizeof(ACL);

    if (!::GetAclInformation(existingAcl, (LPVOID)&aclSizeInfo, (DWORD)sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(),
            TraceTaskCodes::Common,
            TraceType_Dacl,
            "GetAclInformation");
    }

    PVOID pTempAce;
    vector<ULONG> validAceIndexes;
    DWORD dwNewAclSize = aclSizeInfo.AclBytesInUse;
    bool isValidAce;
    ErrorCode validationError;

    for (ULONG i = 0; i < aclSizeInfo.AceCount; i++)
    {
        // get existing ace
        if (!::GetAce(existingAcl, i, &pTempAce))
        {
            return ErrorCode::TraceReturn(
                ErrorCode::FromWin32Error(),
                TraceTaskCodes::Common,
                TraceType_Dacl,
                "GetAce");
        }

        validationError = validationCallback(pTempAce, isValidAce);
        if (!validationError.IsSuccess())
        {
            TraceWarning(
                TraceTaskCodes::Common,
                TraceType_Dacl,
                "Failed to validate ACE from the DACL, ErrorCode={0}.",
                validationError);

            return validationError;
        }

        if (isValidAce)
        {
            validAceIndexes.push_back(i);
        }
        else
        {
            dwNewAclSize -= ((PACE_HEADER)pTempAce)->AceSize;
        }
    }

    for (auto iter = additionalEntries.cbegin(); iter != additionalEntries.cend(); ++iter)
    {
        ASSERT_IFNOT(iter->IsValid(), "The supplied ACEInfo must be valid.");
        dwNewAclSize += iter->GetLength();
    }

    dwNewAclSize = (dwNewAclSize + (sizeof(DWORD) - 1)) & 0xfffffffc;

    // initialize new ACL
    ByteBuffer buffer;
    buffer.resize(dwNewAclSize, 0);
    PACL newAcl = BufferedDacl::GetPACL(buffer);

    if (!::InitializeAcl(newAcl, dwNewAclSize, ACL_REVISION))
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(),
            TraceTaskCodes::Common,
            TraceType_Dacl,
            "InitializeAcl");
    }

    ULONG newIndex = 0;
    for (auto iter = validAceIndexes.begin(); iter != validAceIndexes.end(); ++iter)
    {
        auto index = *iter;

        // get existing ace
        if (!::GetAce(existingAcl, index, &pTempAce))
        {
            return ErrorCode::TraceReturn(
                ErrorCode::FromWin32Error(),
                TraceTaskCodes::Common,
                TraceType_Dacl,
                "GetAce");
        }

        // add it to new acl.
        if (!::AddAce(newAcl, ACL_REVISION, newIndex, pTempAce, ((PACE_HEADER)pTempAce)->AceSize))
        {
            return ErrorCode::TraceReturn(
                ErrorCode::FromWin32Error(),
                TraceTaskCodes::Common,
                TraceType_Dacl,
                "AddAce");
        }

        newIndex++;
    }
    //add additional entries
    for (auto iter = additionalEntries.cbegin(); iter != additionalEntries.cend(); ++iter)
    {
        if (iter->AceType == ACCESS_ALLOWED_ACE_TYPE)
        {
            if (!::AddAccessAllowedAceEx(newAcl, ACL_REVISION, iter->AceFlags, iter->AccessMask, iter->PSid))
            {
                return ErrorCode::TraceReturn(
                    ErrorCode::FromWin32Error(),
                    TraceTaskCodes::Common,
                    TraceType_Dacl,
                    "AddAccessAllowedAceEx");
            }
        }
        else if (iter->AceType == ACCESS_DENIED_ACE_TYPE)
        {
            if (!::AddAccessDeniedAceEx(newAcl, ACL_REVISION, iter->AceFlags, iter->AccessMask, iter->PSid))
            {
                return ErrorCode::TraceReturn(
                    ErrorCode::FromWin32Error(),
                    TraceTaskCodes::Common,
                    TraceType_Dacl,
                    "AddAccessDeniedAceEx");
            }
        }
        else
        {
            Assert::CodingError("Unsupported ACE Type {0}", iter->AceType);
        }
    }

     // create dacl object
    dacl = make_shared<BufferedDacl>(move(buffer));
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode BufferedDacl::ValidateSid(void * pAce, __out bool & isValid)
{
    isValid = false;
    PSID pTempSid;
    auto aceType = ((PACE_HEADER)pAce)->AceType;
    if ((aceType == ACCESS_ALLOWED_ACE_TYPE) || (aceType == ACCESS_DENIED_ACE_TYPE))
    {
        if (aceType == ACCESS_ALLOWED_ACE_TYPE)
        {
            pTempSid = (PSID)&((PACCESS_ALLOWED_ACE)pAce)->SidStart;
        }
        else
        {
            pTempSid = (PSID)&((PACCESS_DENIED_ACE)pAce)->SidStart;
        }

        wstring userName;
        wstring domainName;
        if (Sid::LookupAccount(pTempSid, domainName, userName).IsSuccess())
        {
            isValid = true;
        }
        else
        {
            isValid = false;
        }
    }
    else
    {
        isValid = true; // ignore unknown ace
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode BufferedDacl::ContainsSid(void * pAce, std::vector<std::wstring> const & targetSids, __out bool & isContains)
{
    isContains = false;
    PSID pTempSid;
    auto aceType = ((PACE_HEADER)pAce)->AceType;
    if ((aceType == ACCESS_ALLOWED_ACE_TYPE) || (aceType == ACCESS_DENIED_ACE_TYPE))
    {
        if (aceType == ACCESS_ALLOWED_ACE_TYPE)
        {
            pTempSid = (PSID)&((PACCESS_ALLOWED_ACE)pAce)->SidStart;
        }
        else
        {
            pTempSid = (PSID)&((PACCESS_DENIED_ACE)pAce)->SidStart;
        }

        wstring sidString;
        if (Sid::ToString(pTempSid, sidString).IsSuccess())
        {
            auto iter = ::find(targetSids.begin(), targetSids.end(), sidString);
            isContains = (iter != targetSids.end());
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode BufferedDacl::CreateSPtr(PACL const existingAcl, vector<Dacl::ACEInfo> additionalEntries, __out DaclSPtr & dacl)
{
    if (existingAcl != NULL && !Dacl::IsValid(existingAcl))
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_Dacl,
            "The supplied Dacl is not valid.");
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    // get the size of the ACL to be created
    ACL_SIZE_INFORMATION aclSizeInfo = { 0 };
    aclSizeInfo.AclBytesInUse = sizeof(ACL);

    if (existingAcl != NULL)
    {
        if (!::GetAclInformation(existingAcl, (LPVOID)&aclSizeInfo, (DWORD)sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
        {
            return ErrorCode::TraceReturn(
                ErrorCode::FromWin32Error(),
                TraceTaskCodes::Common,
                TraceType_Dacl,
                "GetAclInformation");
        }
    }

    DWORD dwNewAclSize = aclSizeInfo.AclBytesInUse;
    // add size information of the new ACE entries
    for(auto iter = additionalEntries.cbegin(); iter != additionalEntries.cend(); ++iter)
    {
        ASSERT_IFNOT(iter->IsValid(), "The supplied ACEInfo must be valid.");
        dwNewAclSize += iter->GetLength();
    }

    // align to word boundry
    dwNewAclSize = (dwNewAclSize + (sizeof(DWORD) - 1)) & 0xfffffffc;

    // initialize new ACL
    ByteBuffer buffer;
    buffer.resize(dwNewAclSize, 0);
    PACL newAcl = BufferedDacl::GetPACL(buffer);

    if (!::InitializeAcl(newAcl, dwNewAclSize, ACL_REVISION))
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(),
            TraceTaskCodes::Common,
            TraceType_Dacl,
            "InitializeAcl");
    }

    // copy existing entries if any
    if (existingAcl)
    {
        PVOID pTempAce;

        for (ULONG i = 0; i < aclSizeInfo.AceCount; i++)
        {
            // get existing ace
            if (!::GetAce(existingAcl, i, &pTempAce))
            {
                return ErrorCode::TraceReturn(
                    ErrorCode::FromWin32Error(),
                    TraceTaskCodes::Common,
                    TraceType_Dacl,
                    "GetAce");
            }

            // add it to new acl.
            if (!::AddAce(newAcl, ACL_REVISION, i, pTempAce, ((PACE_HEADER)pTempAce)->AceSize))
            {
                return ErrorCode::TraceReturn(
                    ErrorCode::FromWin32Error(),
                    TraceTaskCodes::Common,
                    TraceType_Dacl,
                    "AddAce");
            }
        }
    }

    // add new entries
    for(auto iter = additionalEntries.cbegin(); iter != additionalEntries.cend(); ++iter)
    {
        if (iter->AceType == ACCESS_ALLOWED_ACE_TYPE)
        {
            if (!::AddAccessAllowedAceEx(newAcl, ACL_REVISION, iter->AceFlags, iter->AccessMask, iter->PSid))
            {
                return ErrorCode::TraceReturn(
                    ErrorCode::FromWin32Error(),
                    TraceTaskCodes::Common,
                    TraceType_Dacl,
                    "AddAccessAllowedAceEx");
            }
        }
        else if (iter->AceType == ACCESS_DENIED_ACE_TYPE)
        {
            if (!::AddAccessDeniedAceEx(newAcl, ACL_REVISION, iter->AceFlags, iter->AccessMask, iter->PSid))
            {
                return ErrorCode::TraceReturn(
                    ErrorCode::FromWin32Error(),
                    TraceTaskCodes::Common,
                    TraceType_Dacl,
                    "AddAccessDeniedAceEx");
            }
        }
        else
        {
            Assert::CodingError("Unsupported ACE Type {0}", iter->AceType);
        }
    }

    // create dacl object
    dacl = make_shared<BufferedDacl>(move(buffer));
    return ErrorCode(ErrorCodeValue::Success);
}


PACL BufferedDacl::GetPACL(ByteBuffer const & buffer)
{
    if (buffer.size() == 0)
    {
        return NULL;
    }
    else
    {
        auto pByte = const_cast<BYTE*>(buffer.data());
        return reinterpret_cast<PACL>(pByte);
    }
}
