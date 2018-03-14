// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ServiceNotificationPageId : public Serialization::FabricSerializable
    {
    public:
        struct Hasher
        {
            size_t operator() (ServiceNotificationPageId const & key) const 
            { 
                return key.NotificationId.Guid.GetHashCode() + key.NotificationId.Index + key.pageIndex_;
            }
            bool operator() (ServiceNotificationPageId const & left, ServiceNotificationPageId const & right) const 
            { 
                return (left == right);
            }
        };

    public:
        ServiceNotificationPageId();

        ServiceNotificationPageId(
            Common::ActivityId const & notificationId,
            uint64 pageIndex);

         __declspec (property(get=get_NotificationId)) Common::ActivityId const & NotificationId;
         Common::ActivityId const & get_NotificationId() const { return notificationId_; }

         __declspec (property(get=get_PageIndex)) uint64 PageIndex;
         uint64 get_PageIndex() const { return pageIndex_; }

        bool operator < (ServiceNotificationPageId const & other) const;
        bool operator == (ServiceNotificationPageId const & other) const;
        bool operator != (ServiceNotificationPageId const & other) const;

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const& f) const;
        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;

        FABRIC_FIELDS_02(notificationId_, pageIndex_);

    private:
        Common::ActivityId notificationId_;
        uint64 pageIndex_;
    };
}
