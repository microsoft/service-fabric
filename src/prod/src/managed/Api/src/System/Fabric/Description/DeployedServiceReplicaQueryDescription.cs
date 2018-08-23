// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;

    internal sealed class DeployedServiceReplicaQueryDescription
    {
        public DeployedServiceReplicaQueryDescription()
            : this(null, null, null, Guid.Empty)
        {
        }

        public DeployedServiceReplicaQueryDescription(string nodeName, Uri applicationName, string serviceManifestNameFilter, Guid partitionIdFilter)
        {
            this.NodeName = nodeName;
            this.ApplicationName = applicationName;
            this.ServiceManifestNameFilter = serviceManifestNameFilter;
            this.PartitionIdFilter = partitionIdFilter;
        }

        public string NodeName { get; set; }
        public Uri ApplicationName { get; set; }
        public string ServiceManifestNameFilter { get; set; }
        public Guid PartitionIdFilter { get; set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_DEPLOYED_REPLICA_QUERY_DESCRIPTION();
            nativeDescription.Reserved = IntPtr.Zero;

            nativeDescription.ApplicationName = pinCollection.AddObject(this.ApplicationName);
            nativeDescription.NodeName = pinCollection.AddObject(this.NodeName);
            nativeDescription.ServiceManifestNameFilter = pinCollection.AddObject(this.ServiceManifestNameFilter);
            nativeDescription.PartitionIdFilter = this.PartitionIdFilter;
            return pinCollection.AddBlittable(nativeDescription);
        }
    }

}