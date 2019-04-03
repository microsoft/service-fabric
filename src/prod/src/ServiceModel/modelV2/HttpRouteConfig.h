// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class HttpRouteConfig;
        using HttpRouteConfigSPtr = std::shared_ptr<HttpRouteConfig>;

        class HttpRouteConfig
            : public ModelType
        {
        public:
            __declspec(property(get = get_Name)) std::wstring const & Name;
            std::wstring const & get_Name() const { return name_; }

            __declspec(property(get = get_Destination)) GatewayDestination const & Destination;
            GatewayDestination const & get_Destination() const { return destination_; }

            __declspec(property(get = get_RouteMatchRule)) HttpRouteMatchRule const & RouteMatchRule;
            HttpRouteMatchRule const & get_RouteMatchRule() const { return routeMatchRule_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(L"name", name_)
                SERIALIZABLE_PROPERTY(L"match", routeMatchRule_)
                SERIALIZABLE_PROPERTY(L"destination", destination_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_03(name_, routeMatchRule_, destination_);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(name_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(routeMatchRule_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(destination_)
            END_DYNAMIC_SIZE_ESTIMATION()

            Common::ErrorCode TryValidate(std::wstring const & traceId) const override;

            void WriteTo(Common::TextWriter &, Common::FormatOptions const &) const;

            std::wstring name_;
            HttpRouteMatchRule routeMatchRule_;
            GatewayDestination destination_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ModelV2::HttpRouteConfig);
