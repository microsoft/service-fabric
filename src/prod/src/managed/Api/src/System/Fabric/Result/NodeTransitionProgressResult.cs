// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    internal sealed class NodeTransitionProgressResult : NativeClient.IFabricNodeTransitionProgressResult, IDisposable
    {
        private readonly IntPtr nativeResult;
        private readonly PinCollection pinCollection;
        private bool disposed;

        public NodeTransitionProgressResult(NodeTransitionProgress progress)
        {
            this.pinCollection = new PinCollection();
            this.nativeResult = progress.ToNative(pinCollection);
        }

        public IntPtr get_Progress()
        {
            if (this.disposed)
            {
                throw new ObjectDisposedException("NodeTransitionProgressResult has been disposed");
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