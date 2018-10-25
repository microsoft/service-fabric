// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient
{
    using System;
    using System.Fabric.Common;
    using System.Globalization;
    using System.Net.Http;
    using System.Threading;
    using System.Threading.Tasks;

    internal abstract class ContainerServiceHttpMessageHandler : HttpMessageHandler
    {
        private const string TraceType = "ContainerServiceHttpMessageHandler";

        protected override Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();

            ProcessUrl(request);
            ProcessHostHeader(request);

            request.Headers.ConnectionClose = true;

            var connection = new ContainerServiceHttpConnection(this, request);
            return connection.SendAsync(cancellationToken);
        }

        internal abstract Task<BufferedClientStream> AcquireClientStreamAsync(CancellationToken cancellationToken);

        internal abstract void ReleaseClientStream(BufferedClientStream clientStream);

        protected void ProcessUrl(HttpRequestMessage request)
        {
            var scheme = request.GetSchemeProperty();
            if (string.IsNullOrWhiteSpace(scheme))
            {
                this.EnsureAbsoluteUri(request);

                scheme = request.RequestUri.Scheme;
                request.SetSchemeProperty(scheme);
            }

            var host = request.GetHostProperty();
            if (string.IsNullOrWhiteSpace(host))
            {
                this.EnsureAbsoluteUri(request);

                host = request.RequestUri.DnsSafeHost;
                request.SetHostProperty(host);
            }

            var connectionHost = request.GetConnectionHostProperty();
            if (string.IsNullOrWhiteSpace(connectionHost))
            {
                request.SetConnectionHostProperty(host);
            }

            var port = request.GetPortProperty();
            if (!port.HasValue)
            {
                this.EnsureAbsoluteUri(request);

                port = request.RequestUri.Port;
                request.SetPortProperty(port);
            }

            var connectionPort = request.GetConnectionPortProperty();
            if (!connectionPort.HasValue)
            {
                request.SetConnectionPortProperty(port);
            }

            var pathAndQuery = request.GetPathAndQueryProperty();
            if (string.IsNullOrWhiteSpace(pathAndQuery))
            {
                pathAndQuery = request.RequestUri.IsAbsoluteUri ?
                    request.RequestUri.PathAndQuery :
                    Uri.EscapeUriString(request.RequestUri.ToString());

                request.SetPathAndQueryProperty(pathAndQuery);
            }

            var addressLine = request.GetAddressLineProperty();
            if (string.IsNullOrEmpty(addressLine))
            {
                request.SetAddressLineProperty(pathAndQuery);
            }
        }

        protected void ProcessHostHeader(HttpRequestMessage request)
        {
            if (string.IsNullOrWhiteSpace(request.Headers.Host))
            {
                var host = request.GetHostProperty();
                var port = request.GetPortProperty().Value;

                request.Headers.Host = (host + ":" + port.ToString(CultureInfo.InvariantCulture));
            }
        }

        private void EnsureAbsoluteUri(HttpRequestMessage request)
        {
            if (!request.RequestUri.IsAbsoluteUri)
            {
                var errMsg = string.Format("ProcessUrl: Missing URL Scheme: {0}", request.RequestUri);

                AppTrace.TraceSource.WriteWarning(TraceType, errMsg);

                throw new InvalidOperationException(errMsg);
            }
        }

    }
}