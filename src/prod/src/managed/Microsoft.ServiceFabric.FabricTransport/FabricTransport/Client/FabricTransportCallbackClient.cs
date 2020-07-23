// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.Client
{
    using System;
    using System.Fabric.Interop;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.FabricTransport;

    internal class FabricTransportCallbackClient : IDisposable
    {
        private readonly TimeSpan defaultTimeout = TimeSpan.FromMinutes(2);
        private NativeServiceCommunication.IFabricClientConnection nativeClientConnection;
        private string clientId;

        public FabricTransportCallbackClient(NativeServiceCommunication.IFabricClientConnection nativeClientConnection)
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

        public Task<byte[]> RequestResponseAsync(byte[] messageHeaders, byte[] requestBody)
        {
            return Utility.WrapNativeAsyncInvokeInMTA<byte[]>(
                (callback) => this.BeginRequest(messageHeaders, requestBody, this.defaultTimeout, callback),
                this.EndRequest,
                CancellationToken.None,
                "RequestResponseAsync");
        }

        public void OneWayMessage(byte[] messageHeaders, byte[] requestBody)
        {
            NativeServiceCommunication.IFabricServiceCommunicationMessage message = new FabricServiceCommunicationMessage(messageHeaders, requestBody);
            Utility.WrapNativeSyncInvokeInMTA(() => this.nativeClientConnection.SendMessage(message), "NativeFabricClientConnection.SendMessage");
        }

        internal NativeCommon.IFabricAsyncOperationContext BeginRequest(
            byte[] headers,
            byte[] body,
            TimeSpan timeout,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            NativeServiceCommunication.IFabricServiceCommunicationMessage message = new FabricServiceCommunicationMessage(headers, body);
            var nativeTimeout = Utility.ToMilliseconds(timeout, "timeout");
            return this.nativeClientConnection.BeginRequest(message, nativeTimeout, callback);
        }

        internal byte[] EndRequest(NativeCommon.IFabricAsyncOperationContext context)
        {
            var message = this.nativeClientConnection.EndRequest(context);
            var msgBytes = Helper.Get_Byte(message.Get_Body());
            GC.KeepAlive(message);
            return msgBytes;
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