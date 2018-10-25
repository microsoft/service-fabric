// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Threading;

    internal static class TimeoutHelper
    {
        public const double DefaultSystemTimerResolution = 1000.0 / 64.0;

        public const int DefaultLockTimeoutInMilliseconds = 4000;

        public const int DefaultReplicationTimeoutInMilliseconds = 30000;  // todo: fine-tune this value

        public const int DefaultRetryStartDelayInMilliseconds = 16;

        public const int DefaultExponentialBackoffFactor = 2;

        public const int DefaultMaxSingleDelayInMilliseconds = 4 * 1024;

        public static readonly TimeSpan DefaultReplicationTimeout = TimeSpan.FromMilliseconds(DefaultReplicationTimeoutInMilliseconds);

        public static readonly TimeSpan DefaultLockTimeout = TimeSpan.FromMilliseconds(DefaultLockTimeoutInMilliseconds);

        public static int GetTimeSpanInMilliseconds(TimeSpan timeout)
        {
            if (Timeout.InfiniteTimeSpan == timeout)
            {
                return Timeout.Infinite;
            }

            if (TimeSpan.Zero == timeout)
            {
                return 0;
            }

            var milliseconds = timeout.TotalMilliseconds;
            if (milliseconds > int.MaxValue)
            {
                return Timeout.Infinite;
            }

            if (TimeSpan.Zero > timeout)
            {
                throw new ArgumentException(SR.Error_Timeout);
            }

            if (0.0 < milliseconds && DefaultSystemTimerResolution > milliseconds)
            {
                milliseconds = 16;
            }

            return (int)milliseconds;
        }
    }
}