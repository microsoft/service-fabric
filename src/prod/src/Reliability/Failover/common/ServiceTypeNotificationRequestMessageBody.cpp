// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ServiceModel;

ServiceTypeNotificationRequestMessageBody::ServiceTypeNotificationRequestMessageBody(std::vector<ServiceTypeInfo> && infos) :
infos_(move(infos))
{
}

ServiceTypeNotificationRequestMessageBody::ServiceTypeNotificationRequestMessageBody(std::vector<ServiceTypeInfo> const & infos) :
infos_(infos)
{
}

ServiceTypeNotificationRequestMessageBody::ServiceTypeNotificationRequestMessageBody()
{
}

ServiceTypeNotificationRequestMessageBody::ServiceTypeNotificationRequestMessageBody(ServiceTypeNotificationRequestMessageBody const & other) :
infos_(other.infos_)
{
}

ServiceTypeNotificationRequestMessageBody::ServiceTypeNotificationRequestMessageBody(ServiceTypeNotificationRequestMessageBody && other) :
infos_(move(other.infos_))
{
}

ServiceTypeNotificationRequestMessageBody & ServiceTypeNotificationRequestMessageBody::operator=(ServiceTypeNotificationRequestMessageBody const & other)
{
    if (this != &other)
    {
        infos_ = other.infos_;
    }

    return *this;
}

ServiceTypeNotificationRequestMessageBody & ServiceTypeNotificationRequestMessageBody::operator=(ServiceTypeNotificationRequestMessageBody && other)
{
    if (this != &other)
    {
        infos_ = std::move(other.infos_);
    }

    return *this;
}

void ServiceTypeNotificationRequestMessageBody::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
{
    for(auto const & it : infos_)
    {
        w << it;
        w << "\r\n";
    }
}

void ServiceTypeNotificationRequestMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->ServiceTypeNotificationRequestMessageBody(contextSequenceId, infos_);
}
