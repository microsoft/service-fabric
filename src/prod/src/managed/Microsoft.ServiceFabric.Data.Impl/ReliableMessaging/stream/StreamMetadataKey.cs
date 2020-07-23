// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    #region using directives

    using System.Collections.Generic;
    using System.Fabric.ReliableMessaging;
    using System.IO;
    using Microsoft.ServiceFabric.Data;

    #endregion

    #region enum StreamMetadataKind

    /// <summary>
    /// This enumerates the metadata kind elements for each kind of messaging streams.
    /// </summary>
    internal enum StreamMetadataKind
    {
        OutboundStableParameters = 101,
        InboundStableParameters = 201,
        ReceiveSequenceNumber = 301,
        SendSequenceNumber = 401,
        DeleteSequenceNumber = 501
    }

    #endregion

    #region StreamMetadataKey

    internal class StreamMetadataKey
    {
        internal Guid StreamId { get; private set; }

        internal StreamMetadataKind MetadataKind { get; private set; }

        /// <summary>
        /// Constructs a Stream Meta data key object
        /// </summary>
        /// <param name="streamId">Unique StreamId</param>
        /// <param name="metadataKind">Kind of Stream MetaData</param>
        internal StreamMetadataKey(Guid streamId, StreamMetadataKind metadataKind)
        {
            this.StreamId = streamId;
            this.MetadataKind = metadataKind;
        }

        /// <summary>
        /// Returns a string that represents the current StreamMetadataKey object.
        /// </summary>
        /// <returns>
        /// A string that represents the current StreamMetadataKey object.
        /// </returns>
        /// <filterpriority>2</filterpriority>
        public override string ToString()
        {
            var result = string.Empty;
            var streamIdString = this.StreamId.ToString();

            switch (this.MetadataKind)
            {
                case StreamMetadataKind.InboundStableParameters:
                    result = "StreamMetadataKind.InboundStableParameters ID::" + streamIdString;
                    break;

                case StreamMetadataKind.OutboundStableParameters:
                    result = "StreamMetadataKind.OutboundStableParameters ID::" + streamIdString;
                    break;

                case StreamMetadataKind.ReceiveSequenceNumber:
                    result = "StreamMetadataKind.ReceiveSequenceNumber ID::" + streamIdString;
                    break;

                case StreamMetadataKind.SendSequenceNumber:
                    result = "StreamMetadataKind.SendSequenceNumber ID::" + streamIdString;
                    break;

                case StreamMetadataKind.DeleteSequenceNumber:
                    result = "StreamMetadataKind.DeleteSequenceNumber ID::" + streamIdString;
                    break;

                default:
                    Diagnostics.Assert(false, "Unknown Stream MetadataKind in StreamMetadataKey.ToString()");
                    break;
            }

            return result;
        }
    }

    #endregion

    #region IComparer and IEquityComparer for StreamMetadataKey

    /// <summary>
    /// Provide overrides for IComparer and IEquityComparer for StreamMetadataKey
    /// </summary>
    internal class StreamMetadataKeyComparer : IComparer<StreamMetadataKey>, IEqualityComparer<StreamMetadataKey>
    {
        /// <summary>
        /// Compares two StreamMetadataKey objects and returns a value indicating whether one is less than, equal to, or greater than the other.
        /// </summary>
        /// <returns>
        /// A signed integer that indicates the relative values of <paramref name="x"/> and <paramref name="y"/>, as shown in the following table.
        /// Value Meaning Less than zero<paramref name="x"/> is less than <paramref name="y"/>.
        /// Zero<paramref name="x"/> equals <paramref name="y"/>.
        /// Greater than zero<paramref name="x"/> is greater than <paramref name="y"/>.
        /// </returns>
        /// <param name="x">The first StreamMetadataKey object to compare.</param>
        /// <param name="y">The second StreamMetadataKey object to compare.</param>
        public int Compare(StreamMetadataKey x, StreamMetadataKey y)
        {
            // Comparison when x == null.
            if (null == x)
            {
                return null == y ? 0 : 1;
            }

            // Comparision when y == null.
            if (null == y)
            {
                return -1;
            }

            // x !null and y !null
            var kindCompare = x.MetadataKind.CompareTo(y.MetadataKind);

            // First check if they are same kind and then check for Stream Id.
            return 0 == kindCompare ? x.StreamId.CompareTo(y.StreamId) : kindCompare;
        }

        /// <summary>
        /// Determines whether the specified StreamMetadataKey objects are equal.
        /// </summary>
        /// <returns>
        /// true if the specified StreamMetadataKey objects are equal; otherwise, false.
        /// </returns>
        /// <param name="x">The first object of type <cref name="StreamMetadataKey"/> to compare.</param>
        /// <param name="y">The second object of type <cref name="StreamMetadataKey"/> to compare.</param>
        public bool Equals(StreamMetadataKey x, StreamMetadataKey y)
        {
            return 0 == this.Compare(x, y);
        }

        /// <summary>
        /// Returns a hash code for the specified StreamMetadataKey object.
        /// </summary>
        /// <returns>
        /// A hash code for the specified StreamMetadataKey object.
        /// </returns>
        /// <param name="obj">The <see cref="StreamMetadataKey"/> for which a hash code is to be returned.</param>
        /// <exception cref="System.ArgumentNullException">The type of <paramref name="obj"/> is a reference type and <paramref name="obj"/> is null.</exception>
        public int GetHashCode(StreamMetadataKey obj)
        {
            return obj.MetadataKind.GetHashCode() ^ obj.StreamId.GetHashCode();
        }
    }

    #endregion

    #region IByteConverter for StreamMetadataKey

    /// <summary>
    /// Provides overrides for IByteConverter.
    /// Converts the StreamMetadataKey from byte array and vice versa. 
    /// </summary>
    internal class StreamMetadataKeyConverter : IStateSerializer<StreamMetadataKey>
    {
        public static StreamMetadataKeyConverter Converter = new StreamMetadataKeyConverter();

        /// <summary>
        /// Converts the StreamMetadataKey to byte array
        /// </summary>
        /// <param name="value">StreamMetadataKey object</param>
        /// <param name="binaryWriter"></param>
        /// <returns>Byte array</returns>
        public void Write(StreamMetadataKey value, BinaryWriter binaryWriter)
        {
            binaryWriter.Write(value.StreamId.ToByteArray());
            binaryWriter.Write((int) value.MetadataKind);
        }

        /// <summary>
        /// Assembles the StreamMetadataKey object from byte array
        /// </summary>
        /// <param name="reader">BinaryReader to deserialize from</param>
        /// <returns>StreamMetadataKey object</returns>
        public StreamMetadataKey Read(BinaryReader reader)
        {
            return new StreamMetadataKey(new Guid(reader.ReadBytes(16)), (StreamMetadataKind) reader.ReadInt32());
        }

        public StreamMetadataKey Read(StreamMetadataKey baseValue, BinaryReader binaryReader)
        {
            return this.Read(binaryReader);
        }

        public void Write(StreamMetadataKey baseValue, StreamMetadataKey targetValue, BinaryWriter binaryWriter)
        {
            this.Write(targetValue, binaryWriter);
        }
    }

    #endregion

    #region IRangePartitioner for StreamMetadataKey

    /// <summary>
    /// Provides overrides for IRangePartitioner.
    /// </summary>
    internal class StreamMetadataKeyPartitioner : Store.IRangePartitioner<StreamMetadataKey>
    {
        public IEnumerable<StreamMetadataKey> Partitions
        {
            get { return null; }
        }
    }

    #endregion
}