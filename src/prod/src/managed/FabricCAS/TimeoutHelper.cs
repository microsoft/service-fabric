// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Hosting.ContainerActivatorService
{
    using System;
    using System.Diagnostics;

    internal struct TimeoutHelper
    {
        public TimeoutHelper(TimeSpan timeout)
        {
            this.originalTimeout = timeout;
            this.stopWatch = Stopwatch.StartNew();
        }

        public bool HasRemainingTime
        {
            get
            {
                return (this.stopWatch.Elapsed < this.originalTimeout);
            }
        }

        public bool HasTimedOut
        {
            get
            {
                return !this.HasRemainingTime;
            }
        }

        public TimeSpan OriginalTimeout
        {
            get { return originalTimeout; }
        }

        public TimeSpan RemainingTime
        {
            get
            {
                if(this.originalTimeout == TimeSpan.MaxValue)
                {
                    return TimeSpan.MaxValue;
                }

                var remainingTime = (this.originalTimeout - this.stopWatch.Elapsed);

                if(remainingTime <= TimeSpan.Zero)
                {
                    return TimeSpan.Zero;
                }

                return remainingTime;
            }

            set
            {
                this.originalTimeout = value;
                this.stopWatch.Restart();
            }
        }

        private readonly Stopwatch stopWatch;
        private TimeSpan originalTimeout;
    }
}