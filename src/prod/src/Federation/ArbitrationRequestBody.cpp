// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace LeaseWrapper;
using namespace Federation;

ArbitrationRequestBody::ArbitrationRequestBody()
    :   historyNeeded_(TimeSpan::Zero),
        monitorLeaseInstance_(0),
        subjectLeaseInstance_(0),
        extraData_(0),
        type_(ArbitrationType::TwoWaySimple)
{
}

ArbitrationRequestBody::ArbitrationRequestBody(
    LeaseAgentInstance const & monitor,
    LeaseAgentInstance const & subject,
    TimeSpan subjectTTL,
    TimeSpan historyNeeded,
    int64 monitorLeaseInstance,
    int64 subjectLeaseInstance,
    int64 extraData,
    ArbitrationType::Enum type)
    :   monitor_(monitor),
        subject_(subject),
        subjectTTL_(subjectTTL),
        historyNeeded_(historyNeeded),
        monitorLeaseInstance_(monitorLeaseInstance),
        subjectLeaseInstance_(subjectLeaseInstance),
        extraData_(extraData),
        type_(type)
{
}

void ArbitrationRequestBody::Normalize()
{
    if (type_ == ArbitrationType::TwoWaySimple &&
        monitorLeaseInstance_ == 0 &&
        subjectLeaseInstance_ == 0 &&
        subjectTTL_ == TimeSpan::MinValue)
    {
        type_ = ArbitrationType::RevertToReject;
    }
}

void ArbitrationRequestBody::WriteTo(TextWriter& w, FormatOptions const &) const
{
    if (type_ == ArbitrationType::Implicit || type_ == ArbitrationType::Query)
    {
        w.Write("{0}-{1}:{2} {3:x}",
            monitor_, subject_, type_, extraData_);
    }
    else
    {
        w.Write("{0}-{1}:{2} {3} {4}/{5}",
            monitor_, subject_, subjectTTL_, type_, monitorLeaseInstance_, subjectLeaseInstance_);
    }
}
