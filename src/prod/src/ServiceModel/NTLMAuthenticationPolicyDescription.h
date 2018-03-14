// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{    
    struct SecurityUserDescription;
    struct NTLMAuthenticationPolicyDescription : public Serialization::FabricSerializable
    {
    public:
        NTLMAuthenticationPolicyDescription();
        NTLMAuthenticationPolicyDescription(NTLMAuthenticationPolicyDescription const & other);
        NTLMAuthenticationPolicyDescription(NTLMAuthenticationPolicyDescription && other);

        NTLMAuthenticationPolicyDescription const & operator = (NTLMAuthenticationPolicyDescription const & other);
        NTLMAuthenticationPolicyDescription const & operator = (NTLMAuthenticationPolicyDescription && other);

        bool operator == (NTLMAuthenticationPolicyDescription const & other) const;
        bool operator != (NTLMAuthenticationPolicyDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

        FABRIC_FIELDS_06(IsEnabled, PasswordSecret, IsPasswordSecretEncrypted, X509StoreLocation, X509StoreName, X509Thumbprint);

    public:
        bool IsEnabled;
        std::wstring PasswordSecret;
        bool IsPasswordSecretEncrypted;
        Common::X509StoreLocation::Enum X509StoreLocation;
        std::wstring X509StoreName;
        std::wstring X509Thumbprint;

    private:
        friend struct SecurityUserDescription;
        friend struct SecurityGroupDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &, bool const isPasswordSecretOptional);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter, bool const isPasswordSecretOptional);
    };
}
