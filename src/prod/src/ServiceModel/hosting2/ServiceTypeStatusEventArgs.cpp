// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ServiceTypeStatusEventArgs::ServiceTypeStatusEventArgs(
    uint64 const sequenceNumber, 
    ServiceTypeIdentifier const & serviceTypeId,
    ServicePackageActivationContext activationContext,
    wstring const & servicePackagePublicActivationId)
    : sequenceNumber_(sequenceNumber)
    , serviceTypeId_(serviceTypeId)
    , activationContext_(activationContext)
    , servicePackagePublicActivationId_(servicePackagePublicActivationId)
{
}

ServiceTypeStatusEventArgs::ServiceTypeStatusEventArgs(ServiceTypeStatusEventArgs const & other)
    : sequenceNumber_(other.sequenceNumber_)
    , serviceTypeId_(other.serviceTypeId_)
    , activationContext_(other.activationContext_)
    , servicePackagePublicActivationId_(other.servicePackagePublicActivationId_)
{
}

ServiceTypeStatusEventArgs::ServiceTypeStatusEventArgs(ServiceTypeStatusEventArgs && other)
    : sequenceNumber_(other.sequenceNumber_)
    , serviceTypeId_(move(other.serviceTypeId_))
    , activationContext_(move(other.activationContext_))
    , servicePackagePublicActivationId_(move(other.servicePackagePublicActivationId_))
{
}

ServiceTypeStatusEventArgs const & ServiceTypeStatusEventArgs::operator = (ServiceTypeStatusEventArgs const & other)
{
    if (this != &other)
    {
        this->serviceTypeId_ = other.serviceTypeId_;
        this->sequenceNumber_ = other.sequenceNumber_;
        this->activationContext_ = other.activationContext_;
        this->servicePackagePublicActivationId_ = other.servicePackagePublicActivationId_;
    }

    return *this;
}

ServiceTypeStatusEventArgs const & ServiceTypeStatusEventArgs::operator = (ServiceTypeStatusEventArgs && other)
{
    if (this != &other)
    {
        this->serviceTypeId_ = move(other.serviceTypeId_);
        this->sequenceNumber_ = other.sequenceNumber_;
        this->activationContext_ = move(other.activationContext_);
        this->servicePackagePublicActivationId_ = move(other.servicePackagePublicActivationId_);
    }

    return *this;
}

wstring const & ServiceTypeStatusEventArgs::get_ServicePackagePublicActivationId() const
{
    return servicePackagePublicActivationId_;
}

void ServiceTypeStatusEventArgs::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write("ServiceTypeStatusEventArgs { ");
    w.Write("SequenceNumber = {0}, ", SequenceNumber);
    w.Write("ServiceTypeId = {0}", ServiceTypeId);
    w.Write("ActivationContext = {0}", activationContext_);
    w.Write("ServicePackagePublicActivationId = {0}", servicePackagePublicActivationId_);
    w.Write("}");
}
