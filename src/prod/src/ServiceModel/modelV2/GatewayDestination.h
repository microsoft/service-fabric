// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class GatewayDestination;
        using GatewayDestinationSPtr = std::shared_ptr<GatewayDestination>;

        class GatewayDestination
            : public ModelType
        {
        public:
            __declspec(property(get = get_ApplicationName)) std::wstring const & ApplicationName;
            std::wstring const & get_ApplicationName() const { return applicationName_; }

            __declspec(property(get = get_ServiceName)) std::wstring const & ServiceName;
            std::wstring const & get_ServiceName() const { return serviceName_; }

            __declspec(property(get = get_EndpointName)) std::wstring const & EndpointName;
            std::wstring const & get_EndpointName() const { return endpointName_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(L"applicationName", applicationName_)
                SERIALIZABLE_PROPERTY(L"serviceName", serviceName_)
                SERIALIZABLE_PROPERTY(L"endpointName", endpointName_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_03(applicationName_, serviceName_, endpointName_);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationName_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceName_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(endpointName_)
            END_DYNAMIC_SIZE_ESTIMATION()

            Common::ErrorCode TryValidate(std::wstring const & traceId) const override;

            void WriteTo(Common::TextWriter &, Common::FormatOptions const &) const
            {
                return;
            }

            Common::ErrorCode FromPublicApi(TRAFFIC_ROUTING_DESTINATION const & publicRoutingDestination);
            void ToPublicApi(__in Common::ScopedHeap &, __out TRAFFIC_ROUTING_DESTINATION &) const;

            std::wstring applicationName_;
            std::wstring serviceName_;
            std::wstring endpointName_;
        };
    }
}
