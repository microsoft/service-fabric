// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

// ********************************************************************************************************************
// OperationState Implementation
//

OperationStatus::OperationStatus()
    : Id(L""),
    State(OperationState::Unknown),
    FailureCount(0),
    lastError_(),
    InternalFailureCount(0)
{
    lastError_.ReadValue();
}

OperationStatus::OperationStatus(wstring const & id)
    : Id(id),
    State(OperationState::Unknown),
    FailureCount(0),
    lastError_(),
    InternalFailureCount(0)
{
    lastError_.ReadValue();
}

OperationStatus::OperationStatus(OperationStatus const & other)
    : Id(other.Id),
    State(other.State),
    FailureCount(other.FailureCount),
    lastError_(),
    InternalFailureCount(other.InternalFailureCount)
{
    LastError = other.lastError_;
}

OperationStatus::OperationStatus(OperationStatus && other)
    : Id(move(other.Id)),
    State(other.State),
    FailureCount(other.FailureCount),
    lastError_(),
    InternalFailureCount(other.InternalFailureCount)
{
    LastError = other.lastError_;
}

OperationStatus & OperationStatus::operator = (OperationStatus const & other)
{
    if (this != &other)
    {
        this->Id = other.Id;
        this->State = other.State;
        this->FailureCount = other.FailureCount;
        this->LastError = other.lastError_;
        this->InternalFailureCount = other.InternalFailureCount;
    }

    return *this;
}

OperationStatus & OperationStatus::operator = (OperationStatus && other)
{
    if (this != &other)
    {
        this->Id = move(other.Id);
        this->State = other.State;
        this->FailureCount = other.FailureCount;
        this->LastError = other.lastError_;
        this->InternalFailureCount = other.InternalFailureCount;
    }

    return *this;
}

void OperationState::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    switch (val)
    {
        case OperationState::Unknown:
            w << L"Unknown";
            return;
        case OperationState::NotStarted:
            w << L"NotStarted";
            return;
        case OperationState::InProgress:
            w << L"InProgress";
            return;
        case OperationState::Completed:
            w << L"Completed";
            return;
        default:
            Assert::CodingError("Unknown OperationState value {0}", (int)val);
    }
}

// ********************************************************************************************************************
// OperationStatus Implementation
//
void OperationStatus::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("{");
    w.Write("Id = {0},", Id);
    w.Write("State = {0},", State);
    w.Write("LastError = {0},", LastError);
    w.Write("FailureCount = {0},", FailureCount);
    w.Write("InternalFailureCount = {0},", InternalFailureCount);
    w.Write("}");
}
