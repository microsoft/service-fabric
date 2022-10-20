// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace CentralSecretService
    {
        ///
        /// Abstract base class for entities stored in the Secret Store.
        ///
        class SecretDataItemBase : public Store::StoreData
        {
            DENY_COPY_ASSIGNMENT(SecretDataItemBase)

        public:
            inline SecretDataItemBase()
                : StoreData()
                , name_()
                , kind_()
                , contentType_()
                , description_()
            {}

            inline SecretDataItemBase(std::wstring const & name)
                : StoreData()
                , name_(name)
                , kind_()
                , contentType_()
                , description_()
            {}
                
            inline SecretDataItemBase(
                std::wstring const & name,
                std::wstring const & kind,
                std::wstring const & contentType,
                std::wstring const & description)
                : StoreData()
                , name_(name)
                , kind_(kind)
                , contentType_(contentType)
                , description_(description)
            {}

            inline virtual ~SecretDataItemBase()
            {}


            //
            // accessors
            //

            __declspec(property(get = get_Name)) std::wstring const & Name;
            std::wstring const & get_Name() const { return name_; }

            __declspec(property(get = get_Kind)) std::wstring const & Kind;
            std::wstring const & get_Kind() const { return kind_; }

            __declspec(property(get = get_ContentType)) std::wstring const & ContentType;
            std::wstring const & get_ContentType() const { return contentType_; }

            __declspec(property(get = get_Description, put = put_Description)) std::wstring const & Description;
            std::wstring const & get_Description() const { return description_; }
            void put_Description(std::wstring const & value) { description_ = value; }

            std::wstring const & get_Type() const override = 0;

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const override = 0;

            virtual SecretReference ToSecretReference() const = 0;
            virtual Secret ToSecret() const = 0;

            inline virtual bool operator ==(SecretDataItemBase const & rhs)
            {
                return (0 == name_.compare(rhs.name_))
                    && (0 == kind_.compare(rhs.kind_))
                    && (0 == contentType_.compare(rhs.contentType_))
                    && (0 == description_.compare(rhs.description_));
            }

            FABRIC_FIELDS_04(
                name_,
                kind_,
                contentType_,
                description_)

        protected:
            std::wstring ConstructKey() const override = 0;

        private:
            std::wstring name_;
            std::wstring kind_;
            std::wstring contentType_;
            std::wstring description_;
        };
    }
}
