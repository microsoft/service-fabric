// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Diagnostics;
using System.Fabric.Common;
using System.Threading;
using System.Threading.Tasks;

namespace System.Fabric.BackupRestore
{
    internal class BackupScheduler
    {
        private readonly BackupTimer _periodicTimer;
        private readonly RescheduleTimer _rescheduleTimer;
        private readonly Action _timerCallback;
        private int _syncPoint;         // This is used to synchronize the timer stop
        private bool _isStopped;
        private readonly BackupMetadata _backupMetadataObj;
        private const string BackupRescheduleTimerTraceType = "BackupRescheduleTimer";

        internal BackupScheduler(BackupMetadata backupMetadata, Action timerCallack)
        {
            this._backupMetadataObj = backupMetadata;
            this._periodicTimer = new BackupTimer(backupMetadata, TimerCallback);
            this._rescheduleTimer = new RescheduleTimer(TimerCallback, BackupRescheduleTimerTraceType);
            this._timerCallback = timerCallack;
        }

        private void TimerCallback()
        {
            if (_isStopped) return;

            var sync = Interlocked.CompareExchange(ref _syncPoint, 1, 0);
            if (sync == 0)
            {
                // Timer is supposed to run, Stop not invoked
                this._timerCallback();
            }

            Debug.Assert(sync != 1, String.Format("{0} Timer callback fired second time before resetting syncpoint", _backupMetadataObj.PartitionId)); // Looks like a case of re-entrancy.. 

            // Else, skip invoking the callback as the timer must have been asked to stop
        }

        internal void ArmTimer(bool checkedForMissedBackups)
        {
            // Reset the syncPoint to 0 back here.
            this._syncPoint = 0;

            // Reset the jitter used in the last backup
            this._backupMetadataObj.JitterInMillisUsedInLastBackup = 0;

            if (!this._isStopped)
            {
                this._rescheduleTimer.Reset();
                this._periodicTimer.ArmTimer(checkedForMissedBackups, false);
            }
        }

        internal void RearmTimer(bool previousBackupFailed)
        {
            // Reset the syncPoint to 0 back here.
            this._syncPoint = 0;

            // Reset the jitter used in the last backup
            this._backupMetadataObj.JitterInMillisUsedInLastBackup = 0;

            if (!this._isStopped)
            {
                if (previousBackupFailed && !this.IsRescheduleCountExhausted())
                {
                    this._rescheduleTimer.Reschedule();
                }
                else if (previousBackupFailed && this.IsRescheduleCountExhausted())
                {
                    this._rescheduleTimer.Reset();
                    this._periodicTimer.ArmTimer(false, true);
                }
                else
                {
                    this._rescheduleTimer.Reset();
                    this._periodicTimer.ArmTimer(false, false);
                }
            }
        }

        internal bool IsRescheduleCountExhausted()
        {
            return this._rescheduleTimer.IsReschduleCountExhausted();
        }

        /// <summary>
        /// Marks the scheduler in stopped state
        /// </summary>
        internal void Stop()
        {
            this._isStopped = true;
            this._rescheduleTimer.Reset();
            this._periodicTimer.Stop();

            Interlocked.CompareExchange(ref this._syncPoint, -1, 0);            // TODO: Do we need this syncPoint?
        }
    }
}