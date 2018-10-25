// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.IO;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// BinaryWriter which explicitly only supports and exposes MemoryStream, and which is extended to serialize additional common
    /// data types and support useful behavior (e.g. writing asynchronously to another stream, and computing the checksum of all
    /// written data)
    /// </summary>
    internal class InMemoryBinaryWriter : BinaryWriter
    {
        public InMemoryBinaryWriter(MemoryStream output)
            : base(output)
        {
        }

        public InMemoryBinaryWriter(MemoryStream output, bool leaveOpen)
            : base(output, new UTF8Encoding(false, true), leaveOpen)
        {
        }

        public new MemoryStream BaseStream
        {
            get { return (MemoryStream) base.BaseStream; }
        }

        public ulong GetChecksum()
        {
            return CRC64.ToCRC64(this.BaseStream.GetBuffer(), 0, checked((int) this.BaseStream.Position));
        }

        public void Write(Guid value)
        {
            if (value == null)
            {
                throw new ArgumentNullException("value");
            }

            var guidBytes = value.ToByteArray();
            Utility.Assert(
                guidBytes.Length == StateManagerConstants.GuidSizeInBytes,
                "InMemoryBinaryWriter guid length is not equal to const");
            this.Write(guidBytes);
        }

        public void Write(Type value)
        {
            if (value == null)
            {
                throw new ArgumentNullException("value");
            }

            this.Write(value.AssemblyQualifiedName);
        }

        public void Write(Uri value)
        {
            if (value == null)
            {
                throw new ArgumentNullException("value");
            }

            this.Write(value.OriginalString);
        }

        public void WriteNullable(byte[] value)
        {
            if (value == null)
            {
                this.Write(-1);
            }
            else
            {
                this.Write(value.Length);
                this.Write(value);
            }
        }

        public void WritePaddingUntilAligned(int desiredAlignment = Constants.X64Alignment)
        {
            ByteAlignedReadWriteHelper.WritePaddingUntilAligned(this, desiredAlignment);
        }

        /// <summary>
        /// Writes contents of the buffer onto the output stream.
        /// </summary>
        /// <param name="outputStream"></param>
        /// <returns></returns>
        public Task WriteAsync(Stream outputStream)
        {
            return outputStream.WriteAsync(this.BaseStream.GetBuffer(), 0, checked((int) this.BaseStream.Position));
        }

        /// <summary>
        /// Checksums the entire buffer.
        /// </summary>
        public ulong GetChecksum(int startOffset, int count)
        {
            return CRC64.ToCRC64(this.BaseStream.GetBuffer(), startOffset, count);
        }

        public bool IsAligned(int desiredAlignment = Constants.X64Alignment)
        {
            return ByteAlignedReadWriteHelper.IsAligned(this.BaseStream, desiredAlignment);
        }

        public void AssertIfNotAligned(int desiredAlignment = Constants.X64Alignment)
        {
            ByteAlignedReadWriteHelper.AssertIfNotAligned(this.BaseStream, desiredAlignment);
        }
    }
}