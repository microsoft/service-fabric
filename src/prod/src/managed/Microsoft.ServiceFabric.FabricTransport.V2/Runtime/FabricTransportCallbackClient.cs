// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.V2.Runtime
{
    using System;
    using System.Fabric.Interop;
    using System.Runtime.InteropServices;

    internal class FabricTransportCallbackClient : IDisposable
    {
        private readonly TimeSpan defaultTimeout = TimeSpan.FromMinutes(2);
        private readonly NativeFabricTransport.IFabricTransportClientConnection nativeClientConnection;
        private readonly string clientId;

        public FabricTransportCallbackClient(
            NativeFabricTransport.IFabricTransportClientConnection nativeClientConnection)
        {
            this.nativeClientConnection = nativeClientConnection;
            var clientId = this.nativeClientConnection.get_ClientId();
            this.clientId = NativeTypes.FromNativeString(clientId);
            ;
        }

        public string GetClientId()
        {
            return this.clientId;
        }


        public void OneWayMessage(FabricTransportMessage requestBody)
        {
            NativeFabricTransport.IFabricTransportMessage message = new NativeFabricTransportMessage(requestBody);
            Utility.WrapNativeSyncInvokeInMTA(() => this.nativeClientConnection.Send(message),
                "NativeFabricClientConnection.SendMessage");
        }

        #region IDisposable Support

        private bool disposedValue = false; // To detect redundant calls

        protected virtual void Dispose(bool disposing)
        {
            if (!this.disposedValue)
            {
                if (disposing)
                {
                    // No other managed resources to dispose.
                }

                if (this.nativeClientConnection != null)
                {
                    Marshal.FinalReleaseComObject(this.nativeClientConnection);
                }

                this.disposedValue = true;
            }
        }

        ~FabricTransportCallbackClient()
        {
            this.Dispose(false);
        }

        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        #endregion
    }
}