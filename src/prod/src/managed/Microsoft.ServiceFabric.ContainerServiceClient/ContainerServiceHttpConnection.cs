// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Globalization;
    using System.Linq;
    using System.Net;
    using System.Net.Http;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    class ContainerServiceHttpConnection : IDisposable
    {
        private const string TraceType = "ContainerServiceHttpConnection";

        private readonly ContainerServiceHttpMessageHandler messageHandler;
        private readonly HttpRequestMessage request;
        private readonly object disposeLock;

        private BufferedClientStream clientStream;

        public ContainerServiceHttpConnection(
            ContainerServiceHttpMessageHandler messageHandler,
            HttpRequestMessage request)
        {
            this.messageHandler = messageHandler;
            this.request = request;
            this.disposeLock = new object();
        }

        public async Task<HttpResponseMessage> SendAsync(CancellationToken cancellationToken)
        {
            HttpResponseMessage response = null;

            try
            {
                var timeoutCts = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
                var timeoutTask = Task.Delay(Timeout.InfiniteTimeSpan, timeoutCts.Token);

                var actualTask = Task.Run(() => this.SendPrivateAsync(cancellationToken));

                var finishedTask = await Task.WhenAny(actualTask, timeoutTask);

                if(finishedTask == actualTask)
                {
                    timeoutCts.Cancel();
                    this.ObserveExceptionIfAny(timeoutTask, false);

                    response = await actualTask;
                }
                else
                {
                    //
                    // The operation has timed out. The request to docker should ideally get
                    // cancelled. But depending on how cancellationtoken is being handled
                    // in .Net stream apis, it may get stuck waiting for docker response.
                    // Await the timeout task to bubble the exception. It will dispose the 
                    // client stream and force fault the connection.
                    //
                    this.ObserveExceptionIfAny(actualTask, true);

                    await timeoutTask;
                }
            }
            catch (Exception)
            {
                // Any errors at this layer abort the connection.
                this.Dispose();

                throw;
            }

            return response;
        }

        private async Task<HttpResponseMessage> SendPrivateAsync(CancellationToken cancellationToken)
        {
            this.clientStream = await this.messageHandler.AcquireClientStreamAsync(cancellationToken);

            await this.SendRequestHeaderAsync(cancellationToken);
            await this.SendRequestContentAsync(cancellationToken);

            // Receive headers
            var responseLines = await this.ReadResponseLinesAsync(cancellationToken);

            // Determine response type (Chunked, Content-Length, opaque, none...)
            // Receive body
            return this.CreateResponseMessage(responseLines);
        }

        private Task SendRequestHeaderAsync(
            CancellationToken cancellationToken)
        {
            var requestBytes = SerializeRequest(this.request);
            return this.clientStream.WriteAsync(requestBytes, 0, requestBytes.Length, cancellationToken);
        }

        private async Task SendRequestContentAsync(CancellationToken cancellationToken)
        {
            if (this.request.Content == null)
            {
                return;
            }

            if (this.request.Content.Headers.ContentLength.HasValue)
            {
                await this.request.Content.CopyToAsync(this.clientStream);
            }
            else
            {
                //
                // The length of the data is unknown. Send it in chunked mode.
                //
                using (var chunkedWriter = new ChunkedRequestWriter(this.clientStream))
                {
                    await this.request.Content.CopyToAsync(chunkedWriter);
                    await chunkedWriter.EndContentAsync(cancellationToken);
                }
            }
        }

        private static byte[] SerializeRequest(HttpRequestMessage request)
        {
            var builder = new StringBuilder();
            builder.Append(request.Method);
            builder.Append(' ');
            builder.Append(request.GetAddressLineProperty());
            builder.Append(" HTTP/");
            builder.Append(request.Version.ToString(2));
            builder.Append("\r\n");

            builder.Append(request.Headers.ToString());

            if (request.Content != null)
            {
                // Force the content to compute its content length if it has not already.
                var contentLength = request.Content.Headers.ContentLength;
                if (contentLength.HasValue)
                {
                    request.Content.Headers.ContentLength = contentLength.Value;
                }

                builder.Append(request.Content.Headers.ToString());
                if (!contentLength.HasValue)
                {
                    // Add header for chunked mode.
                    builder.Append("Transfer-Encoding: chunked\r\n");
                }
            }

            // Headers end with an empty line
            builder.Append("\r\n");

            var requestStr = builder.ToString();

            AppTrace.TraceSource.WriteNoise(
                TraceType,
                "HttpRequestMessage:{0}[{1}]",
                Environment.NewLine,
                requestStr);

            return Encoding.ASCII.GetBytes(requestStr);
        }

        private async Task<List<string>> ReadResponseLinesAsync(CancellationToken cancellationToken)
        {
            var lines = new List<string>();

            var line = await this.clientStream.ReadLineAsync(cancellationToken);

            while (line.Length > 0)
            {
                lines.Add(line);
                line = await this.clientStream.ReadLineAsync(cancellationToken);
            }

            return lines;
        }

        private HttpResponseMessage CreateResponseMessage(List<string> responseLines)
        {
            var responseLine = responseLines.First();
            
            // HTTP/1.1 200 OK
            var responseLineParts = responseLine.Split(new[] { ' ' }, 3);
            
            // TODO: Verify HTTP/1.0 or 1.1.
            if (responseLineParts.Length < 2)
            {
                TraceAndThrow(string.Format("Invalid response line: {0}.", responseLine));
            }

            var statusCode = 0;
            if (int.TryParse(responseLineParts[1], NumberStyles.None, CultureInfo.InvariantCulture, out statusCode))
            {
                // TODO: Validate range
            }
            else
            {
                TraceAndThrow(string.Format("Invalid StatusCode: {0}.", responseLineParts[1]));
            }

            var response = new HttpResponseMessage((HttpStatusCode)statusCode);
            if (responseLineParts.Length >= 3)
            {
                response.ReasonPhrase = responseLineParts[2];
            }

            var content = new ContainerServiceHttpResponseContent(this.messageHandler, this.clientStream);
            response.Content = content;

            foreach (var rawHeader in responseLines.Skip(1))
            {
                var colonOffset = rawHeader.IndexOf(':');
                if (colonOffset <= 0)
                {
                    TraceAndThrow(string.Format("The given header line format is invalid: {0}.", rawHeader));
                }

                var headerName = rawHeader.Substring(0, colonOffset);
                var headerValue = rawHeader.Substring(colonOffset + 2);

                if (!response.Headers.TryAddWithoutValidation(headerName, headerValue))
                {
                    var success = response.Content.Headers.TryAddWithoutValidation(headerName, headerValue);

                    ReleaseAssert.AssertIfNot(success, $"Failed to add response header: {rawHeader}");
                }
            }

            // After headers have been set
            var isChunked = false;
            if (response.Headers.TransferEncodingChunked.HasValue)
            {
                isChunked = response.Headers.TransferEncodingChunked.Value;
            }

            content.ResolveResponseStream(isChunked);

            return response;
        }

        private static void TraceAndThrow(string errMsg, Exception innerEx = null)
        {
            AppTrace.TraceSource.WriteError(TraceType, errMsg);
            throw new HttpRequestException(errMsg, innerEx);
        }

        private void ObserveExceptionIfAny(Task tsk, bool shouldDispose)
        {
            Task.Run(async () =>
            {
                try
                {
                    await tsk.ConfigureAwait(false);
                }
                catch
                {
                    // ignored
                }

                //
                // Dispose client stream at the end
                // to ensure there is no leak.
                //
                if(shouldDispose)
                {
                    this.Dispose();
                }
            });
        }

        public void Dispose()
        {
            Dispose(true);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                lock(this.disposeLock)
                {
                    this.messageHandler.ReleaseClientStream(this.clientStream);
                    this.clientStream = null;
                }
            }
        }
    }
}