// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca 
{
    using FabricDCA;
    using System.Threading;
    using MdsUploaderTest;
    using WEX.TestExecution;

    class SystemTimer
    {
        private Timer timer;
        private string timerId;
        private TimerCallback callback;
        private bool useEvents;
        private WaitHandle[] waitHandles;
        internal static int scaleDown = 1;
        internal static int ScaleDown
        {
            set
            {
                scaleDown = value;
            }
        }
        
        internal SystemTimer(TimerCallback callback, string timerId)
        {
            this.timer = new Timer(this.Callback);
            this.timerId = timerId;
            this.callback = callback;
            this.useEvents = this.timerId.Contains(MdsEtwEventUploader.DtrReadTimerIdSuffix);
            if (this.useEvents)
            {
                this.waitHandles = new WaitHandle[2];
                waitHandles[0] = MdsUploaderTest.StartDtrRead;
                waitHandles[1] = MdsUploaderTest.EndOfTest;
            }
        }

        internal bool Change(long dueTime, long period)
        {
            long dueTimeScaledDown = dueTime / scaleDown;
            return this.timer.Change(dueTimeScaledDown, period);
        }

        internal bool Dispose(WaitHandle notifyObject)
        {
            return this.timer.Dispose(notifyObject);
        }

        private void Callback(object state)
        {
            if (this.useEvents)
            {
                WaitHandle.WaitAny(waitHandles);
            }

            this.callback(state);

            if (this.useEvents)
            {
                bool result = MdsUploaderTest.DtrReadCompleted.Set();
                Verify.IsTrue(result, "Failed to set event to indicate DTR processing pass completion");
            }
        }
    }
}