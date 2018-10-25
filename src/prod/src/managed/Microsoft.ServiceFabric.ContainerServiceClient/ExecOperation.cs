// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient
{
    using System;
    using System.Net;
    using System.Net.Http;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.ContainerServiceClient.Config;
    using Microsoft.ServiceFabric.ContainerServiceClient.Response;

    class ExecOperation
    {
        private readonly ContainerServiceClient client;

        internal ExecOperation(ContainerServiceClient client)
        {
            this.client = client;
        }

        /// <summary>
        ///  Expected return error code:
        ///     201 no error
        ///     404 no such container
        ///     409 container is paused
        ///     500 server error
        /// </summary>
        internal async Task<CreateContainerExecResponse> CreateContainerExecAsync(
            string containerIdOrName,
            ContainerExecConfig containerExecConfig,
            TimeSpan timeout,
            CancellationToken cancellationToken = default(CancellationToken))
        {
            var requestPath = $"containers/{containerIdOrName}/exec";

            var requestContent = JsonRequestContent.GetContent(containerExecConfig);

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

            return ContainerServiceClient.JsonSerializer.DeserializeObject<CreateContainerExecResponse>(response.Body);
        }

        /// <summary>
        ///  Expected return error code:
        ///     200 no error
        ///     404 No such exec instance
        ///     409 Container is stopped or paused
        /// </summary>
        internal async Task StartContainerExecAsync(
            string containerExecId,
            TimeSpan timeout,
            CancellationToken cancellationToken = default(CancellationToken))
        {
            var requestPath = $"exec/{containerExecId}/start";

            var containerExecConfig = new ContainerExecConfig()
            {
                Detach = true
            };

            var requestContent = JsonRequestContent.GetContent(containerExecConfig);

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