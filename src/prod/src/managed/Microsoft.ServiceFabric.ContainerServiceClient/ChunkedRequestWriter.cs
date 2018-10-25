// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient
{
    using System;
    using System.IO;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    internal class ChunkedRequestWriter : Stream
    {
        private static readonly byte[] endContentBytes = Encoding.ASCII.GetBytes("0\r\n\r\n");

        private Stream clientStream;

        public ChunkedRequestWriter(Stream stream)
        {
            this.clientStream = stream;
        }

        #region Overrides

        public override bool CanRead
        {
            get { return false; }
        }

        public override bool CanSeek
        {
            get { return false; }
        }

        public override bool CanWrite
        {
            get { return false; }
        }

        public override long Length
        {
            get { throw new NotImplementedException(); }
        }

        public override long Position
        {
            get { throw new NotImplementedException(); }
            set { throw new NotImplementedException(); }
        }

        public override void Flush()
        {
            this.clientStream.Flush();
        }

        public override Task FlushAsync(CancellationToken cancellationToken)
        {
            return this.clientStream.FlushAsync(cancellationToken);
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
            throw new NotImplementedException();
        }

        public override long Seek(long offset, SeekOrigin origin)
        {
            throw new NotImplementedException();
        }

        public override void SetLength(long value)
        {
            throw new NotImplementedException();
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
            this.WriteAsync(buffer, offset, count, CancellationToken.None).GetAwaiter().GetResult();
        }

        public override async Task WriteAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
        {
            if (count == 0)
            {
                return;
            }

            var chunkSize = Encoding.ASCII.GetBytes(count.ToString("x") + "\r\n");

            await this.clientStream.WriteAsync(chunkSize, 0, chunkSize.Length, cancellationToken).ConfigureAwait(false);
            await this.clientStream.WriteAsync(buffer, offset, count, cancellationToken).ConfigureAwait(false);
            await this.clientStream.WriteAsync(chunkSize, chunkSize.Length - 2, 2, cancellationToken).ConfigureAwait(false);
        }

        #endregion

        public Task EndContentAsync(CancellationToken cancellationToken)
        {
            return this.clientStream.WriteAsync(endContentBytes, 0, endContentBytes.Length, cancellationToken);
        }
    }
}