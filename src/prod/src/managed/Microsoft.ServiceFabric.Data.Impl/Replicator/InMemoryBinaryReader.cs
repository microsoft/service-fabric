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
    /// BinaryReader which explicitly only supports and exposes MemoryStream, and which is extended to deserialize additional common
    /// data types and support useful behavior (e.g. safely reading asynchronously from another stream, and computing the checksum
    /// of all read data)
    /// </summary>
    internal class InMemoryBinaryReader : BinaryReader
    {
        public InMemoryBinaryReader(MemoryStream input)
            : base(input)
        {
        }

        public InMemoryBinaryReader(MemoryStream input, bool leaveOpen)
            : base(input, new UTF8Encoding(), leaveOpen)
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

        public Guid ReadGuid()
        {
            return new Guid(this.ReadBytes(StateManagerConstants.GuidSizeInBytes));
        }

        public byte[] ReadNullableBytes()
        {
            var length = this.ReadInt32();
            return length == -1 ? null : this.ReadBytes(length);
        }

        public Type ReadType(out string typeName)
        {
            typeName = this.ReadString();
            return Utility.GetSerializedType(typeName);
        }

        public Uri ReadUri()
        {
            return new Uri(this.ReadString());
        }

        public Task<int> SafeReadFromAsync(Stream inputStream, int count)
        {
            this.BaseStream.Position = 0;
            this.BaseStream.SetLength(count);
            return inputStream.ReadAsync(this.BaseStream.GetBuffer(), 0, count);
        }

        public void ReadPaddingUntilAligned(bool isReserved, int desiredAlignment = Constants.X64Alignment)
        {
            ByteAlignedReadWriteHelper.ReadPaddingUntilAligned(this, isReserved, desiredAlignment);
        }

        public bool IsAligned(int desiredAlignment = Constants.X64Alignment)
        {
            return ByteAlignedReadWriteHelper.IsAligned(this.BaseStream, desiredAlignment);
        }

        public void ThrowIfNotAligned(int desiredAlignment = Constants.X64Alignment)
        {
            ByteAlignedReadWriteHelper.ThrowIfNotAligned(this.BaseStream, desiredAlignment);
        }
    }
}