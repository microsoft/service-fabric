// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// This supports the Service Fabric infrastructure and is not meant to be used directly from your code.
    /// </summary>
    public sealed class ReplicaInformation
    {
        /// <summary>
        /// This supports the Service Fabric infrastructure and is not meant to be used directly from your code.
        /// </summary>
        public ReplicaInformation()
        {
        }

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <value>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</value>
        public long Id
        {
            get;
            private set;
        }

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <value>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</value>
        public ReplicaRole Role
        {
            get;
            private set;
        }

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <value>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</value>
        public ReplicaStatus Status
        {
            get;
            private set;
        }

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <value>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</value>
        public string ReplicatorAddress
        {
            get;
            private set;
        }

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <value>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</value>
        public long CatchUpCapability
        {
            get;
            private set;
        }

        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        /// <value>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</value>
        public long CurrentProgress
        {
            get;
            private set;
        }

        /// <summary>
        /// FOR INTERNAL USE ONLY.
        /// Gets a value indicating whether the replica must be caught up as part of a WaitForQuorumCatchup.
        /// </summary>
        /// <value>Whether the replica must be caught up as part of a WaitForQuorumCatchup.</value>
        public bool MustCatchup
        {
            get;
            private set;
        }

        internal static unsafe ReplicaInformation[] FromNativeArray(uint replicaCount, IntPtr replicaInfosIntPtr)
        {
            ReplicaInformation[] replicaInfos = new ReplicaInformation[(int)replicaCount];
            var nativeInfos = (NativeTypes.FABRIC_REPLICA_INFORMATION*)replicaInfosIntPtr;

            for (int i = 0; i < replicaInfos.Length; i++)
            {
                replicaInfos[i] = ReplicaInformation.FromNative((IntPtr)(&nativeInfos[i]));
            }

            return replicaInfos;
        }

        internal static unsafe ReplicaInformation FromNative(IntPtr replicaInfoIntPtr)
        {
            var nativeInfo = (NativeTypes.FABRIC_REPLICA_INFORMATION*)replicaInfoIntPtr;
            var info = new ReplicaInformation();

            info.Id = nativeInfo->Id;
            info.Role = (System.Fabric.ReplicaRole)nativeInfo->Role;
            info.Status = (System.Fabric.ReplicaStatus)nativeInfo->Status;
            info.ReplicatorAddress = NativeTypes.FromNativeString(nativeInfo->ReplicatorAddress);
            info.CatchUpCapability = nativeInfo->CatchUpCapability;
            info.CurrentProgress = nativeInfo->CurrentProgress;

            ReleaseAssert.AssertIf(nativeInfo->Reserved == IntPtr.Zero, "ReplicaInfo->Reserved cannot be null");

            var ex1 = (NativeTypes.FABRIC_REPLICA_INFORMATION_EX1*) nativeInfo->Reserved;
            info.MustCatchup = NativeTypes.FromBOOLEAN(ex1->MustCatchup);

            return info;
        }

        internal void ToNative(PinCollection pin, out NativeTypes.FABRIC_REPLICA_INFORMATION native)
        {
            native.Id = this.Id;
            native.Role = (NativeTypes.FABRIC_REPLICA_ROLE)this.Role;
            native.Status = (NativeTypes.FABRIC_REPLICA_STATUS)this.Status;
            native.ReplicatorAddress = pin.AddBlittable(this.ReplicatorAddress);
            native.CatchUpCapability = this.CatchUpCapability;
            native.CurrentProgress = this.CurrentProgress;


            var ex1 = new NativeTypes.FABRIC_REPLICA_INFORMATION_EX1[1];
            ex1[0].MustCatchup = NativeTypes.ToBOOLEAN(this.MustCatchup);
            ex1[0].Reserved = IntPtr.Zero;
            native.Reserved = pin.AddBlittable(ex1);
        }
    }
}