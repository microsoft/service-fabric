// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Federation
{
    using namespace std;
    using namespace Common;

    int const ArbitrationReplyBody::Extended = 1;
    int const ArbitrationReplyBody::Strong = 2;
    int const ArbitrationReplyBody::Continuous = 4;
    int const ArbitrationReplyBody::Delayed = 8;

    ArbitrationReplyBody::ArbitrationReplyBody()
        :   subjectTTL_(TimeSpan::MaxValue),
            monitorTTL_(TimeSpan::MaxValue),
            subjectReported_(false),
            flags_(0)
    {
    }

    ArbitrationReplyBody::ArbitrationReplyBody(TimeSpan subjectTTL, bool subjectReported)
        :   subjectTTL_(subjectTTL),
            monitorTTL_(subjectTTL == TimeSpan::MaxValue ? TimeSpan::Zero : TimeSpan::MaxValue),
            subjectReported_(subjectReported),
            flags_(0)
    {
    }

    ArbitrationReplyBody::ArbitrationReplyBody(TimeSpan subjectTTL, TimeSpan monitorTTL, bool subjectReported, int flags)
        : subjectTTL_(subjectTTL), monitorTTL_(monitorTTL), subjectReported_(subjectReported), flags_(flags)
    {
    }

    void ArbitrationReplyBody::Normalize()
    {
        if (flags_ == 0 && subjectTTL_ == TimeSpan::MaxValue)
        {
            monitorTTL_ = TimeSpan::Zero;
        }
    }

    int ArbitrationReplyBody::CompareTo(ArbitrationReplyBody const & other) const
    {
        bool local1 = (monitorTTL_ == TimeSpan::MaxValue);
        bool local2 = (other.monitorTTL_ == TimeSpan::MaxValue);
        if (local1 != local2)
        {
            return (local1 ? 1 : -1);
        }

        bool remote1 = (subjectTTL_ == TimeSpan::MaxValue);
        bool remote2 = (other.subjectTTL_ == TimeSpan::MaxValue);
        if (remote1 != remote2)
        {
            return (remote1 ? -1 : 1);
        }

        return 0;
    }

    bool ArbitrationReplyBody::IsExtended() const
    {
        return (flags_ & Extended) != 0;
    }

    bool ArbitrationReplyBody::IsStrong() const
    {
        return (flags_ & Strong) != 0;
    }

    bool ArbitrationReplyBody::IsContinuous() const
    {
        return (flags_ & Continuous) != 0;
    }

    bool ArbitrationReplyBody::IsDelayed() const
    {
        return (flags_ & Delayed) != 0;
    }

    void ArbitrationReplyBody::WriteTo(TextWriter& w, FormatOptions const &) const
    {
        w.Write("{0}/{1} {2} {3:x}", monitorTTL_, subjectTTL_, subjectReported_, flags_);
    }

    DelayedArbitrationReplyBody::DelayedArbitrationReplyBody()
    {
    }

    DelayedArbitrationReplyBody::DelayedArbitrationReplyBody(
        LeaseWrapper::LeaseAgentInstance const & monitor,
        LeaseWrapper::LeaseAgentInstance const & subject,
        int64 monitorLeaseInstance,
        int64 subjectLeaseInstance,
        ArbitrationDecision::Enum decision,
        TimeSpan subjectTTL,
        bool subjectReported,
        int flags)
        :   monitor_(monitor),
            subject_(subject),
            monitorLeaseInstance_(monitorLeaseInstance),
            subjectLeaseInstance_(subjectLeaseInstance), 
            decision_(decision),
            subjectTTL_(subjectTTL),
            subjectReported_(subjectReported),
            flags_(flags)
    {
    }

    DelayedArbitrationReplyBody::DelayedArbitrationReplyBody(
        ArbitrationRequestBody const & request,
        ArbitrationDecision::Enum decision,
        TimeSpan subjectTTL,
        bool subjectReported,
        int flags)
        :   monitor_(request.Subject), 
            subject_(request.Monitor),
            monitorLeaseInstance_(request.SubjectLeaseInstance),
            subjectLeaseInstance_(request.MonitorLeaseInstance),
            decision_(decision), 
            subjectTTL_(subjectTTL),
            subjectReported_(subjectReported),
            flags_(flags)
    {
    }

    void DelayedArbitrationReplyBody::WriteTo(TextWriter& w, FormatOptions const &) const
    {
        w.Write("{0}-{1} {2}/{3}:{4} {5}", 
            monitor_, subject_, monitorLeaseInstance_, subjectLeaseInstance_, decision_, subjectTTL_);
    }
}
