// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System.Fabric.Chaos.Common;
    using System.Fabric.Common;
    using System.Fabric.Strings;

    internal class NodeEntity
    {
        private const string TraceType = "Chaos.NodeEntity";

        public NodeEntity(NodeInfo nodeInfo, ClusterStateSnapshot clusterSnapshot)
        {
            this.CurrentNodeInfo = nodeInfo;
            this.ClusterSnapshot = clusterSnapshot;
            this.NodeFlags = ClusterEntityFlags.Excluded;
        }

        public ClusterStateSnapshot ClusterSnapshot { get; internal set; }

        public NodeInfo CurrentNodeInfo { get; private set; }

        public bool IsAvailableToFault 
        { 
            get
            {
                bool isInTransition = this.NodeFlags.HasFlag(ClusterEntityFlags.Faulted)
                                      || this.NodeFlags.HasFlag(ClusterEntityFlags.Unavailable)
                                      || this.IsExcludedForFaults();
                return !isInTransition;
            }
        }

        public bool IsUnsafeToFault
        {
            get
            {
                bool isUnsafe = this.NodeFlags.HasFlag(ClusterEntityFlags.Unsafe);

                return isUnsafe || !this.IsAvailableToFault;
            }
        }

        public ClusterEntityFlags NodeFlags { get; private set; }

        public void MarkNodeAsUnsafeToFault()
        {
            this.NodeFlags |= ClusterEntityFlags.Unsafe;
        }

        public void MarkNodeAsUnavailableForFaults()
        {
            this.NodeFlags |= ClusterEntityFlags.Unavailable;
        }

        public void MarkNodeAsExcludedForFaults()
        {
            this.NodeFlags |= ClusterEntityFlags.Excluded;
        }

        public bool IsExcludedForFaults()
        {
            return this.ClusterSnapshot.ChaosTargetFilterPresent && this.NodeFlags.HasFlag(ClusterEntityFlags.Excluded);
        }

        public void ClearNodeOfExclusionFlags()
        {
            if (!this.IsExcludedForFaults())
            {
                return;
            }

            foreach (var codePackage in this.ClusterSnapshot.GetAllCodePackages())
            {
                if (codePackage.NodeName == this.CurrentNodeInfo.NodeName)
                {
                    codePackage.ClearCodePackageOfExclusionFlags();
                }
            }

            this.NodeFlags &= ~ClusterEntityFlags.Excluded;
        }

        public void FaultNode(Guid activityId = default(Guid))
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0}: Faulting node: '{1}'", activityId, this);

            // First confirm that the node is not already faulted or unavailable due to a fault in a child entity
            ChaosUtility.ThrowOrAssertIfTrue(
                ChaosConstants.FaultNode_NodeInTransition_TelemetryId,
                !this.IsAvailableToFault,
                string.Format(
                    StringResources.ChaosEngineError_NodeEntity_NodeInTransition,
                    this.CurrentNodeInfo.NodeName),
                this.ClusterSnapshot);

            ChaosUtility.ThrowOrAssertIfTrue(
                ChaosConstants.FaultNode_UnsafeNode_TelemetryId,
                !this.ClusterSnapshot.UnsafeModeEnabled && this.IsUnsafeToFault,
                string.Format(
                    StringResources.ChaosEngineError_NodeEntity_UnsafeNode,
                    this.CurrentNodeInfo.NodeName),
                this.ClusterSnapshot);

            TestabilityTrace.TraceSource.WriteInfo("Here are all the codepackages:");

            var cps = this.ClusterSnapshot.GetAllCodePackages();

            foreach(var cp in cps)
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Codepackage={0}", cp);
            }

            // Faulting a node will result in all the code packages and the replicas on the node
            // in turn faulting. In this case faulting all the code packages will result
            // in the replicas inside the code package to also fault. That is why we only fault the 
            // code packages here since that takes care of marking the replicas as faulted
            foreach (var codePackage in this.ClusterSnapshot.GetAllCodePackages())
            {
                if (codePackage.NodeName == this.CurrentNodeInfo.NodeName)
                {
                    // This will also fault the replicas
                    TestabilityTrace.TraceSource.WriteInfo(
                        TraceType, "{0}: Going to fault code package: '{1}' due to faulting node '{2}'", 
                            activityId, 
                            codePackage,
                            this);

                    codePackage.FaultCodePackage(activityId);
                }
            }

            // Mark current node as faulted
            this.NodeFlags |= ClusterEntityFlags.Faulted;
        }

        public override string ToString()
        {
            return string.Format(
                " NodeName: {0}, Up: {1}, IsAvailableToFault: {2}, NodeFlags: {3}",
                this.CurrentNodeInfo.NodeName,
                this.CurrentNodeInfo.IsNodeUp,
                this.IsAvailableToFault,
                this.NodeFlags);
        }
    }
}