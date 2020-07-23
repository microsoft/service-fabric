// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class HttpRouteMatchPath;
        using HttpRouteMatchPathSPtr = std::shared_ptr<HttpRouteMatchPath>;

        class HttpRouteMatchPath
            : public ModelType
        {
        public:
            __declspec(property(get = get_Value)) std::wstring const & Value;
            std::wstring const & get_Value() const { return value_; }

            __declspec(property(get = get_Rewrite)) std::wstring const & Rewrite;
            std::wstring const & get_Rewrite() const { return rewrite_; }

            __declspec(property(get = get_Type)) PathMatchType::Enum const & Type;
            PathMatchType::Enum const & get_Type() const { return type_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(L"value", value_)
                SERIALIZABLE_PROPERTY(L"rewrite", rewrite_)
                SERIALIZABLE_PROPERTY_ENUM(L"type", type_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_03(value_, rewrite_, type_);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(value_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(rewrite_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(type_)
            END_DYNAMIC_SIZE_ESTIMATION()

            Common::ErrorCode TryValidate(std::wstring const & traceId) const override;

            void WriteTo(Common::TextWriter &, Common::FormatOptions const &) const
            {
                return;
            }

            std::wstring value_;
            std::wstring rewrite_;
            PathMatchType::Enum type_;
        };
    }
}
