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

    #region StreamNameKey

    /// <summary>
    /// Stream Name Key uses a combination of StreamName and PartnerId to ensure stream name uniqueness.
    /// </summary>
    internal class StreamNameKey
    {
        // The Stream name 
        internal Uri StreamName { get; private set; }

        // PartnerId (service name + partition info)
        internal PartitionKey PartnerId { get; private set; }

        /// <summary>
        /// Constructs a Stream Name key object.
        /// </summary>
        /// <param name="streamName">Stream Name</param>
        /// <param name="partnerId">Partner Id</param>
        internal StreamNameKey(Uri streamName, PartitionKey partnerId)
        {
            this.StreamName = streamName;
            this.PartnerId = partnerId;
        }

        /// <summary>
        /// String representation of Stream Name Key object
        /// </summary>
        /// <returns>String representation of Stream Name Key object</returns>
        public override string ToString()
        {
            return string.Format("{0} ::=> {1}", this.StreamName, this.PartnerId);
        }
    }

    #endregion

    #region IComparer IEqualityComparer for StreamNameKey

    internal class StreamNameKeyComparer : IComparer<StreamNameKey>, IEqualityComparer<StreamNameKey>
    {
        /// <summary>
        /// Compares two StreamNameKey objects and returns a value indicating whether one is less than, equal to, or greater than the other.
        /// </summary>
        /// <returns>
        /// A signed integer that indicates the relative values of <paramref name="x"/> and <paramref name="y"/>, 
        /// as shown in the following table.
        /// Value Meaning Less than zero<paramref name="x"/> is less than <paramref name="y"/>.
        /// Zero<paramref name="x"/> equals <paramref name="y"/>.
        /// Greater than zero<paramref name="x"/> is greater than <paramref name="y"/>.
        /// </returns>
        /// <param name="x">The first StreamNameKey object to compare.</param><param name="y">The second StreamNameKey object to compare.</param>
        public int Compare(StreamNameKey x, StreamNameKey y)
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

            //Comparison when both x and y are not null.
            var nameCompare = Uri.Compare(x.StreamName, y.StreamName, UriComponents.AbsoluteUri, UriFormat.UriEscaped, StringComparison.Ordinal);

            // Further compare PartnerId if stream name is the same.
            return 0 == nameCompare ? x.PartnerId.CompareTo(y.PartnerId) : nameCompare;
        }

        /// <summary>
        /// Determines whether the specified StreamNameKey objects are equal.
        /// </summary>
        /// <returns>
        /// true if the specified StreamNameKey objects are equal; otherwise, false.
        /// </returns>
        /// <param name="x">The first object of type <cref name="StreamNameKey"/> to compare.</param>
        /// <param name="y">The second object of type <cref name="StreamNameKey"/> to compare.</param>
        public bool Equals(StreamNameKey x, StreamNameKey y)
        {
            return 0 == this.Compare(x, y);
        }

        /// <summary>
        /// Returns a hash code for the specified StreamNameKey object.
        /// </summary>
        /// <returns>
        /// A hash code for the specified StreamNameKey object.
        /// </returns>
        /// <param name="obj">The <see cref="StreamNameKey"/> for which a hash code is to be returned.</param>
        /// <exception cref="System.ArgumentNullException">The type of <paramref name="obj"/> is a reference type and <paramref name="obj"/> is null.</exception>
        public int GetHashCode(StreamNameKey obj)
        {
            var nameCode = obj.StreamName.GetHashCode();
            var partnerCode = obj.PartnerId.GetHashCode();

            return nameCode ^ partnerCode;
        }
    }

    #endregion

    #region IByteConverter for StreamNameKey

    /// <summary>
    /// Provides overrides for IByteConverter.
    /// Converts the stream name key from byte array and vice versa. 
    /// </summary>
    internal class StreamNameKeyConverter : IStateSerializer<StreamNameKey>
    {
        public static StreamNameKeyConverter Converter = new StreamNameKeyConverter();

        /// <summary>
        /// Converts the Stream name key object to byte array
        /// </summary>
        /// <param name="value">Stream name key object</param>
        /// <param name="writer"></param>
        /// <returns>Byte array</returns>
        public void Write(StreamNameKey value, BinaryWriter writer)
        {
            // Encode to local memory stream
            using (var stream = new MemoryStream())
            {
                // Encode the stream name
                writer.Write(value.StreamName.OriginalString);

                // Encode the Partner info
                value.PartnerId.Encode(writer);
            }
        }

        /// <summary>
        /// Assembles the stream name key object from byte array
        /// </summary>
        /// <param name="reader">BinaryReader to deserialize from</param>
        /// <returns>Stream name key object</returns>
        public StreamNameKey Read(BinaryReader reader)
        {
            var encoder = new StringEncoder();

            // Decode Service stream name
            var streamName = new Uri(encoder.Decode(reader));

            // Decode Partner info
            var partnerId = PartitionKey.Decode(reader);

            return new StreamNameKey(streamName, partnerId);
        }

        public StreamNameKey Read(StreamNameKey baseValue, BinaryReader binaryReader)
        {
            return this.Read(binaryReader);
        }

        public void Write(StreamNameKey baseValue, StreamNameKey targetValue, BinaryWriter binaryWriter)
        {
            this.Write(targetValue, binaryWriter);
        }
    }

    #endregion

    #region IRangePartitioner for StreamNameKey

    /// <summary>
    /// Provides overrides for IRangePartitioner.
    /// </summary>
    internal class StreamNameKeyPartitioner : IRangePartitioner<StreamNameKey>
    {
        public IEnumerable<StreamNameKey> Partitions
        {
            get { return null; }
        }
    }

    #endregion
}