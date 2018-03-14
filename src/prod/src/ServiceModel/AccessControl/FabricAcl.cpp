// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace AccessControl;
using namespace Common;
using namespace std;

namespace AccessControl
{
    namespace FabricAceType
    {
        bool IsValid(Enum value)
        {
            return value == FABRIC_SECURITY_ACE_TYPE_ALLOWED;
        }

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case Invalid: w << L"Invalid"; return;
            case Allowed: w << L"Allowed"; return;
            }

            w << "UnknownFabricAceType(" << static_cast<int>(e) << ')';
        }

        ENUM_STRUCTURED_TRACE(FabricAceType, Invalid, Allowed);
    }
}

_Use_decl_annotations_
ErrorCode FabricAce::FromPublic(const FABRIC_SECURITY_ACE & ace, FabricAce & result)
{
    result.type_ = (FabricAceType::Enum)ace.AceType;
    if (!FabricAceType::IsValid(result.type_))
    {
        return ErrorCodeValue::InvalidArgument;
    }

    auto error = SecurityPrincipal::FromPublic(ace.Principal, result.principal_);
    if (!error.IsSuccess())
    {
        return error;
    }

    result.accessMask_ = ace.AccessMask;
    return error;
}

_Use_decl_annotations_
void FabricAce::ToPublic(ScopedHeap & heap, FABRIC_SECURITY_ACE & fabricAce) const
{
    fabricAce.AceType = (FABRIC_SECURITY_ACE_TYPE)type_;
    auto refPtr = heap.AddItem<FABRIC_SECURITY_PRINCIPAL_IDENTIFIER>();
    fabricAce.Principal = refPtr.GetRawPointer();
    principal_->ToPublic(heap, *refPtr);
    fabricAce.AccessMask = accessMask_;
    fabricAce.Reserved = nullptr;
}

FabricAce::FabricAce() : type_(FabricAceType::Invalid), accessMask_(0)
{
}

FabricAceType::Enum FabricAce::Type() const
{
    return type_;
}

SecurityPrincipal const & FabricAce::Principal() const
{
    return *principal_;
}

DWORD FabricAce::AccessMask() const
{
    return accessMask_;
}

void FabricAce::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("{0},{1},{2:x}", type_, *principal_, accessMask_);
}

string FabricAce::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    FabricAceType::Trace::AddField(traceEvent, name + ".type");
    SecurityPrincipal::AddField(traceEvent, name + ".principal");
    traceEvent.AddField<uint32>(name + ".accessMask");
    return "{0},{1},{2:x}";
}

void FabricAce::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<FabricAceType::Enum>(type_);
    context.Write<SecurityPrincipal>(*principal_);
    context.Write<uint32>(accessMask_);
}

FabricAcl::FabricAcl()
{
}

_Use_decl_annotations_
ErrorCode FabricAcl::FromPublic(const FABRIC_SECURITY_ACL * acl, FabricAcl & result)
{
    result = FabricAcl();
    if (!acl)
    {
        return ErrorCodeValue::ArgumentNull;
    }

    for (ULONG i = 0; i < acl->AceCount; ++i)
    {
        FabricAce ace;
        auto error = FabricAce::FromPublic(acl->AceItems[i], ace);
        if (!error.IsSuccess())
        {
            return error;
        }

        result.aceList_.emplace_back(move(ace));
    }

    return ErrorCodeValue::Success;
}

_Use_decl_annotations_
void FabricAcl::ToPublic(ScopedHeap & heap, FABRIC_SECURITY_ACL & fabricAcl) const
{
    fabricAcl.AceCount = (ULONG)aceList_.size();
    fabricAcl.Reserved = nullptr;
    auto refArray = heap.AddArray<FABRIC_SECURITY_ACE>(fabricAcl.AceCount);
    fabricAcl.AceItems = refArray.GetRawArray();

    for (ULONG i = 0; i < fabricAcl.AceCount; ++ i)
    {
        aceList_[i].ToPublic(heap, refArray[i]);
    }
}

bool FabricAcl::AccessCheckX509(wstring const & commonName, DWORD desiredAccess) const
{
    DWORD accessGranted = 0;
    for (auto const & ace : aceList_)
    {
        if ((ace.Type() == FabricAceType::Allowed) && ace.Principal().CheckCommonName(commonName))
        {
            accessGranted |= ace.AccessMask();
            if ((accessGranted & desiredAccess) == desiredAccess)
            {
                return true;
            }
        }
    }

    return false;
}

bool FabricAcl::AccessCheckClaim(Transport::SecuritySettings::RoleClaims const & claims, DWORD desiredAccess) const
{
    DWORD accessGranted = 0;
    for (auto const & ace : aceList_)
    {
        if ((ace.Type() == FabricAceType::Allowed) && ace.Principal().CheckClaim(claims))
        {
            accessGranted |= ace.AccessMask();
            if ((accessGranted & desiredAccess) == desiredAccess)
            {
                return true;
            }
        }
    }

    return false;
}

bool FabricAcl::AccessCheckWindows(HANDLE token, DWORD desiredAccess) const
{
    // Possible optimization: create a security descriptor from all Windows ACE and cache it,
    // for each access check, we just need to call ::AccessCheck() on the security descriptor.
    DWORD accessGranted = 0;
    for (auto const & ace : aceList_)
    {
        if ((ace.Type() == FabricAceType::Allowed) && ace.Principal().CheckTokenMembership(token))
        {
            accessGranted |= ace.AccessMask();
            if ((accessGranted & desiredAccess) == desiredAccess)
            {
                return true;
            }
        }
    }

    return false;
}

vector<FabricAce> const & FabricAcl::AceList() const
{
    return aceList_;
}

void FabricAcl::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    for (auto const & ace : aceList_)
    {
        w.Write("[{0}]", ace);
    }
}

void FabricAcl::WriteToEtw(uint16 contextSequenceId) const
{
    wstring temp;
    StringWriter(temp).Write(*this);
    CommonEventSource::Events->TraceDynamicWString(contextSequenceId, temp);
}
