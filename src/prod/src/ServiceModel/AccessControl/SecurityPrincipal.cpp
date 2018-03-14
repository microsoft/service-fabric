// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace AccessControl;
using namespace Common;
using namespace Serialization;
using namespace std;

static StringLiteral TraceType("SecurityPrincipal");

const SecurityPrincipalType::Enum SecurityPrincipal::type_;

const SecurityPrincipalType::Enum SecurityPrincipalX509::type_;
DEFINE_PRINCIPAL_IDENTIFIER_TYPE(SecurityPrincipalX509)

const SecurityPrincipalType::Enum SecurityPrincipalClaim::type_;
DEFINE_PRINCIPAL_IDENTIFIER_TYPE(SecurityPrincipalClaim)

const SecurityPrincipalType::Enum SecurityPrincipalWindows::type_;
DEFINE_PRINCIPAL_IDENTIFIER_TYPE(SecurityPrincipalWindows)

namespace AccessControl
{
    namespace SecurityPrincipalType
    {
        bool IsValid(Enum value)
        {
            return
                (value == X509) ||
                (value == Windows) ||
                (value == Claim) ||
                (value == Role);
        }

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case Invalid: w << L"Invalid"; return;
            case X509: w << L"X509"; return;
            case Windows: w << L"Windows"; return;
            case Claim: w << L"Claim"; return;
            case Role: w << L"FabricRole"; return;
            }

            w << "UnknownSecurityPrincipalType(" << static_cast<int>(e) << ')';
        }

        BEGIN_ENUM_STRUCTURED_TRACE(SecurityPrincipalType)
            ADD_ENUM_MAP_VALUE_RANGE(SecurityPrincipalType, Invalid, Claim)
            ADD_ENUM_MAP_VALUE(SecurityPrincipalType, Role)
        END_ENUM_STRUCTURED_TRACE(SecurityPrincipalType)
    }
}

ErrorCode SecurityPrincipal::FromPublic(
    FABRIC_SECURITY_PRINCIPAL_IDENTIFIER const * input,
    SPtr & result)
{
    if (!input)
    {
        result = nullptr;
        return ErrorCodeValue::ArgumentNull;
    }

    switch (input->Kind)
    {
    case SecurityPrincipalType::X509:
        result = make_shared<SecurityPrincipalX509>(((FABRIC_SECURITY_X509_PRINCIPAL_IDENTIFIER*)input->Value)->CommonName);
        return result->Status();

    case SecurityPrincipalType::Claim:
        result = make_shared<SecurityPrincipalClaim>(((FABRIC_SECURITY_CLAIM_PRINCIPAL_IDENTIFIER*)input->Value)->Claim);
        return result->Status();

    case SecurityPrincipalType::Windows:
        result = make_shared<SecurityPrincipalWindows>(((FABRIC_SECURITY_WINDOWS_PRINCIPAL_IDENTIFIER*)input->Value)->AccountName);
        return result->Status();

    default:
        return ErrorCodeValue::InvalidArgument;
    }
}

IFabricSerializable * SecurityPrincipal::FabricSerializerActivator(FabricTypeInformation typeInformation)
{
    if (typeInformation.buffer == nullptr || typeInformation.length != sizeof(SecurityPrincipalType::Enum))
    {
        return nullptr;
    }

    switch (*((SecurityPrincipalType::Enum*)typeInformation.buffer))
    {
    case SecurityPrincipalType::X509:
        return new SecurityPrincipalX509();

    case SecurityPrincipalType::Claim:
        return new SecurityPrincipalClaim();

    case SecurityPrincipalType::Windows:
        return new SecurityPrincipalWindows();

    default:
        return nullptr;
    }
}

ErrorCode SecurityPrincipal::Status() const
{
    return error_;
}

bool SecurityPrincipal::CheckCommonName(std::wstring const &) const
{
    return false;
}

bool SecurityPrincipal::CheckClaim(Transport::SecuritySettings::RoleClaims const &) const
{
    return false;
}

bool SecurityPrincipal::CheckTokenMembership(HANDLE) const
{
    return false;
}

void SecurityPrincipal::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    switch (Type())
    {
    case SecurityPrincipalType::Invalid:
        w.Write("{0}:n/a", Type(), ((SecurityPrincipalX509*)this)->CommonName());
        return;

    case SecurityPrincipalType::X509:
        w.Write("{0}:{1}", Type(), ((SecurityPrincipalX509*)this)->CommonName());
        return;

    case SecurityPrincipalType::Claim:
        w.Write("{0}:{1}", Type(), ((SecurityPrincipalClaim*)this)->Claim());
        return;

    case SecurityPrincipalType::Windows:
        {
            wstring accountName;
            auto error = ((SecurityPrincipalWindows*)this)->GetAccountName(accountName);
            if (error.IsSuccess())
            {
                w.Write("{0}:{1}", Type(), accountName);
            }
            else
            {
                w.Write("{0}:{1}", Type(), error);
            }

            return;
        }

    default:
        w.Write("{0}:n/a", Type());
    }
}

string SecurityPrincipal::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    SecurityPrincipalType::Trace::AddField(traceEvent, name + ".type");
    traceEvent.AddField<wstring>(name + ".value");
    return "{0}:{1}";
}

