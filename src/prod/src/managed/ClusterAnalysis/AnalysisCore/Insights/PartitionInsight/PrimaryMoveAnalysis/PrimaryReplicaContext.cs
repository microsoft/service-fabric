// ----------------------------------------------------------------------
//  <copyright file="PrimaryReplicaContext.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.PartitionInsight
{
    using System;
    using System.Globalization;
    using System.Runtime.Serialization;

    [DataContract]
    internal sealed class PrimaryReplicaContext
    {
        public PrimaryReplicaContext(
            Guid partitionId,
            string nodeName,
            string nodeId,
            long dateTimeUtcTicks)
        {
            this.PartitionId = partitionId;
            this.NodeName = nodeName;
            this.NodeId = nodeId;
            this.DateTimeUtcTicks = dateTimeUtcTicks;
        }

        [DataMember]
        public Guid PartitionId { get; set; }

        [DataMember]
        public string NodeName { get; set; }

        [DataMember]
        public string NodeId { get; set; }

        /// <summary>
        /// When the primary replica moved to the node
        /// </summary>
        [DataMember]
        public long DateTimeUtcTicks { get; set; }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "PartitionId: {0}, NodeName: {1}, NodeId: {2} TimeStampUtc: {3}",
                this.PartitionId,
                this.NodeName,
                this.NodeId,
                new DateTime(this.DateTimeUtcTicks, DateTimeKind.Utc));
        }
    }
}