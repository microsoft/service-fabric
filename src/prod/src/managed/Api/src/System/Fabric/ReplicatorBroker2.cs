// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Interop;
    using System.Threading;
    using System.Threading.Tasks;
    using BOOLEAN = System.SByte;

    internal sealed class ReplicatorBroker2 : ReplicatorBroker, NativeRuntime.IFabricReplicatorCatchupSpecificQuorum
    {
        public ReplicatorBroker2(IReplicator replicator) : base(replicator)
        {
        }        
    }
}