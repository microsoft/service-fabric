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

    internal class OperationStreamEx : IOperationStreamEx
    {
        private NativeRuntime.IFabricOperationStreamEx nativeOperationStream;

        public OperationStreamEx(NativeRuntime.IFabricOperationStreamEx nativeOperationStream)
        {
            Requires.Argument("stream", nativeOperationStream).NotNull();

            this.nativeOperationStream = nativeOperationStream;
        }

        public Task<IOperationEx> GetOperationAsync(CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvoke<IOperationEx>(
                this.nativeOperationStream.BeginGetOperation,
                context => OperationEx.CreateFromNative(this.nativeOperationStream.EndGetOperation(context)),
                cancellationToken,
                "OperationStreamEx.GetOperationAsync");
        }
    }
}