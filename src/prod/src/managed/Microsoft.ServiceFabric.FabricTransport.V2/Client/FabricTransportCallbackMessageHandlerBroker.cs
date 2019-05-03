// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.V2.Client
{
    internal class FabricTransportCallbackHandlerBroker : NativeFabricTransport.IFabricTransportCallbackMessageHandler
    {
        private readonly IFabricTransportCallbackMessageHandler callImpl;

        public FabricTransportCallbackHandlerBroker(IFabricTransportCallbackMessageHandler callImpl)
        {
            this.callImpl = callImpl;
        }


        public void HandleOneWay(NativeFabricTransport.IFabricTransportMessage message)
        {
        this.callImpl.OneWayMessage(NativeFabricTransportMessage.ToFabricTransportMessage(message));
        }
    }
}