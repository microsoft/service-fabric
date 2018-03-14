// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ResourceMonitor
    {
        class ResourceMeasurement;
        class ResourceMeasurementResponse : public Serialization::FabricSerializable
        {
        public:

            ResourceMeasurementResponse() = default;
            ResourceMeasurementResponse(std::map<std::wstring, ResourceMeasurement> && resourceUsageReports);

            __declspec(property(get=get_ResourceUsageMeasurement)) std::map<std::wstring, ResourceMeasurement> const & Measurements;
            std::map<std::wstring, ResourceMeasurement> const & get_ResourceUsageMeasurement() const { return resourceMeasurementResponse_; }

            FABRIC_FIELDS_01(resourceMeasurementResponse_);

        private:
            std::map<std::wstring, ResourceMeasurement> resourceMeasurementResponse_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::ResourceMonitor::ResourceMeasurement);
DEFINE_USER_MAP_UTILITY(std::wstring, Management::ResourceMonitor::ResourceMeasurement);
