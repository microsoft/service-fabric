// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.DataStructures;
using System.Fabric.Common;
using System.Linq;
using System.Timers;

namespace System.Fabric.BackupRestore
{
    internal class BRSTimer
    {
        private readonly Timer timer;
        private readonly Action timerCallback;

        internal BRSTimer(Action timerCallback)
        {
            this.timer = new Timer { AutoReset = false };
            this.timer.Elapsed += TimerElapsed;
            this.timerCallback = timerCallback;
        }

        private void TimerElapsed(object sender, ElapsedEventArgs elapsedEventArgs)
        {
            this.timerCallback();
        }

        internal void ArmTimer(double intervalInMilliSeconds)
        {
            this.timer.Interval = intervalInMilliSeconds;
            if (!this.timer.Enabled)
            {
                this.timer.Start();
            }
        }

        internal void Stop()
        {
            this.timer.Stop();
        }
    }
}