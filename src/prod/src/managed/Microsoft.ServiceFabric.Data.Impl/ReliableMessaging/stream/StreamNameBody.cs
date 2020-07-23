// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    using System.IO;
    using Microsoft.ServiceFabric.Data;

    internal class StreamNameBody
    {
        /// <summary>
        /// Unique Stream Id
        /// </summary>
        internal Guid StreamId { get; private set; }

        /// <summary>
        /// Constructs a Stream Name body object. 
        /// </summary>
        /// <param name="streamId">Given Stream Id guid</param>
        internal StreamNameBody(Guid streamId)
        {
            this.StreamId = streamId;
        }

        /// <summary>
        /// Returns a string that represents the current StreamNameBody object.
        /// </summary>
        /// <returns>
        /// A string that represents the current StreamNameBody object.
        /// </returns>
        /// <filterpriority>2</filterpriority>
        public override string ToString()
        {
            return this.StreamId.ToString();
        }
    }

    /// <summary>
    /// Provides overrides for IByteConverter.
    /// Converts the stream store message body to byte array and vice versa. 
    /// </summary>
    internal class StreamNameBodyConverter : IStateSerializer<StreamNameBody>
    {
        public static StreamNameBodyConverter Converter = new StreamNameBodyConverter();

        /// <summary>
        /// Converts the Stream name body to byte array
        /// </summary>
        /// <param name="value">Stream name body object</param>
        /// <param name="writer"></param>
        /// <returns>Byte array</returns>
        public void Write(StreamNameBody value, BinaryWriter writer)
        {
            writer.Write(value.StreamId.ToByteArray());
        }

        /// <summary>
        /// Assembles the stream name body object from byte array
        /// </summary>
        /// <param name="reader">BinaryReader to deserialize from</param>
        /// <returns>Stream name body object</returns>
        public StreamNameBody Read(BinaryReader reader)
        {
            return new StreamNameBody(new Guid(reader.ReadBytes(16)));
        }

        public StreamNameBody Read(StreamNameBody baseValue, BinaryReader binaryReader)
        {
            return this.Read(binaryReader);
        }

        public void Write(StreamNameBody baseValue, StreamNameBody targetValue, BinaryWriter binaryWriter)
        {
            this.Write(targetValue, binaryWriter);
        }
    }
}