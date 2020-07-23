// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.V2.Runtime
{
    using System.Threading.Tasks;

    /// <summary>
    /// Defines the interface that must be implemented by the ServiceRemotingListener to receive messages from the
    /// remoting transport.
    /// </summary>
    internal interface IFabricTransportMessageHandler
    {
        Task<FabricTransportMessage> RequestResponseAsync(FabricTransportRequestContext requestContext,
            FabricTransportMessage fabricTransportMessage);


        void HandleOneWay(FabricTransportRequestContext requestContext, FabricTransportMessage requesTransportMessage);
    }
}