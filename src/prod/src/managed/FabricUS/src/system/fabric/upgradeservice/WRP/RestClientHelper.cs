// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using Net.Http.Headers;
    using System.Collections.Generic;
    using System.Net;
    using System.Net.Http;
    using System.Security.Cryptography.X509Certificates;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Linq;

    internal static class RestClientHelper
    {
        private static readonly TraceType TraceType = new TraceType("RestClientHelper");

        public static async Task<string> InvokeWithCertificatesAsync(
            Uri requestUri,
            HttpMethod method,
            string requestBody,
            List<X509Certificate2> clientCertificates,
            string clusterId,
            CancellationToken token)
        {
            if (clientCertificates == null || !clientCertificates.Any())
            {
                // If no certificates are specified add a null element in the collection
                // InvokeWithCertificateAsync will be called will null clientCertificate
                clientCertificates = new List<X509Certificate2>() {null};
            }

            var exceptions = new List<Exception>();

            string correlationId = Guid.NewGuid().ToString();
            foreach (var clientCertificate in clientCertificates)
            {
                try
                {
                    HttpResponseMessage responseMessage = await InvokeWithCertificateAsync(
                        requestUri,
                        method,
                        requestBody,
                        clientCertificate,
                        clusterId,
                        correlationId,
                        token);

                    string responseString = await responseMessage.Content.ReadAsStringAsync();

                    Trace.WriteInfo(
                        TraceType,
                        "RequestUri: '{0}', Status: '{1}', ReasonPhrase: '{2}', CertificateThumbprint: {3}, CertificateSubject: {4} CorrelationId: {5}.\r\n Content: {6}",
                        requestUri,
                        responseMessage.StatusCode,
                        responseMessage.ReasonPhrase,
                        clientCertificate == null ? "NONE" : clientCertificate.Thumbprint,
                        clientCertificate == null ? "NONE" : clientCertificate.Subject,
                        correlationId,
                        responseString);

                    if (responseMessage.StatusCode == HttpStatusCode.Unauthorized)
                    {
                        continue;
                    }

                    responseMessage.EnsureSuccessStatusCode();

                    return responseString;
                }
                catch (Exception ex)
                {
                    Trace.WriteWarning(
                        TraceType,
                        "RequestUri: '{0}', CertificateThumbprint: {1}, CertificateSubject: {2},  CorrelationId: {3}.\r\n Exception: {4}",
                        requestUri,
                        clientCertificate == null ? "NONE" : clientCertificate.Thumbprint,
                        clientCertificate == null ? "NONE" : clientCertificate.Subject,
                        correlationId,
                        ex);

                    exceptions.Add(ex);
                }
            }

            if (exceptions.Any())
            {
                throw new AggregateException(
                    $"Failed to send request while retrying with all available certificates. Request: {method} {requestUri}!",
                    exceptions);                                      
            }

            StringBuilder certificateListStringBuilder = new StringBuilder();
            clientCertificates.ForEach(cert => certificateListStringBuilder.AppendFormat("{0} ", cert == null ? "NONE" : cert.Thumbprint));

            throw new UnauthorizedAccessException(string.Format(
                "Certificates {0}not authorized to access URL {1}", certificateListStringBuilder.ToString(), requestUri.ToString()));
        }

        private static Task<HttpResponseMessage> InvokeWithCertificateAsync(
            Uri requestUri,
            HttpMethod method,
            string requestBody,
            X509Certificate2 clientCertificate,
            string clusterId,
            string correlationId,
            CancellationToken token)
        {
            HttpClientHandler requestHandler = CreateHttpClientHandler(clientCertificate);
            HttpClient httpClient = new HttpClient(requestHandler, true)
            {
                Timeout = TimeSpan.FromMinutes(1)
            };

            httpClient.DefaultRequestHeaders.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));

            HttpRequestMessage requestMessage = new HttpRequestMessage(method, requestUri)
            {
                Content = new StringContent(requestBody, Encoding.UTF8, "application/json")
            };

            requestMessage.Headers.Add("x-ms-correlation-request-id", correlationId);
            requestMessage.Headers.Add("x-ms-client-request-id", clusterId);

            return httpClient.SendAsync(requestMessage, token);
        }

#if DotNetCoreClr
        private static HttpClientHandler CreateHttpClientHandler(X509Certificate2 clientCertificate)
        {
            HttpClientHandler handler = new HttpClientHandler();

            if (clientCertificate != null)
            {
                handler.ClientCertificateOptions = ClientCertificateOption.Manual;
                handler.ClientCertificates.Add(clientCertificate);
            }

            return handler;
        }
#else
        private static HttpClientHandler CreateHttpClientHandler(X509Certificate2 clientCertificate)
        {
            WebRequestHandler handler = new WebRequestHandler();

            if (clientCertificate != null)
            {
                handler.ClientCertificateOptions = ClientCertificateOption.Manual;
                handler.ClientCertificates.Add(clientCertificate);
            }

            return handler;
        }
#endif
    }
}