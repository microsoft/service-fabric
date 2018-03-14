// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServiceNotificationFilter : public Serialization::FabricSerializable
    {
    public:
        ServiceNotificationFilter();
        ServiceNotificationFilter(ServiceNotificationFilter const &);
        ServiceNotificationFilter(ServiceNotificationFilter &&);
        ServiceNotificationFilter(Common::NamingUri const &, ServiceNotificationFilterFlags const & flags);

        __declspec(property(get=get_FilterId)) uint64 FilterId;
        uint64 get_FilterId() const { return filterId_; }

        __declspec(property(get=get_Name)) Common::NamingUri const & Name;
        Common::NamingUri const & get_Name() const { return name_; }

        __declspec(property(get=get_Flags)) ServiceNotificationFilterFlags const & Flags;
        ServiceNotificationFilterFlags const & get_Flags() const { return flags_; }

        __declspec(property(get=get_IsPrefix)) bool IsPrefix;
        bool get_IsPrefix() const { return flags_.IsNamePrefixSet(); }

        __declspec(property(get=get_IsPrimaryOnly)) bool IsPrimaryOnly;
        bool get_IsPrimaryOnly() const { return flags_.IsPrimaryOnlySet(); }

        void SetFilterId(uint64 id) { filterId_ = id; }

        Common::ErrorCode FromPublicApi(FABRIC_SERVICE_NOTIFICATION_FILTER_DESCRIPTION const &);

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;
        void WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const&) const;

        FABRIC_FIELDS_03(filterId_, name_, flags_);

    private:
        uint64 filterId_;
        Common::NamingUri name_;
        ServiceNotificationFilterFlags flags_;
    };

    typedef std::shared_ptr<ServiceNotificationFilter> ServiceNotificationFilterSPtr;
}
