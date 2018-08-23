// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System.Threading;

    // Wrapper around System.Threading.Timer. Created for unit-testing.
    internal class SystemTimer
    {
        private readonly Timer timer;
        private string timerId;

        internal SystemTimer(TimerCallback callback, string timerId)
        {
            this.timer = new Timer(callback, null, Timeout.Infinite, Timeout.Infinite);
            this.timerId = timerId;
        }

        internal bool Change(long dueTime, long period)
        {
            return this.timer.Change(TimeSpan.FromMilliseconds(dueTime), TimeSpan.FromMilliseconds(period));
        }

        internal bool Dispose(WaitHandle notifyObject)
        {
#if DotNetCoreClrLinux
            this.timer.Dispose();
            ManualResetEvent ev = notifyObject as ManualResetEvent;
            ev.Set();
            return true;
#else
            return this.timer.Dispose(notifyObject);
#endif
        }
    }
}