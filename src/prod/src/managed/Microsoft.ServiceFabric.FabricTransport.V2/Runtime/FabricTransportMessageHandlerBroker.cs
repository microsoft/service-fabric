// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.V2.Runtime
{
    using System;
    using System.Fabric.Interop;
    using System.Threading.Tasks;

    internal class FabricTransportMessageHandlerBroker :
        NativeFabricTransport.IFabricTransportMessageHandler
    {
        private readonly IFabricTransportMessageHandler service;
        private readonly IFabricTransportConnectionHandler nativeConnectionHandler;

        public FabricTransportMessageHandlerBroker(
            IFabricTransportMessageHandler service,
            IFabricTransportConnectionHandler nativeConnectionHandler)
        {
            this.service = service;
            this.nativeConnectionHandler = nativeConnectionHandler;
        }

        public NativeCommon.IFabricAsyncOperationContext BeginProcessRequest(
            IntPtr nativeClientId,
            NativeFabricTransport.IFabricTransportMessage message,
            uint timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var managedTimeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);
            var clientId = NativeTypes.FromNativeString(nativeClientId);
            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) =>
                    this.RequestResponseAsync(clientId, NativeFabricTransportMessage.ToFabricTransportMessage(message), managedTimeout),
                callback,
                "IFabricServiceCommunicationContract.RequestResponseAsync");
        }

        public NativeFabricTransport.IFabricTransportMessage EndProcessRequest(
            NativeCommon.IFabricAsyncOperationContext context)
        {
            return AsyncTaskCallInAdapter.End<NativeFabricTransportMessage>(context);
        }

        public void HandleOneWay(IntPtr nativeClientId,
            NativeFabricTransport.IFabricTransportMessage message)
        {
       
            var clientId = NativeTypes.FromNativeString(nativeClientId);
            var context = new FabricTransportRequestContext(clientId, this.nativeConnectionHandler.GetCallBack);
            this.service.HandleOneWay(context,NativeFabricTransportMessage.ToFabricTransportMessage(message));
        }

        private async Task<NativeFabricTransportMessage> RequestResponseAsync(string clientId,
            FabricTransportMessage fabricTransportMessage, TimeSpan timeout)
        {
            // We have Cancellation Token for Remoting layer , hence timeout is not used here.
            var context = new FabricTransportRequestContext(clientId, this.nativeConnectionHandler.GetCallBack);
            var replyMessage = await this.service.RequestResponseAsync(context, fabricTransportMessage);
            return new NativeFabricTransportMessage(replyMessage);
        }
    }
}