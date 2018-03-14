// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ResourceMonitor
    {
        class ApplicationHostEvent;

        class HostUpdateRM : public Serialization::FabricSerializable
        {
        public:

            static Common::GlobalWString HostUpdateRMAction;

            HostUpdateRM() = default;
            HostUpdateRM(std::vector<ApplicationHostEvent> && events);

            __declspec(property(get=get_Events)) std::vector<ApplicationHostEvent> const & HostEvents;
            std::vector<ApplicationHostEvent> const &  get_Events() const { return events_; }

            FABRIC_FIELDS_01(events_);

        private:
            std::vector<ApplicationHostEvent> events_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::ResourceMonitor::ApplicationHostEvent);
