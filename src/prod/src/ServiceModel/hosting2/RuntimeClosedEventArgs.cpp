// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

RuntimeClosedEventArgs::RuntimeClosedEventArgs(
    uint64 const sequenceNumber, 
    wstring const & hostId, 
    wstring const & runtimeId, 
    vector<ServiceModel::ServiceTypeIdentifier> const & serviceTypes,
    ServiceModel::ServicePackageActivationContext activationContext,
    wstring const & servicePackagePublicActivationId)
    : sequenceNumber_(sequenceNumber)
    , hostId_(hostId)
    , runtimeId_(runtimeId)
    , serviceTypes_(serviceTypes)
    , activationContext_(activationContext)
    , servicePackagePublicActivationId_(servicePackagePublicActivationId)
{
}

RuntimeClosedEventArgs::RuntimeClosedEventArgs(RuntimeClosedEventArgs const & other)
    : sequenceNumber_(other.sequenceNumber_)
    , hostId_(other.hostId_)
    , runtimeId_(other.runtimeId_)
    , serviceTypes_(other.serviceTypes_)
    , activationContext_(other.activationContext_)
    , servicePackagePublicActivationId_(other.servicePackagePublicActivationId_)
{
}

RuntimeClosedEventArgs::RuntimeClosedEventArgs(RuntimeClosedEventArgs && other)
    : sequenceNumber_(other.sequenceNumber_)
    , hostId_(move(other.hostId_))
    , runtimeId_(move(other.runtimeId_))
    , serviceTypes_(move(other.serviceTypes_))
    , activationContext_(move(other.activationContext_))
    , servicePackagePublicActivationId_(move(other.servicePackagePublicActivationId_))
{
}

RuntimeClosedEventArgs const & RuntimeClosedEventArgs::operator = (RuntimeClosedEventArgs const & other)
{
    if (this != &other)
    {
        this->sequenceNumber_ = other.sequenceNumber_;
        this->hostId_ = other.hostId_;
        this->runtimeId_ = other.runtimeId_;
        this->serviceTypes_ = other.serviceTypes_;
        this->activationContext_ = other.activationContext_;
        this->servicePackagePublicActivationId_ = other.servicePackagePublicActivationId_;
    }

    return *this;
}

RuntimeClosedEventArgs const & RuntimeClosedEventArgs::operator = (RuntimeClosedEventArgs && other)
{
    if (this != &other)
    {
        this->sequenceNumber_ = other.sequenceNumber_;
        this->hostId_ = move(other.hostId_);
        this->runtimeId_ = move(other.runtimeId_);
        this->serviceTypes_ = move(other.serviceTypes_);
        this->activationContext_ = move(other.activationContext_);
        this->servicePackagePublicActivationId_ = move(other.servicePackagePublicActivationId_);
    }

    return *this;
}

wstring const & RuntimeClosedEventArgs::get_ServicePackagePublicActivationId() const
{
    return servicePackagePublicActivationId_;
}

void RuntimeClosedEventArgs::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write("RuntimeClosedEventArgs { ");
    w.Write("SequenceNumber = {0}, ", SequenceNumber);
    w.Write("HostId = {0}, ", HostId);
    w.Write("RuntimeId = {0},", RuntimeId);
    w.Write("ServiceTypes = [");
    for(auto iter = ServiceTypes.cbegin(); iter != ServiceTypes.cend(); ++iter)
    {
        w.Write("{0},", iter->ServiceTypeName);
    }
    w.Write("],");
    w.Write("ActivationContext = {0}", activationContext_);
    w.Write("ServicePackagePublicActivationId = {0}", servicePackagePublicActivationId_);
    w.Write("}");
}
