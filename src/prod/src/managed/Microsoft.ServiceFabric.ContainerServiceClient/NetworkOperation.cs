// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient
{
    using System;
    using System.Collections.Generic;
    using System.Net;
    using System.Net.Http;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.ContainerServiceClient.Config;
    using Microsoft.ServiceFabric.ContainerServiceClient.Response;

    class NetworkOperation
    {
        private readonly ContainerServiceClient client;

        internal NetworkOperation(ContainerServiceClient client)
        {
            this.client = client;
        }

        /// <summary>
        ///  Expected return error code:
        ///     200 No error
        ///     500 server error
        /// </summary>
        internal async Task<IList<NetworkListResponse>> ListNetworksAsync(
            NetworksListConfig listConfig,
            TimeSpan timeout)
        {
            var requestPath = "networks";
            var queryString = QueryParameterHelper.BuildQueryString(listConfig);

            var response = await this.client.MakeRequestAsync(
                HttpMethod.Get,
                requestPath,
                queryString,
                null,
                timeout).ConfigureAwait(false);

            if (response.StatusCode != HttpStatusCode.OK)
            {
                throw new ContainerApiException(response.StatusCode, response.Body);
            }

            return ContainerServiceClient.JsonSerializer.DeserializeObject<NetworkListResponse[]>(response.Body);
        }

        /// <summary>
        ///  Expected return error code:
        ///     201 No error
        ///     403 Operation not supported for pre-defined networks
        ///     404 plugin not found
        ///     500 server error
        /// </summary>
        internal async Task<NetworkCreateResponse> CreateNetworkAsync(
            NetworkCreateConfig createConfig,
            TimeSpan timeout)
        {
            var requestPath = "networks/create";

            var requestContent = JsonRequestContent.GetContent(createConfig);

            var response = await this.client.MakeRequestAsync(
                HttpMethod.Post,
                requestPath,
                null,
                requestContent,
                timeout).ConfigureAwait(false);

            if (response.StatusCode != HttpStatusCode.Created)
            {
                throw new ContainerApiException(response.StatusCode, response.Body);
            }

            return ContainerServiceClient.JsonSerializer.DeserializeObject<NetworkCreateResponse>(response.Body);
        }

        /// <summary>
        ///  Expected return error code:
        ///     204 No error
        ///     403 Operation not supported for pre-defined networks
        ///     404 no such network
        ///     500 server error
        /// </summary>
        internal async Task DeleteNetworkAsync(
            string networkIdOrName,
            TimeSpan timeout)
        {
            var requestPath = $"networks/{networkIdOrName}";

            var response = await this.client.MakeRequestAsync(
                HttpMethod.Delete,
                requestPath,
                null,
                null,
                timeout).ConfigureAwait(false);

            if (response.StatusCode != HttpStatusCode.NoContent)
            {
                throw new ContainerApiException(response.StatusCode, response.Body);
            }
        }

        /// <summary>
        ///  Expected return error code:
        ///     200 No error
        ///     403 Operation not supported for swarm scoped networks
        ///     404 Network or container not found
        ///     500 server error
        /// </summary>
        internal async Task ConnectNetworkAsync(
            string networkIdOrName,
            NetworkConnectConfig connectConfig,
            TimeSpan timeout)
        {
            var requestPath = $"networks/{networkIdOrName}/connect";

            var requestContent = JsonRequestContent.GetContent(connectConfig);

            var response = await this.client.MakeRequestAsync(
                HttpMethod.Post,
                requestPath,
                null,
                requestContent,
                timeout).ConfigureAwait(false);

            if (response.StatusCode != HttpStatusCode.OK)
            {
                throw new ContainerApiException(response.StatusCode, response.Body);
            }
        }

    }
}