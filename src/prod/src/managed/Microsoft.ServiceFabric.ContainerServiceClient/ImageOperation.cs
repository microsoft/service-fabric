// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient
{
    using Microsoft.ServiceFabric.ContainerServiceClient.Config;
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Net;
    using System.Net.Http;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    class ImageOperation
    {
        private const string RegistryAuthHeaderKey = "X-Registry-Auth";

        private readonly ContainerServiceClient client;

        internal ImageOperation(ContainerServiceClient client)
        {
            this.client = client;
        }

        /// <summary>
        ///  Expected return error code:
        ///     200 no error
        ///     404 repository does not exist or no read access
        ///     500 server error
        /// </summary>
        internal async Task<Stream> CreateImageAsync(
            string imageName,
            AuthConfig authConfig,
            TimeSpan timeout,
            CancellationToken cancellationToken = default(CancellationToken))
        {
            var requestPath = "images/create";
            var queryString = $"fromImage={Uri.EscapeDataString(imageName)}";

            var headers = GetRegistryAuthHeaders(authConfig);

            var response = await this.client.MakeRequestForHttpResponseAsync(
                HttpMethod.Post,
                requestPath,
                queryString,
                headers,
                null,
                timeout).ConfigureAwait(false);

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
            catch(Exception)
            {
                response.Dispose();
                throw;
            }
        }

        /// <summary>
        ///  Expected return error code:
        ///     200 The image was deleted successfully
        ///     404 No such image
        ///     409 Conflict
        ///     500 server error
        /// </summary>
        internal async Task DeleteImageAsync(
            string imageName,
            TimeSpan timeout,
            CancellationToken cancellationToken = default(CancellationToken))
        {
            var requestPath = $"images/{imageName}";

            var response = await this.client.MakeRequestAsync(
                HttpMethod.Delete,
                requestPath,
                null,
                null,
                timeout).ConfigureAwait(false);

            if (response.StatusCode != HttpStatusCode.OK &&
                response.StatusCode != HttpStatusCode.NotFound)
            {
                throw new ContainerApiException(response.StatusCode, null);
            }
        }

        /// <summary>
        ///  Expected return error code:
        ///     200 List of image layers
        ///     404 No such image
        ///     500 server error
        /// </summary>
        internal async Task GetImageHistoryAsync(
            string imageName,
            TimeSpan timeout,
            CancellationToken cancellationToken = default(CancellationToken))
        {
            var requestPath = $"images/{imageName}/history";

            var response = await this.client.MakeRequestAsync(
                HttpMethod.Get,
                requestPath,
                null,
                null,
                timeout).ConfigureAwait(false);

            if (response.StatusCode != HttpStatusCode.OK)
            {
                throw new ContainerApiException(response.StatusCode, null);
            }
        }

        private static Dictionary<string, string> GetRegistryAuthHeaders(AuthConfig authConfig)
        {
            var authConfigJson = ContainerServiceClient.JsonSerializer.SerializeObject(authConfig);
            var authConfigBytes = Encoding.UTF8.GetBytes(authConfigJson);
            var authConfigBase64Str = Convert.ToBase64String(authConfigBytes);

            return new Dictionary<string, string>
            {
                { RegistryAuthHeaderKey, authConfigBase64Str }
            };
        }
    }
}