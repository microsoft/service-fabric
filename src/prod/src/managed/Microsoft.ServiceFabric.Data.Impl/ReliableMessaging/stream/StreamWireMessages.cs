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

    #endregion

    #region usefull enums

    /// <summary>
    /// Types of Protocol responses (returned values for a given request)
    /// </summary>
    internal enum StreamWireProtocolResponse
    {
        TargetNotAcceptingStreams = 100,
        TargetNotReady = 200,
        StreamAccepted = 300,
        StreamRejected = 400,
        StreamPartnerFaulted = 500,
        CloseStreamCompleted = 600,
        ResetPartnerStreamsCompleted = 700,
        TargetNotified = 800,
        StreamNotFound = 900,
        Unknown = -10000
    }

    /// <summary>
    /// Type of Protocol message kinds
    /// </summary>
    internal enum StreamWireProtocolMessageKind
    {
        ServiceData = 1,
        SequenceAck = 2,
        OpenStream = 3,
        CloseStream = 4,
        OpenStreamResponse = 5,
        CloseStreamResponse = 6,
        ResetPartnerStreams = 7,
        ResetPartnerStreamsResponse = 8,
        DeleteStream = 9,
        DeleteStreamResponse = 10,
        Unknown = -10000
    }

    #endregion

    #region helpfull structures

    /// <summary>
    /// Wire stream protocol version Info.
    /// </summary>
    internal struct StreamProtocolVersion
    {
        internal int MajorVersion;
        internal int MinorVersion;
    }

    #endregion

    #region base stream wire message info (Outbound and InboundStableParameters)

    /// <summary>
    /// Base Stream Wire Message Info
    /// </summary>
    internal abstract class BaseStreamWireMessage
    {
        // Stream Wire Message Header Info:
        // 1. Stream Identifier
        public Guid StreamIdentity { get; protected set; }

        // 2. Stream protocol version
        protected StreamProtocolVersion protocolVersion;

        internal StreamProtocolVersion ProtocolVersion
        {
            get { return this.protocolVersion; }
        }

        // 3. Message Kind
        protected internal StreamWireProtocolMessageKind Kind { get; protected set; }

        // 4. Message Seq. #
        public long MessageSequenceNumber { get; protected set; }

        // 5. Byte length
        protected static int EncodedByteLength
        {
            get { return (sizeof(int)*3) + StreamConstants.SizeOfGuid + sizeof(long); }
        }

        // Message payload Info:
        public byte[] Payload { get; protected set; }
    }

    /// <summary>
    /// OutboundBaseStreamWireMessage Info
    /// </summary>
    internal abstract class OutboundBaseStreamWireMessage : BaseStreamWireMessage, IOperationData
    {
        // Stores the encoded message header info.
        protected byte[] StreamHeaderBuffer;

        /// <summary>
        /// Cstr for OutboundBaseStreamWireMessage given
        /// Message Header Info: Protocol Kind, pertinent stream id and message sequence #
        /// </summary>
        /// <param name="kind">StreamWireProtocolMessageKind</param>
        /// <param name="streamId">Stream Id</param>
        /// <param name="sequenceNumber">Message Sequence number</param>
        protected OutboundBaseStreamWireMessage(StreamWireProtocolMessageKind kind, Guid streamId, long sequenceNumber)
        {
            // Stream Version
            this.protocolVersion.MajorVersion = StreamConstants.CurrentStreamMajorProtocolVersion;
            this.protocolVersion.MinorVersion = StreamConstants.CurrentStreamMinorProtocolVersion;

            // Message Info.
            this.Kind = kind;
            this.StreamIdentity = streamId;
            this.MessageSequenceNumber = sequenceNumber;
            this.Payload = null;

            // Encode
            this.EncodeHeaderBuffer();
        }

        /// <summary>
        /// Encode Header Info
        /// </summary>
        protected void EncodeHeaderBuffer()
        {
            this.StreamHeaderBuffer = new byte[EncodedByteLength];

            using (var stream = new MemoryStream(this.StreamHeaderBuffer))
            {
                stream.Write(BitConverter.GetBytes(this.ProtocolVersion.MajorVersion), 0, sizeof(int));
                stream.Write(BitConverter.GetBytes(this.ProtocolVersion.MinorVersion), 0, sizeof(int));

                stream.Write(BitConverter.GetBytes((int) this.Kind), 0, sizeof(int));
                stream.Write(this.StreamIdentity.ToByteArray(), 0, StreamConstants.SizeOfGuid);
                stream.Write(BitConverter.GetBytes(this.MessageSequenceNumber), 0, sizeof(long));
            }
        }

        /// <summary>
        /// Returns an enumerator that iterates through the collection of byte segments in streamHeaderBuffer
        /// this method's default implementation can be overridden
        /// </summary>
        /// <returns>
        /// A <see cref="System.Collections.IEnumerator"/> that can be used to iterate through the collection.
        /// </returns>
        /// <filterpriority>1</filterpriority>
        public virtual IEnumerator<ArraySegment<byte>> GetEnumerator()
        {
            yield return new ArraySegment<byte>(this.StreamHeaderBuffer);

            if (this.Payload != null)
            {
                yield return new ArraySegment<byte>(this.Payload);
            }

            yield break;
        }

        /// <summary>
        /// Returns an enumerator that iterates through a collection of byte segments in streamHeaderBuffer.
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
    /// InboundBaseStreamWireMessage Info. 
    /// </summary>
    internal class InboundBaseStreamWireMessage : BaseStreamWireMessage, IInboundStreamMessage
    {
        private ArraySegment<byte> headerSegment;

        /// <summary>
        /// Constructs a InboundBaseStreamWireMessage from the Wire message (Byte segments)
        /// </summary>
        /// <param name="wireMessage">Wire Message</param>
        internal InboundBaseStreamWireMessage(IOperationData wireMessage)
        {
            // Retreive message header
            var wireMessageSegmentEnumerator = wireMessage.GetEnumerator();
            var headerExists = wireMessageSegmentEnumerator.MoveNext();
            Diagnostics.Assert(headerExists, "InboundStableParameters wire message missing header segment");
            this.headerSegment = wireMessageSegmentEnumerator.Current;

            // is this the right check regardless of version?
            Diagnostics.Assert(this.headerSegment.Count == EncodedByteLength, "Header segment in stream wire message incorrect length");
            // Decode Header
            this.DecodeHeaderSegment();

            // Retreive message payload
            var segmentExists = wireMessageSegmentEnumerator.MoveNext();
            if (segmentExists)
            {
                var payloadSegment = wireMessageSegmentEnumerator.Current;

                // We rely on the fact that the managed session layer will return a whole byte array not a segment
                // The asserts verify that assumption
                this.Payload = payloadSegment.Array;
                Diagnostics.Assert(payloadSegment.Offset == 0, "Payload segment in inbound session message not whole array");
                Diagnostics.Assert(payloadSegment.Count == this.Payload.Length, "Payload segment in inbound session message not whole array");
            }
            else
            {
                this.Payload = null;
            }
        }

        /// <summary>
        ///  Decode Header segments.
        /// TODO: Verify  supported versions
        /// TODO: Verify  support type ex.
        /// TODO: the exceptions here need to be Asserts?
        /// </summary>
        private void DecodeHeaderSegment()
        {
            var cursor = this.headerSegment.Offset;
            this.protocolVersion.MajorVersion = BitConverter.ToInt32(this.headerSegment.Array, cursor);
            cursor += sizeof(int);
            this.protocolVersion.MinorVersion = BitConverter.ToInt32(this.headerSegment.Array, cursor);
            cursor += sizeof(int);
            this.Kind = (StreamWireProtocolMessageKind) BitConverter.ToInt32(this.headerSegment.Array, cursor);
            cursor += sizeof(int);
            var guidBuffer = new byte[StreamConstants.SizeOfGuid];
            Array.Copy(this.headerSegment.Array, cursor, guidBuffer, 0, StreamConstants.SizeOfGuid);
            this.StreamIdentity = new Guid(guidBuffer);
            cursor += StreamConstants.SizeOfGuid;
            this.MessageSequenceNumber = BitConverter.ToInt64(this.headerSegment.Array, cursor);
        }
    }

    #endregion

    #region All Outbound wire messages (Concrete)

    /// <summary>
    /// Open stream wire message
    /// </summary>
    internal class OutboundOpenStreamWireMessage : OutboundBaseStreamWireMessage
    {
        private Uri streamName;

        // outbound ctor
        internal OutboundOpenStreamWireMessage(Guid streamId, Uri streamName)
            : base(StreamWireProtocolMessageKind.OpenStream, streamId, 0)
        {
            this.streamName = streamName;

            // TODO: Check if we need to throw instead, as this replica
            Diagnostics.Assert((streamId != Guid.Empty), "Stream Id in Outbound Open stream wire message is empty.");
            Diagnostics.Assert((streamName != null), "Stream Name in Open stream wire message is null.");

            // Stream name
            var encoder = new StringEncoder(streamName.OriginalString);
            var streamNameByteLength = encoder.EncodingByteCount;
            // Payload
            this.Payload = new byte[streamNameByteLength];

            var writer = new BinaryWriter(new MemoryStream());
            encoder.Encode(writer);
        }
    }

    /// <summary>
    /// Close stream wire message
    /// </summary>
    internal class OutboundCloseStreamWireMessage : OutboundBaseStreamWireMessage
    {
        // cstor
        internal OutboundCloseStreamWireMessage(Guid streamId, long sequenceNumber)
            : base(StreamWireProtocolMessageKind.CloseStream, streamId, sequenceNumber)
        {
        }
    }

    /// <summary>
    /// Delete stream wire message
    /// </summary>
    internal class OutboundDeleteStreamWireMessage : OutboundBaseStreamWireMessage
    {
        // cstor
        internal OutboundDeleteStreamWireMessage(Guid streamId)
            : base(StreamWireProtocolMessageKind.DeleteStream, streamId, 0)
        {
        }
    }

    /// <summary>
    /// Reset Partner stream wire message
    /// we are cheating and using the StreamId slot to carry the StreamManager era of the ResetPartnerStreams requester
    /// </summary>
    internal class OutboundResetPartnerStreamsWireMessage : OutboundBaseStreamWireMessage
    {
        internal OutboundResetPartnerStreamsWireMessage(Guid era)
            : base(StreamWireProtocolMessageKind.ResetPartnerStreams, era, StreamConstants.PartnerRecoveryPromptMessageSequenceNumber)
        {
            if (era == Guid.Empty)
            {
                // we seem to have made a transition to not primary
                throw new FabricNotPrimaryException();
            }
        }
    }

    /// <summary>
    /// Now that the Stream is open, send the payload using the OutboundStreamPayloadMessage
    /// Used with the session layer
    /// </summary>
    internal class OutboundStreamPayloadMessage : OutboundBaseStreamWireMessage
    {
        internal OutboundStreamPayloadMessage(Guid streamId, long sequenceNumber, StreamStoreMessageBody body)
            : base(StreamWireProtocolMessageKind.ServiceData, streamId, sequenceNumber)
        {
            Diagnostics.Assert((streamId != Guid.Empty), "Stream Id in Outbound  Stream payload wire message is empty.");
            Diagnostics.Assert((body != null), "Stream Message Body in Outbound Stream payload wire Message message is null.");

            this.Payload = body.Payload;
        }
    }

    /// <summary>
    /// Send an Ack, Now that the payload(Message of sequenceNumber for a particular streamId) has been received.
    /// Used with the session layer
    /// </summary>
    internal class OutboundStreamPayloadAckMessage : OutboundBaseStreamWireMessage
    {
        internal OutboundStreamPayloadAckMessage(Guid streamId, long sequenceNumber)
            : base(StreamWireProtocolMessageKind.SequenceAck, streamId, sequenceNumber)
        {
            Diagnostics.Assert((streamId != Guid.Empty), "Stream Id in Outbound Payload Ack stream wire message is empty.");
        }
    }

    #endregion

    #region Outbound Protocol Responses

    /// <summary>
    /// Base Outbound protocol response wire message to encode the appropriate response.
    /// </summary>
    internal class OutboundProtocolResponseWireMessage : OutboundBaseStreamWireMessage
    {
        internal OutboundProtocolResponseWireMessage(Guid streamId, StreamWireProtocolMessageKind messageKind, StreamWireProtocolResponse response)
            : base(messageKind, streamId, StreamConstants.StreamProtocolResponseMessageSequenceNumber)
        {
            this.Payload = BitConverter.GetBytes((int) response);
        }
    }

    /// <summary>
    /// Outbound open stream protocol response 
    /// </summary>
    internal class OutboundOpenStreamResponseWireMessage : OutboundProtocolResponseWireMessage
    {
        internal OutboundOpenStreamResponseWireMessage(Guid streamId, StreamWireProtocolResponse response)
            : base(streamId, StreamWireProtocolMessageKind.OpenStreamResponse, response)
        {
        }
    }

    /// <summary>
    /// Outbound close stream protocol response 
    /// </summary>
    internal class OutboundCloseStreamResponseWireMessage : OutboundProtocolResponseWireMessage
    {
        internal OutboundCloseStreamResponseWireMessage(Guid streamId, StreamWireProtocolResponse response)
            : base(streamId, StreamWireProtocolMessageKind.CloseStreamResponse, response)
        {
        }
    }

    /// <summary>
    /// Reset Partner stream protocol response
    /// we are cheating and using the StreamId slot to carry the StreamManager era of the ResetPartnerStreams requester
    /// </summary>
    internal class OutboundResetPartnerStreamsResponseWireMessage : OutboundProtocolResponseWireMessage
    {
        internal OutboundResetPartnerStreamsResponseWireMessage(Guid era, StreamWireProtocolResponse response)
            : base(era, StreamWireProtocolMessageKind.ResetPartnerStreamsResponse, response)
        {
        }
    }

    /// <summary>
    /// Outbound delete stream protocol response 
    /// </summary>
    internal class OutboundDeleteStreamResponseWireMessage : OutboundProtocolResponseWireMessage
    {
        internal OutboundDeleteStreamResponseWireMessage(Guid streamId, StreamWireProtocolResponse response)
            : base(streamId, StreamWireProtocolMessageKind.DeleteStreamResponse, response)
        {
        }
    }

    #endregion

    #region InboundStableParameters Open Stream Name

    /// <summary>
    /// Validates the stores the Stream Name
    /// </summary>
    internal class InboundOpenStreamName
    {
        internal Uri StreamName { get; private set; }

        internal InboundOpenStreamName(InboundBaseStreamWireMessage inboundBaseMessage)
        {
            Diagnostics.Assert(
                inboundBaseMessage.Kind == StreamWireProtocolMessageKind.OpenStream,
                "Wrong kind of InboundBaseStreamWireMessage used to construct InboundOpenStreamName");
            Diagnostics.Assert(
                inboundBaseMessage.Payload != null,
                "null streamName payload in InboundBaseStreamWireMessage used to construct InboundOpenStreamName");

            var encoder = new StringEncoder();
            var reader = new BinaryReader(new MemoryStream(inboundBaseMessage.Payload));
            this.StreamName = new Uri(encoder.Decode(reader));
            Diagnostics.Assert(reader.PeekChar() == -1, "streamName payload in InboundBaseStreamWireMessage contains more than streamName");
        }
    }

    #endregion

    #region InboundStableParameters Protocol Response Value

    /// <summary>
    /// Validates and store the protocol response.
    /// </summary>
    internal class InboundProtocolResponseValue
    {
        // Wire message protocol response.
        internal StreamWireProtocolResponse Response { get; private set; }

        /// <summary>
        /// Cstor for InboundProtocolResponseValue
        /// Validate the payload for valid protocol response and store.
        /// </summary>
        /// <param name="inboundStreamMessage"></param>
        /// <param name="expectedType"></param>
        internal InboundProtocolResponseValue(InboundBaseStreamWireMessage inboundStreamMessage, StreamWireProtocolMessageKind expectedType)
        {
            Diagnostics.Assert(
                inboundStreamMessage.Kind == expectedType,
                "Unexpected StreamWireProtocolMessageKind in InboundBaseStreamWireMessage used to construct InboundProtocolResponseValue");
            Diagnostics.Assert(
                inboundStreamMessage.Payload != null,
                "null ProtocolResponseValue payload in InboundBaseStreamWireMessage used to construct InboundProtocolResponseValue");
            Diagnostics.Assert(
                inboundStreamMessage.Payload.Length == sizeof(int),
                "payload in InboundBaseStreamWireMessage used to construct InboundProtocolResponseValue contains more than ProtocolResponseValue");

            // Store appropriate decoded Protocol Response
            this.Response = (StreamWireProtocolResponse) BitConverter.ToInt32(inboundStreamMessage.Payload, 0);
        }
    }

    /// <summary>
    /// InboundStableParameters Open Protocol response value
    /// </summary>
    internal class InboundOpenStreamResponseValue : InboundProtocolResponseValue
    {
        internal InboundOpenStreamResponseValue(InboundBaseStreamWireMessage inboundStreamMessage)
            : base(inboundStreamMessage, StreamWireProtocolMessageKind.OpenStreamResponse)
        {
        }
    }

    /// <summary>
    /// InboundStableParameters Close Protocol response value
    /// </summary>
    internal class InboundCloseStreamResponseValue : InboundProtocolResponseValue
    {
        internal InboundCloseStreamResponseValue(InboundBaseStreamWireMessage inboundStreamMessage)
            : base(inboundStreamMessage, StreamWireProtocolMessageKind.CloseStreamResponse)
        {
        }
    }

    /// <summary>
    /// InboundStableParameters Delete Protocol response value
    /// </summary>
    internal class InboundDeleteStreamResponseValue : InboundProtocolResponseValue
    {
        internal InboundDeleteStreamResponseValue(InboundBaseStreamWireMessage inboundStreamMessage)
            : base(inboundStreamMessage, StreamWireProtocolMessageKind.DeleteStreamResponse)
        {
        }
    }

    /// <summary>
    /// InboundStableParameters Reset Partner Protocol response value
    /// </summary>
    internal class InboundResetPartnerStreamsResponseValue : InboundProtocolResponseValue
    {
        internal InboundResetPartnerStreamsResponseValue(InboundBaseStreamWireMessage inboundStreamMessage)
            : base(inboundStreamMessage, StreamWireProtocolMessageKind.ResetPartnerStreamsResponse)
        {
        }
    }

    #endregion
}