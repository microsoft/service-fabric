// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Helpers
{
    using System;
    using System.Net;
    using System.Net.Http;
    using System.Net.Sockets;
    using System.Threading;
    using System.Threading.Tasks;
    using ClusterAnalysis.Common;
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.Common.Runtime;
    using ClusterAnalysis.Common.Util;

    internal class HttpServiceClientHandler : HttpClientHandler
    {
        private const int MaxRetries = 5;

        private const int InitialRetryDelayMs = 50;

        private readonly Random random = new Random();

        private IResolveServiceEndpoint serviceResolver;

        private ILogger logger;

        public HttpServiceClientHandler(IInsightRuntime runtime, IResolveServiceEndpoint serviceEndpointResolver)
        {
            Assert.IsNotNull(runtime, "Runtime can't be null");
            Assert.IsNotNull(serviceEndpointResolver, "Service Endpoint resolver can't be null");
            this.serviceResolver = serviceEndpointResolver;
            this.logger = runtime.GetLogProvider().CreateLoggerInstance("HttpServiceClientHandler");
        }

        /// <summary>
        /// </summary>
        /// <param name="request"></param>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        protected override async Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
        {
            this.logger.LogMessage("SendAsync:: Entering. Request URI is '{0}'", request.RequestUri);
            HttpServiceUriBuilder uriBuilder = new HttpServiceUriBuilder(request.RequestUri);
            int retries = MaxRetries;
            int retryDelay = InitialRetryDelayMs;
            bool resolveAddress = true;

            HttpResponseMessage lastResponse = null;
            Exception lastException = null;

            while (retries-- > 0)
            {
                lastResponse = null;
                cancellationToken.ThrowIfCancellationRequested();

                if (resolveAddress)
                {
                    this.logger.LogMessage("SendAsync:: Resolving Address");
                    request.RequestUri =
                        await
                            this.serviceResolver.GetServiceEndpointAsync(
                                uriBuilder.ServiceName,
                                uriBuilder.ServiceHostNode,
                                uriBuilder.ResourcePathAndQuery,
                                cancellationToken).ConfigureAwait(false);

                    this.logger.LogMessage("SendAsync:: Resolved URI is '{0}'", request.RequestUri);
                    resolveAddress = false;
                }

                try
                {
                    lastResponse = await base.SendAsync(request, cancellationToken);
                    this.logger.LogMessage("SendAsync:: Last Response is '{0}'", lastResponse);

                    if (lastResponse.StatusCode == HttpStatusCode.NotFound || lastResponse.StatusCode == HttpStatusCode.ServiceUnavailable)
                    {
                        this.logger.LogMessage("SendAsync:: Addressed will be resolved again");
                        resolveAddress = true;
                    }
                    else
                    {
                        return lastResponse;
                    }
                }
                catch (TimeoutException te)
                {
                    this.logger.LogMessage("SendAsync:: Encountered Timeout Exception");
                    lastException = te;
                    resolveAddress = true;
                }
                catch (SocketException se)
                {
                    this.logger.LogMessage("SendAsync:: Encountered Socket Exception");
                    lastException = se;
                    resolveAddress = true;
                }
                catch (HttpRequestException hre)
                {
                    this.logger.LogMessage("SendAsync:: Encountered Http Request Exception");
                    lastException = hre;
                    resolveAddress = true;
                }
                catch (Exception ex)
                {
                    lastException = ex;
                    WebException we = ex as WebException ?? ex.InnerException as WebException;

                    if (we != null)
                    {
                        this.logger.LogMessage("SendAsync:: Encountered Web Exception with Response: '{0}'", we.Response);
                        HttpWebResponse errorResponse = we.Response as HttpWebResponse;

                        // the following assumes port sharing
                        // where a port is shared by multiple replicas within a host process using a single web host (e.g., http.sys).
                        if (we.Status == WebExceptionStatus.ProtocolError)
                        {
                            if (errorResponse.StatusCode == HttpStatusCode.NotFound || errorResponse.StatusCode == HttpStatusCode.ServiceUnavailable)
                            {
                                // This could either mean we requested an endpoint that does not exist in the service API (a user error)
                                // or the address that was resolved by fabric client is stale (transient runtime error) in which we should re-resolve.
                                resolveAddress = true;
                            }

                            // On any other HTTP status codes, re-throw the exception to the caller.
                            throw;
                        }

                        if (we.Status == WebExceptionStatus.Timeout || we.Status == WebExceptionStatus.RequestCanceled
                            || we.Status == WebExceptionStatus.ConnectionClosed || we.Status == WebExceptionStatus.ConnectFailure)
                        {
                            resolveAddress = true;
                        }
                    }
                    else
                    {
                        throw;
                    }
                }

                await Task.Delay(retryDelay, cancellationToken);

                retryDelay += retryDelay;
            }

            if (lastResponse != null)
            {
                return lastResponse;
            }
            else
            {
                throw lastException;
            }
        }
    }
}