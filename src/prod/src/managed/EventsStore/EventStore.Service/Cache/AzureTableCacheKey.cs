// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.Cache
{
    using System;
    using System.Globalization;

    internal class AzureTableCacheKey : ICacheKey, ITimeComparable
    {
        public AzureTableCacheKey(string partitionKey, string rowKey, DateTimeOffset dataFetchStart, DateTimeOffset dataFetchEnd, DateTimeOffset timeLogged, DateTimeOffset timeStamp)
        {
            Assert.IsNotEmptyOrNull(partitionKey, "partitionkey != null");
            Assert.IsNotEmptyOrNull(rowKey, "rowKey != null");
            Assert.IsFalse(dataFetchStart == DateTimeOffset.MinValue, "dataFetchStart == min");
            Assert.IsFalse(dataFetchEnd == DateTimeOffset.MinValue, "dataFetchEnd == min");

            this.PartitionKey = partitionKey;
            this.RowKey = rowKey;
            this.TimeLogged = timeLogged;
            this.TimeStamp = timeStamp;
            this.CreatedTime = DateTimeOffset.UtcNow;
            this.DataFetchStartTime = dataFetchStart;
            this.DataFetchEndTime = dataFetchEnd;
        }

        public string PartitionKey { get;}

        public string RowKey { get; }

        /// <summary>
        /// The time the Event was Logged.
        /// </summary>
        public DateTimeOffset TimeLogged { get; }

        /// <summary>
        /// The time the Event was last updated in Azure.
        /// </summary>
        public DateTimeOffset TimeStamp { get; }

        /// <inheritdoc/>
        public DateTimeOffset DataFetchStartTime { get; }

        /// <inheritdoc/>
        public DateTimeOffset DataFetchEndTime { get; }

        public DateTimeOffset OriginalCreateTime {  get { return this.TimeStamp; } }
        /// <inheritdoc/>
        public DateTimeOffset CreatedTime { get; }

        public int CompareTo(object obj)
        {
            if (obj == null)
            {
                throw new ArgumentNullException("obj");
            }

            var other = obj as AzureTableCacheKey;
            if (other == null)
            {
                throw new ArgumentException("obj");
            }

            if (this.PartitionKey == other.PartitionKey && this.RowKey == other.RowKey)
            {
                return 0;
            }

            if (this.TimeLogged == other.TimeLogged)
            {
                return this.TimeStamp.CompareTo(other.TimeStamp);
            }

            return this.TimeLogged.CompareTo(other.TimeLogged);
        }

        public override bool Equals(object obj)
        {
            return this.CompareTo(obj) == 0;
        }

        public override int GetHashCode()
        {
            unchecked
            {
                int hash = 17;
                hash = (hash * 23) + this.PartitionKey.GetHashCode();
                hash = (hash * 23) + this.RowKey.ToString().GetHashCode();
                hash = (hash * 23) + this.CreatedTime.GetHashCode();
                return hash;
            }
        }

        /// <summary>
        /// Compare the current key against a time range.
        /// </summary>
        /// <param name="startTime"></param>
        /// <param name="endTime"></param>
        /// <returns></returns>
        public int CompareTimeRangeOrder(DateTimeOffset startTime, DateTimeOffset endTime)
        {
            if (this.TimeLogged >= startTime && this.TimeLogged < endTime)
            {
                return 0;
            }

            if (this.TimeLogged < startTime)
            {
                return -1;
            }

            return 1;
        }

        public int CompareTimeOrder(DateTimeOffset time)
        {
            return this.TimeLogged.CompareTo(time);
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "PartitionKey: {0}, RowKey: {1}, TimeLogged: {2}, TimeStamp: {3}, KeyCreatedTime: {4}",
                this.PartitionKey,
                this.RowKey,
                this.TimeLogged,
                this.TimeStamp,
                this.CreatedTime);
        }
    }
}