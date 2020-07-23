// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.Runtime
{
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.FabricTransport;

    /// <summary>
    /// Defines the interface that must be implemented by the ServiceRemotingListener to receive messages from the
    /// remoting transport.
    /// </summary>
    internal interface IFabricTransportMessageHandler
    {
        /// <summary>
        /// Handles a message from the client that requires a response from the service.
        /// </summary>
        /// <param name="requestContext">Request context - contains additional information about the request</param>
        /// <param name="messageHeaders">Request message headers</param>
        /// <param name="requestBody">Request message body</param>
        /// <returns>Response body</returns>
        Task<FabricTransportReplyMessage> RequestResponseAsync(FabricTransportRequestContext requestContext,
            byte[] messageHeaders, byte[] requestBody);

        /// <summary>
        /// Handles a one way message from the client.
        /// </summary>
        /// <param name="requestContext">Request context - contains additional information about the request</param>
        /// <param name="messageHeaders">Request message headers</param>
        /// <param name="requestBody">Request message body</param>
        void HandleOneWay(FabricTransportRequestContext requestContext, byte[] messageHeaders, byte[] requestBody);
    }
}