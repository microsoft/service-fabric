// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace CentralSecretService
    {
        class SecretDataItem : public SecretDataItemBase
        {
            DENY_COPY_ASSIGNMENT(SecretDataItem)

        public:
            SecretDataItem();
            
            SecretDataItem(std::wstring const & name, std::wstring const & version);
            SecretDataItem(
                std::wstring const & name,
                std::wstring const & version,
                std::wstring const & value,
                std::wstring const & kind,
                std::wstring const & contentType,
                std::wstring const & description);
            SecretDataItem(SecretReference const &);
            SecretDataItem(Secret const &);

            virtual ~SecretDataItem();

            __declspec(property(get = get_Version)) std::wstring const & Version;
            std::wstring const & get_Version() const { return version_; }

            __declspec(property(get = get_Value)) std::wstring const & Value;
            std::wstring const & get_Value() const { return value_; }

            std::wstring const & get_Type() const override { return Constants::StoreType::Secrets; }

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const override;

            SecretReference ToSecretReference() const override;
            Secret ToSecret() const override;

            static std::wstring ToKeyName(wstring const & secretName, wstring const & secretVersion = L"");

            inline bool operator == (SecretDataItem const & rhs)
            {
                return (((SecretDataItemBase &)*this) == (SecretDataItemBase const &)rhs)
                    && (0 == version_.compare(rhs.version_))
                    && (0 == value_.compare(rhs.value_));
            }

            FABRIC_FIELDS_02(
                version_,
                value_)

        protected:
            std::wstring ConstructKey() const override;

        private:
            std::wstring version_;
            std::wstring value_;
        };
    }
}