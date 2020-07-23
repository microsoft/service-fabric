// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.V2
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.IO;
    using System.Runtime.InteropServices;
   
    internal class NativeMessageStream  : Stream
    {
        private List<Tuple<uint, IntPtr>> bufferList;
        private int length;
        private int currentBufferIndex;
        private int currentBufferReadOffset;
        private bool disposedValue = false;
        private int position;
        private int currentBufferLength;
        private IntPtr currentChunk;

        /// <summary>
        /// Instantiate NativeMessageStream class
        /// </summary>
        /// <param name="bufferList"></param>
        public NativeMessageStream(List<Tuple<uint, IntPtr>> bufferList)
        {
            this.bufferList = bufferList;
            this.length = 0;
            this.SetLength();
            this.Initialize();
        }

        private void Initialize()
        {
            this.currentBufferIndex = 0;
            this.currentBufferReadOffset = 0;
            this.position = 0;

            //For Empty Message
            if (this.length > 0)
            {
                this.currentBufferLength = (int) this.bufferList[this.currentBufferIndex].Item1;
                this.currentChunk = this.bufferList[this.currentBufferIndex].Item2;
            }
            else
            {
                this.currentBufferLength = 0;
                this.currentChunk = IntPtr.Zero;;
            }
		}
    


        public override bool CanRead
        {
            get { return true; }
        }

        public override bool CanSeek
        {
            get { return true; }
        }

        public override bool CanWrite
        {
            get { return false; }
        }

        public override long Length
        {
            get { return this.length; }
        }

        public override long Position
        {
            get
            {
               return this.position;
            }

            set { this.position = (int) value; }
        }


        public override void Flush()
        {
            //no-op
        }

        public override long Seek(long offset, SeekOrigin origin)
        {
            switch (origin)
            {
                case SeekOrigin.Begin:
                    this.Initialize();
                    return this.Position;

            }
            throw new NotImplementedException();

        }

        public override void SetLength(long value)
        {
            throw new NotImplementedException();
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
            if (buffer == null)
                throw new ArgumentNullException();

            if (offset + count > buffer.Length)
                throw new ArgumentOutOfRangeException();

            if (offset < 0 || count < 0)
                throw new ArgumentOutOfRangeException();

            var bytesToRead = count;
            //Number of bytes to read is more than number of unread bytes in buffer
            if (this.length - this.position < bytesToRead)
            {
                bytesToRead = Convert.ToInt32(this.Length - this.position);
            }

            var bytesRead = 0;
            while (bytesToRead > 0)
            {
                //check if we have read the current Buffer
                if (this.currentBufferReadOffset == this.currentBufferLength)
                {
                    // exit if no more buffer are currently available
                    if (this.currentBufferIndex + 1 == this.bufferList.Count)
                    {
                        break;
                    }

                    this.currentBufferIndex++;
                    this.currentBufferReadOffset = 0;
                    this.currentBufferLength = (int)this.bufferList[this.currentBufferIndex].Item1;
                    this.currentChunk = this.bufferList[this.currentBufferIndex].Item2;
                }

                //Read the remaining bytes for the current buffer.
                var readCount = Math.Min(bytesToRead, this.currentBufferLength - this.currentBufferReadOffset);

                Marshal.Copy(this.currentChunk + sizeof(byte) * this.currentBufferReadOffset, buffer, offset, readCount);

                offset += readCount;
                bytesToRead -= readCount;
                bytesRead += readCount;
                this.position += readCount;
                this.currentBufferReadOffset += readCount;
            }

            return bytesRead;
        }


        public override int ReadByte()
        {
            //Number of bytes to read is more than number of unread bytes in buffer

            if (this.length - this.position < 1)
            {
                return -1;
            }
            
			//Read from next buffer
            if (this.currentBufferReadOffset == this.currentBufferLength)
            {

                this.currentBufferReadOffset = 0;
                this.currentBufferIndex++;
                this.currentBufferLength = (int)this.bufferList[this.currentBufferIndex].Item1;
                this.currentChunk = this.bufferList[this.currentBufferIndex].Item2;
            }

            var byteread = Marshal.ReadByte(this.currentChunk + sizeof(byte) * this.currentBufferReadOffset);
            this.position++;
            this.currentBufferReadOffset++;

            return byteread;
        }


        public override void Write(byte[] buffer, int offset, int count)
        {
            throw new NotImplementedException();
        }

        private void SetLength()
        {
            for(int i =0;i < this.bufferList.Count;i++)
            {
                this.length += (int)this.bufferList[i].Item1;
            }
        }

        protected override void Dispose(bool disposing)
        {
            base.Dispose(disposing);
            if (!this.disposedValue)
            {
                //if true ,release  managed resources
                if (disposing)
                {
                    this.bufferList.Clear();
                }
                
                this.disposedValue = true;
            }
        }
        
    }
}