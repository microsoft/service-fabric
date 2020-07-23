// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;

    internal class TimeoutHelper
    {
        private DateTime deadline;

        public TimeoutHelper(TimeSpan timeout)
        {
            this.deadline = (timeout == TimeSpan.MaxValue) ? DateTime.MaxValue : DateTime.Now.Add(timeout);
        }

        public static bool HasExpired(TimeoutHelper timeoutHelper)
        {
            return timeoutHelper.GetRemainingTime() == TimeSpan.Zero;
        }

        public TimeSpan GetRemainingTime()
        {
            if (this.deadline == DateTime.MaxValue)
            {
                return TimeSpan.MaxValue;
            }

            TimeSpan remaining = this.deadline - DateTime.Now;
            return remaining <= TimeSpan.Zero ? TimeSpan.Zero : remaining;
        }

        public void ThrowIfExpired()
        {
            if (this.GetRemainingTime() == TimeSpan.Zero)
            {
                throw new TimeoutException("Operation timed out");
            }
        }
    }
}