// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class HttpRouteMatchHeader;
        using HttpRouteMatchHeaderSPtr = std::shared_ptr<HttpRouteMatchHeader>;

        class HttpRouteMatchHeader
            : public ModelType
        {
        public:
            __declspec(property(get = get_Name)) std::wstring const & Name;
            std::wstring const & get_Name() const { return name_; }

            __declspec(property(get = get_Value)) std::wstring const & Value;
            std::wstring const & get_Value() const { return value_; }

            __declspec(property(get = get_Type)) HeaderMatchType::Enum const & Type;
            HeaderMatchType::Enum const & get_Type() const { return type_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(L"name", name_)
                SERIALIZABLE_PROPERTY(L"value", value_)
                SERIALIZABLE_PROPERTY_ENUM(L"type", type_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_03(name_, value_, type_);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(name_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(value_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(type_)
            END_DYNAMIC_SIZE_ESTIMATION()

            Common::ErrorCode TryValidate(std::wstring const & traceId) const override;

            void WriteTo(Common::TextWriter &, Common::FormatOptions const &) const;

            std::wstring name_;
            std::wstring value_;
            HeaderMatchType::Enum type_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ModelV2::HttpRouteMatchHeader);