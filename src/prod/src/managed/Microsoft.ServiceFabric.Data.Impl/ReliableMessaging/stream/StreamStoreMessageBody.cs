// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    #region using directives

    using System.IO;
    using Microsoft.ServiceFabric.Data;

    #endregion

    /// <summary>
    /// Represents the body of the stream store message object.
    /// </summary>
    internal class StreamStoreMessageBody
    {
        internal byte[] Payload { get; private set; }

        internal StreamStoreMessageBody(byte[] payload)
        {
            this.Payload = payload;
        }
    }

    /// <summary>
    /// Provides overrides for IByteConverter.
    /// Converts the stream store message body to byte array and vice versa. 
    /// </summary>
    internal class StreamStoreMessageBodyConverter : IStateSerializer<StreamStoreMessageBody>
    {
        public static StreamStoreMessageBodyConverter Converter = new StreamStoreMessageBodyConverter();

        public StreamStoreMessageBody Read(StreamStoreMessageBody baseValue, BinaryReader binaryReader)
        {
            return this.Read(binaryReader);
        }

        public StreamStoreMessageBody Read(BinaryReader binaryReader)
        {
            var count = binaryReader.ReadInt32();

            if (count == 0)
            {
                return new StreamStoreMessageBody(null);
            }

            var bytes = binaryReader.ReadBytes(count);
            return new StreamStoreMessageBody(bytes);
        }

        public void Write(StreamStoreMessageBody baseValue, StreamStoreMessageBody targetValue, BinaryWriter binaryWriter)
        {
            this.Write(targetValue, binaryWriter);
        }

        public void Write(StreamStoreMessageBody value, BinaryWriter binaryWriter)
        {
            if (value.Payload == null)
            {
                binaryWriter.Write(0);
                return;
            }

            binaryWriter.Write(value.Payload.Length);
            binaryWriter.Write(value.Payload);
        }
    }
}