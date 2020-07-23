// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.Runtime
{
    using System;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.FabricTransport.Client;

    internal interface IFabricTransportConnectionHandler
    {
        Task ConnectAsync(FabricTransportCallbackClient fabricTransportServiceRemotingCallback, TimeSpan timeout);

        Task DisconnectAsync(string clientId, TimeSpan timeout);

        FabricTransportCallbackClient GetCallBack(string clientId);
    }
}