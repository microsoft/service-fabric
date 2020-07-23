// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.DataStructures;
using System.Fabric.Common;
using System.Globalization;
using System.Linq;
using System.Timers;

namespace System.Fabric.BackupRestore
{
    internal class BackupTimer
    {
        private readonly BackupPolicy policy;
        private readonly BackupMetadata backupMetadata;
        private static readonly Random Random = new Random();
        private int jitter;
        private const string TraceType = "BackupTimer";
        private BRSTimer backupTimer;
        
        internal BackupTimer(BackupMetadata metadata, Action timerCallback)
        {
            this.policy = metadata.Policy;
            this.backupMetadata = metadata;
            backupTimer = new BRSTimer(timerCallback);

            if (metadata.Policy.PolicyType == BackupPolicyType.ScheduleBased)
            {
                // Order the time of day and day of week lists 

                var derivedPolicy = (ScheduleBasedBackupPolicy) metadata.Policy;
                derivedPolicy.RunTimes = derivedPolicy.RunTimes.OrderBy(t => t).ToList();
                derivedPolicy.RunDays = derivedPolicy.RunDays.OrderBy(t => t).ToList();
            }

            LoadJiterConfig();
        }

        #region Internal Methods

        internal void ArmTimer(bool checkForMissedBackups, bool postFailedReschedule)
        {
            var currentDateTime = DateTime.UtcNow;
            double dueTime = 0;
            bool dueTimeInitialized = false;

            if (checkForMissedBackups)
            {
                
                // Check first if the timer was missed last time (affecting RPO), if so schedule backup now
                var missedBackup = CheckForMissedBackup(currentDateTime);
                if (missedBackup)
                {
                    // Schedule backup now
                    dueTime = 0;
                    dueTimeInitialized = true;
                }
            }

            if (!dueTimeInitialized)
            {
                var currentTime = currentDateTime;
                if (this.backupMetadata.LastRecoveryPoint != null && this.backupMetadata.LastRecoveryPoint.BackupTime > currentDateTime)
                {
                    currentTime = this.backupMetadata.LastRecoveryPoint.BackupTime;
                }

                var scheduledTimespan = GetNextScheduledRunTime(currentTime, postFailedReschedule);
                dueTime = (scheduledTimespan - currentDateTime.TimeOfDay).TotalSeconds;
            }

            var lowJitter = (int) (dueTime*1000 < this.jitter ? dueTime*(-1000) : jitter * -1);
      
            var jitterInMillis = Random.Next(lowJitter, jitter);
            
            // Store the jitter used so that it can be used to calculate the last recovery point time
            this.backupMetadata.JitterInMillisUsedInLastBackup = jitterInMillis;

            double intervalInMilliSeconds = dueTime * 1000 + jitterInMillis;

            AppTrace.TraceSource.WriteInfo(TraceType, "{0} Scheduling backup timer for interval {1}", this.backupMetadata.PartitionId, intervalInMilliSeconds);
            Console.WriteLine("Scheduling backup timer for interval {0}", intervalInMilliSeconds);

            this.backupTimer.ArmTimer(intervalInMilliSeconds);
        }

        internal TimeSpan GetNextScheduledRunTimeForTest(DateTime currentDateTime, bool postFailedReschedule)
        {
            var dueTimespan = GetNextScheduledRunTime(currentDateTime, postFailedReschedule);
            return (dueTimespan - currentDateTime.TimeOfDay);
        }

        internal bool CheckForMissedBackupForTest(DateTime currentDateTime)
        {
            return CheckForMissedBackup(currentDateTime);
        }

        internal void Stop()
        {
            this.backupTimer.Stop();
            AppTrace.TraceSource.WriteInfo(TraceType, "{0} Backup timer stopped", this.backupMetadata.PartitionId);
        }

        #endregion

        #region Private Methods

        private void LoadJiterConfig()
        {
            string jitterInSecondsString = null;

            try
            {
                var configStore = NativeConfigStore.FabricGetConfigStore();
                jitterInSecondsString = configStore.ReadUnencryptedString(BackupRestoreContants.BrsConfigSectionName, BackupRestoreContants.JitterInBackupsKeyName);
            }
            catch (Exception) { }

            var jitterInSeconds = String.IsNullOrEmpty(jitterInSecondsString) ? BackupRestoreContants.JitterInBackupsDefault : int.Parse(jitterInSecondsString, CultureInfo.InvariantCulture);
            this.jitter = Math.Abs(jitterInSeconds) *1000;
        }

