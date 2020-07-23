// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    #region using directives

    using System.Collections.Generic;
    using System.Fabric.Store;
    using System.IO;
    using Microsoft.ServiceFabric.Data;

    #endregion

    #region StreamStoreMessageKey

    /// <summary>
    /// Represents the Stream store message key info.
    /// This key is used for both inbound and outbound message stores
    /// 
    /// </summary>
    internal class StreamStoreMessageKey
    {
        #region properties

        // The message stream id
        internal Guid StreamId { get; private set; }

        // For inbound messages the sequence number represents the stream sequence number assigned in commit order
        // For outbound messages it represents the send sequence number assigned in order of SendAsync invocations
        internal long StreamSequenceNumber { get; private set; }

        #endregion

        #region cstor

        /// <summary>
        /// Instantiates a new stream store message key
        /// </summary>
        /// <param name="streamId">Specific stream id</param>
        /// <param name="streamSequenceNumber">Stream sequence number</param>
        internal StreamStoreMessageKey(Guid streamId, long streamSequenceNumber)
        {
            this.StreamId = streamId;
            this.StreamSequenceNumber = streamSequenceNumber;
        }

        #endregion

        /// <summary>
        /// Returns a string that represents the current stream store message key object.
        /// </summary>
        /// <returns>
        /// A string that represents the current stream store message key object.
        /// </returns>
        /// <filterpriority>2</filterpriority>
        public override string ToString()
        {
            return string.Format("StreamMessageKey::{0}::{1}", this.StreamId, this.StreamSequenceNumber);
        }
    }

    #endregion StreamStoreMessageKey

    #region IComparer and IEquityComparer for StreamStoreMessageKey

    /// <summary>
    /// Provide overrides for IComparer and IEquityComparer for StreamStoreMessageKey
    /// </summary>
    internal class StreamStoreMessageKeyComparer : IComparer<StreamStoreMessageKey>, IEqualityComparer<StreamStoreMessageKey>
    {
        /// <summary>
        /// Compares two objects and returns a value indicating whether one is less than, equal to, or greater than the other.
        /// </summary>
        /// <returns>
        /// A signed integer that indicates the relative values of <paramref name="x"/> and <paramref name="y"/>, as shown in the following table.Value Meaning Less than zero<paramref name="x"/> is less than <paramref name="y"/>.Zero<paramref name="x"/> equals <paramref name="y"/>.Greater than zero<paramref name="x"/> is greater than <paramref name="y"/>.
        /// </returns>
        /// <param name="x">The first object to compare.</param><param name="y">The second object to compare.</param>
        public int Compare(StreamStoreMessageKey x, StreamStoreMessageKey y)
        {
            // Comparison when x == null.
            if (null == x)
            {
                return null == y ? 0 : 1;
            }

            // Comparison when y == null.
            if (null == y)
            {
                return -1;
            }

            // x !null and y !null
            // Compare if x and y belong to the same stream
            var streamIdCompare = x.StreamId.CompareTo(y.StreamId);

            // if same stream, further check for stream sequence number
            if (0 == streamIdCompare)
            {
                var streamSeqNumCompare = x.StreamSequenceNumber.CompareTo(y.StreamSequenceNumber);
                return streamSeqNumCompare;
            }

            return streamIdCompare;
        }

        /// <summary>
        /// Determines whether the specified objects are equal.
        /// </summary>
        /// <returns>
        /// true if the specified objects are equal; otherwise, false.
        /// </returns>
        /// <param name="x">The first object of type <cref name="Stream.StreamStoreMessageKey"/> to compare.</param><param name="y">The second object of type <cref name="Stream.StreamStoreMessageKey"/> to compare.</param>
        public bool Equals(StreamStoreMessageKey x, StreamStoreMessageKey y)
        {
            return 0 == this.Compare(x, y);
        }

        /// <summary>
        /// Returns a hash code for the specified object.
        /// </summary>
        /// <returns>
        /// A hash code for the specified object.
        /// </returns>
        /// <param name="obj">The <see cref="System.Object"/> for which a hash code is to be returned.</param>
        /// <exception cref="System.ArgumentNullException">The type of <paramref name="obj"/> is a reference type and <paramref name="obj"/> is null.</exception>
        public int GetHashCode(StreamStoreMessageKey obj)
        {
            return obj.StreamSequenceNumber.GetHashCode() ^ obj.StreamId.GetHashCode();
        }
    }

    #endregion

    #region IByteConverter for StreamStoreMessageKey

    /// <summary>
    /// Provides overrides for IByteConverter.
    /// Converts the stream store message key from byte array and vice versa. 
    /// </summary>
    internal class StreamStoreMessageKeyConverter : IStateSerializer<StreamStoreMessageKey>
    {
        public static StreamStoreMessageKeyConverter Converter = new StreamStoreMessageKeyConverter();

        /// <summary>
        /// Converts the Stream store message key to byte array
        /// </summary>
        /// <param name="value">Stream store message key object</param>
        /// <param name="binaryWriter"></param>
        /// <returns>Byte array</returns>
        public void Write(StreamStoreMessageKey value, BinaryWriter binaryWriter)
        {
            binaryWriter.Write(value.StreamSequenceNumber);
            binaryWriter.Write(value.StreamId.ToByteArray());
        }

        /// <summary>
        /// Assembles the stream store message key object from byte array
        /// </summary>
        /// <param name="binaryReader">BinaryReader to deserialize from</param>
        /// <returns>Stream store message key object</returns>
        public StreamStoreMessageKey Read(BinaryReader binaryReader)
        {
            var streamSequenceNumber = binaryReader.ReadInt64();
            var streamIdBytes = binaryReader.ReadBytes(16);

            return new StreamStoreMessageKey(streamId: new Guid(streamIdBytes), streamSequenceNumber: streamSequenceNumber);
        }

        public StreamStoreMessageKey Read(StreamStoreMessageKey baseValue, BinaryReader binaryReader)
        {
            return this.Read(binaryReader);
        }

        public void Write(StreamStoreMessageKey baseValue, StreamStoreMessageKey targetValue, BinaryWriter binaryWriter)
        {
            this.Write(targetValue, binaryWriter);
        }
    }

    #endregion

    #region IRangePartitioner for StreamStoreMessageKey

    /// <summary>
    /// Provides overrides for IRangePartitioner.
    /// </summary>
    internal class StreamStoreMessageKeyPartitioner : IRangePartitioner<StreamStoreMessageKey>
    {
        public IEnumerable<StreamStoreMessageKey> Partitions
        {
            get { return null; }
        }
    }

    #endregion
}