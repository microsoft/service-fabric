// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.V2.Client
{
    using System;

    internal class FabricTransportClientConnectionEventHandlerBroker :
        NativeFabricTransport.IFabricTransportClientEventHandler
    {
        private readonly IFabricTransportClientEventHandler clientConnectionHandler;

        public FabricTransportClientConnectionEventHandlerBroker(
            IFabricTransportClientEventHandler clientConnectionHandler)
        {
            this.clientConnectionHandler = clientConnectionHandler;
        }

        void NativeFabricTransport.IFabricTransportClientEventHandler.OnConnected(IntPtr connectionAddress)
        {
            this.clientConnectionHandler.OnConnected();
        }

        void NativeFabricTransport.IFabricTransportClientEventHandler.OnDisconnected(IntPtr connectionAddress,
            int errorCode)
        {
            this.clientConnectionHandler.OnDisconnected();
        }
    }
}