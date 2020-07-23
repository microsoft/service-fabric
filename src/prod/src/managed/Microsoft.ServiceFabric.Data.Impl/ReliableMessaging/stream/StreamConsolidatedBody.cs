// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    #region using directives

    using System.Collections;
    using System.Collections.Generic;
    using System.Fabric.ReliableMessaging;
    using System.IO;
    using Microsoft.ServiceFabric.Data;

    #endregion

    /// <summary>
    /// StreamStoreConsolidatedBody Info
    /// </summary>
    internal class StreamStoreConsolidatedBody
    {
        #region properties

        // The store kind
        internal StreamStoreKind StoreKind { get; private set; }

        // The appropriate store kind body
        internal StreamStoreMessageBody MessageBody { get; private set; }

        internal StreamMetadataBody MetadataBody { get; private set; }

        internal StreamNameBody NameBody { get; private set; }

        #endregion

        #region cstor

        /// <summary>
        /// StreamStore kind -- StreamStoreConsolidatedBody Cstr
        /// </summary>
        /// <param name="messageBody">StreamStoreMessageBody </param>
        internal StreamStoreConsolidatedBody(StreamStoreMessageBody messageBody)
        {
            this.StoreKind = StreamStoreKind.MessageStore;
            this.MessageBody = messageBody;
            this.MetadataBody = null;
            this.NameBody = null;
        }

        /// <summary>
        /// StreamMetadata Store kind -- StreamStoreConsolidatedBody Cstr
        /// </summary>
        /// <param name="metadataBody">StreamMetadataBody </param>
        internal StreamStoreConsolidatedBody(StreamMetadataBody metadataBody)
        {
            this.StoreKind = StreamStoreKind.MetadataStore;
            this.MetadataBody = metadataBody;
            this.MessageBody = null;
            this.NameBody = null;
        }

        /// <summary>
        /// StreamName kind -- StreamStoreConsolidatedBody Cstr
        /// </summary>
        /// <param name="nameBody">StreamNameBody </param>
        internal StreamStoreConsolidatedBody(StreamNameBody nameBody)
        {
            this.StoreKind = StreamStoreKind.NameStore;
            this.NameBody = nameBody;
            this.MessageBody = null;
            this.MetadataBody = null;
        }

        #endregion

        /// <summary>
        /// Returns a string that represents the current StreamStoreConsolidatedBody object.
        /// </summary>
        /// <returns>
        /// A string that represents the current StreamStoreConsolidatedBody object.
        /// </returns>
        /// <filterpriority>2</filterpriority>
        public override string ToString()
        {
            string result = null;

            switch (this.StoreKind)
            {
                case StreamStoreKind.MetadataStore:
                    result = this.MetadataBody.ToString();
                    break;
                case StreamStoreKind.MessageStore:
                    result = this.MessageBody.ToString();
                    break;
                case StreamStoreKind.NameStore:
                    result = this.NameBody.ToString();
                    break;
                default:
                    Diagnostics.Assert(false, "Unknown store kind in StreamStoreConsolidatedBody.ToString()");
                    break;
            }

            return result ?? string.Empty;
        }
    }

    /// <summary>
    /// Iterator functionality
    /// </summary>
    internal class StreamStoreBodyEnumerable : IEnumerable<byte[]>
    {
        private byte[] tagBytes;
        private byte[] contentBytes;

        internal StreamStoreBodyEnumerable(StreamStoreKind tag, byte[] content)
        {
            this.contentBytes = content;
            this.tagBytes = BitConverter.GetBytes((int) tag);
        }

        /// <summary>
        /// Returns an enumerator that iterates through the collection.
        /// </summary>
        /// <returns>
        /// A <see cref="System.Collections.IEnumerator"/> that can be used to iterate through the collection.
        /// </returns>
        /// <filterpriority>1</filterpriority>
        public IEnumerator<byte[]> GetEnumerator()
        {
            yield return this.tagBytes;
            yield return this.contentBytes;
        }

        /// <summary>
        /// Returns an enumerator that iterates through a collection.
        /// </summary>
        /// <returns>
        /// An <see cref="System.Collections.IEnumerator"/> object that can be used to iterate through the collection.
        /// </returns>
        /// <filterpriority>2</filterpriority>
        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }
    }

    /// <summary>
    /// Provides overrides for IByteEnumerableConverter.
    /// Converts the stream store message body to iterable byte array and vice versa. 
    /// </summary>
    internal class StreamStoreConsolidatedBodyConverter : IStateSerializer<StreamStoreConsolidatedBody>
    {
        public void Write(StreamStoreConsolidatedBody currentValue, StreamStoreConsolidatedBody newValue, BinaryWriter binaryWriter)
        {
            // null is the default for StreamStoreConsolidatedBody and the store expects us to be able to (de)serialize the default
            if (newValue == null)
            {
                binaryWriter.Write(false);
                return;
            }

            binaryWriter.Write(BitConverter.GetBytes((int) newValue.StoreKind));

            if (newValue.StoreKind == StreamStoreKind.MessageStore)
            {
                StreamStoreMessageBodyConverter.Converter.Write(newValue.MessageBody, binaryWriter);
            }
            else if (newValue.StoreKind == StreamStoreKind.MetadataStore)
            {
                StreamMetadataBodyConverter.Converter.Write(newValue.MetadataBody, binaryWriter);
            }
            else
            {
                StreamNameBodyConverter.Converter.Write(newValue.NameBody, binaryWriter);
            }
        }

        public StreamStoreConsolidatedBody Read(BinaryReader reader)
        {
            // null is the default for StreamStoreConsolidatedBody and the store expects us to be able to (de)serialize the default
            var exists = reader.ReadBoolean();
            if (exists == false)
            {
                return null;
            }

            var tagExists = reader.ReadBoolean();
            Diagnostics.Assert(tagExists, "No tag in StreamStoreConsolidatedBody FromEnumerableByteArray");

            var kind = (StreamStoreKind) reader.ReadInt32();

            var contentExists = reader.ReadBoolean();
            Diagnostics.Assert(contentExists, "No content in StreamStoreConsolidatedBody FromEnumerableByteArray");

            StreamStoreConsolidatedBody result;
            if (kind == StreamStoreKind.MessageStore)
            {
                var body = StreamStoreMessageBodyConverter.Converter.Read(reader);
                result = new StreamStoreConsolidatedBody(body);
            }
            else if (kind == StreamStoreKind.MetadataStore)
            {
                var body = StreamMetadataBodyConverter.Converter.Read(reader);
                result = new StreamStoreConsolidatedBody(body);
            }
            else
            {
                var body = StreamNameBodyConverter.Converter.Read(reader);
                result = new StreamStoreConsolidatedBody(body);
            }

            return result;
        }

        public StreamStoreConsolidatedBody Read(StreamStoreConsolidatedBody baseValue, BinaryReader binaryReader)
        {
            throw new NotImplementedException();
        }

        public void Write(StreamStoreConsolidatedBody value, BinaryWriter binaryWriter)
        {
            throw new NotImplementedException();
        }
    }
}