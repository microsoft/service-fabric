// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Service
{
    using System;
    using System.Fabric.BackupRestore.Common.Model;
    using System.Fabric.BackupRestore.Enums;
    using System.Timers;
    using System.Fabric.BackupRestore;

    internal class RetentionScheduler
    {
        internal RetentionTimer retentionTimer;
        internal RescheduleTimer retentionRescheduleTimer;
        private string backupPolicyName;
        private const string TraceType = "RetentionRescheduler";
        private Action<string> callback;
        private bool isStopped;

        internal RetentionScheduler(string backupPolicyName, RetentionPolicy retentionPolicy, Action<string> callback)
        {
            this.backupPolicyName = backupPolicyName;
            this.retentionTimer = new RetentionTimer(this.timerCallback, retentionPolicy);
            this.retentionRescheduleTimer = new RescheduleTimer(this.timerCallback, "RetentionRescheduler");
            this.callback = callback;
        }

        internal void ArmTimer(RetentionMetadata retentionMetadata, bool updatePolicyToAddRetention = false)
        {
            if(!this.isStopped)
            {
                this.retentionTimer.armTimer(false, retentionMetadata, updatePolicyToAddRetention);
                this.retentionRescheduleTimer.Reset();
            }
        }

        internal void RearmTimer(bool postFailedScheduled, RetentionMetadata retentionMetadata)
        {
            if(postFailedScheduled && ! this.retentionRescheduleTimer.IsReschduleCountExhausted())
            {
                this.retentionRescheduleTimer.Reschedule();
            }
            else if(postFailedScheduled && this.retentionRescheduleTimer.IsReschduleCountExhausted())
            {
                this.retentionRescheduleTimer.Reset();
                this.retentionTimer.armTimer(true, retentionMetadata);
            }
            else
            {
                this.retentionTimer.armTimer(false, retentionMetadata);
            }
        }

        internal void Stop()
        {
            this.retentionTimer.Stop();
            this.retentionRescheduleTimer.Reset();
            this.isStopped = true;
        }

        private void timerCallback()
        {
            this.callback(this.backupPolicyName);
        }
    }
}