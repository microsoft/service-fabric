// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Testability.Common;

    internal class DelayStrategy : IStrategy
    {
        private readonly Func<TimeSpan, TimeSpan> getNextDelayTime;
        private TimeSpan delayTime;

        public DelayStrategy(TimeSpan delayTime, Func<TimeSpan, TimeSpan> getNextDelayTime)
        {
            ThrowIf.OutOfRange(delayTime, TimeSpan.Zero, TimeSpan.MaxValue, "delayTime");
            ThrowIf.Null(getNextDelayTime, "getNextDelayTime");

            this.delayTime = delayTime;
            this.getNextDelayTime = getNextDelayTime;
        }

        public async Task PerformAsync(IRetryContext context, CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteNoise("DelayStrategy", "{0}: Sleeping '{1}' seconds.", context.ActivityId, this.delayTime.TotalSeconds);

            await AsyncWaiter.WaitAsync(this.delayTime, cancellationToken);

            TimeSpan nextSleepTime = this.getNextDelayTime(this.delayTime);

            if (nextSleepTime != this.delayTime)
            {
                TestabilityTrace.TraceSource.WriteNoise("DelayStrategy", "{0}: Increasing current delay time from '{1}' to '{2}' seconds.", context.ActivityId, this.delayTime.TotalSeconds, nextSleepTime.TotalSeconds);
                this.delayTime = nextSleepTime;
            }
        }
    }
}


#pragma warning restore 1591