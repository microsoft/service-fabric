// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.V2.Runtime
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using Microsoft.ServiceFabric.FabricTransport;
    using Microsoft.ServiceFabric.FabricTransport.Client;

    internal class FabricTransportConnectionHandlerBroker : NativeFabricTransport.IFabricTransportConnectionHandler
    {
        private IFabricTransportConnectionHandler connectionHandler;

        public FabricTransportConnectionHandlerBroker(IFabricTransportConnectionHandler serviceConnectionHandler)
        {
            this.connectionHandler = serviceConnectionHandler;
        }

        public NativeCommon.IFabricAsyncOperationContext BeginProcessConnect(
            NativeFabricTransport.IFabricTransportClientConnection nativeClientConnection,
            uint timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var clientConnectionmanagedTimeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);
            var clientConnection = new FabricTransportCallbackClient(nativeClientConnection);
            AppTrace.TraceSource.WriteInfo("FabricTransportConnectionHandlerBroker", "Client {0} Connecting", clientConnection.GetClientId());
            return
                Utility.WrapNativeAsyncMethodImplementation(
                    (cancellationToken) => this.connectionHandler.ConnectAsync(clientConnection, clientConnectionmanagedTimeout),
                    callback,
                    "IFabricTransportConnectionHandler.ConnectAsync");
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
            AppTrace.TraceSource.WriteInfo("FabricTransportConnectionHandlerBroker", "Client {0} DisConnecting ", clientId);
            return
                Utility.WrapNativeAsyncMethodImplementation(
                    (cancellationToken) => this.connectionHandler.DisconnectAsync(clientId, clientConnectionmanagedTimeout),
                    callback,
                    "IFabricTransportConnectionHandler.DisconnectAsync");
        }

        public void EndProcessDisconnect(NativeCommon.IFabricAsyncOperationContext context)
        {
            AsyncTaskCallInAdapter.End(context);
        }
    }
}