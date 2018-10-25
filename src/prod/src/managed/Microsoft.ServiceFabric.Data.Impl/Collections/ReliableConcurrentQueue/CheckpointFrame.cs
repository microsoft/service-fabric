// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections.ReliableConcurrentQueue
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Threading.Tasks;

    using Microsoft.ServiceFabric.Replicator;

    // exposed only for testing
    internal class CheckpointFrame<T>
    {
        private readonly IStateSerializer<T> valueSerializer;

        private int listElementsBytes;

        private InMemoryBinaryWriter writer;

        public int Size
        {
            get
            {
                return sizeof(int) // listElementsCount
                       + sizeof(int) // listElementsBytes
                       + this.listElementsBytes // listElements
                       + sizeof(ulong); // CRC 
            }
        }

        public CheckpointFrame(IStateSerializer<T> valueSerializer)
        {
            this.valueSerializer = valueSerializer;
            this.writer = new InMemoryBinaryWriter(new MemoryStream());
            this.ListElementsCount = 0;
            this.listElementsBytes = 0;
        }

        public void AddListElement(IListElement<T> listElement)
        {
            var positionBeforeListElements = (int)this.writer.BaseStream.Position;
            this.writer.Write(listElement.Id);
            this.valueSerializer.Write(listElement.Value, this.writer);
            this.listElementsBytes += (int)this.writer.BaseStream.Position - positionBeforeListElements;
            this.ListElementsCount++;
        }

        public int ListElementsCount { get; private set; }

        public void Reset()
        {
            this.ListElementsCount = 0;
            this.listElementsBytes = 0;
            this.writer.BaseStream.Position = 0;
        }

        public async Task WriteAsync(Stream stream)
        {
            // chunk is full, write it to the stream
            var listElementsCountSegment = new ArraySegment<byte>(BitConverter.GetBytes(this.ListElementsCount));
            var listElementsBytesSegment = new ArraySegment<byte>(BitConverter.GetBytes(this.listElementsBytes));

            var crc =
                CRC64.ToCRC64(
                    new[]
                        {
                            listElementsCountSegment, 
                            listElementsBytesSegment, 
                            new ArraySegment<byte>(this.writer.BaseStream.GetBuffer(), 0, (int)this.writer.BaseStream.Position), 
                        });

            // write the crc to the end of the values memorystream to avoid the extra stream write
            this.writer.Write(crc);

            await stream.WriteAsync(listElementsCountSegment.Array, listElementsCountSegment.Offset, listElementsCountSegment.Count).ConfigureAwait(false);
            await stream.WriteAsync(listElementsBytesSegment.Array, listElementsBytesSegment.Offset, listElementsBytesSegment.Count).ConfigureAwait(false);
            await stream.WriteAsync(this.writer.BaseStream.GetBuffer(), 0, (int)this.writer.BaseStream.Position).ConfigureAwait(false);
        }

        public static async Task<IEnumerable<IListElement<T>>> ReadAsync(Stream stream, IStateSerializer<T> stateSerializer, string traceType)
        {
            var listElementsCountSegment = new ArraySegment<byte>(new byte[sizeof(int)]);
            var listElementsBytesSegment = new ArraySegment<byte>(new byte[sizeof(int)]);

            var listElementsCount = await SerializationHelper.ReadIntAsync(listElementsCountSegment, stream).ConfigureAwait(false);
            if (listElementsCount < 0)
            {
                throw new InvalidDataException(string.Format("Unexpected listElementsCount: {0}", listElementsCount));
            }

            var listElementsBytes = await SerializationHelper.ReadIntAsync(listElementsBytesSegment, stream).ConfigureAwait(false);
            if (listElementsBytes < 0)
            {
                throw new InvalidDataException(string.Format("Unexpected listElementsBytes: {0}", listElementsBytes));
            }

            using (var reader = new InMemoryBinaryReader(new MemoryStream()))
            {
                reader.BaseStream.SetLength(listElementsBytes + sizeof(ulong));
                await
                    SerializationHelper.ReadBytesAsync(new ArraySegment<byte>(reader.BaseStream.GetBuffer()), listElementsBytes + sizeof(ulong), stream)
                        .ConfigureAwait(false);

                var listElements = new IListElement<T>[listElementsCount];
                for (var i = 0; i < listElementsCount; i++)
                {
                    var id = reader.ReadInt64();
                    var value = stateSerializer.Read(reader); // if this tries to read beyond the end of the stream, listElementsBytes was incorrect (too small)
                    listElements[i] = DataStore<T>.CreateQueueListElement(id, value, traceType, ListElementState.EnqueueApplied);
                }

                var readCRC = reader.ReadUInt64();
                var calcCRC =
                    CRC64.ToCRC64(new[] { listElementsCountSegment, listElementsBytesSegment, new ArraySegment<byte>(reader.BaseStream.GetBuffer(), 0, listElementsBytes), });
                if (readCRC != calcCRC)
                {
                    throw new InvalidDataException(string.Format("CRC mismatch.  Read: {0} Calculated: {1}", readCRC, calcCRC));
                }

                return listElements;
            }
        }
    }
}