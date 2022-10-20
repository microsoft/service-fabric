// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;

    public class ReplicaSetSize
    {
        public int TargetReplicaSetSize { get; set; }

        public int MinReplicaSetSize { get; set; }

        public override string ToString()
        {
            return string.Format("[TargetReplicaSetSize={0}, MinReplicaSetSize={1}]", this.TargetReplicaSetSize, this.MinReplicaSetSize);
        }

        public override bool Equals(object obj) 
        {
            var other = obj as ReplicaSetSize;

            return other != null
                && (other.MinReplicaSetSize == this.MinReplicaSetSize)
                && (other.TargetReplicaSetSize == this.TargetReplicaSetSize);
        }

        public override int GetHashCode()
        {
            var hash = 27;
            hash = (13 * hash) + this.MinReplicaSetSize.GetHashCode();
            hash = (13 * hash) + this.TargetReplicaSetSize.GetHashCode();
            return hash;
        }
    }

    // public class PaasClusterUpgradeDescription
    // {
    //    public string TargetClusterManifestBase64 { get; set; }

    // public Uri TargetMsiUri { get; set; }

    // public string TargetMsiVersion { get; set; }

    // // The upgrade policy to use
    //    public PaasClusterUpgradePolicy UpgradePolicy { get; set; }
    //// }

    // public class PaasNodeStatusInfo
    // {
    //    public string NodeName { get; set; }
        
    // public string NodeType { get; set; }

    // public NodeState NodeState { get; set; }

    // public WrpNodeDeactivationIntent NodeDeactivationIntent { get; set; }

    // public long IntentionInstance { get; set; }

    // public override string ToString()
    //    {
    //        return string.Format(
    //            "[NodeName={0}, NodeType={1}, NodeState={2}, NodeDeactivationIntent={3}, IntentionInstance={4}]", 
    //            this.NodeName, 
    //            this.NodeType, 
    //            this.NodeState, 
    //            this.NodeDeactivationIntent,
    //            this.IntentionInstance);
    //    }
    // }
}