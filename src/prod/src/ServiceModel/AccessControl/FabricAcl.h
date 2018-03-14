// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace AccessControl
{
    namespace FabricAceType
    {
        enum Enum
        {
            Invalid = FABRIC_SECURITY_ACE_TYPE_INVALID,
            Allowed = FABRIC_SECURITY_ACE_TYPE_ALLOWED,
        };

        bool IsValid(Enum value);
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

        DECLARE_ENUM_STRUCTURED_TRACE(FabricAceType);
    }

    class FabricAce
        : public Serialization::FabricSerializable
        , public Common::TextTraceComponent<Common::TraceTaskCodes::AccessControl>
    {
    public:
        static Common::ErrorCode FromPublic(const FABRIC_SECURITY_ACE & ace, _Out_ FabricAce & result);
        void ToPublic(Common::ScopedHeap & heap, _Out_ FABRIC_SECURITY_ACE & fabricAce) const;

        FabricAce();

        FabricAceType::Enum Type() const;
        SecurityPrincipal const & Principal() const;
        DWORD AccessMask() const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;

        FABRIC_FIELDS_03(type_, principal_, accessMask_);

    private:
        FabricAceType::Enum type_;
        SecurityPrincipal::SPtr principal_;
        uint32 accessMask_;
    };

    class FabricAcl
        : public Serialization::FabricSerializable
        , public Common::TextTraceComponent<Common::TraceTaskCodes::AccessControl>
    {
    public:
        static Common::ErrorCode FromPublic(const FABRIC_SECURITY_ACL * acl, _Out_ FabricAcl & result);
        void ToPublic(Common::ScopedHeap & heap, _Out_ FABRIC_SECURITY_ACL & fabricAcl) const;

        FabricAcl();

        bool AccessCheckX509(std::wstring const & commonName, DWORD desiredAccess) const;
        bool AccessCheckClaim(Transport::SecuritySettings::RoleClaims const & claims, DWORD desiredAccess) const;
        bool AccessCheckWindows(HANDLE token, DWORD desiredAccess) const;

        std::vector<FabricAce> const & AceList() const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_01(aceList_);

    private:
        std::vector<FabricAce> aceList_;
    };
}

DEFINE_USER_ARRAY_UTILITY(AccessControl::FabricAce);
