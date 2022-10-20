// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.Runtime
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using Microsoft.ServiceFabric.FabricTransport;
    using Microsoft.ServiceFabric.FabricTransport.Client;

    internal class FabricTransportServiceConnectionHandlerBroker : NativeServiceCommunication.IFabricServiceConnectionHandler
    {
        private IFabricTransportConnectionHandler connectionHandler;

        public FabricTransportServiceConnectionHandlerBroker(IFabricTransportConnectionHandler serviceConnectionHandler)
        {
            this.connectionHandler = serviceConnectionHandler;
        }

        public NativeCommon.IFabricAsyncOperationContext BeginProcessConnect(
            NativeServiceCommunication.IFabricClientConnection nativeClientConnection,
            uint timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var clientConnectionmanagedTimeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);
            var clientConnection = new FabricTransportCallbackClient(nativeClientConnection);
            AppTrace.TraceSource.WriteInfo("FabricTransportServiceConnectionHandlerBroker", "Client {0} Connecting", clientConnection.GetClientId());
            return
                Utility.WrapNativeAsyncMethodImplementation(
                    (cancellationToken) => this.connectionHandler.ConnectAsync(clientConnection, clientConnectionmanagedTimeout),
                    callback,
                    "IFabricServiceConnectionHandler.ConnectAsync");
        }

        public void EndProcessConnect(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }

        public NativeCommon.IFabricAsyncOperationContext BeginProcessDisconnect(
            IntPtr nativeClientId,
            uint timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var clientConnectionmanagedTimeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);
            var clientId = NativeTypes.FromNativeString(nativeClientId);
            AppTrace.TraceSource.WriteInfo("FabricTransportServiceConnectionHandlerBroker", "Client {0} DisConnecting ", clientId);
            return
                Utility.WrapNativeAsyncMethodImplementation(
                    (cancellationToken) => this.connectionHandler.DisconnectAsync(clientId, clientConnectionmanagedTimeout),
                    callback,
                    "IFabricServiceConnectionHandler.DisconnectAsync");
        }

        public void EndProcessDisconnect(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }
    }
}