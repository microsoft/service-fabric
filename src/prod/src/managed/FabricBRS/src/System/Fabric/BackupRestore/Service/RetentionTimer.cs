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
    using System.Fabric.Common;

    internal class RetentionTimer
    {
        private BRSTimer timer;
        private readonly RetentionPolicy retentionPolicy;
        private DateTime lastTimerSetTime;
        private const string TraceType = "RetentionsTimer";
        private const int jitterHigh = 20;
        private const int jitterLow = 1;

        internal RetentionTimer(Action timerCallback, RetentionPolicy retentionPolicy)
        {
            this.timer = new BRSTimer(timerCallback);
            this.lastTimerSetTime = DateTime.MinValue;
            this.retentionPolicy = retentionPolicy;
        }

        internal void armTimer(bool postFailedReschedule, RetentionMetadata retentionMetadata, bool updatePolicyToAddRetention = false)
        {

            TimeSpan dueTime = TimeSpan.Zero;
            Random rand = new Random();
            dueTime = dueTime + TimeSpan.FromMilliseconds((double)rand.Next(jitterLow, jitterHigh));
            if (updatePolicyToAddRetention)
            {
                this.timer.ArmTimer(dueTime.TotalMilliseconds);
                return;
            }
            TimeSpan timeSinceLastRetention = postFailedReschedule == true ? TimeSpan.Zero : GetTimeSinceLastRetentionCompletion(DateTime.Now, retentionMetadata);
            TimeSpan dueTimeForNextRetention = GetDueTimeForNextRetentionSchedule(retentionMetadata);
            if (dueTimeForNextRetention >= timeSinceLastRetention)
            {
                dueTime = dueTime + (dueTimeForNextRetention - timeSinceLastRetention);
            }

            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, " Timer armed to run after {0} milliseconds ", dueTime.TotalMilliseconds);
            this.timer.ArmTimer(dueTime.TotalMilliseconds);
        }

        internal TimeSpan GetDueTimeForNextRetentionSchedule(RetentionMetadata retentionMetadata)
        {
            if (this.retentionPolicy.RetentionPolicyType == RetentionPolicyType.Basic)
            {
                var derivedRetentionPolicy = (BasicRetentionPolicy)this.retentionPolicy;
                var retentionDuration = derivedRetentionPolicy.RetentionDuration;

                if(retentionMetadata.LastRetentionStartTime == DateTime.MinValue)
                {
                    return TimeSpan.Zero;
                }

                if ((int)retentionDuration.TotalDays != 0)
                {
                    return TimeSpan.FromDays(1);
                }
                else if ((int)retentionDuration.TotalHours != 0)
                {
                    return TimeSpan.FromHours(1);
                }
                else
                {
                    throw new ArgumentException(StringResources.InvalidRetentionInterval);
                }
            }
            else
            {
                // Implement the timer code for advanced.
                throw new NotImplementedException();
            }
        }

        internal TimeSpan GetTimeSinceLastRetentionCompletion(DateTime currentDateTime, RetentionMetadata retentionMetadata)
        {
            TimeSpan timeSinceLastRetentionCompletion = TimeSpan.Zero; // Returning zero means we will wait for retention time.

            if (retentionMetadata.LastRetentionCompletionTime != DateTime.MinValue)
            {
                // It means retention completed atleast once. => We need to wait on the basis of time.
                timeSinceLastRetentionCompletion = currentDateTime - retentionMetadata.LastRetentionCompletionTime;
            }
            else if(retentionMetadata.LastRetentionStartTime != DateTime.MinValue)
            {
                // It means retention never completed once, but we have waited => We need retention right now.
                timeSinceLastRetentionCompletion = currentDateTime - retentionMetadata.LastRetentionCompletionTime;
            }

            return timeSinceLastRetentionCompletion;
        }

        internal void Stop()
        {
            this.timer.Stop();
        }
    }
}