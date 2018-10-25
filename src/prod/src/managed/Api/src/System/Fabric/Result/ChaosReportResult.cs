// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Interop;

    internal sealed class ChaosReportResult : NativeClient.IFabricChaosReportResult, IDisposable
    {
        private readonly IntPtr nativeResult;
        private readonly PinCollection pinCollection;
        private bool disposed;

        public ChaosReportResult(ChaosReport report)
        {
            this.pinCollection = new PinCollection();
            this.nativeResult = report.ToNative(pinCollection);
        }

        public IntPtr get_ChaosReportResult()
        {
            if (this.disposed)
            {
                throw new ObjectDisposedException("ChaosReportResult has been disposed");
            }

            return nativeResult;
        }

        public void Dispose()
        {
            this.pinCollection.Dispose();
            this.disposed = true;
        }
    }
}