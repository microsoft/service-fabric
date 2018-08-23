// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient
{
    using System.IO;
    using System.Net.Http;
    using System.Threading.Tasks;

    internal class ContainerServiceHttpResponseContent : HttpContent
    {
        private readonly ContainerServiceHttpMessageHandler messageHandler;
        private readonly BufferedClientStream clientStream;

        private Stream responseReader;

        internal ContainerServiceHttpResponseContent(
            ContainerServiceHttpMessageHandler messageHandler,
            BufferedClientStream clientStream)
        {
            this.clientStream = clientStream;
            this.messageHandler = messageHandler;
        }

        internal void ResolveResponseStream(bool isChunked)
        {
            if (isChunked)
            {
                this.responseReader = new ChunkedResponseReader(this.clientStream);
            }
            else if (Headers.ContentLength.HasValue)
            {
                this.responseReader = new ContentLengthResponseReader(this.clientStream, Headers.ContentLength.Value);
            }
            else
            {
                this.responseReader = this.clientStream;
            }
        }

        protected override Task SerializeToStreamAsync(Stream stream, System.Net.TransportContext context)
        {
            return this.responseReader.CopyToAsync(stream);
        }

        protected override Task<Stream> CreateContentReadStreamAsync()
        {
            return Task.FromResult(this.responseReader);
        }

        protected override bool TryComputeLength(out long length)
        {
            length = 0;
            return false;
        }

        protected override void Dispose(bool disposing)
        {
            try
            {
                if (disposing)
                {
                    if(this.responseReader != null)
                    {
                        this.responseReader.Dispose();
                        this.responseReader = null;
                    }

                    this.messageHandler.ReleaseClientStream(this.clientStream);
                }
            }
            finally
            {
                base.Dispose(disposing);
            }
        }
    }
}