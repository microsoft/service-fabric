// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.Runtime
{
    using System;
    using System.Fabric.Interop;
    using System.Threading.Tasks;

    internal class FabricTransportMessageHandlerBroker :
        NativeServiceCommunication.IFabricCommunicationMessageHandler
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
            NativeServiceCommunication.IFabricServiceCommunicationMessage message,
            uint timeoutMilliseconds,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var body = Helper.Get_Byte(message.Get_Body());
            var headers = Helper.Get_Byte(message.Get_Headers());
            var managedTimeout = TimeSpan.FromMilliseconds(timeoutMilliseconds);
            var clientId = NativeTypes.FromNativeString(nativeClientId);
            return Utility.WrapNativeAsyncMethodImplementation(
                (cancellationToken) => this.RequestResponseAsync(clientId, headers, body, managedTimeout),
                callback,
                "IFabricServiceCommunicationContract.RequestResponseAsync");
        }

        public NativeServiceCommunication.IFabricServiceCommunicationMessage EndProcessRequest(
            NativeCommon.IFabricAsyncOperationContext context)
        {
            return AsyncTaskCallInAdapter.End<FabricServiceCommunicationMessage>(context);
        }

        public void HandleOneWay(IntPtr nativeClientId,
            NativeServiceCommunication.IFabricServiceCommunicationMessage message)
        {
            var body = Helper.Get_Byte(message.Get_Body());
            var headers = Helper.Get_Byte(message.Get_Headers());
            var clientId = NativeTypes.FromNativeString(nativeClientId);
            var context = new FabricTransportRequestContext(clientId, this.nativeConnectionHandler.GetCallBack);
            this.service.HandleOneWay(context, headers, body);
        }

     
        private async Task<FabricServiceCommunicationMessage> RequestResponseAsync(string clientId, byte[] header,
            byte[] body, TimeSpan timeout)
        {
            // We have Cancellation Token for Remoting layer , hence timeout is not used here.
            var context = new FabricTransportRequestContext(clientId, this.nativeConnectionHandler.GetCallBack);

            byte[] replyheader = null;
            byte[] replyBody = null;
            var replyMessage = await this.service.RequestResponseAsync(context, header, body);
            if (replyMessage.IsException)
            {
                replyheader = replyMessage.GetBody();
                replyBody = null;
            }
            else
            {
                replyheader = null;
                replyBody = replyMessage.GetBody();
            }

            return new FabricServiceCommunicationMessage(replyheader, replyBody);
        }
    }
}