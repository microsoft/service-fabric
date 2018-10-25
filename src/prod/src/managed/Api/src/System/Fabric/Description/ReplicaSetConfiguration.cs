// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Interop;

    /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
    public class ReplicaSetConfiguration
    {
        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <value>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</value>
        public long WriteQuorum { get; private set; }

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <value>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</value>
        public IList<ReplicaInformation> Replicas { get; private set; }

        internal static unsafe ReplicaSetConfiguration FromNative(IntPtr replicaSetIntPtr)
        {
            var nativeConfiguration = (NativeTypes.FABRIC_REPLICA_SET_CONFIGURATION*)replicaSetIntPtr;
            if (nativeConfiguration == null)
            {
                return null;
            }

            var configuration = new ReplicaSetConfiguration();

            configuration.WriteQuorum = (long)nativeConfiguration->WriteQuorum;

            configuration.Replicas = new List<ReplicaInformation>();
            IntPtr replicas = nativeConfiguration->Replicas;
            var nativeInfos = (NativeTypes.FABRIC_REPLICA_INFORMATION*)replicas;

            for (int i = 0; i < nativeConfiguration->ReplicaCount; i++)
            {
                configuration.Replicas.Add(ReplicaInformation.FromNative((IntPtr)(&nativeInfos[i])));
            }

            return configuration;
        }

        internal unsafe void ToNative(PinCollection pin, out NativeTypes.FABRIC_REPLICA_SET_CONFIGURATION native)
        {
            var replicasLength = this.Replicas.Count;

            native.ReplicaCount = (uint)replicasLength;
            native.WriteQuorum = (uint)this.WriteQuorum;

            unsafe
            {
                var nativeReplicas = new NativeTypes.FABRIC_REPLICA_INFORMATION[replicasLength];
                for (int i = 0; i < replicasLength; i++)
                {
                    this.Replicas[i].ToNative(pin, out nativeReplicas[i]);
                }

                native.Replicas = (IntPtr)pin.AddBlittable(nativeReplicas);
            }

            native.Reserved = IntPtr.Zero;
        }
    }
}