// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class HttpHostNameConfig;
        using HttpHostNameConfigSPtr = std::shared_ptr<HttpHostNameConfig>;

        class HttpHostNameConfig
            : public ModelType
        {
        public:
            __declspec(property(get = get_Name)) std::wstring const & Name;
            std::wstring const & get_Name() const { return name_; }

            __declspec(property(get = get_Routes)) std::vector<HttpRouteConfig> const & Routes;
            std::vector<HttpRouteConfig> const & get_Routes() const { return routes_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(L"name", name_)
                SERIALIZABLE_PROPERTY_IF(L"routes", routes_, (routes_.size() != 0))
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_02(name_, routes_);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(name_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(routes_)
            END_DYNAMIC_SIZE_ESTIMATION()

            Common::ErrorCode TryValidate(std::wstring const & traceId) const override;

            void WriteTo(Common::TextWriter &, Common::FormatOptions const &) const;

            std::wstring name_;
            std::vector<HttpRouteConfig> routes_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ModelV2::HttpHostNameConfig);
