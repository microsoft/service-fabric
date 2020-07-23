// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class HttpRouteMatchRule;
        using HttpRouteMatchRuleSPtr = std::shared_ptr<HttpRouteMatchRule>;

        class HttpRouteMatchRule
            : public ModelType
        {
        public:
            __declspec(property(get = get_RouteMatchPath)) HttpRouteMatchPath const & RouteMatchPath;
            HttpRouteMatchPath const & get_RouteMatchPath() const { return routeMatchPath_; }

            __declspec(property(get = get_Headers)) std::vector<HttpRouteMatchHeader> const & Headers;
            std::vector<HttpRouteMatchHeader> const & get_Headers() const { return routeMatchHeaders_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(L"path", routeMatchPath_)
                SERIALIZABLE_PROPERTY_IF(L"headers", routeMatchHeaders_, (routeMatchHeaders_.size() != 0))
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_02(routeMatchPath_, routeMatchHeaders_);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(routeMatchPath_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(routeMatchHeaders_)
            END_DYNAMIC_SIZE_ESTIMATION()

            Common::ErrorCode TryValidate(std::wstring const & traceId) const override;

            void WriteTo(Common::TextWriter &, Common::FormatOptions const &) const
            {
                return;
            }

            HttpRouteMatchPath routeMatchPath_;
            std::vector<HttpRouteMatchHeader> routeMatchHeaders_;
        };
    }
}
