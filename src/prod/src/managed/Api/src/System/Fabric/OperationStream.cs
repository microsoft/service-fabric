// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Threading;
    using System.Threading.Tasks;

    internal sealed class OperationStream : IOperationStream2
    {
        private readonly NativeRuntime.IFabricOperationStream2 nativeOperationStream;

        public OperationStream(NativeRuntime.IFabricOperationStream nativeOperationStream)
        {
            Requires.Argument("nativeOperationStream", nativeOperationStream).NotNull();

            this.nativeOperationStream = (NativeRuntime.IFabricOperationStream2)nativeOperationStream;
        }

        public Task<IOperation> GetOperationAsync(CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvoke<IOperation>(this.GetOperationBeginWrapper, this.GetOperationEndWrapper, cancellationToken, "OperationStream.GetOperation");
        }

        public void ReportFault(FaultType faultType)
        { 
            Utility.WrapNativeSyncInvoke(() => this.ReportFaultHelper(faultType), "OperationStream.ReportFault");
        }

        private NativeCommon.IFabricAsyncOperationContext GetOperationBeginWrapper(NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return this.nativeOperationStream.BeginGetOperation(callback);
        }

        private IOperation GetOperationEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            NativeRuntime.IFabricOperation operation = this.nativeOperationStream.EndGetOperation(context);
            return Operation.CreateFromNative(operation);
        }

        private void ReportFaultHelper(FaultType faultType)
        {
            this.nativeOperationStream.ReportFault((NativeTypes.FABRIC_FAULT_TYPE)faultType);
        }

        private sealed class Operation : IOperation
        {
            private readonly NativeRuntime.IFabricOperation nativeOperation;
            private readonly OperationData operationData;
            private readonly NativeTypes.FABRIC_OPERATION_METADATA operationMetadata;

            private unsafe Operation(NativeRuntime.IFabricOperation nativeOperation)
            {
                this.nativeOperation = nativeOperation;
                this.operationData = Utility.WrapNativeSyncInvoke<OperationData>(this.GetBuffers, "Operation.Data.Ctor");
                
                var metadata = (NativeTypes.FABRIC_OPERATION_METADATA*)this.nativeOperation.get_Metadata();
                this.operationMetadata.Type = metadata->Type;
                this.operationMetadata.SequenceNumber = metadata->SequenceNumber;
                this.operationMetadata.AtomicGroupId = metadata->AtomicGroupId;
            }

            public OperationType OperationType
            {
                get
                {
                    return (OperationType)this.operationMetadata.Type;
                }
            }

            public long SequenceNumber
            {
                get
                {
                    return this.operationMetadata.SequenceNumber;
                }
            }

            public long AtomicGroupId
            {
                get
                {
                    return this.operationMetadata.AtomicGroupId;
                }
            }

            public OperationData Data
            {
                get
                {                    
                    return this.operationData;
                }
            }

            public void Acknowledge()
            {
                //// Calls native code, requires UnmanagedCode permission
                this.nativeOperation.Acknowledge();
            }

            internal static Operation CreateFromNative(NativeRuntime.IFabricOperation nativeOperation)
            {
                return (nativeOperation != null) ? new Operation(nativeOperation) : null;
            }

            private unsafe OperationData GetBuffers()
            {
                uint count;
                IntPtr data = this.nativeOperation.GetData(out count);
                var ret = OperationData.CreateFromNative(count, data);
                GC.KeepAlive(this.nativeOperation);
                return ret;
            }
        }
    }
}