        private TimeSpan GetTimeSinceLastBackup(DateTime currenDateTime)
        {
            var lastBackupTime = DateTime.MinValue;
            var lastRecoveryPoint = this.backupMetadata.LastRecoveryPoint;
            if (null != lastRecoveryPoint)
            {
                lastBackupTime = lastRecoveryPoint.BackupTime;
            }

            if (DateTime.MinValue == lastBackupTime ||
                lastBackupTime > currenDateTime)
            {
                return TimeSpan.Zero;
            }

            var elapsedTimespan = (currenDateTime - lastBackupTime);
            return elapsedTimespan;
        }

        private bool CheckForMissedBackup(DateTime currentDateTime)
        {
            // If there hasn't been any backup taken we assume no missed backups
            if (null == this.backupMetadata.LastRecoveryPoint) return false;

            // It could be that due to jitter added in backups, the last recovery point information saved is post the current time
            // When storing last recovery point, we store the time when it was intended to backup excluding the jitter
            if (this.backupMetadata.LastRecoveryPoint.BackupTime > currentDateTime) return false;

            // Check for missed backups since the last backup or the last time policy was updated.
            var startTime = GetTimeSinceLastBackup(currentDateTime);
            if (this.backupMetadata.PolicyUpdateTime > this.backupMetadata.LastRecoveryPoint.BackupTime)
            {
                startTime = currentDateTime - this.backupMetadata.PolicyUpdateTime;
            }

            if (BackupPolicyType.FrequencyBased == policy.PolicyType)
            {
                var derivedPolicy = (FrequencyBasedBackupPolicy)this.policy;
                
                // If the time since last backup is more than the current configured frequency, we assume we missed backup
                var frequencyTimespan = new TimeSpan(
                    derivedPolicy.RunFrequencyType == BackupPolicyRunFrequency.Hours ? (int)derivedPolicy.RunFrequency : 0,
                    derivedPolicy.RunFrequencyType == BackupPolicyRunFrequency.Minutes ? (int)derivedPolicy.RunFrequency : 0,
                    0);

                if (startTime > frequencyTimespan) return true;
            }
            else
            {
                // Policy is schedule based
                var derivedPolicy = (ScheduleBasedBackupPolicy)this.policy;

                // Get last time as per policy when backup was supposed to be scheduled
                TimeSpan timeSinceLastScheduledPoint;
                TimeSpan scheduledTimespan;
                switch (derivedPolicy.RunSchedule)
                {
                    case BackupPolicyRunSchedule.Daily:
                        try
                        {
                            scheduledTimespan = derivedPolicy.RunTimes.Last(t => t < currentDateTime.TimeOfDay);
                            timeSinceLastScheduledPoint = currentDateTime.TimeOfDay - scheduledTimespan;
                        }
                        catch (InvalidOperationException)
                        {
                            // Means we missed the timer yesterday
                            scheduledTimespan = derivedPolicy.RunTimes.Last().Subtract(new TimeSpan(1, 0, 0, 0));
                            timeSinceLastScheduledPoint = currentDateTime.TimeOfDay - scheduledTimespan;
                        }

                        break;
                    case BackupPolicyRunSchedule.Weekly:
                        DayOfWeek scheduledDay;
                        

                        try
                        {
                            // Lets find out last day when it was to be scheduled
                            scheduledDay = derivedPolicy.RunDays.Last(d => d <= currentDateTime.DayOfWeek);
                        }
                        catch (InvalidOperationException)
                        {
                            scheduledDay = derivedPolicy.RunDays.Last();
                        }

                        // If the schedule day is same as today, verify if the time to schedule is in past or future
                        if (scheduledDay == currentDateTime.DayOfWeek)
                        {
                            try
                            {
                                scheduledTimespan = derivedPolicy.RunTimes.Last(t => t < currentDateTime.TimeOfDay);
                                timeSinceLastScheduledPoint = currentDateTime.TimeOfDay - scheduledTimespan;
                                break;
                            }
                            catch (InvalidOperationException)
                            {
                                // Means we need to schedule for a previous day in days of week
                            }

                            try
                            {
                                scheduledDay = derivedPolicy.RunDays.Last(d => d < currentDateTime.DayOfWeek);
                            }
                            catch (InvalidOperationException)
                            {
                                scheduledDay = derivedPolicy.RunDays.Last();
                            }
                        }

                        // Let's get the number of days in between
                        var daysSinceLastScheduledRun = (currentDateTime.DayOfWeek - scheduledDay + 7) % 7;
                        if (daysSinceLastScheduledRun == 0) daysSinceLastScheduledRun += 7;

                        timeSinceLastScheduledPoint = new TimeSpan(daysSinceLastScheduledRun, 0, 0, 0) + currentDateTime.TimeOfDay - derivedPolicy.RunTimes.Last();

                        break;

                    default:
                        throw new ArgumentOutOfRangeException();
                }

                if (startTime > timeSinceLastScheduledPoint) return true;
            }

            return false;
        }

