// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca 
{
    using System;
    using System.Threading;
    using DCAHostTest;
    using WEX.TestExecution;
    using System.Collections.Generic;

    // Wrapper around System.Threading.Timer. Created for unit-testing.
    class SystemTimer
    {
        private const string AppInstanceEtlReadTimerId = "AppInstanceEtlReadTimer";

        private Timer timer;
        private string timerId;
        private TimerCallback callback;
        private int callbackCount;

        public SystemTimer(TimerCallback callback, string timerId)
        {
            this.timer = new Timer(this.CallbackMethod);
            this.callback = callback;
            this.timerId = timerId;
        }

        public bool Change(long dueTime, long period)
        {
            return this.timer.Change(dueTime, period);
        }

        public bool Dispose(WaitHandle notifyObject)
        {
            return this.timer.Dispose(notifyObject);
        }

        public void CallbackMethod(object state)
        {
            if (AppInstanceEtlReadTimerId.Equals(this.timerId))
            {
                if (this.callbackCount >= DCAHostTest.CurrentTest.BeginEtlRead.Length)
                {
                    // The interesting parts of this test are over. Just make this a no-op.
                    return;
                }

                // Wait until we are allowed to run this callback
                bool eventSetResult = DCAHostTest.CurrentTest.BeginEtlRead[this.callbackCount].WaitOne();
                Verify.IsTrue(eventSetResult);

                // Increment the callback count before invoking the callback 
                // because the callback restarts the timer. So the next 
                // invocation of this method can happen before we return
                // from the callback below. If that happens, we want to make
                // sure it waits on the right event in the event array.
                (this.callbackCount)++;

                // Invoke the callback
                this.callback(null);

                // Indicate that we're done running this callback
                bool eventWaitResult = DCAHostTest.CurrentTest.EtlReadEnded[this.callbackCount-1].Set();
                Verify.IsTrue(eventWaitResult);
            }
        }
    }
}