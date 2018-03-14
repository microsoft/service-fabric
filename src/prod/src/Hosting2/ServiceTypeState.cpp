// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

ServiceTypeState::ServiceTypeState()
    : Status(ServiceTypeStatus::Invalid),
    RuntimeId(),
    CodePackageName(),
    HostId(),
    FailureId(),
    LastSequenceNumber(0)
{
}

ServiceTypeState::ServiceTypeState(
    ServiceTypeStatus::Enum status, 
    uint64 lastSequenceNumber)
    : Status(status),
    RuntimeId(),
    CodePackageName(),
    HostId(),
    FailureId(),
    LastSequenceNumber(lastSequenceNumber)
{
}

ServiceTypeState::ServiceTypeState(ServiceTypeState const & other)
    : Status(other.Status),
    RuntimeId(other.RuntimeId),
    CodePackageName(other.CodePackageName),
    HostId(other.HostId),
    FailureId(other.FailureId),
    LastSequenceNumber(other.LastSequenceNumber)
{
}

ServiceTypeState::ServiceTypeState(ServiceTypeState && other)
    : Status(other.Status),
    RuntimeId(move(other.RuntimeId)),
    CodePackageName(move(other.CodePackageName)),
    HostId(move(other.HostId)),
    FailureId(move(other.FailureId)),
    LastSequenceNumber(other.LastSequenceNumber)
{
}

ServiceTypeState const & ServiceTypeState::operator = (ServiceTypeState const & other)
{
    if (this != & other)
    {
        this->Status = other.Status;
        this->RuntimeId = other.RuntimeId;
        this->CodePackageName = other.CodePackageName;
        this->HostId = other.HostId;
        this->FailureId = other.FailureId;
        this->LastSequenceNumber = other.LastSequenceNumber;
    }

    return *this;
}

ServiceTypeState const & ServiceTypeState::operator = (ServiceTypeState && other)
{
    if (this != & other)
    {
        this->Status = other.Status;
        this->RuntimeId = move(other.RuntimeId);
        this->CodePackageName = move(other.CodePackageName);
        this->HostId = move(other.HostId);
        this->FailureId = move(other.FailureId);
        this->LastSequenceNumber = other.LastSequenceNumber;
    }

    return *this;
}

void ServiceTypeState::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write("{ ");
    w.Write("Status={0},", Status);
    w.Write("RuntimeId={0},", RuntimeId);
    w.Write("CodePackageName={0},", CodePackageName);
    w.Write("HostId={0},", HostId);
    w.Write("FailureId={0},", FailureId);
    w.Write("LastSequenceNumber={0}", LastSequenceNumber);
    w.Write("}");
}
