// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Replicator
{
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Threading;
    using System.Threading.Tasks;

    internal class OperationEx : IOperationEx
    {
        private NativeRuntime.IFabricOperationEx nativeOperation;

        private long operationSequenceNumber = 0;
        private long atomicGroupId = 0;
        private OperationTypeEx type = OperationTypeEx.Invalid;

        private OperationEx(NativeRuntime.IFabricOperationEx nativeOperation)
        {
            Requires.Argument("nativeOperation", nativeOperation).NotNull();

            this.nativeOperation = nativeOperation;

            Utility.WrapNativeSyncInvoke(this.InitializeMetadata, "OperationEx.GetMetadata");
        }

        public OperationTypeEx OperationType
        {
            get { return this.type; }
        }

        public long SequenceNumber
        {
            get { return this.operationSequenceNumber; }
        }

        public long AtomicGroupId
        {
            get { return this.atomicGroupId; }
        }

        public IOperationData ServiceMetadata
        {
            get
            {
                return Utility.WrapNativeSyncInvoke<IOperationData>(this.GetServiceMetdata, "OperationEx.ServiceMetadata");
            }
        }

        public IOperationData Data
        {
            get
            {
                return Utility.WrapNativeSyncInvoke<IOperationData>(this.GetData, "OperationEx.Data");
            }
        }

        internal static OperationEx CreateFromNative(NativeRuntime.IFabricOperationEx nativeOperation)
        {
            return null != nativeOperation ? new OperationEx(nativeOperation) : null;
        }

        public void Acknowledge()
        {
            Utility.WrapNativeSyncInvoke(this.nativeOperation.Acknowledge, "OperationEx.Acknowledge");
        }

        private IOperationData GetServiceMetdata()
        {
            uint count = 0;
            IntPtr buffer = this.nativeOperation.GetServiceMetadata(out count);
            return OperationData.CreateFromNative(count, buffer);
        }

        private IOperationData GetData()
        {
            NativeRuntime.IFabricOperationData nativeOperationData = this.nativeOperation as NativeRuntime.IFabricOperationData;
            return new ReadOnlyOperationData(nativeOperationData);
        }

        private unsafe void InitializeMetadata()
        {
            unsafe
            {
                NativeTypes.FABRIC_OPERATION_METADATA_EX* metadata = (NativeTypes.FABRIC_OPERATION_METADATA_EX*)this.nativeOperation.GetMetadata();

                this.operationSequenceNumber = metadata->SequenceNumber;
                this.atomicGroupId = metadata->AtomicGroupId;
                this.type = (OperationTypeEx)metadata->Type;
            }
        }
    }
}