void SecurityPrincipal::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<SecurityPrincipalType::Enum>(Type());
    switch (Type())
    {
    case SecurityPrincipalType::X509:
        context.Write(((SecurityPrincipalX509*)this)->CommonName());
        return;

    case SecurityPrincipalType::Claim:
        context.Write(((SecurityPrincipalClaim*)this)->Claim());
        return;

    case SecurityPrincipalType::Windows:
        {
            wstring accountName;
            auto error = ((SecurityPrincipalWindows*)this)->GetAccountName(accountName);
            if (error.IsSuccess())
            {
                context.Write(accountName);
            }
            else
            {
                context.Write(error.ErrorCodeValueToString());
            }

            return;
        }

    default:
        context.Write(wstring(L""));
    }
}

SecurityPrincipalX509::SecurityPrincipalX509()
{
}

SecurityPrincipalX509::SecurityPrincipalX509(wstring const & commonName) : commonName_(commonName)
{
    StringUtility::TrimSpaces(commonName_);
}

_Use_decl_annotations_
void SecurityPrincipalX509::ToPublic(Common::ScopedHeap & heap, FABRIC_SECURITY_PRINCIPAL_IDENTIFIER & securityPrincipal)
{
    securityPrincipal.Kind = FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND_X509;
    auto refPtr = heap.AddItem<FABRIC_SECURITY_X509_PRINCIPAL_IDENTIFIER>();
    securityPrincipal.Value = refPtr.GetRawPointer();
    refPtr->CommonName = heap.AddString(commonName_);
}

bool SecurityPrincipalX509::CheckCommonName(wstring const & commonName) const
{
    return StringUtility::AreEqualCaseInsensitive(commonName, commonName_);
}

wstring const & SecurityPrincipalX509::CommonName() const
{
    return commonName_;
}

SecurityPrincipalClaim::SecurityPrincipalClaim()
{
}

SecurityPrincipalClaim::SecurityPrincipalClaim(wstring const & claim)
{
    error_ = TryParseClaim(claim, claimType_, claimValue_);
}

_Use_decl_annotations_
void SecurityPrincipalClaim::ToPublic(Common::ScopedHeap & heap, FABRIC_SECURITY_PRINCIPAL_IDENTIFIER & securityPrincipal)
{
    securityPrincipal.Kind = FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND_CLAIM;
    auto refPtr = heap.AddItem<FABRIC_SECURITY_CLAIM_PRINCIPAL_IDENTIFIER>();
    securityPrincipal.Value = refPtr.GetRawPointer();
    refPtr->Claim = heap.AddString(Claim());
}

ErrorCode SecurityPrincipalClaim::TryParseClaim(
    wstring const & claim,
    wstring & claimType,
    wstring & claimValue)
{
    vector<wstring> claimParts;
    StringUtility::Split<wstring>(claim, claimParts, L"=", true);
    if (claimParts.size() != 2)
    {
        return ErrorCodeValue::InvalidArgument;
    }

    StringUtility::TrimWhitespaces(claimParts.front());
    claimType = claimParts.front();

    StringUtility::TrimWhitespaces(claimParts.back());
    claimValue = claimParts.back();

    if (claimType.empty() || claimValue.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCodeValue::Success;
}

bool SecurityPrincipalClaim::CheckClaim(Transport::SecuritySettings::RoleClaims const & claims) const
{
    return claims.Contains(claimType_, claimValue_);
}

wstring SecurityPrincipalClaim::Claim() const
{
    return claimType_ + L"=" + claimValue_;
}

SecurityPrincipalWindows::SecurityPrincipalWindows()
{
}

SecurityPrincipalWindows::SecurityPrincipalWindows(std::wstring const & accountName)
{
    error_ = BufferedSid::Create(accountName, sid_);
}

_Use_decl_annotations_
void SecurityPrincipalWindows::ToPublic(Common::ScopedHeap & heap, FABRIC_SECURITY_PRINCIPAL_IDENTIFIER & securityPrincipal)
{
    securityPrincipal.Kind = FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND_WINDOWS;
    auto refPtr = heap.AddItem<FABRIC_SECURITY_WINDOWS_PRINCIPAL_IDENTIFIER>();
    securityPrincipal.Value = refPtr.GetRawPointer();
    wstring accountName;
    auto error = GetAccountName(accountName);
    TESTASSERT_IFNOT(error.IsSuccess());
    refPtr->AccountName = heap.AddString(accountName);
}

bool SecurityPrincipalWindows::CheckTokenMembership(HANDLE token) const
{
#ifdef PLATFORM_UNIX
    Assert::CodingError("not implemented");
    return false;
#else
    BOOL isMember;
    if (::CheckTokenMembership(token, (PSID)sid_.data(), &isMember) == FALSE)
    {
        WriteWarning(TraceType, "CheckTokenMembership failed: {0}", ErrorCode::FromWin32Error());
        return false;
    }

    return isMember == TRUE;
#endif
}

_Use_decl_annotations_
ErrorCode SecurityPrincipalWindows::GetAccountName(wstring & accountName) const
{
    // This object was created by deserialization
    wstring domain;
    wstring account;
    auto error = Sid::LookupAccount((PSID)sid_.data(), domain, account);
    if (error.IsSuccess())
    {
        accountName = move(domain) +L"\\" + move(account);
        return error;
    }

    return Sid::ToString((PSID)sid_.data(), accountName);
}


