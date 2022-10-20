// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace CentralSecretService
    {
        class Secret : public ServiceModel::ClientServerMessageBody
        {
        public:
            Secret();
            Secret(std::wstring const & name);

            Secret(
                std::wstring const & name,
                std::wstring const & version, 
                std::wstring const & value,
                std::wstring const & kind,
                std::wstring const & contentType,
                std::wstring const & description);
            virtual ~Secret();

            __declspec (property(get = get_Name)) std::wstring const & Name;
            std::wstring const & get_Name() const { return name_; }

            __declspec (property(get = get_Version, put = set_Version)) std::wstring const & Version;
            std::wstring const & get_Version() const { return version_; }
            void set_Version(std::wstring const & version) { this->version_ = version; }

            __declspec (property(get = get_Value, put = set_Value)) std::wstring const & Value;
            std::wstring const & get_Value() const { return value_; }
            void set_Value(std::wstring const & value) { this->value_ = value; }

            __declspec (property(get = get_Kind)) std::wstring const & Kind;
            std::wstring const & get_Kind() const { return kind_; }

            __declspec (property(get = get_ContentType)) std::wstring const & ContentType;
            std::wstring const & get_ContentType() const { return contentType_; }

            __declspec (property(get = get_Description)) std::wstring const & Description;
            std::wstring const & get_Description() const { return description_; }

            Common::ErrorCode FromPublicApi(__in FABRIC_SECRET const & secret);
            Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_SECRET & secret) const;
            Common::ErrorCode Validate() const;

            ///
            /// The public model of the secret distinguishes between the secret resource as a container
            /// of metadata and versioned values on one side, and the value of a single version of a 
            /// secret resource on the other. Currently, there is a single data store for both types of
            /// object, which uses the Secret data type for each entry. The difference between entry types
            /// is inferred, then, from the content of the incoming object (the public API model), as follows:
            ///
            ///   - a secret resource incoming object may not specify a version or a value - the value
            ///     field is always empty. A secret resource must always specify a kind; it may or may not 
            ///    specify values for the ContentType and Description fields, respectively.
            ///
            ///   - a  secret value incoming object must specify a value for the version and the value fields,
            ///     respectively. It may not specify values for any of the other fields - content type, description,
            ///     or kind.
            ///
            /// The following methods attempt to detect what type of object is being represented by the Secret
            /// object, and are meant to be used upon adding the object to the store. XORing these methods for
            /// an incoming object must be true.
            /// 
            static bool TryValidateAsSecretResource(Secret const & secret);
            static Common::ErrorCode ValidateAsSecretResource(Secret const & secret);

            static bool TryValidateAsVersionedSecretValue(Secret const & secret);
            static Common::ErrorCode ValidateAsVersionedSecretValue(Secret const & secret);

            void WriteTo(__in Common::TextWriter & writer, Common::FormatOptions const & options) const;

            std::wstring ToResourceName() const;

            static bool ValidateCharacters(wstring const & secretName);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN()
                SERIALIZABLE_PROPERTY(L"Name", name_)
                SERIALIZABLE_PROPERTY(L"Version", version_)
                SERIALIZABLE_PROPERTY_IF(L"Value", value_, !value_.empty())
                SERIALIZABLE_PROPERTY(L"Kind", kind_)
                SERIALIZABLE_PROPERTY(L"ContentType", contentType_)
                SERIALIZABLE_PROPERTY(L"Description", description_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_06(
                name_,
                version_,
                value_,
                kind_,
                contentType_,
                description_)

        private:
            std::wstring name_;
            std::wstring version_;
            std::wstring value_;
            std::wstring kind_;
            std::wstring contentType_;
            std::wstring description_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::CentralSecretService::Secret)