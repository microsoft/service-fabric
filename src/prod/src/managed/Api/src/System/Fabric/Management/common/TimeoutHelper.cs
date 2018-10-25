// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Common
{
    using System;
    using System.Fabric.Strings;

    internal class TimeoutHelper
    {        
        private DateTime deadline;

        public TimeoutHelper(TimeSpan timeout)
        {            
            deadline = (timeout == TimeSpan.MaxValue) ? DateTime.MaxValue : DateTime.Now.Add(timeout);
        }

        public TimeSpan GetRemainingTime()
        {
            if (deadline == DateTime.MaxValue)
            {
                return TimeSpan.MaxValue;
            }

            TimeSpan remaining = deadline - DateTime.Now;
            return remaining <= TimeSpan.Zero ? TimeSpan.Zero : remaining;
        }

        public void ThrowIfExpired()
        {
            if (this.GetRemainingTime() == TimeSpan.Zero)
            {
                throw new TimeoutException(StringResources.Error_OperationTimedOut);
            }
        }
    }
}