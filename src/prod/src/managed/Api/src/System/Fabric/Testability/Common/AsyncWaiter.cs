// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Common
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;

    // Use these methods instead of Task.Delay to ensure proper mapping of TimeSpan to milliseconds
    // (i.e. duration > Int32.MaxValue means infinite delay, or "-1").
    internal static class AsyncWaiter
    {
        public static Task WaitAsync(TimeSpan duration)
        {
            return WaitAsync(duration, CancellationToken.None);
        }

        public static Task WaitAsync(TimeSpan duration, CancellationToken cancellationToken)
        {
            ThrowIf.OutOfRange(duration, TimeSpan.Zero, TimeSpan.MaxValue, "duration");
            int durationInMilliseconds;
            if (duration.TotalMilliseconds > int.MaxValue)
            {
                durationInMilliseconds = -1;
            }
            else
            {
                durationInMilliseconds = (int)duration.TotalMilliseconds;
            }

            return Task.Delay(durationInMilliseconds, cancellationToken);
        }
    }
}


#pragma warning restore 1591