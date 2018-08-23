// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient
{
    using System;
    using System.IO;
    using System.Net;
    using System.Net.Http;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.ContainerServiceClient.Config;
    using Microsoft.ServiceFabric.ContainerServiceClient.Response;

    class SystemOperation
    {
        private readonly ContainerServiceClient client;

        internal SystemOperation(ContainerServiceClient client)
        {
            this.client = client;
        }

        /// <summary>
        ///  Expected return error code:
        ///     200 no error
        ///     500 server error
        /// </summary>
        internal async Task PingAsync(TimeSpan timeout)
        {
            var requestPath = "_ping";

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
        }

        /// <summary>
        ///  Expected return error code:
        ///     200 no error
        ///     500 server error
        /// </summary>
        internal async Task<VersionResponse> GetVersionAsync(TimeSpan timeout)
        {
            var requestPath = "version";

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

            return ContainerServiceClient.JsonSerializer.DeserializeObject<VersionResponse>(response.Body);
        }

        /// <summary>
        ///  Expected return error code:
        ///     200 no error
        ///     400 bad parameter
        ///     500 server error
        /// </summary>
        internal async Task<Stream> MonitorEventAsync(ContainerEventsConfig eventsConfig)
        {
            var requestPath = "events";
            var queryString = QueryParameterHelper.BuildQueryString(eventsConfig);

            var response = await this.client.MakeRequestForHttpResponseAsync(
                HttpMethod.Get,
                requestPath,
                queryString,
                null,
                null,
                Timeout.InfiniteTimeSpan).ConfigureAwait(false);

            if (response.StatusCode != HttpStatusCode.OK)
            {
                var statusCode = response.StatusCode;

                response.Dispose();

                throw new ContainerApiException(statusCode, null);
            }

            try
            {
                return await response.Content.ReadAsStreamAsync().ConfigureAwait(false);
            }
            catch (Exception)
            {
                response.Dispose();
                throw;
            }
        }
    }
}