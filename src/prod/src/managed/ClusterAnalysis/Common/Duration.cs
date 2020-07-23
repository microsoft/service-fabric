// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common
{
    using System;
    using System.Globalization;
    using ClusterAnalysis.Common.Util;

    /// <summary>
    /// Encapsulate a time period.
    /// </summary>
    public class Duration
    {
        /// <summary>
        /// Represent an empty time period
        /// </summary>
        public static readonly Duration EmptyDuration = new Duration(DateTime.MinValue, DateTime.MinValue);

        /// <summary>
        /// Create an Instance of <see cref="Duration"/>
        /// </summary>
        /// <param name="startTime">Start time in UTC</param>
        /// <param name="endTime">End time in UTC</param>
        public Duration(DateTime startTime, DateTime endTime)
        {
            if (endTime < startTime)
            {
                // throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "Endtime: {0} is < StartTime: {1}", endTime, startTime));
            }

            // TODO: Think of something better to enforce this.
            this.StartTime = DateTime.SpecifyKind(startTime, DateTimeKind.Utc);
            this.EndTime = DateTime.SpecifyKind(endTime, DateTimeKind.Utc);
        }

        /// <summary>
        /// Create an Instance of <see cref="Duration"/>
        /// </summary>
        /// <param name="startTimeTicks">Start time in UTC</param>
        /// <param name="endTimeTicks">End time in UTC</param>
        public Duration(long startTimeTicks, long endTimeTicks)
        {
            if (endTimeTicks < startTimeTicks)
            {
                // throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "Endtime: {0} is < StartTime: {1}", endTimeTicks, startTimeTicks));
            }

            this.StartTime = new DateTime(startTimeTicks, DateTimeKind.Utc);
            this.EndTime = new DateTime(endTimeTicks, DateTimeKind.Utc);
        }

        /// <summary>
        /// Start Time
        /// </summary>
        public DateTime StartTime { get; private set; }

        /// <summary>
        /// End Time
        /// </summary>
        public DateTime EndTime { get; private set; }

        public bool Includes(Duration other)
        {
            Assert.IsNotNull(other);
            return this.StartTime <= other.StartTime && this.EndTime >= other.EndTime;
        }

        public override string ToString()
        {
            return string.Format(
                "StartTime:'{0}', EndTime:'{1}'",
                this.StartTime.ToString("yyyy-MM-dd HH:mm:ss.fff", CultureInfo.InvariantCulture),
                this.EndTime.ToString("yyyy-MM-dd HH:mm:ss.fff", CultureInfo.InvariantCulture));
        }
    }
}