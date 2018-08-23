// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient
{
    using System.Net;

    internal class ContainerApiResponse
    {
        internal ContainerApiResponse(
            HttpStatusCode statusCode,
            string body)
        {
            this.StatusCode = statusCode;
            this.Body = body;
        }

        public HttpStatusCode StatusCode { get; private set; }

        public string Body { get; private set; }
    }
}