// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Timers;

namespace System.Fabric.BackupRestore
{
    internal class RescheduleTimer
    {
        private readonly Timers.Timer _rescheduleTimer;
        private const int RescheduleMaxCount = 5;
        private const int RetryIntervalInMinutes = 1;
        private static readonly Random Random = new Random();
        private uint _rescheduleCount;
        private const int JitterLow = 95;
        private const int JitterHigh = 105;
        private string TraceType;
        private readonly Action _timerCallback;
        private bool _timerEnabledOnce;

        internal RescheduleTimer(Action timerCallback, string TraceType)
        {
            this._rescheduleTimer = new Timers.Timer {AutoReset = false};
            this._rescheduleTimer.Elapsed += RescheduleTimerOnElapsed;
            this._timerCallback = timerCallback;
            this.TraceType = TraceType;
        }

        private void RescheduleTimerOnElapsed(object sender, ElapsedEventArgs elapsedEventArgs)
        {
            this._timerCallback();
        }

        internal void Reschedule()
        {
            _rescheduleCount++;
            if (_rescheduleCount > RescheduleMaxCount)
            {
                this._rescheduleCount = 0;
                throw new OperationCanceledException(String.Format("Exceeded max reschedule attempts configured: {0}", RescheduleMaxCount));
            }

            this._rescheduleTimer.Interval = RetryIntervalInMinutes * 60 * 10 * Random.Next(JitterLow, JitterHigh + 1);

            if (!this._timerEnabledOnce)
            {
                this._rescheduleTimer.Start();
                this._timerEnabledOnce = true;
            }
        }

        internal bool IsReschduleCountExhausted()
        {
            return (_rescheduleCount == RescheduleMaxCount);
        }

        internal void Reset ()
        {
            this._rescheduleCount = 0;
            this._rescheduleTimer.Stop();
        }
    }
}