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

    class VolumeOperation
    {
        private readonly ContainerServiceClient client;

        internal VolumeOperation(ContainerServiceClient client)
        {
            this.client = client;
        }

        /// <summary>
        ///  Expected return error code:
        ///     201 The volume was created successfully
        ///     500 server error
        /// </summary>
        internal async Task CreateVolumeAsync(
            VolumeConfig volumeConfig,
            TimeSpan timeout,
            CancellationToken cancellationToken = default(CancellationToken))
        {
            var requestPath = "volumes/create";

            var requestContent = JsonRequestContent.GetContent(volumeConfig);

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
        }
    }
}