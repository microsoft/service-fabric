// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationInstance>/<Environment>/<Principals>/<Users>/<User>
    struct PrincipalsDescription;
    struct SecurityUserDescription : public Serialization::FabricSerializable
    {
    public:
        SecurityUserDescription();
        SecurityUserDescription(SecurityUserDescription const & other);
        SecurityUserDescription(SecurityUserDescription && other);

        SecurityUserDescription const & operator = (SecurityUserDescription const & other);
        SecurityUserDescription const & operator = (SecurityUserDescription && other);

        bool operator == (SecurityUserDescription const & other) const;
        bool operator != (SecurityUserDescription const & other) const;

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
        void ToPublicApi(__in Common::ScopedHeap & heap, __in std::map<std::wstring, std::wstring> const& sids, __out FABRIC_SECURITY_USER_DESCRIPTION & publicDescription) const;

        void clear();
        FABRIC_FIELDS_10(Name, AccountName, AccountType, Password, IsPasswordEncrypted, ParentSystemGroups, ParentApplicationGroups, PerformInteractiveLogon, LoadProfile, NTLMAuthenticationPolicy);

    public:
        std::wstring Name;
        std::wstring AccountName;
        bool IsPasswordEncrypted;
        std::wstring Password;        
        
        SecurityPrincipalAccountType::Enum AccountType;
        bool PerformInteractiveLogon;
        bool LoadProfile;

        NTLMAuthenticationPolicyDescription NTLMAuthenticationPolicy;

        std::vector<std::wstring> ParentSystemGroups;
        std::vector<std::wstring> ParentApplicationGroups;

    private:
        friend struct PrincipalsDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        void ParseMemberOf(Common::XmlReaderUPtr const & xmlReader);

		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
		
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::SecurityUserDescription);
