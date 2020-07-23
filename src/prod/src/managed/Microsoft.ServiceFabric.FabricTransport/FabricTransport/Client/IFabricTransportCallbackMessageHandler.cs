// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.Client
{
    using System.Threading.Tasks;

    internal interface IFabricTransportCallbackMessageHandler
    {
        /// <summary>
        /// Sends a message to the client and gets the response.
        /// </summary>
        /// <param name="messageHeaders">Message headers</param>
        /// <param name="requestBody">Message body</param>
        /// <returns>Response body</returns>
        Task<byte[]> RequestResponseAsync(byte[] messageHeaders, byte[] requestBody);

        /// <summary>
        /// Sends a one way message to the client.
        /// </summary>
        /// <param name="messageHeaders">Message headers</param>
        /// <param name="requestBody">Message body</param>
        void OneWayMessage(byte[] messageHeaders, byte[] requestBody);
    }
}