// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.PerformanceTracker
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using ClusterAnalysis.Common.Util;

    public class PerformanceSession
    {
        private Dictionary<string, int> sessionDataInternal;

        private ITimeProvider timeProvider;

        public PerformanceSession(string sessionName, ITimeProvider timeProvider)
        {
            Assert.IsNotEmptyOrNull(sessionName);
            Assert.IsNotNull(timeProvider);
            this.timeProvider = timeProvider;
            this.SessionName = sessionName;
            this.SessionCreateTime = DateTimeOffset.UtcNow;
            this.sessionDataInternal = new Dictionary<string, int>();
            this.StartTime = DateTimeOffset.MinValue;
            this.EndTime = DateTimeOffset.MaxValue;
        }

        public DateTimeOffset SessionCreateTime { get; }

        public DateTimeOffset StartTime { get; private set; }

        public DateTimeOffset EndTime { get; private set; }

        public bool HasStarted
        {
            get { return this.StartTime != DateTimeOffset.MinValue; }
        }

        public bool HasEnded
        {
            get { return this.EndTime != DateTimeOffset.MaxValue; }
        }

        public string SessionName { get; }

        public IEnumerable<KeyValuePair<string, int>> CacheItems
        {
            get
            {
                // we are not exposing the raw dictionary now
                foreach (var item in this.sessionDataInternal)
                {
                    yield return item;
                }
            }
        }

        public int this[string key]
        {
            get { return this.sessionDataInternal[key]; }
        }

        public void StartSession()
        {
            if (this.StartTime != DateTimeOffset.MinValue)
            {
                throw new Exception(string.Format(CultureInfo.InvariantCulture, "Session {0} already started at time: {1}", this.SessionName, this.StartTime));
            }

            this.StartTime = this.timeProvider == null ? DateTimeOffset.UtcNow : this.timeProvider.UtcNow;
        }

        public void EndSession()
        {
            Assert.IsTrue(this.HasStarted, "Session Needs to be started first");
            if (this.EndTime != DateTimeOffset.MaxValue)
            {
                throw new Exception(string.Format(CultureInfo.InvariantCulture, "Session {0} already ended at time: {1}", this.SessionName, this.EndTime));
            }

            this.EndTime = this.timeProvider == null ? DateTimeOffset.UtcNow : this.timeProvider.UtcNow;
        }

        public void AddOrIncrementDataPoint(string name)
        {
            this.AddOrIncrementDataPoint(name, 1);
        }

        public void AddOrIncrementDataPoint(string name, int count)
        {
            Assert.IsTrue(this.HasStarted, "Session Needs to be started first");
            if (this.sessionDataInternal.ContainsKey(name))
            {
                this.sessionDataInternal[name] += count;
            }
            else
            {
                this.sessionDataInternal.Add(name, count);
            }
        }

        public override bool Equals(object obj)
        {
            if (obj == null)
            {
                return false;
            }

            var other = obj as PerformanceSession;
            if (other == null)
            {
                return false;
            }

            return this.SessionName == other.SessionName && this.SessionCreateTime == other.SessionCreateTime;
        }

        public override int GetHashCode()
        {
            unchecked
            {
                int hash = 17;
                hash = (hash * 23) + this.SessionName.GetHashCode();
                hash = (hash * 23) + this.SessionCreateTime.GetHashCode();
                return hash;
            }
        }
    }
}