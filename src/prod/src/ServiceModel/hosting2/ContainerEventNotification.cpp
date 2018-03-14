// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ContainerEventNotification::ContainerEventNotification()
    : sinceTime_()
    , untilTime_()
    , eventList_()
{
}

void ContainerEventNotification::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerEventNotification { ");

    w.Write("SinceTime={0}, ", sinceTime_);
    w.Write("UntilTime={0}, ", untilTime_);
    w.Write("EventList=[ ");

    for (auto containerEvent : eventList_)
    {
        w.Write("{0} ", containerEvent);
    }

    w.Write("]}");
}

ErrorCode ContainerEventNotification::FromPublicApi(
    FABRIC_CONTAINER_EVENT_NOTIFICATION const & notification)
{
    sinceTime_ = notification.SinceTime;
    untilTime_ = notification.UntilTime;

    auto error = PublicApiHelper::FromPublicApiList<
        ContainerEventDescription,
        FABRIC_CONTAINER_EVENT_DESCRIPTION_LIST>(notification.EventList, eventList_);

    return error;
}
