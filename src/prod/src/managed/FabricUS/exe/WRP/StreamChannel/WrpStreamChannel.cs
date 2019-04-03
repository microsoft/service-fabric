// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.IO;
    using System.Linq;
    using System.Net;
    using System.Net.Http;
    using System.Net.Security;
    using System.Security.Cryptography.X509Certificates;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    internal class WrpStreamChannel : WrpGatewayClient
    {
        private const string StreamChannelPartailPath = "streams";

        //// In case of some corner network issue,which leads the client be hard to detect the connection lost
        //// set this timeout shorter one, so that every request to SFRP can get a chance to succeed. 
        private readonly TimeSpan defaultTimeout = TimeSpan.FromSeconds(30);
        private readonly TimeSpan failureRetryInterval = TimeSpan.FromSeconds(10);
        private readonly TraceType traceType = new TraceType("WrpStreamChannel");
        private Uri streamChannelUri;
        private string httpGatewayListenHost;
        private int httpGatewayListenPort;
        private bool httpGatewayListenAddressResolved = false;

        public WrpStreamChannel(
            Uri baseUrl,
            string clusterId,
            IConfigStore configStore,
            string configSectionName)
            : base(baseUrl, clusterId, configStore, configSectionName)
        {
        }

        protected override TraceType TraceType
        {
            get { return this.traceType; }
        }

        #region Test Hook
        internal HttpMessageHandler MockTestHandler
        {
            get;
            set;
        }
        #endregion


        public Task StartStreamChannel(CancellationToken token)
        {
            var httpGatewayListenAddress = ConfigStore.ReadUnencryptedString("FabricNode", "HttpGatewayListenAddress");

            if (!string.IsNullOrWhiteSpace(httpGatewayListenAddress))
            {
                var parts = httpGatewayListenAddress.Split(':');
                if (parts != null && parts.Count() == 2)
                {
                    this.httpGatewayListenHost = parts[0];
                    int.TryParse(parts[1], out this.httpGatewayListenPort);
                }
            }

            if (!string.IsNullOrWhiteSpace(this.httpGatewayListenHost) && this.httpGatewayListenPort > 0)
            {
                this.httpGatewayListenAddressResolved = true;
                Trace.WriteInfo(TraceType, "Resolved HttpGatewayListenAddress endpoint {0}:{1}", this.httpGatewayListenHost, this.httpGatewayListenPort);
            }
            else
            {
                this.httpGatewayListenAddressResolved = false;
                Trace.WriteWarning(TraceType, "Resolve HttpGatewayListenAddress endpoint failed {0}", httpGatewayListenAddress);
            }

            return StartStreamChannelInternalAsync(token);
        }

        public Task<StreamChannelPutResponse> GetWrpServiceResponse(
            Uri requestUri,
            StreamResponse request,
            CancellationToken token)
        {
            Trace.WriteInfo(TraceType, "WrpStreamChannel GetWrpServiceResponse start");
            var response = this.GetWrpServiceResponseAsync<StreamResponse, StreamChannelPutResponse>(requestUri, HttpMethod.Put, request, token);
            Trace.WriteInfo(TraceType, "WrpStreamChannel GetWrpServiceResponse end");
            return response;
        }

        public void OnCancel(object waitTask)
        {
            //disable CS4014
            var tmp = TrackOldTask((Task)waitTask);
        }

        #region private Helper Method

        private async Task StartStreamChannelInternalAsync(CancellationToken token)
        {
            const int retyTimes = 24 * 60 / 5; // retry at max 1 day ahead
            int ahead = -1;
            while (!token.IsCancellationRequested && ++ahead <= retyTimes)
            {
                ClusterStreamingId streamingId = null;
                try
                {
                    streamingId = new ClusterStreamingId((DateTime.Now + TimeSpan.FromMinutes(5 * ahead)).Ticks);
                    var partialPath = string.Format("/{0}/{1}", StreamChannelPartailPath, streamingId);
                    var uriBuilder = new UriBuilder(this.BaseUrl);
                    uriBuilder.Path += partialPath;
                    this.streamChannelUri = uriBuilder.Uri;

#if DotNetCoreClr

                    using (var streamChannel = await this.InvokeRequestWithCertAsync(
                            GetCertificates(),
                            null,
                            client => this.GetStreamAsync(client, this.streamChannelUri)))
#else
                    using (var streamChannel = await this.InvokeRequestWithCertAsync(
                            GetCertificates(),
                            null,
                            client => this.GetStreamAsync(client, this.streamChannelUri))
                            )
#endif
                    {
                        if (streamChannel != null)
                        {
                            Trace.WriteInfo(TraceType, "WrpStreamChannel connected");

                            await this.ProcessStreamChannelRequestAsync(streamChannel, token).ConfigureAwait(false);

                            Trace.WriteInfo(TraceType, "WrpStreamChannel disconnected");
                        }
                        else
                        {
                            Trace.WriteError(
                                    TraceType,
                                    "Failed to StartStreamChannel");
                        }
                    }

                    await Task.Delay(this.failureRetryInterval, token);
                }
                catch (StreamChannelException streamChannelException)
                {
                    Trace.WriteWarning(TraceType, "WrpStreamChannel StartStreamChannelInternalAsync has error {0}", streamChannelException.ErrorDetails);

                    if (streamChannelException.ErrorDetails != null && streamChannelException.ErrorDetails.Error != null)
                    {
                        if (streamChannelException.ErrorDetails.Error.Code == "ConnectUsingLowerStreamId")
                        {
                            Trace.WriteWarning(TraceType, "WrpStreamChannel connecting using lower stream id {0}", streamingId);
                            continue;
                        }
                    }

                    throw;
                }
            }

        }

#if DotNetCoreClr

        private async Task<TResult> InvokeRequestWithCertAsync<TResult>(
            List<X509Certificate2> clientCertificates,
            Func<HttpRequestMessage, X509Certificate2, X509Chain, SslPolicyErrors, bool> certValidationCallback,
            Func<HttpClient, Task<TResult>> func)
        {
            if (clientCertificates != null && clientCertificates.Any())
            {
                for (var i = 0; i < clientCertificates.Count; i++)
                {
                    try
                    {
                        var clientHandler = new HttpClientHandler()
                        {
                            ClientCertificateOptions = ClientCertificateOption.Manual
                        };

                        clientHandler.ClientCertificates.Add(clientCertificates[i]);
                        if (certValidationCallback != null)
                        {
                            clientHandler.ServerCertificateCustomValidationCallback = certValidationCallback;
                        }

                        var client = new HttpClient(clientHandler)
                        {
                            Timeout = this.defaultTimeout
                        };

                        return await func(client);
                    }
                    catch (WebException we)
                    {
                        Trace.WriteWarning(
                            TraceType,
                            "Client call to {0} with thumbprint {1} failed with {2}. Description: {3}",
                            this.streamChannelUri.ToString(),
                            clientCertificates[i].Thumbprint,
                            we.Status,
                            we.Response != null ? ((HttpWebResponse)we.Response).StatusDescription : null);

                        if (we.Response != null &&
                            ((HttpWebResponse)we.Response).StatusCode == HttpStatusCode.Unauthorized &&
                            i + 1 < clientCertificates.Count)
                        {
                            continue;
                        }

                        throw;
                    }
                    catch (Exception e)
                    {
                        Trace.WriteWarning(
                            TraceType,
                            "Client call to {0} with thumbprint {1} failed with {2}",
                            this.BaseUrl.ToString(),
                            clientCertificates[i].Thumbprint,
                            e.ToString());

                        if (i + 1 < clientCertificates.Count)
                        {
                            continue;
                        }

                        throw;
                    }
                }

                return default(TResult);
            }
            else
            {
                var clientHandler = new HttpClientHandler()
                {
                    ClientCertificateOptions = ClientCertificateOption.Automatic
                };

                if (certValidationCallback != null)
                {
                    clientHandler.ServerCertificateCustomValidationCallback += certValidationCallback;
                }

                var client = new HttpClient(clientHandler)
                {
                    Timeout = this.defaultTimeout
                };

                return await func(client);
            }
        }
#else
        private async Task<TResult> InvokeRequestWithCertAsync<TResult>(
            List<X509Certificate2> clientCertificates,
            RemoteCertificateValidationCallback certValidationCallback,
            Func<HttpClient, Task<TResult>> func)
        {
            if (clientCertificates != null && clientCertificates.Any())
            {
                for (var i = 0; i < clientCertificates.Count; i++)
                {
                    try
                    {
                        var clientHandler = new WebRequestHandler
                        {
                            ClientCertificateOptions = ClientCertificateOption.Manual
                        };

                        clientHandler.ClientCertificates.Add(clientCertificates[i]);
                        if (certValidationCallback != null)
                        {
                            clientHandler.ServerCertificateValidationCallback += certValidationCallback;
                        }

                        var client = new HttpClient(clientHandler)
                        {
                            Timeout = this.defaultTimeout
                        };

                        return await func(client);
                    }
                    catch (WebException we)
                    {
                        Trace.WriteWarning(
                            TraceType,
                            "Client call to {0} with thumbprint {1} failed with {2}. Description: {3}",
                            this.streamChannelUri.ToString(),
                            clientCertificates[i].Thumbprint,
                            we.Status,
                            we.Response != null ? ((HttpWebResponse)we.Response).StatusDescription : null);

                        if (we.Response != null &&
                            ((HttpWebResponse)we.Response).StatusCode == HttpStatusCode.Unauthorized &&
                            i + 1 < clientCertificates.Count)
                        {
                            continue;
                        }

                        throw;
                    }
                    catch (Exception e)
                    {
                        Trace.WriteWarning(
                            TraceType,
                            "Client call to {0} with thumbprint {1} failed with {2}",
                            this.BaseUrl.ToString(),
                            clientCertificates[i].Thumbprint,
                            e.ToString());

                        if (i + 1 < clientCertificates.Count)
                        {
                            continue;
                        }

                        throw;
                    }
                }

                return default(TResult);
            }
            else
            {
                HttpClient client = null;
                if (MockTestHandler != null)
                {
                    client = new HttpClient(MockTestHandler)
                    {
                        Timeout = this.defaultTimeout
                    };
                }
                else
                {
                    client = new HttpClient()
                    {
                        Timeout = this.defaultTimeout
                    };
                }

                return await func(client);
            }
        }
#endif

        #region Process StreamRequest 

        private async Task ProcessStreamChannelRequestAsync(Stream stream, CancellationToken cancellationToken)
        {
            try
            {
                using (var reader = new WrpStreamReader(stream, cancellationToken))
                {
                    reader.SetReadTimeout((int)this.defaultTimeout.TotalMilliseconds);

                    while (!await reader.IsEndOfStream() && !cancellationToken.IsCancellationRequested)
                    {
                        var lengthArrary = new char[4];
                        await reader.ReadAsync(lengthArrary, 0, 4);
                        var length = (lengthArrary[3] << 24) + (lengthArrary[2] << 16) + (lengthArrary[1] << 8) + lengthArrary[0];
                        var readBuffer = new char[length];
                        var readLength = 0;
                        if ((readLength = await reader.ReadAsync(readBuffer, 0, readBuffer.Length)) == length)
                        {
                            var streamRequestString = Encoding.UTF8.GetBytes(readBuffer);
                            var streamRequest = JsonSerializationHelper.DeserializeObject<StreamRequest>(
                                streamRequestString,
                                JsonSerializationHelper.DefaultJsonSerializerSettings);

                            if (streamRequest.PingRequest)
                            {
                                Trace.WriteNoise(TraceType, "WrpStreamChannel: Received a ping request");
                                continue;
                            }

                            this.ProcessEachStreamRequestAsync(streamRequest, cancellationToken);
                        }
                        else
                        {
                            Trace.WriteError(TraceType, "Expected read length is {0}, but actually read length is {1}, close the stream now .", length, readLength);

                            // something corrupt there, need to exit to avoid further corruption
                            return;
                        }
                    }

                    Trace.WriteInfo(TraceType, "WrpStreamChannel: Reached the end of the stream");
                }
            }
            catch (Exception e)
            {
                Trace.WriteWarning(TraceType, "WrpStreamChannel: ProcessStreamChannelRequest thrown {0}", e);

                while (e.InnerException != null)
                {
                    e = e.InnerException;
                }

                if (e is WebException || e is HttpRequestException)
                {
                    var webException = (WebException)e;
                    if (webException.Status == WebExceptionStatus.Timeout)
                    {
                        Trace.WriteWarning(TraceType, "WrpStreamChannel:Client timeout");
                    }
                }
                else
                {
                    throw;
                }
            }
        }

        ////Query from local cluster
        private async Task<StreamResponse> ForwardWrpStreamChannelRequestAsync(
            StreamRequest streamRequest,
            CancellationToken cancellationToken)
        {
            var request = this.httpGatewayListenAddressResolved 
                ? streamRequest.Replace(this.httpGatewayListenHost, this.httpGatewayListenPort) 
                : streamRequest;
            
            Trace.WriteInfo(TraceType, "ForwardWrpStreamChannelRequestAsync to SF Gateway. Request = {0}", request.ToString());

#if DotNetCoreClr


            var responseMessage = await InvokeRequestWithCertAsync(
                 GetCertificates(),
                 CertificateChainValidator<HttpRequestMessage>,
                 async client =>               
                    {
                        var requestMessage = this.BuildRequestMessage(request);
                        return await client.SendAsync(requestMessage, cancellationToken);
                    });

#else

            var responseMessage = await InvokeRequestWithCertAsync<HttpResponseMessage>(
                GetCertificates(),
                CertificateChainValidator,
                async client =>               
                    {
                        var requestMessage = this.BuildRequestMessage(request);
                        return await client.SendAsync(requestMessage, cancellationToken);
                    });

#endif
            Trace.WriteInfo(TraceType, "ForwardWrpStreamChannelRequestAsync to SF Gateway. Done");

            var headerDictionary = new Dictionary<string, string>();
            if (responseMessage.Headers != null)
            {
                foreach (var kvp in responseMessage.Headers)
                {
                    headerDictionary[kvp.Key] = string.Join(", ", kvp.Value);
                }
            }

            var responseContent = responseMessage.Content;

            var mediaType = responseContent == null
                   ? null
                   : responseContent.Headers.ContentType.MediaType;
            var charSet = responseContent == null
                ? null
                : responseContent.Headers.ContentType.CharSet;
            var content = responseContent == null
                ? null
                : await responseContent.ReadAsStringAsync();

            var streamResponse = new StreamResponse()
            {
                Headers = headerDictionary,
                StatusCode = responseMessage.StatusCode,
                RequestId = streamRequest.RequestId,
                Content = content,
                ReasonPhrase = responseMessage.ReasonPhrase,
                RequestReceivedStamp = DateTime.UtcNow,
                MediaType = mediaType,
                CharSet = charSet
            };

            Trace.WriteInfo(TraceType, "ForwardWrpStreamChannelRequestAsync to SF Gateway. Response = {0}", streamResponse.ToString());

            return streamResponse;
        }

        private HttpRequestMessage BuildRequestMessage(StreamRequest streamRequest)
        {
            Uri queryUri = null;
            if (!Uri.TryCreate(streamRequest.ClusterQueryUri, UriKind.Absolute, out queryUri))
            {
                throw new ArgumentException(streamRequest.ClusterQueryUri);
            }
            
            var requestMediaType = streamRequest.MediaType;
            var encoding = string.IsNullOrEmpty(streamRequest.CharSet) ? Encoding.UTF8 : Encoding.GetEncoding(streamRequest.CharSet);

            var request = new HttpRequestMessage(new HttpMethod(streamRequest.Method), queryUri)
            {
                Content = streamRequest.Content == null ? null : new StringContent(streamRequest.Content, encoding, requestMediaType)
            };

            if (streamRequest.Headers != null)
            {
                foreach (var header in streamRequest.Headers)
                {
                    request.Headers.Add(header.Key, header.Value);
                }
            }

            return request;
        }

        private bool CertificateChainValidator<T>(
            T sender,
            X509Certificate2 cert,
            X509Chain chain,
            SslPolicyErrors sslPolicyErrors)
        {
            if (cert == null ||
                sslPolicyErrors == SslPolicyErrors.RemoteCertificateNotAvailable)
            {
                Trace.WriteWarning(
                    TraceType,
                    "Unauthorized request by sender - {0} using null certificate.",
                    sender);

                return false;
            }

            IList<string> serverCertThumbprints = null;
            IList<KeyValuePair<string, string>> x509NamesAndIssuers = null;

            if (TryGetServerCertThumbprints(out serverCertThumbprints)
                && serverCertThumbprints != null
                && serverCertThumbprints.Count > 0)
            {
                var certThumbprint = cert.Thumbprint;
                if (certThumbprint != null &&
                    !serverCertThumbprints.Contains(certThumbprint, StringComparer.OrdinalIgnoreCase))
                {
                    var serverCerts = new StringBuilder();
                    foreach (var serverCert in serverCertThumbprints)
                    {
                        serverCerts.Append(serverCert).Append("|");
                    }

                    Trace.WriteWarning(
                        TraceType,
                        "Unauthorized request by sender - {0} using certificate - {1}- Thumbprint {2} vs expected thumbprint {3}",
                        sender,
                        cert.ToString(),
                        cert.Thumbprint,
                        serverCerts.ToString());

                    return false;
                }
            }
            ////  TODO current SFRP only use thumbprint, so we may ingore this for now
            ////  we should add chain validation 
            else if (TryGetServerX509Names(out x509NamesAndIssuers)
                     && x509NamesAndIssuers != null
                     && x509NamesAndIssuers.Count > 0)
            {
                foreach (var x509NamesKvp in x509NamesAndIssuers)
                {
                    var simpleName = cert.GetNameInfo(X509NameType.SimpleName, false);
                    var dnsAlternativeName = cert.GetNameInfo(X509NameType.DnsFromAlternativeName, false);
                    var dnsName = cert.GetNameInfo(X509NameType.DnsName, false);

                    if (cert.Subject.Equals(x509NamesKvp.Key, StringComparison.OrdinalIgnoreCase) ||
                       (simpleName != null && simpleName.Equals(x509NamesKvp.Key, StringComparison.OrdinalIgnoreCase)) ||
                       (dnsAlternativeName != null && dnsAlternativeName.Equals(x509NamesKvp.Key, StringComparison.OrdinalIgnoreCase)) ||
                       (dnsName != null && dnsName.Equals(x509NamesKvp.Key, StringComparison.OrdinalIgnoreCase)))
                    {
                        return true;
                    }
                }

                var serverCerts = new StringBuilder();
                foreach (var serverCert in x509NamesAndIssuers)
                {
                    serverCerts.Append(string.Concat(serverCert.Key, " ", serverCert.Value)).Append("|");
                }

                Trace.WriteWarning(
                    TraceType,
                    "Unauthorized request by sender - {0} using certificate - {1}- Names {2} vs expected Names {3}",
                    sender,
                    cert.ToString(),
                    cert.SubjectName.Name,
                    serverCerts.ToString());

                return false;

            }

            return true;
        }

#if !DotNetCoreClr

        private bool CertificateChainValidator(
            object sender,
            X509Certificate certificate,
            X509Chain chain,
            SslPolicyErrors sslPolicyErrors)
        {
            if (certificate == null)
            {
                Trace.WriteWarning(TraceType,
                    "Unauthorized request by sender - {0} using null certificate.",
                    sender);
                return false;
            }

            return CertificateChainValidator<object>(sender, new X509Certificate2(certificate), chain, sslPolicyErrors);
        }

#endif

        private async Task<StreamChannelPutResponse> ReturnWrpStreamChannelResponseAsync(
            StreamResponse streamResponse,
            CancellationToken cancellationToken)
        {
            try
            {
                Trace.WriteInfo(TraceType, "WrpStreamChannel: ReturnWrpStreamChannelResponseAsync Start");
                var response = await this.GetWrpServiceResponse(this.streamChannelUri, streamResponse, cancellationToken);
                Trace.WriteInfo(TraceType, "WrpStreamChannel: ReturnWrpStreamChannelResponseAsync End");
                return response;
            }
            catch (Exception e)
            {
                Trace.WriteWarning(TraceType, "ReturnWrpStreamChannelResponse thrown {0}", e);
                throw;
            }
        }

        private void ProcessEachStreamRequestAsync(StreamRequest streamRequest, CancellationToken cancellationToken)
        {
            Trace.WriteInfo(TraceType, "WrpStreamChannel: ProcessEachStreamRequestAsync Start");

            Task.Run(
                () => this.ForwardWrpStreamChannelRequestAsync(streamRequest, cancellationToken), cancellationToken)
                .ContinueWith(
                async (respone) =>
                {
                    StreamResponse streamResponse = null;
                    if (respone.IsFaulted)
                    {
                        Exception ex = respone.Exception;
                        while (ex is AggregateException && ex.InnerException != null)
                        {
                            ex = ex.InnerException;
                        }

                        Trace.WriteWarning(TraceType, "ForwardWrpStreamChannelRequest thrown {0}", ex);
                        if (ex is WebException)
                        {
                            var webException = (WebException)ex;
                            var response = (HttpWebResponse)webException.Response;
                            streamResponse = new StreamResponse()
                            {
                                StatusCode = response.StatusCode,
                                ReasonPhrase = ex.ToString(),
                                RequestId = streamRequest.RequestId
                            };
                        }
                        else
                        {
                            streamResponse = new StreamResponse()
                            {
                                StatusCode = HttpStatusCode.InternalServerError,
                                ReasonPhrase = ex.ToString(),
                                RequestId = streamRequest.RequestId
                            };
                        }
                    }
                    else if (respone.IsCanceled)
                    {
                        Trace.WriteWarning(TraceType, "ForwardWrpStreamChannelRequest has been canceled");
                        streamResponse = new StreamResponse()
                        {
                            StatusCode = HttpStatusCode.InternalServerError,
                            RequestId = streamRequest.RequestId
                        };
                    }
                    else
                    {
                        streamResponse = respone.Result;
                    }

                    await ReturnWrpStreamChannelResponseAsync(streamResponse, cancellationToken);
                },
                cancellationToken);

            Trace.WriteInfo(TraceType, "WrpStreamChannel: ProcessEachStreamRequestAsync end");
        }

        #region Certificate Configuration

        private bool TryGetServerCertThumbprints(out IList<string> serverCertThumbprints)
        {
            var serverThumbprints = this.ConfigStore.ReadUnencryptedString(
                Constants.SecuritySection.SecuritySectionName,
                Constants.SecuritySection.ServerCertThumbprints);

            if (string.IsNullOrWhiteSpace(serverThumbprints))
            {
                serverCertThumbprints = null;
                return false;
            }
            else
            {
                serverCertThumbprints = serverThumbprints.Split(',', ';').Select(s => s.Replace(" ", "")).ToList();
                return true;
            }
        }

        private bool TryGetServerX509Names(out IList<KeyValuePair<string, string>> x509NamesAndIssuers)
        {
            var keys = this.ConfigStore.GetKeys(
                Constants.ServerX509NamesSection.ServerX509NamesSectionName,
                string.Empty);

            if (keys != null)
            {
                x509NamesAndIssuers = new List<KeyValuePair<string, string>>();
                foreach (var key in keys)
                {
                    var value = this.ConfigStore.ReadUnencryptedString(
                        Constants.ServerX509NamesSection.ServerX509NamesSectionName,
                        key);

                    x509NamesAndIssuers.Add(new KeyValuePair<string, string>(key, value));
                }

                return true;
            }
            else
            {
                x509NamesAndIssuers = null;
                return false;
            }
        }

        #endregion

        private async Task TrackOldTask(Task waitTask)
        {
            var maxWaitTime = 3 * this.defaultTimeout.TotalMilliseconds;
            if (waitTask != null)
            {
                Trace.WriteInfo(TraceType, "WrpStreamChannel track old task to exit :{0}", waitTask.Status);

                Task waitTimeoutTask = Task.Delay(TimeSpan.FromMilliseconds(maxWaitTime));
                if (await Task.WhenAny(waitTask, waitTimeoutTask) == waitTimeoutTask)
                {
                    Trace.WriteInfo(TraceType, "WrpStreamChannel the old task status {0} timeout to exit", waitTask.Status);

                    ReleaseAssert.AssertIfNot(
                           false,
                           "It has long running task does not exit for :{0} milliseconds",
                           3 * this.defaultTimeout.TotalMilliseconds);
                }
            }
        }

        private async Task<Stream> GetStreamAsync(HttpClient client, Uri requestUri)
        {
            var response = await client.GetAsync(requestUri, HttpCompletionOption.ResponseHeadersRead).ConfigureAwait(false);
            if (!response.IsSuccessStatusCode)
            {
                if (response.Content != null)
                {
                    throw new StreamChannelException(JsonSerializationHelper.DeserializeObject<ErrorDetails>(await response.Content.ReadAsByteArrayAsync()));
                }
            }

            response.EnsureSuccessStatusCode();
            var c = response.Content;
            return c != null ? await c.ReadAsStreamAsync().ConfigureAwait(false) : Stream.Null;
        }

        #endregion

        #endregion
    }
}