// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationInstance>/<Environment>/<Principals>/<Groups>/<Group>
    struct PrincipalsDescription;
    struct SecurityGroupDescription : public Serialization::FabricSerializable
    {
    public:
        SecurityGroupDescription();
        SecurityGroupDescription(SecurityGroupDescription const & other);
        SecurityGroupDescription(SecurityGroupDescription && other);

        SecurityGroupDescription const & operator = (SecurityGroupDescription const & other);
        SecurityGroupDescription const & operator = (SecurityGroupDescription && other);

        bool operator == (SecurityGroupDescription const & other) const;
        bool operator != (SecurityGroupDescription const & other) const;

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
        void ToPublicApi(__in Common::ScopedHeap & heap, __in std::map<std::wstring, std::wstring> const& sids, __out FABRIC_SECURITY_GROUP_DESCRIPTION & publicDescription) const;

        void clear();
        FABRIC_FIELDS_05(Name, DomainGroupMembers, SystemGroupMembers, DomainUserMembers, NTLMAuthenticationPolicy);


    public:
        std::wstring Name;
        std::vector<std::wstring> DomainGroupMembers;
        std::vector<std::wstring> SystemGroupMembers;
        std::vector<std::wstring> DomainUserMembers;

        NTLMAuthenticationPolicyDescription NTLMAuthenticationPolicy;

    private:
        friend struct PrincipalsDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        void ParseMembership(Common::XmlReaderUPtr const & xmlReader);

		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::SecurityGroupDescription);

