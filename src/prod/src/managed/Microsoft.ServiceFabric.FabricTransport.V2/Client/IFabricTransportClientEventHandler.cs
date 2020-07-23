// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.V2.Client
{
    internal interface IFabricTransportClientEventHandler
    {
        void OnConnected();
        void OnDisconnected();
    }
}