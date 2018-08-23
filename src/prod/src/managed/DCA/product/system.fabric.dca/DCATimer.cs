// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca 
{
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Threading;

    // This class implements DCA's timer
    internal class DcaTimer
    {
        // Constants
        private const string TraceType = "Timer";
        private const int DueTimeVariationMaxPercent = 15;

        private readonly string timerId;
        private readonly SystemTimer timer;
        private readonly bool randomlyVaryDueTime;
        private readonly Random random;
        private long dueTime;
        private bool disposed;
    
        internal DcaTimer(string timerId, TimerCallback callback, long dueTime)
                    : this(timerId, callback, dueTime, false)
        {
        }

        internal DcaTimer(string timerId, TimerCallback callback, TimeSpan dueTime)
                    : this(timerId, callback, (long)dueTime.TotalMilliseconds, false)
        {
        }

        internal DcaTimer(string timerId, TimerCallback callback, TimeSpan dueTime, bool randomlyVaryDueTime)
            : this(timerId, callback, (long)dueTime.TotalMilliseconds, randomlyVaryDueTime)
        {
        }

        internal DcaTimer(string timerId, TimerCallback callback, long dueTime, bool randomlyVaryDueTime)
        {
            this.timerId = timerId;
            this.dueTime = dueTime;
            this.randomlyVaryDueTime = randomlyVaryDueTime;
            this.timer = new SystemTimer(callback, this.timerId);
            this.DisposedEvent = new ManualResetEvent(false);
            this.random = new Random();
            this.disposed = false;
        }

        // This event is signaled when the timer object has been disposed of
        internal ManualResetEvent DisposedEvent { get; private set; }

        internal void Start(TimeSpan dueTime)
        {
            this.Start((long)dueTime.TotalMilliseconds);
        }

        internal void Start(long dueTime)
        {
            if (dueTime != Timeout.Infinite)
            {
                this.dueTime = dueTime;
            }

            int variationPercent = this.randomlyVaryDueTime ? this.random.Next(DueTimeVariationMaxPercent) : 0;
            variationPercent = ((variationPercent % 2) == 0) ? variationPercent : (-1 * variationPercent);
            long variation = (this.dueTime * variationPercent) / 100;
            long effectiveDueTime = this.dueTime + variation;

            this.InternalStart(effectiveDueTime);
        }

        internal void Start()
        {
            this.Start(Timeout.Infinite);
        }

        internal void StartOnce(TimeSpan dueTime)
        {
            this.InternalStart((long)dueTime.TotalMilliseconds);
        }
    
        internal void StopAndDispose()
        {
            Utility.TraceSource.WriteInfo(
                TraceType,
                "The {0} timer is being stopped and disposed of ...",
                this.timerId);
                
            bool stopResult = true;
            bool disposeResult = true;

            lock (this)
            {
                stopResult = this.timer.Change(Timeout.Infinite, Timeout.Infinite);
                if (stopResult)
                {
                    disposeResult = this.timer.Dispose(this.DisposedEvent);
                }

                this.disposed = true;
            }

            System.Fabric.Interop.Utility.ReleaseAssert(
                stopResult,
                string.Format(CultureInfo.CurrentCulture, StringResources.DCAError_StopTimerFailed_Formatted, this.timerId));
            
            System.Fabric.Interop.Utility.ReleaseAssert(
                disposeResult,
                string.Format(CultureInfo.CurrentCulture, StringResources.DCAError_DisposeTimerFailed_Formatted, this.timerId));
        }

        private void InternalStart(long effectiveDueTime)
        {
            Utility.TraceSource.WriteInfo(
                TraceType,
                "The {0} timer is being started for {1} milliseconds...",
                this.timerId,
                effectiveDueTime);

            bool result = true;
            bool disposedBeforeStart = false;
            lock (this)
            {
                if (false == this.disposed)
                {
                    result = this.timer.Change(effectiveDueTime, Timeout.Infinite);
                }
                else
                {
                    disposedBeforeStart = true;
                }
            }

            if (disposedBeforeStart)
            {
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "The {0} timer cannot be started because it has already been disposed.",
                    this.timerId);
            }

            System.Fabric.Interop.Utility.ReleaseAssert(
                result,
                string.Format(CultureInfo.CurrentCulture, StringResources.DCAError_StartTimerFailed_Formatted, this.timerId));
        }
    }
}