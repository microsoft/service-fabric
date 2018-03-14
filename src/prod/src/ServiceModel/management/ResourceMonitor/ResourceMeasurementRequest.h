// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ResourceMonitor
    {
        class ResourceMeasurementRequest : public Serialization::FabricSerializable
        {
        public:

            static Common::GlobalWString ResourceMeasureRequestAction;

            ResourceMeasurementRequest() = default;
            ResourceMeasurementRequest(std::vector<std::wstring> && hosts, std::wstring const & nodeId);

            __declspec(property(get=get_Hosts)) std::vector<std::wstring> const & Hosts;
            std::vector<std::wstring> const & get_Hosts() const { return hosts_; }

            __declspec(property(get=get_NodeId)) std::wstring const & NodeId;
            std::wstring const & get_NodeId() const { return nodeId_; }

            FABRIC_FIELDS_02(hosts_, nodeId_);

        private:
            std::vector<std::wstring> hosts_;
            std::wstring nodeId_;
        };
    }
}
