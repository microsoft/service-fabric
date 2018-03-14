// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class SecurityPrincipalInformation : public Serialization::FabricSerializable
    {

    public:
        SecurityPrincipalInformation();
        SecurityPrincipalInformation(
            std::wstring const & name,
            std::wstring const & principalId,
            std::wstring const & sid,
            bool isLocalUser);

        SecurityPrincipalInformation(SecurityPrincipalInformation const & other);
        SecurityPrincipalInformation(SecurityPrincipalInformation && other);

        SecurityPrincipalInformation const & operator = (SecurityPrincipalInformation const & other);
        SecurityPrincipalInformation const & operator = (SecurityPrincipalInformation && other);

        bool operator == (SecurityPrincipalInformation const & other) const;
        bool operator != (SecurityPrincipalInformation const & other) const;

        __declspec(property(get=get_Name)) std::wstring const & Name;
        std::wstring const & get_Name() const { return name_; }

         __declspec(property(get=get_SecurityPrincipalId)) std::wstring const & SecurityPrincipalId;
        std::wstring const & get_SecurityPrincipalId() const { return principalId_; }

        __declspec(property(get=get_SidString)) std::wstring const & SidString;
        std::wstring const & get_SidString() const { return sid_; }

        __declspec(property(get = get_IsLocalPrincipal)) bool const & IsLocalUser;
        bool const & get_IsLocalPrincipal() const { return isLocal_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_04(name_, principalId_, sid_, isLocal_);

    private:
        std::wstring name_;
        std::wstring principalId_;
        std::wstring sid_;
        bool isLocal_;
    };
}
DEFINE_USER_ARRAY_UTILITY(Hosting2::SecurityPrincipalInformation);
