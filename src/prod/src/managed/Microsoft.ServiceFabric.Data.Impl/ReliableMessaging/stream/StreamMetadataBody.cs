// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    #region using directives

    using System.Fabric.ReliableMessaging;
    using System.Globalization;
    using System.IO;
    using Microsoft.ServiceFabric.Data;

    #endregion

    #region StreamMetadataBody

    /// <summary>
    /// Enumerates the types of Streams metadatabodies
    /// 1. OutboundStreamMetadatabody
    /// 2. InboundStreamMetadatabody
    /// 3. ReceiveStreamSequenceNumber
    /// 4. SendStreamSequenceNumber
    /// 5. DeleteStreamSequenceNumber
    /// </summary>
    internal class StreamMetadataBody
    {
        protected internal StreamMetadataKind MetadataKind { get; protected set; }
    }

    #endregion

    #region BaseStreamMetadataBody

    /// <summary>
    /// Common info for Outbound and Inbound Stream meta data body
    /// </summary>
    internal abstract class BaseStreamMetadataBody : StreamMetadataBody
    {
        private PersistentStreamState currentState;

        #region properties

        // StreamName attributes
        internal Uri StreamName { get; private set; }

        internal PartitionKey PartnerId { get; private set; }

        // Current State of the stream
        internal PersistentStreamState CurrentState
        {
            get { return this.currentState; }
            set
            {
                this.currentState = value;
                this.CurrentStateDate = DateTime.UtcNow;
            }
        }

        // Current State Last modified date 
        internal DateTime CurrentStateDate { get; set; }

        // Last close message sequence number
        internal long CloseMessageSequenceNumber { get; set; }

        // BaseStreamMetadataBody Encode byte count
        internal int BaseEncodeLength { get; private set; }

        #endregion

        /// <summary>
        /// Set the BaseStreamMetadataBody properties 
        /// </summary>
        /// <param name="metadataKind">Kind of StreamMetadata</param>
        /// <param name="streamName">Name of the stream </param>
        /// <param name="partnerId">Partner Id</param>
        /// <param name="currentState">Current persistent state of the stream</param>
        /// <param name="closeMessageSequenceNumber">Closing sequence number of the message</param>
        protected void InitializeBaseParameters(
            StreamMetadataKind metadataKind, Uri streamName, PartitionKey partnerId, PersistentStreamState currentState, long closeMessageSequenceNumber)
        {
            this.MetadataKind = metadataKind;
            this.StreamName = streamName;
            this.PartnerId = partnerId;
            this.CurrentState = currentState;
            this.CloseMessageSequenceNumber = closeMessageSequenceNumber;
            var encoder = new StringEncoder(this.StreamName.OriginalString);
            this.BaseEncodeLength = (sizeof(int)*2) + (sizeof(long)*2) + encoder.EncodingByteCount + this.PartnerId.EncodingByteCount;
        }

        /// <summary>
        /// Write BaseStreamMetadataBody to Memory Stream
        /// </summary>
        /// <param name="writer">BinaryWriter to serialize to.</param>
        protected void WriteBaseParameters(BinaryWriter writer)
        {
            writer.Write((int) this.MetadataKind);
            writer.Write((int) this.CurrentState);
            writer.Write(this.CurrentStateDate.Ticks);
            writer.Write(this.CloseMessageSequenceNumber);

            var encoder = new StringEncoder(this.StreamName.OriginalString);
            encoder.Encode(writer);
            this.PartnerId.Encode(writer);
        }

        /// <summary>
        /// Read BaseStreamMetadataBody from Memory Stream
        /// </summary>
        /// <param name="reader">BinaryReader to deserialize from</param>
        protected void ReadBaseParameters(BinaryReader reader)
        {
            this.MetadataKind = (StreamMetadataKind) reader.ReadInt32();

            this.CurrentState = (PersistentStreamState) reader.ReadInt32();

            this.CurrentStateDate = DateTime.FromBinary(reader.ReadInt64());

            this.CloseMessageSequenceNumber = reader.ReadInt64();

            var encoder = new StringEncoder();
            this.StreamName = new Uri(encoder.Decode(reader));
            this.PartnerId = PartitionKey.Decode(reader);

            this.BaseEncodeLength = (sizeof(int)*2) + (sizeof(long)*2) + encoder.EncodingByteCount + this.PartnerId.EncodingByteCount;
        }

        /// <summary>
        /// Returns a string that represents the current BaseStreamMetadataBody object.
        /// </summary>
        /// <returns>
        /// A string that represents the current BaseStreamMetadataBody object.
        /// </returns>
        /// <filterpriority>2</filterpriority>
        protected string GetBaseString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "StreamName = {0} CurrentState = {1} CurrentStateDate = {2} Partner = {3} CloseMessageSequenceNumber = {4}",
                this.StreamName,
                this.CurrentState,
                this.CurrentStateDate,
                this.PartnerId,
                this.CloseMessageSequenceNumber);
        }
    }

    #endregion

    #region OutboundStreamStableParameters

    internal class OutboundStreamStableParameters : BaseStreamMetadataBody
    {
        #region Properties

        internal int MessageQuota { get; private set; }

        internal StreamWireProtocolResponse AcceptanceResponse { get; set; }

        #endregion

        #region cstor

        /// <summary>
        /// Used only for decoding from a byte array
        /// </summary>
        private OutboundStreamStableParameters()
        {
        }

        internal OutboundStreamStableParameters(Uri streamName, PartitionKey partnerId, int messageQuota)
        {
            // Init base parameters
            this.InitializeBaseParameters(
                StreamMetadataKind.OutboundStableParameters,
                streamName,
                partnerId,
                PersistentStreamState.Initialized,
                StreamConstants.InitialValueOfLastSequenceNumberInStream);

            this.AcceptanceResponse = StreamWireProtocolResponse.Unknown;
            this.MessageQuota = messageQuota;
        }

        /// <summary>
        /// Copy constructor used for updates to avoid in memory update that would change "committed" state in the store before commit 
        /// especially when the object is acquired through Get
        /// </summary>
        /// <param name="stableParameters"></param>
        internal OutboundStreamStableParameters(OutboundStreamStableParameters stableParameters)
        {
            // Init base parameters
            this.InitializeBaseParameters(
                StreamMetadataKind.OutboundStableParameters,
                stableParameters.StreamName,
                stableParameters.PartnerId,
                stableParameters.CurrentState,
                stableParameters.CloseMessageSequenceNumber);

            this.AcceptanceResponse = stableParameters.AcceptanceResponse;
            this.MessageQuota = stableParameters.MessageQuota;
        }

        #endregion

        #region IByteConverter

        /// <summary>
        /// Converts the OutboundStreamStableParameters to byte array
        /// </summary>
        /// <returns>Byte array</returns>
        public void ToByteArray(OutboundStreamStableParameters value, BinaryWriter writer)
        {
            try
            {
                value.WriteBaseParameters(writer);
                writer.Write(value.MessageQuota);
                writer.Write((int) value.AcceptanceResponse);
            }
            catch (Exception e)
            {
                Tracing.WriteExceptionAsError("OutboundStreamStableParameters.ToByteArray", e, "Content: {0}", this);
            }
        }

        /// <summary>
        /// Assembles the OutboundStreamStableParameters object from byte array
        /// </summary>
        /// <param name="reader">BinaryReader to deserialize from</param>
        /// <returns>OutboundStreamStableParameters object</returns>
        public static OutboundStreamStableParameters FromByteArray(BinaryReader reader)
        {
            var result = new OutboundStreamStableParameters();

            var cursor = 0;
            result.ReadBaseParameters(reader);
            Diagnostics.Assert(
                result.MetadataKind == StreamMetadataKind.OutboundStableParameters,
                "Unexpected StreamMetadataType when decoding OutboundStreamStableParameters.FromByteArray");

            result.MessageQuota = reader.ReadInt32();
            Diagnostics.Assert(result.MessageQuota > 0, "Non-positive message quota found when decoding OutboundStreamStableParameters.FromByteArray");
            cursor += sizeof(int);

            result.AcceptanceResponse = (StreamWireProtocolResponse) reader.ReadInt32();

            return result;
        }

        #endregion

        /// <summary>
        /// Returns a string that represents the current OutboundStreamStableParameters object.
        /// </summary>
        /// <returns>
        /// A string that represents the current OutboundStreamStableParameters object.
        /// </returns>
        /// <filterpriority>2</filterpriority>
        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "{0} MessageQuota = {1}", this.GetBaseString(), this.MessageQuota);
        }
    }

    #endregion

    #region InboundStreamStableParameters

    internal class InboundStreamStableParameters : BaseStreamMetadataBody
    {
        #region cstor

        /// <summary>
        /// Used only for decoding from a byte array
        /// </summary>
        private InboundStreamStableParameters()
        {
        }

        internal InboundStreamStableParameters(Uri streamName, PartitionKey partnerId, PersistentStreamState currentState)
        {
            this.InitializeBaseParameters(
                StreamMetadataKind.InboundStableParameters,
                streamName,
                partnerId,
                currentState,
                StreamConstants.InitialValueOfLastSequenceNumberInStream);
        }

        /// <summary>
        /// Copy constructor used for updates to avoid in memory update that would change "committed" state in the store before commit 
        /// especially when the object is acquired through Get
        /// </summary>
        /// <param name="stableParameters"></param>
        internal InboundStreamStableParameters(InboundStreamStableParameters stableParameters)
        {
            this.InitializeBaseParameters(
                StreamMetadataKind.InboundStableParameters,
                stableParameters.StreamName,
                stableParameters.PartnerId,
                stableParameters.CurrentState,
                stableParameters.CloseMessageSequenceNumber);
        }

        #endregion

        #region IByteConverter

        /// <summary>
        /// Converts the InboundStreamStableParameters to byte array
        /// </summary>
        /// <returns>Byte array</returns>
        public void ToByteArray(InboundStreamStableParameters value, BinaryWriter writer)
        {
            try
            {
                value.WriteBaseParameters(writer);
            }
            catch (Exception e)
            {
                Tracing.WriteExceptionAsError("InboundStreamStableParameters.ToByteArray", e, "Content: {0}", this);
            }
        }

        /// <summary>
        /// Assembles the OutboundStreamStableParameters object from byte array
        /// </summary>
        /// <param name="reader">BinaryReader to deserialize from</param>
        /// <returns>OutboundStreamStableParameters object</returns>
        public static InboundStreamStableParameters FromByteArray(BinaryReader reader)
        {
            var result = new InboundStreamStableParameters();
            result.ReadBaseParameters(reader);
            Diagnostics.Assert(
                result.MetadataKind == StreamMetadataKind.InboundStableParameters,
                "Unexpected StreamMetadataType when decoding InboundStreamStableParameters.FromByteArray");

            return result;
        }

        #endregion
    }

    #endregion

    #region StreamSequenceNumber

    /// <summary>
    /// Common base structure and functionality for ReceiveSequenceNumber/SendSequenceNumber and DeleteSequenceNumber StreamMetadataBody
    /// </summary>
    internal abstract class StreamSequenceNumber : StreamMetadataBody
    {
        // The sequence number of the OpenStream message is always 0
        // data content messages will have sequence numbers starting with 1 which is defined as the constant FirstPayloadMessageSequenceNumber
        internal long NextSequenceNumber { get; private set; }

        /// <summary>
        /// Base Constructor 
        /// </summary>
        /// <param name="nextSequenceNumber"></param>
        internal StreamSequenceNumber(long nextSequenceNumber)
        {
            this.NextSequenceNumber = nextSequenceNumber;
        }

        /// <summary>
        /// Converts the StreamSequenceNumber to byte array
        /// </summary>
        /// <returns>Byte array</returns>
        public void ToByteArray(BinaryWriter writer)
        {
            writer.Write((int) this.MetadataKind);
            writer.Write(this.NextSequenceNumber);
        }

        /// <summary>
        /// Assembles the StreamSequenceNumber object from byte array
        /// </summary>
        /// <param name="reader">BinaryReader to deserialize from</param>
        /// <returns>StreamSequenceNumber object</returns>
        public static StreamSequenceNumber FromByteArray(BinaryReader reader)
        {
            var metadataKind = (StreamMetadataKind) reader.ReadInt32();
            var nextStreamSequenceNumber = reader.ReadInt64();

            StreamSequenceNumber result = null;

            // Assemble the right stream metadata
            switch (metadataKind)
            {
                case StreamMetadataKind.ReceiveSequenceNumber:
                    result = new ReceiveStreamSequenceNumber(nextStreamSequenceNumber);
                    break;
                case StreamMetadataKind.SendSequenceNumber:
                    result = new SendStreamSequenceNumber(nextStreamSequenceNumber);
                    break;
                case StreamMetadataKind.DeleteSequenceNumber:
                    result = new DeleteStreamSequenceNumber(nextStreamSequenceNumber);
                    break;
            }

            return result;
        }

        /// <summary>
        /// Returns a string that represents the current StreamSequenceNumber object.
        /// </summary>
        /// <returns>
        /// A string that represents the current StreamSequenceNumber object.
        /// </returns>
        /// <filterpriority>2</filterpriority>
        public override string ToString()
        {
            return this.NextSequenceNumber.ToString(CultureInfo.InvariantCulture);
        }
    }

    #endregion

    #region Receive/Send/Delete StreamSequenceNumber

    /// <summary>
    /// ReceiveStreamSequenceNumber Info
    /// </summary>
    internal class ReceiveStreamSequenceNumber : StreamSequenceNumber
    {
        internal ReceiveStreamSequenceNumber(long nextSequenceNumber)
            : base(nextSequenceNumber)
        {
            this.MetadataKind = StreamMetadataKind.ReceiveSequenceNumber;
        }
    }

    /// <summary>
    /// SendStreamSequenceNumber Info
    /// </summary>
    internal class SendStreamSequenceNumber : StreamSequenceNumber
    {
        internal SendStreamSequenceNumber(long nextSequenceNumber)
            : base(nextSequenceNumber)
        {
            this.MetadataKind = StreamMetadataKind.SendSequenceNumber;
        }
    }

    /// <summary>
    /// DeleteStreamSequenceNumber Info
    /// </summary>
    internal class DeleteStreamSequenceNumber : StreamSequenceNumber
    {
        internal DeleteStreamSequenceNumber(long nextSequenceNumber)
            : base(nextSequenceNumber)
        {
            this.MetadataKind = StreamMetadataKind.DeleteSequenceNumber;
        }
    }

    #endregion

    #region IByteConverter for StreamMetadataBody

    /// <summary>
    /// Provides overrides for IByteConverter.
    /// Converts the StreamMetadataBody object from byte array and vice versa. 
    /// </summary>
    internal class StreamMetadataBodyConverter : IStateSerializer<StreamMetadataBody>
    {
        public static StreamMetadataBodyConverter Converter = new StreamMetadataBodyConverter();

        /// <summary>
        /// Converts the StreamMetadataBody object to byte array
        /// </summary>
        /// <param name="value">StreamMetadataBody object</param>
        /// <param name="binaryWriter"></param>
        /// <returns>Byte array</returns>
        public void Write(StreamMetadataBody value, BinaryWriter binaryWriter)
        {
            switch (value.MetadataKind)
            {
                case StreamMetadataKind.ReceiveSequenceNumber:
                case StreamMetadataKind.SendSequenceNumber:
                case StreamMetadataKind.DeleteSequenceNumber:
                    var numberValue = (StreamSequenceNumber) value;
                    numberValue.ToByteArray(binaryWriter);
                    break;
                case StreamMetadataKind.OutboundStableParameters:
                    var outboundParamsValue = (OutboundStreamStableParameters) value;
                    outboundParamsValue.ToByteArray(outboundParamsValue, binaryWriter);
                    break;
                case StreamMetadataKind.InboundStableParameters:
                    var inboundParamsValue = (InboundStreamStableParameters) value;
                    inboundParamsValue.ToByteArray(inboundParamsValue, binaryWriter);
                    break;
                default:
                    Diagnostics.Assert(false, "Unknown Stream MetadataKind in StreamMetadataBodyConverter.ToByteArray()");
                    break;
            }
        }

        /// <summary>
        /// Assembles the StreamMetadataBody object from byte array
        /// </summary>
        /// <param name="reader">BinaryReader to deserialize from</param>
        /// <returns>StreamMetadataBody object</returns>
        public StreamMetadataBody Read(BinaryReader reader)
        {
            var metadataKind = (StreamMetadataKind) reader.ReadInt32();
            StreamMetadataBody result = null;

            switch (metadataKind)
            {
                case StreamMetadataKind.ReceiveSequenceNumber:
                case StreamMetadataKind.SendSequenceNumber:
                case StreamMetadataKind.DeleteSequenceNumber:
                    result = StreamSequenceNumber.FromByteArray(reader);
                    break;
                case StreamMetadataKind.OutboundStableParameters:
                    result = OutboundStreamStableParameters.FromByteArray(reader);
                    break;
                case StreamMetadataKind.InboundStableParameters:
                    result = InboundStreamStableParameters.FromByteArray(reader);
                    break;
                default:
                    Diagnostics.Assert(false, "Unknown Stream MetadataKind in StreamMetadataBody.FromByteArray()");
                    break;
            }

            return result;
        }

        /// <summary>
        /// Converts IEnumerable of byte[] to T
        /// </summary>
        /// <param name="baseValue"></param>
        /// <param name="reader">BinaryReader to deserialize from</param>
        /// <returns>Deserialized version of T.</returns>
        public StreamMetadataBody Read(StreamMetadataBody baseValue, BinaryReader reader)
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
        public void Write(StreamMetadataBody currentValue, StreamMetadataBody newValue, BinaryWriter binaryWriter)
        {
            this.Write(newValue, binaryWriter);
        }
    }

    #endregion
}