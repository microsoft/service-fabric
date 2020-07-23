// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <RepositoryCredentials> element.
    struct RepositoryCredentialsDescription : 
        public Serialization::FabricSerializable,
        public Common::IFabricJsonSerializable
    {
    public:
        RepositoryCredentialsDescription(); 

        RepositoryCredentialsDescription(RepositoryCredentialsDescription const & other) = default;
        RepositoryCredentialsDescription(RepositoryCredentialsDescription && other) = default;

        RepositoryCredentialsDescription & operator = (RepositoryCredentialsDescription const & other) = default;
        RepositoryCredentialsDescription & operator = (RepositoryCredentialsDescription && other) = default;

        bool operator == (RepositoryCredentialsDescription const & other) const;
        bool operator != (RepositoryCredentialsDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_REPOSITORY_CREDENTIAL_DESCRIPTION & fabricCredential) const;

        FABRIC_FIELDS_05(AccountName, Password, IsPasswordEncrypted, Email, Type);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(RepositoryCredentialsDescription::AccountNameParameter, AccountName)
            SERIALIZABLE_PROPERTY(RepositoryCredentialsDescription::PasswordParameter, Password)
            SERIALIZABLE_PROPERTY_IF(RepositoryCredentialsDescription::EmailParameter, Email, !Email.empty())
            SERIALIZABLE_PROPERTY(RepositoryCredentialsDescription::TypeParameter, Type)
        END_JSON_SERIALIZABLE_PROPERTIES()

    public:
        std::wstring AccountName;
        std::wstring Password;
        std::wstring Email;
        bool IsPasswordEncrypted;
        std::wstring Type;

        static Common::WStringLiteral const AccountNameParameter;
        static Common::WStringLiteral const PasswordParameter;
        static Common::WStringLiteral const EmailParameter;
        static Common::WStringLiteral const TypeParameter;

    private:
        friend struct DigestedCodePackageDescription;
        friend struct ServicePackagePoliciesDescription;
        friend struct ContainerPoliciesDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
DEFINE_USER_ARRAY_UTILITY(ServiceModel::RepositoryCredentialsDescription);
