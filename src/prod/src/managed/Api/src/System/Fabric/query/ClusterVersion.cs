// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Common.Serialization;

    internal sealed class ClusterVersion
    {
        internal ClusterVersion(
            string clusterVersion)
        {
            this.Version = clusterVersion;
        }

        private ClusterVersion() : this(null) { }
        
        public string Version
        {
            get;
            private set;
        }
    }
}