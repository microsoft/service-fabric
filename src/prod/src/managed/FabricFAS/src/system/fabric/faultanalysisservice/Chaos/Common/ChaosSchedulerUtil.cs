//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Chaos
{
    using System.Collections.Generic;
    using System.Fabric.Chaos.Common;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Common;
    using System.Fabric.JsonSerializerImpl;
    using System.Fabric.Strings;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;
    using Text;

    internal static class ChaosSchedulerUtil
    {
        private const string TraceType = "ChaosSchedulerUtil";

        private static JsonSerializerSettings chaosDescriptionSerializationSettings;
        private static JsonSerializerSettings chaosScheduleDescriptionSerializationSettings;

        static ChaosSchedulerUtil()
        {
            chaosDescriptionSerializationSettings = new JsonSerializerSettings
            {
                ReferenceLoopHandling = ReferenceLoopHandling.Serialize,
                NullValueHandling = NullValueHandling.Ignore,
                Converters = new List<JsonConverter>
                {
                    new StringEnumConverter(),
                    new ChaosParametersJsonConverter(),
                }
            };

            chaosScheduleDescriptionSerializationSettings = new JsonSerializerSettings
            {
                DateTimeZoneHandling = DateTimeZoneHandling.Utc,
                ContractResolver = new DictionaryAsArrayResolver(),
                Converters = new List<JsonConverter>
                {
                    new ChaosParametersJsonConverter(),
                    new StringEnumConverter(),
                    new Newtonsoft.Json.Converters.IsoDateTimeConverter
                    {
                        DateTimeFormat = "yyyy'-'MM'-'dd'T'HH':'mm':'ss.fff'Z'"
                    }
                }
            };
        }

#region  JSON serialization/deserialization

        public static string SerializeChaosDescription(ChaosDescription description)
        {
            return JsonConvert.SerializeObject(
                description,
                chaosDescriptionSerializationSettings);
        }

        public static string SerializeChaosScheduleDescription(ChaosScheduleDescription description)
        {
            return JsonConvert.SerializeObject(
                description,
                chaosScheduleDescriptionSerializationSettings);
        }

        public static ChaosScheduleDescription DeserializeChaosScheduleDescription(string json)
        {
            return JsonConvert.DeserializeObject<ChaosScheduleDescription>(
                json,
                chaosScheduleDescriptionSerializationSettings);
        }

#endregion

        public static Dictionary<DayOfWeek, List<ChaosScheduleItem>> GetSortedChaosScheduleItemsList(ChaosSchedule schedule)
        {
            var result = new Dictionary<DayOfWeek, List<ChaosScheduleItem>>();

            foreach (DayOfWeek day in Enum.GetValues(typeof(DayOfWeek)))
            {
                result.Add(day, new List<ChaosScheduleItem>());
            }

            for (int jobIndex = 0; jobIndex < schedule.Jobs.Count; jobIndex++)
            {
                var job = schedule.Jobs[jobIndex];
                foreach (DayOfWeek day in Enum.GetValues(typeof(DayOfWeek)).Cast<DayOfWeek>())
                {
                    if (job.Days.GetDayValueByEnum(day))
                    {
                        foreach (var time in job.Times)
                        {
                            result[day].Add(new ChaosScheduleItem(jobIndex, job.ChaosParameters, time));
                        }
                    }
                }
            }

            // for each Day, sort the items by starttime from earliest to latest.
            foreach (DayOfWeek day in Enum.GetValues(typeof(DayOfWeek)))
            {
                result[day].Sort((a, b) => a.Time.StartTime.CompareTo(b.Time.StartTime));
            }

            return result;
        }

        public static async Task VerifyChaosScheduleAsync(
            ChaosSchedule schedule,
            FabricClient fabricClient,
            CancellationToken cancellationToken)
        {
            if (schedule == null)
            {
                throw new System.ArgumentNullException("Schedule", StringResources.ChaosScheduler_ScheduleIsNull);
            }

            if (schedule.StartDate == null)
            {
                throw new System.ArgumentNullException("StartDate", StringResources.ChaosScheduler_ScheduleStartDateIsNull);
            }

            if (schedule.ExpiryDate == null)
            {
                throw new System.ArgumentNullException("ExpiryDate", StringResources.ChaosScheduler_ScheduleExpiryDateIsNull);
            }

            if (schedule.StartDate < ChaosConstants.FileTimeMinDateTime)
            {
                throw new System.ArgumentException(string.Format(StringResources.ChaosScheduler_ScheduleStartDateBeforeFileTimeEpoch, schedule.StartDate), "StartDate");
            }

            if (schedule.ExpiryDate < ChaosConstants.FileTimeMinDateTime)
            {
                throw new System.ArgumentException(string.Format(StringResources.ChaosScheduler_ScheduleExpiryDateBeforeFileTimeEpoch, schedule.ExpiryDate), "ExpiryDate");
            }

            if (schedule.ExpiryDate < schedule.StartDate)
            {
                throw new System.ArgumentException(string.Format(StringResources.ChaosScheduler_ScheduleExpiryDateBeforeStartDate, schedule.ExpiryDate, schedule.StartDate), "ExpiryDate");
            }

            if (schedule.ChaosParametersDictionary == null)
            {
                throw new System.ArgumentNullException("ChaosParametersDictionary", StringResources.ChaosScheduler_ScheduleParametersDictionaryIsNull);
            }

            foreach (var chaosParamDictionaryEntry in schedule.ChaosParametersDictionary)
            {
                await ChaosUtil.ValidateChaosTargetFilterAsync(
                    fabricClient,
                    chaosParamDictionaryEntry.Value,
                    new TimeSpan(0, 1, 0),
                    new TimeSpan(0, 1, 0),
                    cancellationToken).ConfigureAwait(false);
            }

            if (schedule.Jobs == null)
            {
                throw new System.ArgumentNullException("Jobs", StringResources.ChaosScheduler_ScheduleJobsIsNull);
            }

            // Validate each of the items before validating the combination of the items
            foreach (var job in schedule.Jobs)
            {
                ChaosSchedulerUtil.VerifyChaosScheduleJob(job);
            }

            ChaosSchedulerUtil.FindMissingChaosParameterReferences(schedule);
            ChaosSchedulerUtil.FindScheduleConflicts(schedule);
        }

        /// <summary>
        /// Create a schedule that runs chaos 24/7 using a particular ChaosParameters. This is useful for create schedules that simulate the old StartChaos
        /// </summary>
        /// <param name="chaosParameters">Parameteres to run chaos with.</param>
        /// <param name="startTime">Time for schedule to become active</param>
        /// <param name="endTime">Time for schedule to expire</param>
        /// <returns>A 24/7 schedule that uses one set of ChaosParameters.</returns>
        public static ChaosSchedule Create247Schedule(ChaosParameters chaosParameters, DateTime startTime, DateTime endTime)
        {
            var chaosParametersDictionary = new Dictionary<string, ChaosParameters>();
            chaosParametersDictionary.Add(ChaosConstants.ChaosScheduler_DefaultParameterKey, chaosParameters);

            List<ChaosScheduleTimeRangeUtc> times = new List<ChaosScheduleTimeRangeUtc>() { ChaosScheduleTimeRangeUtc.WholeDay };
            var everyDay = new HashSet<DayOfWeek>() { DayOfWeek.Sunday, DayOfWeek.Monday, DayOfWeek.Tuesday, DayOfWeek.Wednesday, DayOfWeek.Thursday, DayOfWeek.Friday, DayOfWeek.Saturday };
            ChaosScheduleJobActiveDays activeEveryday = new ChaosScheduleJobActiveDays(everyDay);
            ChaosScheduleJob job = new ChaosScheduleJob(ChaosConstants.ChaosScheduler_DefaultParameterKey, activeEveryday, times);
            var chaosScheduleJobs = new List<ChaosScheduleJob>() { job };

            ChaosSchedule schedule = new ChaosSchedule(startTime, endTime, chaosParametersDictionary, chaosScheduleJobs);

            return schedule;
        }

        private static void VerifyChaosScheduleJob(ChaosScheduleJob job)
        {
            if (job.Days == null)
            {
                throw new System.ArgumentNullException("Days", StringResources.ChaosScheduler_ScheduleJobDaysIsNull);
            }

            if (job.Days.NoActiveDays())
            {
                throw new System.ArgumentException(StringResources.ChaosScheduler_ScheduleJobNoDaysSet, "Days");
            }

            if (job.Times == null)
            {
                throw new System.ArgumentNullException("Times", StringResources.ChaosScheduler_ScheduleJobTimesIsNull);
            }

            if (job.Times.Count < 1)
            {
                throw new System.ArgumentException(StringResources.ChaosScheduler_ScheduleJobEndTimeAfterEndOfDay, "Times");
            }

            foreach (var time in job.Times)
            {
                ChaosSchedulerUtil.VerifyChaosScheduleTimeRangeUtc(time);
            }
        }

        private static void VerifyChaosScheduleTimeRangeUtc(ChaosScheduleTimeRangeUtc timerange)
        {
            if (timerange.StartTime == null)
            {
                throw new System.ArgumentNullException(
                    "StartTime",
                    StringResources.ChaosScheduler_ScheduleJobTimeIsNull);
            }

            if (timerange.EndTime == null)
            {
                throw new System.ArgumentNullException(
                    "EndTime",
                    StringResources.ChaosScheduler_ScheduleJobTimeIsNull);
            }

            ChaosSchedulerUtil.VerifyChaosScheduleTimeUtc(timerange.StartTime);
            ChaosSchedulerUtil.VerifyChaosScheduleTimeUtc(timerange.EndTime);

            if (timerange.StartTime > timerange.EndTime)
            {
                throw new System.ArgumentException(
                    string.Format(
                        StringResources.ChaosScheduler_ScheduleJobStartTimeAfterEndTime,
                        timerange.StartTime.ToString(),
                        timerange.EndTime.ToString()),
                    "StartTime");
            }

            if (timerange.StartTime < ChaosScheduleTimeUtc.StartOfDay)
            {
                throw new System.ArgumentException(
                    string.Format(
                        StringResources.ChaosScheduler_ScheduleJobStartTimeBeforeStartOfDay,
                        timerange.StartTime.ToString(),
                        ChaosScheduleTimeUtc.StartOfDay.ToString()),
                    "StartTime");
            }

            if (timerange.StartTime > ChaosScheduleTimeUtc.EndOfDay)
            {
                throw new System.ArgumentException(
                    string.Format(
                        StringResources.ChaosScheduler_ScheduleJobStartTimeAfterEndOfDay,
                        timerange.StartTime.ToString(),
                        ChaosScheduleTimeUtc.EndOfDay.ToString()),
                    "StartTime");
            }

            if (timerange.EndTime < ChaosScheduleTimeUtc.StartOfDay)
            {
                throw new System.ArgumentException(
                    string.Format(
                        StringResources.ChaosScheduler_ScheduleJobEndTimeBeforeStartOfDay,
                        timerange.EndTime.ToString(),
                        ChaosScheduleTimeUtc.StartOfDay.ToString()),
                    "EndTime");
            }

            if (timerange.EndTime > ChaosScheduleTimeUtc.EndOfDay)
            {
                throw new System.ArgumentException(
                    string.Format(
                        StringResources.ChaosScheduler_ScheduleJobEndTimeAfterEndOfDay,
                        timerange.EndTime.ToString(),
                        ChaosScheduleTimeUtc.EndOfDay.ToString()),
                    "EndTime");
            }
        }

        private static void VerifyChaosScheduleTimeUtc(ChaosScheduleTimeUtc time)
        {
            if (time.Hour < 0)
            {
                throw new System.ArgumentException(string.Format(StringResources.ChaosScheduler_TimeHourUnderLimit, time.Hour), "Hour");
            }
            else if (time.Hour > 23)
            {
                throw new System.ArgumentException(string.Format(StringResources.ChaosScheduler_TimeHourOverLimit, time.Hour), "Hour");
            }
            else if (time.Minute < 0)
            {
                throw new System.ArgumentException(string.Format(StringResources.ChaosScheduler_TimeMinuteUnderLimit, time.Minute), "Minute");
            }
            else if (time.Minute > 59)
            {
                throw new System.ArgumentException(string.Format(StringResources.ChaosScheduler_TimeMinuteOverLimit, time.Minute), "Minute");
            }
        }

        private static void FindMissingChaosParameterReferences(ChaosSchedule schedule)
        {
            bool hasError = false;
            var squashedSchedule = ChaosSchedulerUtil.GetSortedChaosScheduleItemsList(schedule);

            var errorStrings = new List<string>() { StringResources.ChaosScheduler_NonExistentChaosParameter };

            foreach (DayOfWeek day in ChaosSchedule.AllDaysOfWeek)
            {
                var dayErrors = ChaosSchedulerUtil.FindMissingChaosParameterReferencesForDay(schedule, squashedSchedule[day]);
                var dayHasError = dayErrors.Count > 0;
                hasError = hasError || dayHasError;
                if (dayHasError)
                {
                    errorStrings.Add(string.Format("{0}: {1}", Enum.GetName(typeof(DayOfWeek), day), JsonConvert.SerializeObject(dayErrors)));
                }
            }

            if (hasError)
            {
                throw new System.ArgumentException(string.Join("\n", errorStrings).Replace(", [", ",\n["));
            }
        }

        private static List<ChaosScheduleItem> FindMissingChaosParameterReferencesForDay(ChaosSchedule schedule, List<ChaosScheduleItem> dayItems)
        {
            var result = new List<ChaosScheduleItem>();
            foreach (var scheduleItem in dayItems)
            {
                if (!schedule.ChaosParametersDictionary.ContainsKey(scheduleItem.ChaosParameters))
                {
                    result.Add(scheduleItem);
                }
            }

            return result;
        }

        /// <summary>
        /// Nice prints out the sets of conflicts.
        /// </summary>
        /// <param name="conflicts">A list of sets of schedule items that conflict.</param>
        /// <returns>A nicely formatted string representation of the conflicting scheduling items detailing the Job it originated from and the time range involved for each item in the set.</returns>
        private static string PrettyPrintScheduleConflicts(List<HashSet<ChaosScheduleItem>> conflicts)
        {
            var formattedConflicts = new StringBuilder();

            char[] listEndToTrim = { ',', '\n', ' ' };

            foreach (var conflictSet in conflicts)
            {
                var formattedConflictSet = new StringBuilder();

                int conflictingItemIndex = 0;
                foreach (var conflictingItem in conflictSet)
                {
                    formattedConflictSet.Append(
                        string.Format(
                            "{0}{{ JobIndex:{1}, Timerange:{2:D2}:{3:D2}~{4:D2}:{5:D2} }},\n",
                            conflictingItemIndex == 0 ? string.Empty : "  ",
                            conflictingItem.JobIndex,
                            conflictingItem.Time.StartTime.Hour,
                            conflictingItem.Time.StartTime.Minute,
                            conflictingItem.Time.EndTime.Hour,
                            conflictingItem.Time.EndTime.Minute));
                    conflictingItemIndex += 1;
                }

                formattedConflicts.Append(string.Format(" [{0}],\n \n", formattedConflictSet.ToString().TrimEnd(listEndToTrim)));
            }

            return string.Format("[\n{0}\n]", formattedConflicts.ToString().TrimEnd(listEndToTrim));
        }

        private static void FindScheduleConflicts(ChaosSchedule schedule)
        {
            bool hasError = false;
            var squashedSchedule = ChaosSchedulerUtil.GetSortedChaosScheduleItemsList(schedule);

            var errorString = new StringBuilder();

            foreach (DayOfWeek day in ChaosSchedule.AllDaysOfWeek)
            {
                var conflicts = ChaosSchedulerUtil.FindScheduleConflictsForDay(squashedSchedule[day]);
                bool dayHasError = conflicts.Count > 0;

                hasError = hasError || dayHasError;
                if (dayHasError)
                {
                    errorString.AppendLine(string.Format("{0} :\n{1}", Enum.GetName(typeof(DayOfWeek), day), ChaosSchedulerUtil.PrettyPrintScheduleConflicts(conflicts)));
                }
            }

            if (hasError)
            {
                var ret = string.Format("{0}\n \n{1}", errorString.ToString(), StringResources.ChaosScheduler_ScheduleConflict);

                if (ret.Length > Constants.ErrorMessageMaxCharLength)
                {
                    var moreString = string.Format("...\n \n{0}\n{1}\n", StringResources.ChaosScheduler_ScheduleConflict, StringResources.ChaosScheduler_MoreScheduleConflicts);
                    ret = ret.Substring(0, Constants.ErrorMessageMaxCharLength - moreString.Length) + moreString;
                }

                throw new System.ArgumentException(ret);
            }
        }

        private static List<HashSet<ChaosScheduleItem>> FindScheduleConflictsForDay(List<ChaosScheduleItem> dayItems)
        {
            var conflicts = new List<HashSet<ChaosScheduleItem>>();
            var conflictSets = new List<HashSet<ChaosScheduleItem>>();

            // Sort by starting time, earliest to latest.
            dayItems.Sort((a, b) => a.Time.StartTime.CompareTo(b.Time.StartTime));

            foreach (var scheduleItem in dayItems)
            {
                bool overLappingSet = false;

                if (conflictSets.Count > 0)
                {
                    /*
                     * Only have to compare to the latest conflict set because dayItems is sorted by starting time earliest to latest.
                     * This means the current scheduleItem can only start on or after all members in the latest conflictSet which
                     * starts after all the ending times of the previous conflict set.
                     */
                    foreach (var conflictSetMember in conflictSets[conflictSets.Count - 1])
                    {
                        if (scheduleItem.Time.StartTime < conflictSetMember.Time.EndTime)
                        {
                            overLappingSet = true;
                            break;
                        }
                    }
                }

                if (overLappingSet)
                {
                    conflictSets[conflictSets.Count - 1].Add(scheduleItem);
                }
                else
                {
                    // the scheduleItem starts after all members of the last conflictSet (if one existed)
                    conflictSets.Add(new HashSet<ChaosScheduleItem>() { scheduleItem });
                }
            }

            foreach (var conflictSet in conflictSets)
            {
                if (conflictSet.Count > 1)
                {
                    // Only a conflict set of two or more schedule items have a conflict.
                    conflicts.Add(conflictSet);
                }
            }

            return conflicts;
        }
    }
}