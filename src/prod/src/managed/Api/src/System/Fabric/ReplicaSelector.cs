// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using Globalization;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Query;
    using System.Fabric.Strings;

    /// <summary>
    /// This is a helper class for selecting replicas. 
    /// </summary>
    /// <remarks>
    /// It allows the user to select replicas to be targeted by the testability APIs. 
    /// The selection can be a particular replica of a partition based on the ReplicaId or Role or a random replica of the partition.
    /// </remarks>
    [Serializable]
    public class ReplicaSelector
    {
        private const string ReplicaKind = "ReplicaKind";
        private const string ReplicaKindPrimary = "ReplicaKindPrimary";
        private const string ReplicaKindRandomSecondary = "ReplicaKindRandomSecondary";
        private const string ReplicaOrInstanceIdParameter = "ReplicaOrInstanceId";

        private readonly long replicaOrInstanceId;
        private readonly PartitionSelector partitionSelector;
        private readonly ReplicaSelectorType selectorType;

        private ReplicaSelector(PartitionSelector partitionSelector, ReplicaSelectorType selectorType, long replicaOrInstanceId)
        {
            Requires.ThrowIfNull(partitionSelector, "partitionSelector");
            this.partitionSelector = partitionSelector;
            this.selectorType = selectorType;
            this.replicaOrInstanceId = replicaOrInstanceId;
            this.PowershellParameters = new Dictionary<string, string>();
            switch (this.selectorType)
            {
                case ReplicaSelectorType.Primary:
                    this.PowershellParameters.Add(ReplicaKind, ReplicaKindPrimary);
                    break;

                case ReplicaSelectorType.RandomSecondary:
                    this.PowershellParameters.Add(ReplicaKind, ReplicaKindRandomSecondary);
                    break;

                case ReplicaSelectorType.ReplicaId:
                    this.PowershellParameters.Add(ReplicaOrInstanceIdParameter, this.replicaOrInstanceId.ToString());
                    break;

                case ReplicaSelectorType.Random:
                    break;

                default:
                    throw new FabricInvalidReplicaSelectorException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_InvalidReplicaSelector, this.selectorType), FabricErrorCode.InvalidReplicaSelector);                    
            }
        }

        internal Dictionary<string, string> PowershellParameters
        {
            get;
            private set;
        }

        [Serializable]
        private enum ReplicaSelectorType
        {
            Primary,
            RandomSecondary,
            ReplicaId,
            Random
        }

        /// <summary>
        /// The PartitionSelector specifying the partition for which the replica is being selected by the ReplicaSelector.
        /// </summary>
        /// <value>The PartitionSelector object.</value>
        public PartitionSelector PartitionSelector
        {
            get
            {
                return this.partitionSelector;
            }
        }

        /// <summary>
        /// Selects a random replica for the given partition specified by the PartitionSelector.
        /// </summary>
        /// <param name="partitionSelector">PartitionSelector which indicates the partition whose replica needs to be selected.</param>
        /// <returns>Constructed ReplicaSelector.</returns>
        public static ReplicaSelector RandomOf(PartitionSelector partitionSelector)
        {
            return new ReplicaSelector(partitionSelector, ReplicaSelectorType.Random, -1);
        }

        /// <summary>
        /// Selects the primary replica for the given partition specified by the PartitionSelector.
        /// </summary>
        /// <param name="partitionSelector">PartitionSelector which indicates the partition whose replica needs to be selected.</param>
        /// <returns>Constructed ReplicaSelector.</returns>
        public static ReplicaSelector PrimaryOf(PartitionSelector partitionSelector)
        {
            return new ReplicaSelector(partitionSelector, ReplicaSelectorType.Primary, -1);
        }

        /// <summary>
        /// Selects a random secondary replica for the given partition specified by the PartitionSelector.
        /// </summary>
        /// <param name="partitionSelector">PartitionSelector which indicates the partition whose replica needs to be selected.</param>
        /// <returns>Constructed ReplicaSelector.</returns>
        public static ReplicaSelector RandomSecondaryOf(PartitionSelector partitionSelector)
        {
            return new ReplicaSelector(partitionSelector, ReplicaSelectorType.RandomSecondary, -1);
        }

        /// <summary>
        /// Selects a replica based on the ReplicaId for the given partition specified by the PartitionSelector.
        /// </summary>
        /// <param name="partitionSelector">PartitionSelector which indicates the partition whose replica needs to be selected.</param>
        /// <param name="replicaOrInstanceId">ReplicaOrInstanceId for the replica or instance to be selected.</param>
        /// <returns>A <see cref="System.Fabric.ReplicaSelector"/> based on the input passed in. </returns>
        public static ReplicaSelector ReplicaIdOf(PartitionSelector partitionSelector, long replicaOrInstanceId)
        {
            return new ReplicaSelector(partitionSelector, ReplicaSelectorType.ReplicaId, replicaOrInstanceId);
        }


        /// <summary>
        /// Compares whether two ReplicaSelectors are the same.
        /// </summary>
        /// <param name="obj">ReplicaSelector to compare to.</param>
        /// <returns>true if same false if not.</returns>
        public override bool Equals(object obj)
        {
            ReplicaSelector replicaObj = obj as ReplicaSelector;

            if (replicaObj == null)
            {
                return false;
            }
            if (replicaObj.replicaOrInstanceId != replicaOrInstanceId)
            {
                return false;
            }
            if (replicaObj.selectorType != selectorType)
            {
                return false;
            }
            return partitionSelector.Equals(replicaObj.partitionSelector);
        }

        /// <summary>
        /// Calls the base GetHashCode()
        /// </summary>
        /// <returns></returns>
        public override int GetHashCode()
        {
            return base.GetHashCode();
        }

        /// <summary>
        /// String representation of the replica selector.
        /// </summary>
        /// <returns>A string representation of the selector.</returns>
        public override string ToString()
        {
            return string.Format("{0}, {1} {2}", partitionSelector, selectorType, selectorType == ReplicaSelectorType.ReplicaId ? replicaOrInstanceId.ToString() : "").Trim();
        }

        internal Replica GetSelectedReplica(Replica[] results, Random random, bool skipReplicasNotUp)
        {            
            if (results.Length == 0)
            {
                throw new FabricInvalidReplicaSelectorException();
            }

            // Filtered set is for when random/primary or secondary need to be selected to ensure no idle/down replicas are in selection
            List<Replica> filteredResults = new List<Replica>();
            foreach (Replica result in results)
            {
                if (!this.IsInvalidReplica(result.ReplicaStatus) || !skipReplicasNotUp)
                {
                    filteredResults.Add(result);
                }
            }

            Replica replicaResult = null;
            switch (this.selectorType)
            {
                case ReplicaSelectorType.Primary:
                    foreach (Replica result in filteredResults)
                    {
                        StatefulServiceReplica statefulResult = result as StatefulServiceReplica;                        
                        if (statefulResult == null)
                        {
                            throw new FabricException(FabricErrorCode.ServiceTypeMismatch);
                        }

                        if (statefulResult.ReplicaRole == ReplicaRole.Primary)
                        {
                            replicaResult = result;
                            break;
                        }
                    }

                    break;

                case ReplicaSelectorType.RandomSecondary:
                    List<Replica> secondaries = new List<Replica>();
                    foreach (Replica result in filteredResults)
                    {
                        StatefulServiceReplica statefulResult = result as StatefulServiceReplica;                        
                        if (statefulResult == null)
                        {
                            throw new FabricException(FabricErrorCode.ServiceTypeMismatch);
                        }

                        if (statefulResult.ReplicaRole == ReplicaRole.ActiveSecondary)
                        {
                            secondaries.Add(result);
                        }
                    }
                    
                    if (secondaries.Count == 0)
                    {
                        throw new FabricInvalidReplicaSelectorException(StringResources.Error_NoSecondariesInReplicaSet, FabricErrorCode.InvalidReplicaSelector);
                    }

                    int secondaryIndex = random.Next(secondaries.Count);
                    replicaResult = secondaries[secondaryIndex];

                    break;

                case ReplicaSelectorType.ReplicaId:
                    // If specific replica is targeted we search for it in original result rather than filtered set
                    foreach (Replica result in results)
                    {
                        if (result.Id == this.replicaOrInstanceId)
                        {
                            replicaResult = result;
                        }
                    }

                    break;

                case ReplicaSelectorType.Random:
                    int index = random.Next(filteredResults.Count);
                    replicaResult = filteredResults[index];
                    break;

                default:
                    break;
            }

            // replace this string with toString() overload.
            if (replicaResult == null)
            {
                throw new FabricException(StringHelper.Format(StringResources.Error_ReplicaNotFoundForSelector, this.selectorType, this.replicaOrInstanceId), FabricErrorCode.ReplicaDoesNotExist);
            }

            return replicaResult;
        }

        private bool IsInvalidReplica(ServiceReplicaStatus status)
        {
            return status == ServiceReplicaStatus.Invalid || status == ServiceReplicaStatus.Down || status == ServiceReplicaStatus.Dropped;
        }
    }
}