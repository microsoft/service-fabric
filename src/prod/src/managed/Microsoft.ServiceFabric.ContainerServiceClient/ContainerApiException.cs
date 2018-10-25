// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient
{
    using System;
    using System.Net;

    class ContainerApiException : Exception
    {
        public HttpStatusCode StatusCode { get; private set; }

        public string ResponseBody { get; private set; }

        public ContainerApiException(HttpStatusCode statusCode, string responseBody)
            : base($"Docker API responded with status code={statusCode}, response={responseBody}")
        {
            this.StatusCode = statusCode;
            this.ResponseBody = responseBody;
        }
    }
}