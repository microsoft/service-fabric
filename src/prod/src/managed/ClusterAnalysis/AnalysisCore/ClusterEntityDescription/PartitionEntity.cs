// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.ClusterEntityDescription
{
    using System;
    using System.Fabric.Query;
    using System.Globalization;
    using System.Runtime.Serialization;
    using ClusterAnalysis.Common.Util;

    /// <summary>
    /// Wrapper over Cluster Partition for easy unit testability. All props are internal so that
    /// they can be easily set by Unit test.
    /// </summary>
    [DataContract]
    public class PartitionEntity : BaseEntity
    {
        public PartitionEntity(Partition partitionObj)
        {
            Assert.IsNotNull(partitionObj, "Partition object can't be null");
            this.Id = partitionObj.PartitionInformation.Id;
            this.ServiceKind = partitionObj.ServiceKind;
            this.PartitionStatus = partitionObj.PartitionStatus;
        }

        internal PartitionEntity()
        {
        }

        [DataMember(IsRequired = true)]
        public ApplicationEntity Application { get; internal set; }

        [DataMember(IsRequired = true)]
        public ServiceEntity Service { get; internal set; }

        [DataMember(IsRequired = true)]
        public Guid Id { get; internal set; }

        [DataMember(IsRequired = true)]
        public ServiceKind ServiceKind { get; internal set; }

        [DataMember(IsRequired = true)]
        public ServicePartitionStatus PartitionStatus { get; internal set; }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "PartitionId: '{0}', ServiceKind: '{1}', PartitionStatus: '{2}'",
                this.Id,
                this.ServiceKind,
                this.PartitionStatus);
        }

        /// <inheritdoc />
        public override bool Equals(BaseEntity other)
        {
            Assert.IsNotNull(other);
            var otherObj = other as PartitionEntity;
            if (otherObj == null)
            {
                return false;
            }

            return this.Id == otherObj.Id;
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                int hash = 17;

                // Guid GetHascode only consider first 4 bytes so we get duplicates for system partition.
                // So we convert the Guid to Byte Code Array and then run HashCode on each Byte (you can't do that for
                // array as it would return arbitrary value)
                var byteCode = this.Id.ToByteArray();
                foreach (var oneByte in byteCode)
                {
                    hash = (hash * 23) + oneByte.GetHashCode();
                }

                return hash;
            }
        }

        /// <inheritdoc />
        public override int GetUniqueIdentity()
        {
            return this.GetHashCode();
        }
    }
}