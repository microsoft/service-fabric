// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Reliability;

ServiceNotificationFilter::ServiceNotificationFilter()
    : filterId_(0)
    , name_()
    , flags_()
{
}

ServiceNotificationFilter::ServiceNotificationFilter(ServiceNotificationFilter const & other)
    : filterId_(other.filterId_)
    , name_(other.name_)
    , flags_(other.flags_)
{
}

ServiceNotificationFilter::ServiceNotificationFilter(ServiceNotificationFilter && other)
    : filterId_(move(other.filterId_))
    , name_(move(other.name_))
    , flags_(move(other.flags_))
{
}

ServiceNotificationFilter::ServiceNotificationFilter(
    NamingUri const & name,
    ServiceNotificationFilterFlags const & flags)
    : filterId_(0)
    , name_(name)
    , flags_(flags)
{
}

ErrorCode ServiceNotificationFilter::FromPublicApi(
    FABRIC_SERVICE_NOTIFICATION_FILTER_DESCRIPTION const & publicDescription)
{
    auto error = NamingUri::TryParse(publicDescription.Name, "Name", name_);
    if (!error.IsSuccess())
    {
        return error;
    }

    flags_.FromPublicApi(publicDescription.Flags);

    return ErrorCodeValue::Success;
}

string ServiceNotificationFilter::AddField(Common::TraceEvent & traceEvent, string const & name)
{   
    string format = "(id={0} name={1} flags={2})";
    size_t index = 0;

    traceEvent.AddEventField<uint64>(format, name + ".filterId", index);
    traceEvent.AddEventField<Uri>(format, name + ".name", index);
    traceEvent.AddEventField<uint>(format, name + ".flags", index);

    return format;
}

void ServiceNotificationFilter::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<uint64>(filterId_);
    context.Write<NamingUri>(name_);
    context.Write<uint>(*flags_);
}

void ServiceNotificationFilter::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const&) const
{
    writer.Write(
        "(id={0} name={1} flags={2})",
        filterId_,
        name_,
        flags_);
}
