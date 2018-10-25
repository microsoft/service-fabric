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
    using Microsoft.ServiceFabric.ContainerServiceClient.Response;
    using Microsoft.ServiceFabric.ContainerServiceClient.Config;
    using System.Collections.Generic;

    internal class ContainerOperation
    {
        private readonly ContainerServiceClient client;

        internal ContainerOperation(ContainerServiceClient client)
        {
            this.client = client;
        }

        /// <summary>
        ///  Expected return error code:
        ///     201 Container created successfully
        ///     400 bad parameter
        ///     404 no such container
        ///     406 impossible to attach
        ///     409 conflict
        ///     500 server error
        /// </summary>
        internal async Task<CreateContainerResponse> CreateContainerAsync(
            string containerName,
            ContainerConfig containerConfig,
            TimeSpan timeout)
        {
            var requestPath = "containers/create";
            var queryString = $"name={containerName}";

            var requestContent = JsonRequestContent.GetContent(containerConfig);

            var response = await this.client.MakeRequestAsync(
                HttpMethod.Post,
                requestPath,
                queryString,
                requestContent,
                timeout).ConfigureAwait(false);

            if(response.StatusCode != HttpStatusCode.Created)
            {
                throw new ContainerApiException(response.StatusCode, response.Body);
            }

            return ContainerServiceClient.JsonSerializer.DeserializeObject<CreateContainerResponse>(response.Body);
        }

        /// <summary>
        ///  Expected return error code:
        ///     200 No error
        ///     404 no such container
        ///     500 server error
        /// </summary>
        internal async Task<ContainerInspectResponse> InspectContainersAsync(
            string containerIdOrName,
            TimeSpan timeout)
        {
            var requestPath = $"containers/{containerIdOrName}/json";

            var response = await this.client.MakeRequestAsync(
                HttpMethod.Get,
                requestPath,
                null,
                null,
                timeout).ConfigureAwait(false);

            if (response.StatusCode != HttpStatusCode.OK)
            {
                throw new ContainerApiException(response.StatusCode, response.Body);
            }

            return ContainerServiceClient.JsonSerializer.DeserializeObject<ContainerInspectResponse>(response.Body);
        }

        /// <summary>
        ///  Expected return error code:
        ///     200 No error
        ///     500 server error
        /// </summary>
        internal async Task<ContainersPruneResponse> PruneContainersAsync(
            ContainersPruneConfig pruneConfig,
            TimeSpan timeout)
        {
            var requestPath = "containers/prune";
            var queryString = QueryParameterHelper.BuildQueryString(pruneConfig);

            var response = await this.client.MakeRequestAsync(
                HttpMethod.Post,
                requestPath,
                queryString,
                null,
                timeout).ConfigureAwait(false);

            if (response.StatusCode != HttpStatusCode.OK)
            {
                throw new ContainerApiException(response.StatusCode, response.Body);
            }

            return ContainerServiceClient.JsonSerializer.DeserializeObject<ContainersPruneResponse>(response.Body);
        }

        /// <summary>
        ///  Expected return error code:
        ///     204 no error
        ///     304 container already started
        ///     404 no such container
        ///     500 server error
        /// </summary>
        internal async Task StartContainerAsync(
            string containerIdOrName,
            TimeSpan timeout,
            CancellationToken cancellationToken = default(CancellationToken))
        {
            var requestPath = $"containers/{containerIdOrName}/start";

            var response = await this.client.MakeRequestAsync(
                HttpMethod.Post,
                requestPath,
                null,
                null,
                timeout).ConfigureAwait(false);

            if (response.StatusCode != HttpStatusCode.NoContent &&
                response.StatusCode != HttpStatusCode.NotModified)
            {
                throw new ContainerApiException(response.StatusCode, response.Body);
            }
        }

        /// <summary>
        ///  Expected return error code:
        ///     204 no error
        ///     304 container already stopped
        ///     404 no such container
        ///     500 server error
        /// </summary>
        internal async Task StopContainerAsync(
            string containerIdOrName,
            int waitBeforeKillSec,
            TimeSpan timeout,
            CancellationToken cancellationToken = default(CancellationToken))
        {
            var requestPath = $"containers/{containerIdOrName}/stop";
            var queryString = $"t={waitBeforeKillSec}";

            var response = await this.client.MakeRequestAsync(
                HttpMethod.Post,
                requestPath,
                queryString,
                null,
                timeout).ConfigureAwait(false);

            if (response.StatusCode != HttpStatusCode.NoContent &&
                response.StatusCode != HttpStatusCode.NotModified &&
                response.StatusCode != HttpStatusCode.NotFound)
            {
                throw new ContainerApiException(response.StatusCode, response.Body);
            }
        }

        /// <summary>
        ///  Expected return error code:
        ///     204 no error
        ///     400 bad parameter
        ///     404 no such container
        ///     409 conflict
        ///     500 server error
        /// </summary>
        internal async Task RemoveContainerAsync(
            string containerIdOrName,
            bool force,
            TimeSpan timeout,
            CancellationToken cancellationToken = default(CancellationToken))
        {
            var requestPath = $"containers/{containerIdOrName}";
            var queryString = $"force={force.ToString().ToLower()}";

            var response = await this.client.MakeRequestAsync(
                HttpMethod.Delete,
                requestPath,
                queryString,
                null,
                timeout).ConfigureAwait(false);

            if (response.StatusCode != HttpStatusCode.NoContent &&
                response.StatusCode != HttpStatusCode.NotFound)
            {
                throw new ContainerApiException(response.StatusCode, response.Body);
            }
        }
    }
}