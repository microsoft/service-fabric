// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient
{
    using System;
    using System.Fabric.Common;
    using System.Globalization;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;

    internal class ChunkedResponseReader : Stream
    {
        #region Private Members

        private const string TraceType = "ChunkedResponseStream";

        private readonly BufferedClientStream clientStream;

        private long chunkBytesRemaining;
        private bool isDisposed;
        private bool isEndOfResponse;

        #endregion

        public ChunkedResponseReader(BufferedClientStream clientStream)
        {
            this.clientStream = clientStream;
            this.chunkBytesRemaining = 0;
            this.isDisposed = false;
            this.isEndOfResponse = false;
        }

        #region Overrides

        public override bool CanRead
        {
            get { return !this.isDisposed; }
        }

        public override bool CanSeek
        {
            get { return false; }
        }

        public override bool CanTimeout
        {
            get { return this.clientStream.CanTimeout; }
        }

        public override bool CanWrite
        {
            get { return false; }
        }

        public override long Length
        {
            get { throw new NotSupportedException(); }
        }

        public override long Position
        {
            get { throw new NotSupportedException(); }
            set { throw new NotSupportedException(); }
        }

        public override int ReadTimeout
        {
            get
            {
                ThrowIfDisposed();
                return this.clientStream.ReadTimeout;
            }
            set
            {
                ThrowIfDisposed();
                this.clientStream.ReadTimeout = value;
            }
        }

        public override int WriteTimeout
        {
            get
            {
                ThrowIfDisposed();
                return this.clientStream.WriteTimeout;
            }
            set
            {
                ThrowIfDisposed();
                this.clientStream.WriteTimeout = value;
            }
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
            return ReadAsync(buffer, offset, count, CancellationToken.None).GetAwaiter().GetResult();
        }

        public async override Task<int> ReadAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
        {
            ThrowIfDisposed();

            if (this.isEndOfResponse)
            {
                return 0;
            }

            cancellationToken.ThrowIfCancellationRequested();

            if (this.chunkBytesRemaining == 0)
            {
                string headerLine = await this.clientStream.ReadLineAsync(cancellationToken).ConfigureAwait(false);

                if (!long.TryParse(headerLine, NumberStyles.HexNumber, CultureInfo.InvariantCulture, out this.chunkBytesRemaining))
                {
                    AppTrace.TraceSource.WriteError(
                        TraceType,
                        "ReadAsync: Invalid chunk header: {0}.",
                        headerLine);

                    throw new IOException("ReadAsync: Invalid chunk header: " + headerLine);
                }
            }

            int read = 0;
            if (this.chunkBytesRemaining > 0)
            {
                int toRead = (int)Math.Min(count, this.chunkBytesRemaining);

                read = await this.clientStream.ReadAsync(buffer, offset, toRead, cancellationToken).ConfigureAwait(false);
                if (read == 0)
                {
                    throw new EndOfStreamException();
                }

                this.chunkBytesRemaining -= read;
            }

            if (this.chunkBytesRemaining == 0)
            {
                // End of chunk, read the terminator CRLF
                var trailer = await this.clientStream.ReadLineAsync(cancellationToken).ConfigureAwait(false);
                if (trailer.Length > 0)
                {
                    AppTrace.TraceSource.WriteError(
                        TraceType,
                        "ReadAsync: Invalid chunk trailer.");

                    throw new IOException("Invalid chunk trailer");
                }

                if (read == 0)
                {
                    this.isEndOfResponse = true;
                }
            }

            return read;
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                //
                // If response byte are still remaining, we would need to drain the
                // stream to resue it or we can discard the stream.
                //
                this.clientStream.Dispose();
            }

            this.isDisposed = true;
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
            throw new NotSupportedException();
        }

        public override Task WriteAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public override long Seek(long offset, SeekOrigin origin)
        {
            throw new NotSupportedException();
        }

        public override void SetLength(long value)
        {
            throw new NotSupportedException();
        }

        public override void Flush()
        {
            throw new NotSupportedException();
        }

        #endregion

        private void ThrowIfDisposed()
        {
            if (this.isDisposed)
            {
                throw new ObjectDisposedException(typeof(ChunkedResponseReader).FullName);
            }
        }
    }
}