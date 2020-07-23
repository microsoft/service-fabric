// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace CentralSecretService
    {
        class SecretMetadataDataItem : public SecretDataItemBase
        {
        public:
            SecretMetadataDataItem();
            SecretMetadataDataItem(std::wstring const & name);
            SecretMetadataDataItem(SecretReference const &);
            SecretMetadataDataItem(Secret const &);

            virtual ~SecretMetadataDataItem();

            __declspec(property(get = get_LatestVersionNumber, put = put_LatestVersionNumber)) int const & LatestVersionNumber;
            int const & get_LatestVersionNumber() const { return latestVersionNumber_; }
            void put_LatestVersionNumber(int const value) { latestVersionNumber_ = value; }

            __declspec(property(get = get_LatestVersion)) wstring const LatestVersion;
            wstring const get_LatestVersion() const;

            std::wstring const & get_Type() const override { return Constants::StoreType::SecretsMetadata; }

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const override;

            SecretReference ToSecretReference() const override;
            Secret ToSecret() const override;

            virtual bool IsValidUpdate(SecretMetadataDataItem const &) const;
            virtual bool TryUpdateTo(SecretMetadataDataItem const &);

            FABRIC_FIELDS_02(
                key_,
                latestVersionNumber_)

        protected:
            std::wstring ConstructKey() const override;

        private:
            int latestVersionNumber_;
            std::wstring key_;          // holds the 'normalized' (lower case) name
        };
    }
}