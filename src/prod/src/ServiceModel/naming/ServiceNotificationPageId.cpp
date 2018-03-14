// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
 
using namespace std;
using namespace Common;
using namespace Naming;

ServiceNotificationPageId::ServiceNotificationPageId()
    : notificationId_()
    , pageIndex_(0)
{
}

ServiceNotificationPageId::ServiceNotificationPageId(
    ActivityId const & notificationId,
    uint64 pageIndex)
    : notificationId_(notificationId)
    , pageIndex_(pageIndex)
{
}

bool ServiceNotificationPageId::operator < (ServiceNotificationPageId const & other) const
{
    if (notificationId_ < other.notificationId_)
    {
        return true;
    }

    return pageIndex_ < other.pageIndex_;
}

bool ServiceNotificationPageId::operator == (ServiceNotificationPageId const & other) const
{
    if (notificationId_ != other.notificationId_)
    {
        return false;
    }

    return pageIndex_ == other.pageIndex_;
}

bool ServiceNotificationPageId::operator != (ServiceNotificationPageId const & other) const 
{ 
    return !(*this == other); 
}

void ServiceNotificationPageId::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w << notificationId_ << L":" << pageIndex_;
}

string ServiceNotificationPageId::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    traceEvent.AddField<Common::Guid>(name + ".aid");
    traceEvent.AddField<uint64>(name + ".aindex");
    traceEvent.AddField<uint64>(name + ".pindex");
    
    return "{0}:{1}:{2}";
}

void ServiceNotificationPageId::FillEventData(TraceEventContext & context) const
{
    context.Write<Common::Guid>(notificationId_.Guid);
    context.Write<uint64>(notificationId_.Index);
    context.Write<uint64>(pageIndex_);
}
