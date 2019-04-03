// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Stubs
{
    using System.Fabric.Interop;

    class StateReplicatorStubBase : NativeRuntime.IFabricStateReplicator2
    {

        public virtual NativeCommon.IFabricAsyncOperationContext BeginReplicate(NativeRuntime.IFabricOperationData operationData, NativeCommon.IFabricAsyncOperationCallback callback, out long sequenceNumber)
        {
            throw new NotImplementedException();
        }

        public virtual long EndReplicate(NativeCommon.IFabricAsyncOperationContext context)
        {
            throw new NotImplementedException();
        }

        public virtual NativeRuntime.IFabricOperationStream GetCopyStream()
        {
            return new OperationStream();
        }

        public virtual NativeRuntime.IFabricOperationStream GetReplicationStream()
        {
            return new OperationStream();
        }

        public virtual void UpdateReplicatorSettings(IntPtr fabricReplicatorSettings)
        {
            throw new NotImplementedException();
        }

        public virtual NativeRuntime.IFabricReplicatorSettingsResult GetReplicatorSettings()
        {
            return null;
        }

        private class OperationStream : NativeRuntime.IFabricOperationStream
        {
            public NativeCommon.IFabricAsyncOperationContext BeginGetOperation(NativeCommon.IFabricAsyncOperationCallback callback)
            {
                throw new NotImplementedException();
            }

            public NativeRuntime.IFabricOperation EndGetOperation(NativeCommon.IFabricAsyncOperationContext context)
            {
                throw new NotImplementedException();
            }
        }
    }
}