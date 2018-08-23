// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    internal sealed class FabricContainerApiExecutionResult : 
        NativeContainerActivatorService.IFabricContainerApiExecutionResult, IDisposable
    {
        private readonly IntPtr nativeResult;
        private readonly PinCollection pinCollection;
        private bool disposed;

        internal FabricContainerApiExecutionResult(ContainerApiExecutionResponse response)
        {
            this.pinCollection = new PinCollection();
            this.nativeResult = response.ToNative(pinCollection);
        }

        IntPtr NativeContainerActivatorService.IFabricContainerApiExecutionResult.get_Result()
        {
            if (this.disposed)
            {
                throw new ObjectDisposedException("FabricContainerApiExecutionResult has been disposed");
            }

            return this.nativeResult;
        }

        public void Dispose()
        {
            this.pinCollection.Dispose();
            this.disposed = true;
        }
    }
}