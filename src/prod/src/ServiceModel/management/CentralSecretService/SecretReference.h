// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace CentralSecretService
    {
        class SecretReference : public ServiceModel::ClientServerMessageBody
        {
        public:
            SecretReference();
            SecretReference(std::wstring const & name);
            SecretReference(std::wstring const & name, std::wstring const & version);

            __declspec (property(get = get_Name)) std::wstring const & Name;
            std::wstring const & get_Name() const { return name_; }

            __declspec (property(get = get_Version)) std::wstring const & Version;
            std::wstring const & get_Version() const { return version_; }

            Common::ErrorCode FromPublicApi(__in FABRIC_SECRET_REFERENCE const & reference);
            Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_SECRET_REFERENCE & reference) const;
            Common::ErrorCode Validate(bool strict = false, bool asResource = true) const;

            void WriteTo(__in Common::TextWriter & writer, Common::FormatOptions const & options) const;

            std::wstring ToResourceName() const;

            static Common::ErrorCode FromResourceName(
                __in std::wstring const & resourceName, 
                __out SecretReference & secretRef);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN()
                SERIALIZABLE_PROPERTY(L"Name", name_)
                SERIALIZABLE_PROPERTY_IF(L"Version", version_, !version_.empty())
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_02(
                name_,
                version_)
        private:
            std::wstring name_;
            std::wstring version_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::CentralSecretService::SecretReference)