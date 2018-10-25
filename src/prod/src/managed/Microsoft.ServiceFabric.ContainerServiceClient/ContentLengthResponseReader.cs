// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient
{
    using System;
    //using System.Fabric.Common;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;

    internal class ContentLengthResponseReader : Stream
    {
        #region Private Members

        private readonly Stream clientStream;
        private long bytesRemaining;
        private bool isDisposed;

        #endregion

        public ContentLengthResponseReader(Stream inner, long contentLength)
        {
            this.clientStream = inner;
            this.bytesRemaining = contentLength;
            this.isDisposed = false;
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
            if (this.isDisposed)
            {
                return 0;
            }

            var toRead = (int)Math.Min(count, bytesRemaining);

            var read = this.clientStream.Read(buffer, offset, toRead);

            this.UpdateBytesRemaining(read);

            return read;
        }

        public async override Task<int> ReadAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
        {
            if (this.isDisposed)
            {
                return 0;
            }

            cancellationToken.ThrowIfCancellationRequested();

            var toRead = (int)Math.Min(count, bytesRemaining);
            var read = await clientStream.ReadAsync(buffer, offset, toRead, cancellationToken);

            this.UpdateBytesRemaining(read);

            return read;
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                //
                // We will need to drain the remaining response bytes
                // to re-use the stream.
                //
                clientStream.Dispose();
            }
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

        private void UpdateBytesRemaining(int read)
        {
            this.bytesRemaining -= read;

            if (this.bytesRemaining <= 0)
            {
                isDisposed = true;
            }

            //ReleaseAssert.AssertIfNot(
            //    this.bytesRemaining >= 0, 
            //    "Negative bytes remaining: {0} ",
            //    bytesRemaining);
        }

        private void ThrowIfDisposed()
        {
            if (this.isDisposed)
            {
                throw new ObjectDisposedException(typeof(ContentLengthResponseReader).FullName);
            }
        }
    }
}