// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Reliability::ReplicationComponent;

SecondaryReplicatorTraceInfo::SecondaryReplicatorTraceInfo(SecondaryReplicatorTraceInfo && other)
    : lowLSN_(other.lowLSN_)
    , highLSN_(other.highLSN_)
    , receiveTime_(other.receiveTime_)
{
}

SecondaryReplicatorTraceInfo::SecondaryReplicatorTraceInfo(
    __in FABRIC_SEQUENCE_NUMBER lowLSN,
    __in FABRIC_SEQUENCE_NUMBER highLSN)
    : lowLSN_(lowLSN)
    , highLSN_(highLSN)
    , receiveTime_(DateTime::Now())
{
}

void SecondaryReplicatorTraceInfo::Update(
    __in FABRIC_SEQUENCE_NUMBER const & lowLSN,
    __in FABRIC_SEQUENCE_NUMBER const & highLSN)
{
    lowLSN_ = lowLSN;
    highLSN_ = highLSN;
    receiveTime_ = DateTime::Now();
}

void SecondaryReplicatorTraceInfo::Reset()
{
    lowLSN_ = -1;
    highLSN_ = -1;
    receiveTime_ = DateTime::MaxValue;
}

void SecondaryReplicatorTraceInfo::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    // Only write valid receive messages
    if (lowLSN_ == highLSN_)
    {
        writer.Write(
            "\r\nLSN: {0}, ReceiveTime: {1}",
            lowLSN_,
            receiveTime_);
    }
    else
    {
        writer.Write(
            "\r\nLSN: (Low={0}, High={1}) ReceiveTime: {2}",
            lowLSN_,
            highLSN_,
            receiveTime_);
    }
}

void SecondaryReplicatorTraceInfo::WriteToEtw(uint16 contextSequenceId) const
{
    if (lowLSN_ == highLSN_)
    {
        ReplicatorEventSource::Events->SecondaryTraceInfo(
            contextSequenceId,
            lowLSN_,
            receiveTime_.ToString());
    }
    else
    {
        ReplicatorEventSource::Events->SecondaryTraceBatchInfo(
            contextSequenceId,
            lowLSN_,
            highLSN_,
            receiveTime_.ToString());
    }
    
}
