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

    #region StreamStoreType

    /// <summary>
    /// Enumerates the types of store kinds for the flattened store
    /// </summary>
    internal enum StreamStoreKind : int
    {
        MetadataStore = 1001,
        MessageStore = 2001,
        NameStore = 3001
    }

    #endregion

    #region StreamStoreConsolidatedKey

    /// <summary>
    /// Represents StreamStoreConsolidatedKey object.
    /// </summary>
    internal class StreamStoreConsolidatedKey
    {
        #region properties

        // Kind of store
        internal StreamStoreKind StoreKind { get; private set; }

        // Store specific Key
        internal StreamStoreMessageKey MessageKey { get; private set; }

        internal StreamMetadataKey MetadataKey { get; private set; }

        internal StreamNameKey NameKey { get; private set; }

        internal Guid StreamId
        {
            get
            {
                if (this.StoreKind == StreamStoreKind.MessageStore)
                {
                    return this.MessageKey.StreamId;
                }

                if (this.StoreKind == StreamStoreKind.MetadataStore)
                {
                    return this.MetadataKey.StreamId;
                }

                Diagnostics.Assert(false, "Attempt to access StreamId for a nameStore key");
                return Guid.Empty;
            }
        }

        #endregion

        #region cstor

        /// <summary>
        /// Message store consolidated key
        /// </summary>
        /// <param name="messageKey"></param>
        internal StreamStoreConsolidatedKey(StreamStoreMessageKey messageKey)
        {
            this.StoreKind = StreamStoreKind.MessageStore;
            this.MessageKey = messageKey;
            this.MetadataKey = null;
            this.NameKey = null;
        }

        /// <summary>
        /// Metadata store consolidated key
        /// </summary>
        /// <param name="metadataKey"></param>
        internal StreamStoreConsolidatedKey(StreamMetadataKey metadataKey)
        {
            this.StoreKind = StreamStoreKind.MetadataStore;
            this.MetadataKey = metadataKey;
            this.MessageKey = null;
            this.NameKey = null;
        }

        /// <summary>
        /// Name store consolidated key
        /// </summary>
        /// <param name="nameKey"></param>
        internal StreamStoreConsolidatedKey(StreamNameKey nameKey)
        {
            this.StoreKind = StreamStoreKind.NameStore;
            this.NameKey = nameKey;
            this.MessageKey = null;
            this.MetadataKey = null;
        }

        #endregion

        /// <summary>
        /// Returns a string that represents the current StreamStoreConsolidatedKey object.
        /// </summary>
        /// <returns>
        /// A string that represents the current StreamStoreConsolidatedKey object.
        /// </returns>
        /// <filterpriority>2</filterpriority>
        public override string ToString()
        {
            string result = null;

            switch (this.StoreKind)
            {
                case StreamStoreKind.MetadataStore:
                    result = this.MetadataKey.ToString();
                    break;
                case StreamStoreKind.MessageStore:
                    result = this.MessageKey.ToString();
                    break;
                case StreamStoreKind.NameStore:
                    result = this.NameKey.ToString();
                    break;
                default:
                    Diagnostics.Assert(false, "Unknown store kind in StreamStoreConsolidatedKey.ToString()");
                    break;
            }

            return result ?? string.Empty;
        }
    }

    #endregion

    #region IComparer and IEquityComparer for StreamStoreConsolidatedKey

    /// <summary>
    /// Provide overrides for IComparer and IEquityComparer for StreamStoreConsolidatedKey
    /// </summary>
    internal class StreamStoreConsolidatedKeyComparer : IComparer<StreamStoreConsolidatedKey>, IEqualityComparer<StreamStoreConsolidatedKey>
    {
        /// <summary>
        /// Compares two StreamStoreConsolidatedKey objects and returns a value indicating whether one is less than, equal to, or greater than the other.
        /// </summary>
        /// <returns>
        /// A signed integer that indicates the relative values of <paramref name="x"/> and <paramref name="y"/>, as shown in the following table.
        /// Value Meaning Less than zero<paramref name="x"/> is less than <paramref name="y"/>.
        /// Zero<paramref name="x"/> equals <paramref name="y"/>.
        /// Greater than zero<paramref name="x"/> is greater than <paramref name="y"/>.
        /// </returns>
        /// <param name="x">The first StreamStoreConsolidatedKey object to compare.</param>
        /// <param name="y">The second StreamStoreConsolidatedKey object to compare.</param>
        public int Compare(StreamStoreConsolidatedKey x, StreamStoreConsolidatedKey y)
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

            // x !null and y !null, 
            // Check the Store Kind
            var kindCompare = x.StoreKind.CompareTo(y.StoreKind);

            // if they are same kind and then check individual store key objects.
            if (0 == kindCompare)
            {
                if (x.StoreKind == StreamStoreKind.MessageStore)
                {
                    var messageKeyComparercomparer = new StreamStoreMessageKeyComparer();
                    return messageKeyComparercomparer.Compare(x.MessageKey, y.MessageKey);
                }

                if (x.StoreKind == StreamStoreKind.MetadataStore)
                {
                    var metadataKeyComparercomparer = new StreamMetadataKeyComparer();
                    return metadataKeyComparercomparer.Compare(x.MetadataKey, y.MetadataKey);
                }

                var nameKeyComparercomparer = new StreamNameKeyComparer();
                return nameKeyComparercomparer.Compare(x.NameKey, y.NameKey);
            }

            // Not the same kind
            return kindCompare;
        }

        /// <summary>
        /// Determines whether the specified StreamStoreConsolidatedKey objects are equal.
        /// </summary>
        /// <returns>
        /// true if the specified StreamStoreConsolidatedKey objects are equal; otherwise, false.
        /// </returns>
        /// <param name="x">The first object of type <cref name="StreamStoreConsolidatedKey"/> to compare.</param>
        /// <param name="y">The second object of type <cref name="StreamStoreConsolidatedKey"/> to compare.</param>
        public bool Equals(StreamStoreConsolidatedKey x, StreamStoreConsolidatedKey y)
        {
            return 0 == this.Compare(x, y);
        }

        /// <summary>
        /// Returns a hash code for the specified StreamStoreConsolidatedKey object.
        /// </summary>
        /// <returns>
        /// A hash code for the specified StreamStoreConsolidatedKey object.
        /// </returns>
        /// <param name="obj">The <see cref="StreamMetadataKey"/> for which a hash code is to be returned.</param>
        /// <exception cref="System.ArgumentNullException">The type of <paramref name="obj"/> is a reference type and <paramref name="obj"/> is null.</exception>
        public int GetHashCode(StreamStoreConsolidatedKey obj)
        {
            var kindCode = obj.StoreKind.GetHashCode();
            int keyCode;

            if (obj.StoreKind == StreamStoreKind.MessageStore)
            {
                var messageKeyComparer = new StreamStoreMessageKeyComparer();
                keyCode = messageKeyComparer.GetHashCode(obj.MessageKey);
            }
            else if (obj.StoreKind == StreamStoreKind.MetadataStore)
            {
                var streamMetadataKeyComparer = new StreamMetadataKeyComparer();
                keyCode = streamMetadataKeyComparer.GetHashCode(obj.MetadataKey);
            }
            else
            {
                var streamNameKeyComparer = new StreamNameKeyComparer();
                keyCode = streamNameKeyComparer.GetHashCode(obj.NameKey);
            }

            return kindCode ^ keyCode;
        }
    }

    #endregion

    #region IByteConverter for StreamStoreConsolidatedKey

    /// <summary>
    /// Provides overrides for IByteConverter.
    /// Converts the StreamStoreConsolidatedKey from byte array and vice versa. 
    /// </summary>
    internal class StreamStoreConsolidatedKeyConverter : IStateSerializer<StreamStoreConsolidatedKey>
    {
        /// <summary>
        /// Converts the StreamStoreConsolidatedKey to byte array
        /// </summary>
        /// <param name="value">StreamStoreConsolidatedKey object</param>
        /// <param name="binaryWriter"></param>
        /// <returns>Byte array</returns>
        public void Write(StreamStoreConsolidatedKey value, BinaryWriter binaryWriter)
        {
            binaryWriter.Write((int) value.StoreKind);

            if (value.StoreKind == StreamStoreKind.MessageStore)
            {
                var streamStoreMessageKeyConverter = new StreamStoreMessageKeyConverter();
                streamStoreMessageKeyConverter.Write(value.MessageKey, binaryWriter);
            }
            else if (value.StoreKind == StreamStoreKind.MetadataStore)
            {
                var streamMetadataKeyConverter = new StreamMetadataKeyConverter();
                streamMetadataKeyConverter.Write(value.MetadataKey, binaryWriter);
            }
            else
            {
                var streamNameKeyConverter = new StreamNameKeyConverter();
                streamNameKeyConverter.Write(value.NameKey, binaryWriter);
            }
        }

        /// <summary>
        /// Assembles the StreamStoreConsolidatedKey object from byte array
        /// </summary>
        /// <param name="reader">BinaryReader to deserialize from</param>
        /// <returns>StreamStoreConsolidatedKey object</returns>
        public StreamStoreConsolidatedKey Read(BinaryReader reader)
        {
            var kind = (StreamStoreKind) reader.ReadInt32();

            StreamStoreConsolidatedKey result;

            if (kind == StreamStoreKind.MessageStore)
            {
                var key = StreamStoreMessageKeyConverter.Converter.Read(reader);
                result = new StreamStoreConsolidatedKey(key);
            }
            else if (kind == StreamStoreKind.MetadataStore)
            {
                var key = StreamMetadataKeyConverter.Converter.Read(reader);
                result = new StreamStoreConsolidatedKey(key);
            }
            else
            {
                var key = StreamNameKeyConverter.Converter.Read(reader);
                result = new StreamStoreConsolidatedKey(key);
            }

            return result;
        }

        /// <summary>
        /// Converts IEnumerable of byte[] to T
        /// </summary>
        /// <param name="baseValue"></param>
        /// <param name="reader">BinaryReader to deserialize from</param>
        /// <returns>Deserialized version of T.</returns>
        public StreamStoreConsolidatedKey Read(StreamStoreConsolidatedKey baseValue, BinaryReader reader)
        {
            return this.Read(reader);
        }

        /// <summary>
        /// Converts T to byte array.
        /// </summary>
        /// <param name="currentValue"></param>
        /// <param name="newValue">T to be serialized.</param>
        /// <param name="binaryWriter"></param>
        /// <returns>Serialized version of T.</returns>
        public void Write(StreamStoreConsolidatedKey currentValue, StreamStoreConsolidatedKey newValue, BinaryWriter binaryWriter)
        {
            this.Write(newValue, binaryWriter);
        }
    }

    #endregion

    #region IRangePartitioner for StreamStoreConsolidatedKey

    /// <summary>
    /// Provides overrides for IRangePartitioner.
    /// </summary>
    internal class StreamStoreConsolidatedKeyPartitioner : Store.IRangePartitioner<StreamStoreConsolidatedKey>
    {
        public IEnumerable<StreamStoreConsolidatedKey> Partitions
        {
            get { return null; }
        }
    }

    #endregion
}