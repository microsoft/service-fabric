// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Replication
{
    using System;
    using System.Fabric.Interop;

    internal class NativeReplicator : NativeRuntime.IFabricReplicator
    {
        public virtual NativeCommon.IFabricAsyncOperationContext BeginOpen(NativeCommon.IFabricAsyncOperationCallback callback)
        {
            throw new NotImplementedException("BeginOpen");
        }

        public virtual NativeCommon.IFabricStringResult EndOpen(NativeCommon.IFabricAsyncOperationContext context)
        {
            throw new NotImplementedException("EndOpen");
        }

        public virtual NativeCommon.IFabricAsyncOperationContext BeginChangeRole(IntPtr epoch, NativeTypes.FABRIC_REPLICA_ROLE role, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            throw new NotImplementedException("BeginChangeRole");
        }

        public virtual void EndChangeRole(NativeCommon.IFabricAsyncOperationContext context)
        {
            throw new NotImplementedException("EndChangeRole");
        }

        public virtual NativeCommon.IFabricAsyncOperationContext BeginClose(NativeCommon.IFabricAsyncOperationCallback callback)
        {
            throw new NotImplementedException("BeginClose");
        }

        public virtual void EndClose(NativeCommon.IFabricAsyncOperationContext context)
        {
            throw new NotImplementedException("EndClose");
        }

        public virtual void Abort()
        {
            throw new NotImplementedException("Abort");
        }

        public virtual void GetCurrentProgress(out Int64 lastSequenceNumber)
        {
            throw new NotImplementedException("GetCurrentProgress");
        }

        public virtual void GetCatchUpCapability(out Int64 fromSequenceNumber)
        {
            throw new NotImplementedException("GetCatchUpCapability");
        }


        public virtual NativeCommon.IFabricAsyncOperationContext BeginUpdateEpoch(IntPtr epoch, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            throw new NotImplementedException();
        }

        public virtual void EndUpdateEpoch(NativeCommon.IFabricAsyncOperationContext context)
        {
            throw new NotImplementedException();
        }
    }
}