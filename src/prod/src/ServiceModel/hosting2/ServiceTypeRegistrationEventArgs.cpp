// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

ServiceTypeRegistrationEventArgs::ServiceTypeRegistrationEventArgs(
    uint64 const sequenceNumber,
    ServiceTypeRegistrationSPtr const & registration)
    : sequenceNumber_(sequenceNumber), registration_(registration)
{
}

ServiceTypeRegistrationEventArgs::ServiceTypeRegistrationEventArgs(ServiceTypeRegistrationEventArgs const & other)
    : sequenceNumber_(other.sequenceNumber_), registration_(other.registration_)
{
}

ServiceTypeRegistrationEventArgs::ServiceTypeRegistrationEventArgs(ServiceTypeRegistrationEventArgs && other)
    : sequenceNumber_(other.sequenceNumber_), registration_(move(other.registration_))
{
}

ServiceTypeRegistrationEventArgs const & ServiceTypeRegistrationEventArgs::operator = (ServiceTypeRegistrationEventArgs const & other)
{
    if (this != &other)
    {
        this->sequenceNumber_ = other.sequenceNumber_;
        this->registration_ = other.registration_;
    }

    return *this;
}

ServiceTypeRegistrationEventArgs const & ServiceTypeRegistrationEventArgs::operator = (ServiceTypeRegistrationEventArgs && other)
{
    if (this != &other)
    {
        this->sequenceNumber_ = other.sequenceNumber_;
        this->registration_ = move(other.registration_);
    }

    return *this;
}

void ServiceTypeRegistrationEventArgs::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write("ServiceTypeRegistrationEventArgs { ");
    w.Write("SequenceNumber = {0}, ", SequenceNumber);
    w.Write("Registration = {");
    w.Write(*registration_);
    w.Write("}");
}