        private TimeSpan GetNextScheduledRunTime(DateTime currentDateTime, bool postFailedReschedule)
        {
            if (BackupPolicyType.FrequencyBased == policy.PolicyType)
            {
                var derivedPolicy = (FrequencyBasedBackupPolicy)this.policy;

                var elapsedTimespanSinceLastBackup = postFailedReschedule == true ? TimeSpan.Zero : GetTimeSinceLastBackup(currentDateTime);
                var dueTimespan = BackupPolicyRunFrequency.Hours == derivedPolicy.RunFrequencyType ? new TimeSpan((int)derivedPolicy.RunFrequency, 0, 0) : new TimeSpan(0, (int)derivedPolicy.RunFrequency, 0);

                // Let's take into account the time since last backup in calculating the next scheduled time to maintain consistent frequency
                var actualDueTimespan = dueTimespan - elapsedTimespanSinceLastBackup;

                // If the time since last backup exceeds the due timespan, we should schedule backup now
                return currentDateTime.TimeOfDay + (actualDueTimespan > TimeSpan.Zero ? actualDueTimespan : TimeSpan.Zero);
            }
            else
            {
                // Policy is schedule based
                var derivedPolicy = (ScheduleBasedBackupPolicy)this.policy;
                TimeSpan scheduleTimeSpan;
                switch (derivedPolicy.RunSchedule)
                {
                    case BackupPolicyRunSchedule.Daily:
                        try
                        {
                            scheduleTimeSpan = derivedPolicy.RunTimes.First(t => t >= currentDateTime.TimeOfDay);
                        }
                        catch (InvalidOperationException)
                        {
                            // Means we need to schedule for some time tomorrow. 
                            scheduleTimeSpan = derivedPolicy.RunTimes.First().Add(new TimeSpan(1, 0, 0, 0));
                        }

                        return scheduleTimeSpan;

                    case BackupPolicyRunSchedule.Weekly:
                        DayOfWeek scheduleDay;

                        try
                        {
                            // Lets find out first day to schedule next
                            scheduleDay = derivedPolicy.RunDays.First(d => d >= currentDateTime.DayOfWeek);
                        }
                        catch (InvalidOperationException)
                        {
                            scheduleDay = derivedPolicy.RunDays.First();
                        }

                        // If the schedule day is same as today, verify if the time to schedule is in past or future
                        if (scheduleDay == currentDateTime.DayOfWeek)
                        {
                            try
                            {
                                scheduleTimeSpan = derivedPolicy.RunTimes.First(t => t >= currentDateTime.TimeOfDay);
                                return scheduleTimeSpan;
                            }
                            catch (InvalidOperationException)
                            {
                                // Means we need to schedule for next day in days of week
                            }

                            try
                            {
                                scheduleDay = derivedPolicy.RunDays.First(d => d > currentDateTime.DayOfWeek);
                            }
                            catch (InvalidOperationException)
                            {
                                scheduleDay = derivedPolicy.RunDays.First();
                            }
                        }

                        // Let's get the number of days in between
                        var daysTillNextRun = (scheduleDay - currentDateTime.DayOfWeek + 7) % 7;
                        if (daysTillNextRun == 0) daysTillNextRun += 7;
                        
                        scheduleTimeSpan = new TimeSpan(daysTillNextRun, 0, 0, 0) + derivedPolicy.RunTimes.First();
                        return scheduleTimeSpan;

                    default:
                        throw new ArgumentOutOfRangeException();
                }
            }
        }

        #endregion
    }
}