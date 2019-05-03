// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.RootCause
{
    using System.Runtime.Serialization;
    using ClusterAnalysis.AnalysisCore.ClusterEntityDescription;
    using ClusterAnalysis.Common.Util;

    /// <summary>
    /// </summary>
    [DataContract]
    [KnownType(typeof(ReplicaEntity))]
    public class ReplicaOpenFailed : IRootCause
    {
        /// <summary>
        /// </summary>
        /// <param name="replica"></param>
        /// <param name="failureMsg"></param>
        /// <param name="impact"></param>
        public ReplicaOpenFailed(ReplicaEntity replica, string failureMsg, string impact)
        {
            Assert.IsNotNull(replica, "ReplicaEntity can't be null");
            Assert.IsNotEmptyOrNull(impact, "impact");
            Assert.IsNotEmptyOrNull(failureMsg, "failureMsg");
            this.FailingReplica = replica;
            this.Failure = failureMsg;
            this.Impact = impact;
        }

        /// <summary>
        /// </summary>
        [DataMember(IsRequired = true)]
        public ReplicaEntity FailingReplica { get; private set; }

        /// <inheritdoc />
        [DataMember(IsRequired = true)]
        public string Failure { get; private set; }

        /// <inheritdoc />
        [DataMember(IsRequired = true)]
        public string Impact { get; private set; }

        /// <inheritdoc />
        public string GetUserFriendlySummary()
        {
            return string.Format(
                "Replica Open Failed for Replica ID: '{0}' In partition Id: '{1}' with Failure: '{2}' and Impact '{3}'",
                this.FailingReplica.Id,
                this.FailingReplica.PartitionId,
                this.Failure,
                this.Impact);
        }
    }
}