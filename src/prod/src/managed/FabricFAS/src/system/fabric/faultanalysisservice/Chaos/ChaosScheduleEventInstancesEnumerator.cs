//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Chaos
{
    using System.Collections;
    using System.Collections.Generic;
    using System.Fabric.Chaos.DataStructures;
    using Fabric.Common;

    internal sealed class ChaosScheduleEventInstancesEnumerator
        : IEnumerator<ChaosScheduleEventInstance>
    {
        private List<ChaosScheduleEventInstance> eventInstances;
        private int position;

        internal ChaosScheduleEventInstancesEnumerator(ChaosSchedule schedule, DateTime dayInWeek)
        {
            /*
            ChaosScheduleEventInstancesEnumerator that returns event instances based on the schedule definition.
            Schedules repeat on a weekly basis starting with Sunday as the first day of the week.

            In order to generate the event instances:
            1. Determine the starting Sunday of the week for which dayInWeek is in. This is the starting datetime of that week's repetition cycle.
            2. Convert the ChaosScheduleJobs of the schedule to a list of ChaosScheduleItems for each day of the week.
            3. Turn each schedule item into an ChaosScheduleEventInstance by using the day of the week and time as an offset to the starting Sunday
            4. Left join event instances if the start time of the future event is within one minute of the previous event's end time and they use the same ChaosParameters
            5. If there are 2 or more event instances left, join the last item to the first item if the last item's endtime is within a minute of the first item's start time plus a week (next cycle start).
            6. If there is 1 event instance left and it's end time is within one minute of it's start time plus 7 days (one cycle), then this event continues itself and
               should be just one long run until the schedule expires, so set the end time to the schedule's expiry date
             */
            this.position = -1;
            this.eventInstances = new List<ChaosScheduleEventInstance>();

            while (dayInWeek.DayOfWeek != DayOfWeek.Sunday)
            {
                dayInWeek = dayInWeek.AddDays(-1);
            }

            // dayInWeek is now the start of Sunday of the week.
            dayInWeek = new DateTime(dayInWeek.Year, dayInWeek.Month, dayInWeek.Day, 0, 0, 0, 0, DateTimeKind.Utc);

            var squashedSchedule = ChaosSchedulerUtil.GetSortedChaosScheduleItemsList(schedule);

            var fullEventInstances = new List<ChaosScheduleEventInstance>();

            foreach (DayOfWeek dayOfWeek in new List<DayOfWeek>() { DayOfWeek.Sunday, DayOfWeek.Monday, DayOfWeek.Tuesday, DayOfWeek.Wednesday, DayOfWeek.Thursday, DayOfWeek.Friday, DayOfWeek.Saturday })
            {
                foreach (ChaosScheduleItem scheduleEvent in squashedSchedule[dayOfWeek])
                {
                    fullEventInstances.Add(
                        new ChaosScheduleEventInstance(
                            scheduleEvent.ChaosParameters,
                            schedule.ChaosParametersDictionary[scheduleEvent.ChaosParameters],
                            dayInWeek.Add(new TimeSpan(scheduleEvent.Time.StartTime.Hour, scheduleEvent.Time.StartTime.Minute, 0)),
                            dayInWeek.Add(new TimeSpan(scheduleEvent.Time.EndTime.Hour, scheduleEvent.Time.EndTime.Minute, 0))));
                }

                dayInWeek = dayInWeek.AddDays(1);
            }

            foreach (var eventInstance in fullEventInstances)
            {
                if (this.eventInstances.Count == 0)
                {
                    this.eventInstances.Add(eventInstance);
                }
                else
                {
                    var lastEventInstance = this.eventInstances[this.eventInstances.Count - 1];
                    if (eventInstance.Start - lastEventInstance.End <= new TimeSpan(0, 1, 0) && lastEventInstance.ChaosParametersReferenceName == eventInstance.ChaosParametersReferenceName)
                    {
                        lastEventInstance.End = eventInstance.End;
                    }
                    else
                    {
                        this.eventInstances.Add(eventInstance);
                    }
                }
            }

            if (this.eventInstances.Count > 1)
            {
                if (this.eventInstances[0].Start.AddDays(7) - this.eventInstances[this.eventInstances.Count - 1].End <= new TimeSpan(0, 1, 0))
                {
                    this.eventInstances[0].Start = this.eventInstances[this.eventInstances.Count - 1].Start.AddDays(-7);
                    this.eventInstances.RemoveAt(this.eventInstances.Count - 1);
                }
            }

            if (this.eventInstances.Count == 1 && this.eventInstances[0].Start.AddDays(7) - this.eventInstances[0].End <= new TimeSpan(0, 1, 0))
            {
                this.eventInstances[0].End = new DateTime(schedule.ExpiryDate.Ticks, DateTimeKind.Utc);
            }
        }

        object IEnumerator.Current
        {
            get
            {
                return this.Current;
            }
        }

        public ChaosScheduleEventInstance Current
        {
            get
            {
                try
                {
                    return this.eventInstances[this.position];
                }
                catch (IndexOutOfRangeException)
                {
                    throw new InvalidOperationException("Did not call MoveNext prior to this method.");
                }
            }
        }

        public bool HasEvents()
        {
            return this.eventInstances.Count > 0;
        }

        public bool MoveNext()
        {
            if (this.eventInstances.Count > 0)
            {
                if (this.position > -1)
                {
                    this.eventInstances[this.position].UpdateToNextInstance();
                }

                this.position = (this.position + 1) % this.eventInstances.Count;
            }
            else
            {
                this.position = 0;
            }

            return this.position < this.eventInstances.Count;
        }

        public void Reset()
        {
            this.position = -1;
        }

        public void Dispose()
        {
            return;
        }
    }
}