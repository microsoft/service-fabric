// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Linq;
    using System.Net.Http;
    using System.Threading;
    using System.Threading.Tasks;

    internal class ContainerServiceClient
    {
        private const string UserAgent = "Microsoft.ServiceFabric";

        internal static readonly JsonSerializer JsonSerializer = new JsonSerializer();

        private readonly Uri requestBaseUri;
        private readonly Uri serviceBaseUri;
        private readonly HttpClient client;

        private bool isContainerServiceManaged;

        internal ContainerServiceClient(string serviceUri)
        {
            this.serviceBaseUri = new Uri(serviceUri);

            HttpMessageHandler messageHandler = null;
            var uriScheme = this.serviceBaseUri.Scheme.ToLowerInvariant();

#if DotNetCoreClrLinux
            if (uriScheme == "unix")
            {
                var pipeString = this.serviceBaseUri.LocalPath;
                this.requestBaseUri = new UriBuilder("http", this.serviceBaseUri.Segments.Last()).Uri;
                messageHandler = new UnixDomainSocketHttpMessageHandler(this, pipeString);
            }
#else
            if (uriScheme == "npipe")
            {
                var pipeName = this.serviceBaseUri.Segments[2];
                this.requestBaseUri = new UriBuilder("http", pipeName).Uri;
                messageHandler = new NamedPipeHttpMessageHandler(this, pipeName);
            }
#endif

            if(messageHandler == null)
            {
                // Use default client
                this.client = new HttpClient()
                {
                    Timeout = Timeout.InfiniteTimeSpan
                };
            }
            else
            {
                this.client = new HttpClient(messageHandler, true)
                {
                    Timeout = Timeout.InfiniteTimeSpan
                };
            }

            this.ContainerOperation = new ContainerOperation(this);
            this.VolumeOperation = new VolumeOperation(this);
            this.ExecOperation = new ExecOperation(this);
            this.NetworkOperation = new NetworkOperation(this);
            this.SystemOperation = new SystemOperation(this);
            this.ImageOperation = new ImageOperation(this);
        }

        internal HttpClient HttpClient
        {
            get { return this.client; }
        }

        internal Uri ServiceBaseUri
        {
            get { return this.serviceBaseUri; }
        }

        internal bool IsContainerServiceManaged
        {
            get { return this.isContainerServiceManaged; }
            set { this.isContainerServiceManaged = value; }
        }

        internal ContainerOperation ContainerOperation { get; private set; }

        internal VolumeOperation VolumeOperation { get; private set; }

        internal ExecOperation ExecOperation { get; private set; }

        internal NetworkOperation NetworkOperation { get; private set; }

        internal SystemOperation SystemOperation { get; private set; }

        internal ImageOperation ImageOperation { get; private set; }

        internal Uri GetRequestUri(string requestPathWithQueryParams)
        {
            var baseUriStr = this.requestBaseUri.ToString();

            var pathWithQueryParam = requestPathWithQueryParams;
            if (requestPathWithQueryParams.StartsWith("/"))
            {
                pathWithQueryParam = requestPathWithQueryParams.Substring(1);
            }

            var requestUriStr = string.Format("{0}{1}", baseUriStr, pathWithQueryParam);

            return new Uri(requestUriStr);
        }

        internal async Task<ContainerApiResponse> MakeRequestAsync(
            HttpMethod method,
            string requestPath,
            string queryString,
            HttpContent requestContent,
            TimeSpan timeout)
        {
            var response = await this.MakeRequestForHttpResponseAsync(
                method, requestPath, queryString, null, requestContent, timeout).ConfigureAwait(false);

            using (response)
            {
                var responseBody = await response.Content.ReadAsStringAsync().ConfigureAwait(false);

                return new ContainerApiResponse(response.StatusCode, responseBody);
            }
        }

        /// <summary>
        ///  Caller of this function own the HttpResponseMessage and should ensure
        ///  dispose it after reading the response.
        /// </summary>
        internal Task<HttpResponseMessage> MakeRequestForHttpResponseAsync(
            HttpMethod method,
            string requestPath,
            string queryString,
            IDictionary<string, string> headers,
            HttpContent requestContent,
            TimeSpan timeout)
        {
            var requestUri = this.BuildUri(requestPath, queryString);
            return this.MakeRequestPrivateAsync(method, requestUri, headers, requestContent, timeout);
        }

        internal Task<HttpResponseMessage> MakeRequestForHttpResponseAsync(
            HttpMethod method,
            string requestPathWithQueryParams,
            IDictionary<string, string> headers,
            HttpContent requestContent,
            TimeSpan timeout)
        {
            var requestUri = this.GetRequestUri(requestPathWithQueryParams);
            return this.MakeRequestPrivateAsync(method, requestUri, headers, requestContent, timeout);
        }

#region Private Helpers

        private async Task<HttpResponseMessage> MakeRequestPrivateAsync(
            HttpMethod method,
            Uri requestUri,
            IDictionary<string, string> headers,
            HttpContent requestContent,
            TimeSpan timeout)
        {
            var request = PrepareRequest(method, requestUri, headers, requestContent);

            var tokenSource = new CancellationTokenSource(timeout);

            try
            {
                return await this.client.SendAsync(
                    request, HttpCompletionOption.ResponseHeadersRead, tokenSource.Token);
            }
            catch(OperationCanceledException)
            {
                if(tokenSource.IsCancellationRequested)
                {
                    throw new FabricException(
                        $"Docker operation timed out. RequestUri={requestUri}",
                        FabricErrorCode.OperationTimedOut);
                }

                throw;
            }
        }

        private static HttpRequestMessage PrepareRequest(
            HttpMethod method, 
            Uri requestUri,
            IDictionary<string, string> headers,
            HttpContent content)
        {
            var request = new HttpRequestMessage(method, requestUri);

            request.Headers.Add("User-Agent", UserAgent);

            if (headers != null)
            {
                foreach (var header in headers)
                {
                    request.Headers.Add(header.Key, header.Value);
                }
            }

            if (content != null)
            {
                request.Content = content;
            }

            return request;
        }

        private Uri BuildUri(string requestPath, string queryString)
        {
            var builder = new UriBuilder(this.requestBaseUri)
            {
                Path = requestPath
            };

            if (!string.IsNullOrEmpty(queryString))
            {
                builder.Query = queryString;
            }

            return builder.Uri;
        }

#endregion
    }
}