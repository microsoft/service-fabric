// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class ContainerEventNotification : public Serialization::FabricSerializable
    {
    public:
        ContainerEventNotification();

        __declspec(property(get = get_SinceTime)) int64 SinceTime;
        int64 get_SinceTime() const { return sinceTime_; }

        __declspec(property(get = get_UntilTime)) int64 UntilTime;
        int64 get_UntilTime() const { return untilTime_; }

        __declspec(property(get = get_EventList)) std::vector<ContainerEventDescription> const & EventList;
        std::vector<ContainerEventDescription> const & get_EventList() const { return eventList_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        Common::ErrorCode FromPublicApi(FABRIC_CONTAINER_EVENT_NOTIFICATION const & notification);

        FABRIC_FIELDS_03(sinceTime_, untilTime_, eventList_);

    private:
        int64 sinceTime_;
        int64 untilTime_;
        std::vector<ContainerEventDescription> eventList_;
    };
}
