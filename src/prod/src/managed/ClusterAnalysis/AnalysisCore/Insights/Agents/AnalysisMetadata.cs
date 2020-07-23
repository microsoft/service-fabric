// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.Agents
{
    using System;
    using System.Runtime.Serialization;
    using Common;
    using Common.Util;

    [DataContract]
    internal class AnalysisMetadata : IHasUniqueIdentity, IComparable<AnalysisMetadata>
    {
        public AnalysisMetadata(Guid analysisKey)
        {
            Assert.IsFalse(analysisKey == Guid.Empty, "AnalysisKey != empty");
            this.Key = analysisKey;
            this.SchedulingInfo = new AnalysisSchedulingInformation();
        }

        [DataMember]
        public Guid Key { get; private set; }

        [DataMember(IsRequired = true)]
        public DateTimeOffset CreateTime { get; internal set; }

        [DataMember(IsRequired = true)]
        public DateTimeOffset LastInvokedTime { get; internal set; }

        [DataMember(IsRequired = true)]
        public bool HasRegisteredInterest { get; internal set; }

        [DataMember(IsRequired = true)]
        public AnalysisSchedulingInformation SchedulingInfo { get; private set; }

        public int CompareTo(AnalysisMetadata other)
        {
            if (other == null)
            {
                throw new ArgumentNullException("other");
            }

            if (this.Key == other.Key)
            {
                return 0;
            }

            var nextActivationTimeFirst = this.SchedulingInfo.GetNextActivationTimeUtc();
            var nextActivationTimeSecond = other.SchedulingInfo.GetNextActivationTimeUtc();

            if (nextActivationTimeFirst.HasValue && nextActivationTimeSecond.HasValue)
            {
                return nextActivationTimeFirst.Value.CompareTo(nextActivationTimeSecond.Value);
            }

            if (nextActivationTimeFirst.HasValue)
            {
                return 1;
            }

            if (nextActivationTimeSecond.HasValue)
            {
                return -1;
            }

            return other.CreateTime.CompareTo(this.CreateTime);
        }

        public Guid GetUniqueIdentity()
        {
            return this.Key;
        }
    }
}