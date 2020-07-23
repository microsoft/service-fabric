// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using System.Collections.Generic;
    using System.IO;

    internal class GenerationConfig
    {
        public GenerationConfig()
        {
            this.IsolationLevel = null;
            this.RemoveServiceFabricRuntimeAccess = false;
            this.ReplicaRestartWaitDurationSeconds = 30;
            this.QuorumLossWaitDurationSeconds = 30;
            this.ContainersRetentionCount = "1";
            this.TargetReplicaCount = 0;
        }

        public bool RemoveServiceFabricRuntimeAccess;

        public int ReplicaRestartWaitDurationSeconds;

        public int QuorumLossWaitDurationSeconds;

        public string IsolationLevel;

        public string ContainersRetentionCount;

        public int TargetReplicaCount;
    }
}

