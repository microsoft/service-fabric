// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ServiceModel;

ServiceTypeNotificationReplyMessageBody::ServiceTypeNotificationReplyMessageBody(std::vector<ServiceTypeInfo> && infos) :
infos_(move(infos))
{
}

ServiceTypeNotificationReplyMessageBody::ServiceTypeNotificationReplyMessageBody(std::vector<ServiceTypeInfo> const & infos) :
infos_(infos)
{
}

ServiceTypeNotificationReplyMessageBody::ServiceTypeNotificationReplyMessageBody()
{
}

ServiceTypeNotificationReplyMessageBody::ServiceTypeNotificationReplyMessageBody(ServiceTypeNotificationReplyMessageBody const & other) :
infos_(other.infos_)
{
}

ServiceTypeNotificationReplyMessageBody::ServiceTypeNotificationReplyMessageBody(ServiceTypeNotificationReplyMessageBody && other) :
infos_(move(other.infos_))
{
}

ServiceTypeNotificationReplyMessageBody & ServiceTypeNotificationReplyMessageBody::operator=(ServiceTypeNotificationReplyMessageBody const & other)
{
    if (this != &other)
    {
        infos_ = other.infos_;
    }

    return *this;
}

ServiceTypeNotificationReplyMessageBody & ServiceTypeNotificationReplyMessageBody::operator=(ServiceTypeNotificationReplyMessageBody && other)
{
    if (this != &other)
    {
        infos_ = std::move(other.infos_);
    }

    return *this;
}

void ServiceTypeNotificationReplyMessageBody::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
{
    for (auto const & it : infos_)
    {
        w << it;
        w << "\r\n";
    }
}

void ServiceTypeNotificationReplyMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->ServiceTypeNotificationReplyMessageBody(contextSequenceId, infos_);
}
