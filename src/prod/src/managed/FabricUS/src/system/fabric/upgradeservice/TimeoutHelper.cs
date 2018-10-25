// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System;

    internal class TimeoutHelper
    {        
        private readonly DateTime deadline;
        private readonly TimeSpan operationTimeout;

        public TimeoutHelper(TimeSpan timeout, TimeSpan operationTimeout)
        {
            deadline = (timeout == TimeSpan.MaxValue) ? DateTime.MaxValue : DateTime.Now.Add(timeout);
            this.operationTimeout = operationTimeout;
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

        public TimeSpan GetOperationTimeout()
        {
            var timeout = this.GetRemainingTime();
            return timeout > this.operationTimeout ? this.operationTimeout : timeout;
        }

        public void ThrowIfExpired()
        {
            if (this.IsExpired())
            {
                throw new TimeoutException("Error_OperationTimedOut");
            }
        }

        public bool IsExpired()
        {
            return this.GetRemainingTime() == TimeSpan.Zero;
        }
    }
}