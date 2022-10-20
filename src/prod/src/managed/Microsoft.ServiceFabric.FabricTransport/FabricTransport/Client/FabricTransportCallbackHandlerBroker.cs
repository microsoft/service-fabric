// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.Client
{
    using System;
    using System.Fabric.Interop;
    using System.Runtime.InteropServices;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.FabricTransport;

    internal class FabricTransportCallbackHandlerBroker : NativeServiceCommunication.IFabricCommunicationMessageHandler
    {
        private IFabricTransportCallbackMessageHandler callImpl;

        public FabricTransportCallbackHandlerBroker(IFabricTransportCallbackMessageHandler callImpl)
        {
            this.callImpl = callImpl;
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
                "IServiceCommunicationCallbackContract.RequestResponseAsync");
        }

        public NativeServiceCommunication.IFabricServiceCommunicationMessage EndProcessRequest(NativeCommon.IFabricAsyncOperationContext context)
        {
            var body = AsyncTaskCallInAdapter.End<byte[]>(context);
            return new FabricServiceCommunicationMessage(null, body);
        }

        public void HandleOneWay(IntPtr nativeClientId, NativeServiceCommunication.IFabricServiceCommunicationMessage message)
        {
            var body = Helper.Get_Byte(message.Get_Body());
            var headers = Helper.Get_Byte(message.Get_Headers());
            this.callImpl.OneWayMessage(headers, body);
        }

        private Task<byte[]> RequestResponseAsync(string clientId, byte[] header, byte[] body, TimeSpan timeout)
        {
            // We have Cancellation Token for Remoting layer , hence timeout is not used here.
            return this.callImpl.RequestResponseAsync(header, body);
        }
    }
}