// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class TypeSafeString : public Serialization::FabricSerializable
        {
        public:
            TypeSafeString() : value_(L"") { }

            explicit TypeSafeString(std::wstring const & value) : value_(value) { }
            explicit TypeSafeString(std::wstring && value) : value_(std::move(value)) { }

            explicit TypeSafeString(TypeSafeString const & other) : value_(other.value_) { }
            explicit TypeSafeString(TypeSafeString && other) : value_(std::move(other.value_)) { }

            __declspec(property(get=get_Value)) std::wstring const & Value;
            std::wstring const & get_Value() const { return value_; }

            size_t size() const { return value_.size(); }

            void operator = ( TypeSafeString const & other ) { value_ = other.value_; }

            void operator = ( TypeSafeString && other ) { value_ = std::move(other.value_); }

            bool operator == ( TypeSafeString const & other ) const { return Common::StringUtility::AreEqualCaseInsensitive(value_, other.value_); }
            bool operator != ( TypeSafeString const & other ) const { return !(*this == other); }

            bool operator < ( TypeSafeString const & other ) const { return value_ < other.value_; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const { w << value_; }

            FABRIC_FIELDS_01(value_);

        public:

            struct Hasher
            {
                size_t operator() (TypeSafeString const & key) const 
                {
                    return Common::StringUtility::GetHash(key.Value);
                };

                bool operator() (TypeSafeString const & left, TypeSafeString const & right) const 
                {
                    return (left == right);
                }
            };

        protected:
            std::wstring value_;
        };
    }
}
