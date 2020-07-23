// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Store
{
    using System;

    /// <summary>
    /// Define age based retention policy
    /// </summary>
    public class AgeBasedRetentionPolicy : DataRetentionPolicy
    {
        /// <summary>
        /// One day
        /// </summary>
        public static readonly AgeBasedRetentionPolicy OneHour = new AgeBasedRetentionPolicy(TimeSpan.FromDays(1), TimeSpan.FromHours(1));

        /// <summary>
        /// One day
        /// </summary>
        public static readonly AgeBasedRetentionPolicy OneDay = new AgeBasedRetentionPolicy(TimeSpan.FromDays(1), TimeSpan.FromHours(1));

        /// <summary>
        /// Two day
        /// </summary>
        public static readonly AgeBasedRetentionPolicy TwoDay = new AgeBasedRetentionPolicy(TimeSpan.FromDays(2), TimeSpan.FromHours(2));

        /// <summary>
        /// Three day
        /// </summary>
        public static readonly AgeBasedRetentionPolicy ThreeDay = new AgeBasedRetentionPolicy(TimeSpan.FromDays(3), TimeSpan.FromHours(3));

        /// <summary>
        /// Five day
        /// </summary>
        public static readonly AgeBasedRetentionPolicy FiveDay = new AgeBasedRetentionPolicy(TimeSpan.FromDays(5), TimeSpan.FromHours(3));

        /// <summary>
        /// One Week
        /// </summary>
        public static readonly AgeBasedRetentionPolicy OneWeek = new AgeBasedRetentionPolicy(TimeSpan.FromDays(7), TimeSpan.FromHours(6));

        /// <summary>
        /// 2 Week Policy
        /// </summary>
        public static readonly AgeBasedRetentionPolicy TwoWeek = new AgeBasedRetentionPolicy(TimeSpan.FromDays(14), TimeSpan.FromHours(12));

        /// <summary>
        /// One month (30 days)
        /// </summary>
        public static readonly AgeBasedRetentionPolicy OneMonth = new AgeBasedRetentionPolicy(TimeSpan.FromDays(30), TimeSpan.FromHours(12));

        /// <summary>
        /// Till Eternity.
        /// </summary>
        public static readonly AgeBasedRetentionPolicy Forever = new AgeBasedRetentionPolicy(TimeSpan.MaxValue, TimeSpan.MaxValue);

        /// <summary>
        /// Create an instance of <see cref="AgeBasedRetentionPolicy"/>
        /// </summary>
        /// <param name="maxAge"></param>
        /// <param name="scrubDuration"></param>
        public AgeBasedRetentionPolicy(TimeSpan maxAge, TimeSpan scrubDuration)
        {
            this.MaxAgeOfRecord = maxAge;
            this.GapBetweenScrubs = scrubDuration;
        }

        /// <summary>
        /// Max age of the Record. Records older than this are removed
        /// </summary>
        public TimeSpan MaxAgeOfRecord { get; }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            if (obj == null)
            {
                return false;
            }

            var other = obj as AgeBasedRetentionPolicy;
            if (other == null)
            {
                return false;
            }

            return this.MaxAgeOfRecord == other.MaxAgeOfRecord && this.GapBetweenScrubs == other.GapBetweenScrubs;
        }

        public override int GetHashCode()
        {
            unchecked
            {
                int hash = 17;
                hash = (hash * 23) + this.MaxAgeOfRecord.GetHashCode();
                hash = (hash * 23) + this.GapBetweenScrubs.GetHashCode();
                return hash;
            }
        }
    }
}