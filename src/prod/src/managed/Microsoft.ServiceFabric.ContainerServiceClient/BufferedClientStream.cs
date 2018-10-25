// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient
{
    using System;
    using System.Fabric.Common;
    using System.IO;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    internal class BufferedClientStream : Stream
    {
        #region Private Members

        private const string TraceType = "BufferedClientStream";

        private const char CR = '\r';
        private const char LF = '\n';
        private const int BufferSize = 1024;

        private readonly byte[] bytebuffer;
        private readonly Stream baseStream;

        private int bufferReadOffset;
        private int bufferedByteCount;
        private bool isDisposed;

        #endregion

        public BufferedClientStream(Stream baseStream)
        {
            this.baseStream = baseStream;
            this.bytebuffer = new byte[BufferSize];
            this.bufferReadOffset = 0;
            this.bufferedByteCount = 0;
            this.isDisposed = false;
        }

        #region Overrides

        public override bool CanRead
        {
            get
            {
                return (this.baseStream.CanRead || bufferedByteCount > 0);
            }
        }

        public override bool CanSeek
        {
            get { return false; }
        }

        public override bool CanTimeout
        {
            get { return this.baseStream.CanTimeout; }
        }

        public override bool CanWrite
        {
            get { return this.baseStream.CanWrite; }
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

        public override long Seek(long offset, SeekOrigin origin)
        {
            throw new NotSupportedException();
        }

        public override void SetLength(long value)
        {
            throw new NotSupportedException();
        }

        protected override void Dispose(bool disposing)
        {
            if (!this.isDisposed)
            {
                if (disposing)
                {
                    this.baseStream.Dispose();
                }

                this.isDisposed = true;
            }
        }

        public override void Flush()
        {
            this.baseStream.Flush();
        }

        public override Task FlushAsync(CancellationToken cancellationToken)
        {
            return this.baseStream.FlushAsync(cancellationToken);
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
            this.baseStream.Write(buffer, offset, count);
        }

        public override Task WriteAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
        {
            return this.baseStream.WriteAsync(buffer, offset, count, cancellationToken);
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
            int read = ReadBuffer(buffer, offset, count);
            if (read > 0)
            {
                return read;
            }

            return this.baseStream.Read(buffer, offset, count);
        }

        public override Task<int> ReadAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
        {
            int read = ReadBuffer(buffer, offset, count);
            if (read > 0)
            {
                return Task.FromResult(read);
            }

            return this.baseStream.ReadAsync(buffer, offset, count, cancellationToken);
        }

        #endregion

        public async Task<string> ReadLineAsync(CancellationToken cancellationToken)
        {
            ThrowIfDisposed();

            var foundCR = false;
            var foundLF = false;
            var foundCRLF = false;

            var builder = new StringBuilder();

            do
            {
                if (this.bufferedByteCount == 0)
                {
                    await EnsureBufferedAsync(cancellationToken).ConfigureAwait(false);

                    if (this.bufferedByteCount == 0)
                    {
                        AppTrace.TraceSource.WriteError(
                            TraceType,
                            "ReadLineAsync: Unexpected end of stream.");

                        throw new IOException("ReadLineAsync: Unexpected end of stream");
                    }
                }

                //
                // Docker stream is UTF-8 encoded.
                // TODO: Use encoding to convert from byte to char
                //
                char ch = (char)this.bytebuffer[this.bufferReadOffset];

                this.bufferReadOffset++;
                this.bufferedByteCount--;

                if (ch == CR)
                {
                    foundCR = true;
                }
                else if (ch == LF)
                {
                    if(foundCR)
                    {
                        foundCRLF = true;
                    }
                    else
                    {
                        foundLF = true;
                    }
                }

                if(ch != CR && ch != LF)
                {
                    builder.Append(ch);
                }
            }
            while (!foundCRLF && !foundLF);

            return builder.ToString();
        }

        #region Private Helpers

        private int ReadBuffer(byte[] buffer, int offset, int count)
        {
            if (this.bufferedByteCount > 0)
            {
                int toCopy = Math.Min(this.bufferedByteCount, count);
                Buffer.BlockCopy(this.bytebuffer, this.bufferReadOffset, buffer, offset, toCopy);

                this.bufferReadOffset += toCopy;
                this.bufferedByteCount -= toCopy;

                return toCopy;
            }

            return 0;
        }

        private async Task EnsureBufferedAsync(CancellationToken cancellationToken)
        {
            if (this.bufferedByteCount == 0)
            {
                this.bufferReadOffset = 0;

                this.bufferedByteCount = await this.baseStream.ReadAsync(
                    this.bytebuffer,
                    this.bufferReadOffset,
                    this.bytebuffer.Length,
                    cancellationToken)
                    .ConfigureAwait(false);
            }
        }

        private void ThrowIfDisposed()
        {
            if (this.isDisposed)
            {
                throw new ObjectDisposedException(typeof(BufferedClientStream).FullName);
            }
        }

        #endregion
    }
}