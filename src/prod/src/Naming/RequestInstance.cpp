// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace Common;
    using namespace std;
    using namespace Transport;

    StringLiteral const TraceComponent("RequestInstance");

    RequestInstance::RequestInstance()
        : traceId_()
        , instance_(0)
    {
    }

    RequestInstance::RequestInstance(wstring const & traceId)
        : traceId_(traceId)
        , instance_(0)
    {
    }


    void RequestInstance::SetTraceId(wstring const & traceId)
    {
        traceId_ = traceId;
    }

    __int64 RequestInstance::GetNextInstance()
    {
        AcquireExclusiveLock lock(lock_);

        auto next = DateTime::Now().Ticks;

        // Ensure that requests occurring within the same tick use different instances
        // to prevent false duplicate rejection.
        //
        // Furthermore, this prevents clock skew issues if failover of the request
        // sender occurs to a node whose clock is behind since we will update
        // the next instance to be used based on the StaleMessage rejection reply.
        //
        if (next <= instance_)
        {
            next = ++instance_;
        }
        else
        {
            instance_ = next;
        }

        return next;
    }

    void RequestInstance::UpdateToHigherInstance(__int64 higherInstance)
    {
        AcquireExclusiveLock lock(lock_);

        if (instance_ < higherInstance)
        {
            WriteInfo(
                TraceComponent,
                "{0}: updating request instance: {1} -> {2}",
                this->TraceId,
                instance_,
                higherInstance);

            instance_ = higherInstance;
        }
    }
}
