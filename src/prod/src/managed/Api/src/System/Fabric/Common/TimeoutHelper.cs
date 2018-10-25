// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Fabric.Strings;

    internal class TimeoutHelper
    {        
        private DateTime deadline;

        public TimeoutHelper(TimeSpan timeout)
        {            
            deadline = (timeout == TimeSpan.MaxValue) ? DateTime.MaxValue : DateTime.UtcNow.Add(timeout);
        }

        public TimeSpan GetRemainingTime()
        {
            if (deadline == DateTime.MaxValue)
            {
                return TimeSpan.MaxValue;
            }

            TimeSpan remaining = deadline - DateTime.UtcNow;
            return remaining <= TimeSpan.Zero ? TimeSpan.Zero : remaining;
        }

        public void ThrowIfExpired()
        {
            if (this.GetRemainingTime() == TimeSpan.Zero)
            {
                throw new TimeoutException(StringResources.Error_OperationTimedOut);
            }
        }

        public static bool HasExpired(TimeoutHelper timeoutHelper)
        {
            return timeoutHelper.GetRemainingTime() == TimeSpan.Zero;
        }
    }
}