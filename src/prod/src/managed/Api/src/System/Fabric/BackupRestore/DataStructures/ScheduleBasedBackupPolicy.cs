// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Collections.Generic;
using System.Fabric.BackupRestore.Interop;
using System.Fabric.Interop;
using System.Globalization;
using System.Linq;
using System.Text;

namespace System.Fabric.BackupRestore.DataStructures
{
    /// <summary>
    /// Frequency based backup scheduling policy
    /// </summary>
    [Serializable]
    internal class ScheduleBasedBackupPolicy : BackupPolicy
    {
        /// <summary>
        /// Run frequency type
        /// </summary>
        public BackupPolicyRunSchedule RunSchedule { get; set; }

        /// <summary>
        /// List of run times during a day
        /// </summary>
        public IList<TimeSpan> RunTimes { get; set; }

        /// <summary>
        /// List of run days of week
        /// </summary>
        public IList<DayOfWeek> RunDays { get; set; }

        /// <summary>
        /// Constructor
        /// </summary>
        public ScheduleBasedBackupPolicy()
            : base(BackupPolicyType.ScheduleBased)
        {
            RunTimes = new List<TimeSpan>();
            RunDays = new List<DayOfWeek>();
        }

        /// <summary>
        /// Gets a string representation of the schedule based backup policy object
        /// </summary>
        /// <returns>A string representation of the schedule based backup policy object</returns>
        public override string ToString()
        {
            var sb = new StringBuilder(base.ToString());
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "RunSchedule={0}", this.RunSchedule));

            if (this.RunTimes.Any())
            {
                sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "RunTimes:"));
                foreach (var runTime in this.RunTimes)
                {
                    sb.AppendLine(runTime.ToString());
                }
            }

            if (this.RunDays.Any())
            {
                sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "RunDays:"));
                foreach (var runDay in this.RunDays)
                {
                    sb.AppendLine(runDay.ToString());
                }
            }

            return sb.ToString();
        }

        #region Interop Helpers

        internal override IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeBackupPolicy = GetNativeBackupPolicy(pinCollection);
            var nativeSchedulePolicyDescription = new NativeBackupRestoreTypes.FABRIC_SCHEDULE_BASED_BACKUP_POLICY
            {
                RunScheduleType = (NativeBackupRestoreTypes.FABRIC_BACKUP_POLICY_RUN_SCHEDULE_MODE) RunSchedule,
                RunTimesList = ToNativeRunTimes(pinCollection, RunTimes),
                RunDays = ToRunDaysByte(RunDays),
            };
            
            nativeBackupPolicy.PolicyDescription = pinCollection.AddBlittable(nativeSchedulePolicyDescription);

            return pinCollection.AddBlittable(nativeBackupPolicy);
        }

        private static byte ToRunDaysByte(IList<DayOfWeek> runDays)
        {
            byte returnValue = 0;
            if (runDays == null) return returnValue;
            ;
            foreach (var runDay in runDays)
            {
                returnValue |= (byte) (1 << (int)runDay);
            }
            
            return returnValue;
        }

        private static IntPtr ToNativeRunTimes(PinCollection pin, IList<TimeSpan> runTimes)
        {
            if (runTimes == null)
            {
                return IntPtr.Zero;
            }

            var nativeRunTimesArray = new UInt32[runTimes.Count];
            for (var i = 0; i < runTimes.Count; ++i)
            {
                nativeRunTimesArray[i] = (UInt32)runTimes[i].TotalSeconds;
            }

            var nativeRuntimeList = new NativeBackupRestoreTypes.FABRIC_BACKUP_SCHEDULE_RUNTIME_LIST
            {
                Count = (uint)nativeRunTimesArray.Length,
                RunTimes = pin.AddBlittable(nativeRunTimesArray),
            };

            return pin.AddBlittable(nativeRuntimeList);
        }


        internal static unsafe ScheduleBasedBackupPolicy FromNative(NativeBackupRestoreTypes.FABRIC_BACKUP_POLICY backupPolicy)
        {
            var scheduleBasedPolicy = new ScheduleBasedBackupPolicy();
            scheduleBasedPolicy.PopulateBackupPolicyFromNative(backupPolicy);

            var scheduleBasedPolicyDescription =
                *(NativeBackupRestoreTypes.FABRIC_SCHEDULE_BASED_BACKUP_POLICY*)backupPolicy.PolicyDescription;

            scheduleBasedPolicy.RunSchedule = (BackupPolicyRunSchedule) scheduleBasedPolicyDescription.RunScheduleType;
            scheduleBasedPolicy.RunDays = GetRunDaysFromByte(scheduleBasedPolicyDescription.RunDays);
            scheduleBasedPolicy.RunTimes = GetRunTimesFromNative(scheduleBasedPolicyDescription.RunTimesList);
            return scheduleBasedPolicy;
        }

        private static unsafe IList<TimeSpan> GetRunTimesFromNative(IntPtr pointer)
        {
            var runTimesList = new List<TimeSpan>();
            var nativeRunTimesListPtr = (NativeBackupRestoreTypes.FABRIC_BACKUP_SCHEDULE_RUNTIME_LIST*) pointer;
            var nativeRunTimesArrayPtr = (UInt32*) nativeRunTimesListPtr->RunTimes;
            for (var i = 0; i < nativeRunTimesListPtr->Count; i++)
            {
                var nativeItem = *(nativeRunTimesArrayPtr + i);
                var managedTimeSpan = new TimeSpan(0, 0, 0, (int)nativeItem);
                runTimesList.Add(managedTimeSpan);
            }

            return runTimesList;
        }

        private static IList<DayOfWeek> GetRunDaysFromByte(byte runDays)
        {
            var runDaysList = new List<DayOfWeek>();
            for (var i = 0; i < 7; i++)
            {
                if ((runDays & (0x1 << i)) != 0)
                {
                    runDaysList.Add((DayOfWeek)i);
                }
            }
            return runDaysList;
        }

        #endregion
    }
}