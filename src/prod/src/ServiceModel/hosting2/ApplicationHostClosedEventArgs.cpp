// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

ApplicationHostClosedEventArgs::ApplicationHostClosedEventArgs(
    uint64 const sequenceNumber, 
    wstring const & hostId,
    vector<ServiceModel::ServiceTypeIdentifier> const & serviceTypes,
    ServiceModel::ServicePackageActivationContext isolationContext,
    wstring const & servicePackagePublicActivationId,
    Common::ActivityDescription const & activityDescription)
    : sequenceNumber_(sequenceNumber)
    , hostId_(hostId)
    , serviceTypes_(serviceTypes)
    , activationContext_(isolationContext)
    , servicePackagePublicActivationId_(servicePackagePublicActivationId)
    , activityDescription_(activityDescription)
{
}

ApplicationHostClosedEventArgs::ApplicationHostClosedEventArgs(ApplicationHostClosedEventArgs const & other)
    : sequenceNumber_(other.sequenceNumber_)
    , hostId_(other.hostId_)
    , serviceTypes_(other.serviceTypes_)
    , activationContext_(other.activationContext_)
    , servicePackagePublicActivationId_(other.servicePackagePublicActivationId_)
    , activityDescription_(other.activityDescription_)
{
}

ApplicationHostClosedEventArgs::ApplicationHostClosedEventArgs(ApplicationHostClosedEventArgs && other)
    : sequenceNumber_(other.sequenceNumber_)
    , hostId_(move(other.hostId_))
    , serviceTypes_(move(other.serviceTypes_))
    , activationContext_(move(other.activationContext_))
    , servicePackagePublicActivationId_(move(other.servicePackagePublicActivationId_))
    , activityDescription_(move(other.activityDescription_))
{
}

ApplicationHostClosedEventArgs const & ApplicationHostClosedEventArgs::operator = (ApplicationHostClosedEventArgs const & other)
{
    if (this != &other)
    {
        this->sequenceNumber_ = other.sequenceNumber_;
        this->hostId_ = other.hostId_;
        this->serviceTypes_ = other.serviceTypes_;
        this->activationContext_ = other.activationContext_;
        this->servicePackagePublicActivationId_ = other.servicePackagePublicActivationId_;
        this->activityDescription_ = other.activityDescription_;
    }

    return *this;
}

ApplicationHostClosedEventArgs const & ApplicationHostClosedEventArgs::operator = (ApplicationHostClosedEventArgs && other)
{
    if (this != &other)
    {
        this->sequenceNumber_ = other.sequenceNumber_;
        this->hostId_ = move(other.hostId_);
        this->serviceTypes_ = move(other.serviceTypes_);
        this->activationContext_ = move(other.activationContext_);
        this->servicePackagePublicActivationId_ = move(other.servicePackagePublicActivationId_);
        this->activityDescription_ = other.activityDescription_;
    }

    return *this;
}

wstring const & ApplicationHostClosedEventArgs::get_ServicePackagePublicActivationId() const
{
    return servicePackagePublicActivationId_;
}

void ApplicationHostClosedEventArgs::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write("ApplicationHostClosedEventArgs { ");
    w.Write("SequenceNumber = {0}, ", SequenceNumber);
    w.Write("HostId = {0},", HostId);
    w.Write("ServiceTypes = [");
    for(auto iter = serviceTypes_.cbegin(); iter != serviceTypes_.cend(); ++iter)
    {
        w.Write("{0},", iter->ServiceTypeName);
    }
    w.Write("],");
    w.Write("ActivaionContext = {0}", activationContext_);
    w.Write("ServicePackagePublicActivationId = {0}", servicePackagePublicActivationId_);
    w.Write("ActivityDescription = {0}", activityDescription_);
    w.Write("}");
}
