// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.Client
{
    using System;
    using System.Fabric.Interop;
    using Microsoft.ServiceFabric.FabricTransport;

    internal class FabricTransportNativeClientConnectionEventHandler : NativeServiceCommunication.IFabricServiceConnectionEventHandler
    {
        private IFabricTransportClientConnectionHandler clientConnectionHandler;

        public FabricTransportNativeClientConnectionEventHandler(IFabricTransportClientConnectionHandler clientConnectionHandler)
        {
            this.clientConnectionHandler = clientConnectionHandler;
        }

        void NativeServiceCommunication.IFabricServiceConnectionEventHandler.OnConnected(IntPtr connectionAddress)
        {
            this.clientConnectionHandler.OnConnected();
        }

        void NativeServiceCommunication.IFabricServiceConnectionEventHandler.OnDisconnected(IntPtr connectionAddress, int errorCode)
        {
            this.clientConnectionHandler.OnDisconnected();
        }
    }
}