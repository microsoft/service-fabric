// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Reliability::ReplicationComponent;
using namespace Transport;
using namespace Common;
using namespace std;

namespace TransportTraceTagPrefix
{
    static Common::GlobalWString ReplicationAck = Common::make_global<std::wstring>(L"Ack");
    static Common::GlobalWString ReplicationOperation = Common::make_global<std::wstring>(L"R");
    static Common::GlobalWString CopyOperation = Common::make_global<std::wstring>(L"C");
    static Common::GlobalWString CopyContextOperation = Common::make_global<std::wstring>(L"CC");
    static Common::GlobalWString StartCopy = Common::make_global<std::wstring>(L"SC");
    static Common::GlobalWString CopyContextAck = Common::make_global<std::wstring>(L"CCAck");
    static Common::GlobalWString RequestAck = Common::make_global<std::wstring>(L"RAck");
    static Common::GlobalWString InduceFault = Common::make_global<std::wstring>(L"InduceFault");
}

GlobalWString ReplicationTransport::ReplicationAckAction = make_global<std::wstring>(L"ReplicationAck");
GlobalWString ReplicationTransport::ReplicationOperationAction = make_global<std::wstring>(L"ReplicationOperation");
GlobalWString ReplicationTransport::ReplicationOperationBodyAction = make_global<std::wstring>(L"ReplicationOperationBody");
GlobalWString ReplicationTransport::CopyOperationAction = make_global<std::wstring>(L"CopyOperation");
GlobalWString ReplicationTransport::CopyContextOperationAction = make_global<std::wstring>(L"CopyContextOperation");
GlobalWString ReplicationTransport::CopyContextAckAction = make_global<std::wstring>(L"CopyContextAckAction");
GlobalWString ReplicationTransport::StartCopyAction = make_global<std::wstring>(L"StartCopy");
GlobalWString ReplicationTransport::RequestAckAction = make_global<std::wstring>(L"RequestAck");
GlobalWString ReplicationTransport::InduceFaultAction = make_global<std::wstring>(L"InduceFault");

ReplicationTransport::ReplicationTransport(
    wstring const & endpoint, 
    wstring const & id) :
    unicastTransport_(DatagramTransportFactory::CreateTcp(endpoint, id, L"Replication")),
    endpoint_(),
    publishEndpoint_(),
    demuxer_(),
    securitySettings_(),
    securitySettingsLock_(),
    maxMessageSize_(0),
    maxPendingUnsentBytes_(-1)
{
    REGISTER_MESSAGE_HEADER(ReplicationActorHeader);
    REGISTER_MESSAGE_HEADER(ReplicationOperationHeader);
    REGISTER_MESSAGE_HEADER(ReplicationOperationBodyHeader)
    REGISTER_MESSAGE_HEADER(CopyOperationHeader);
    REGISTER_MESSAGE_HEADER(ReplicationFromHeader);
    REGISTER_MESSAGE_HEADER(CopyContextOperationHeader);
    REGISTER_MESSAGE_HEADER(OperationAckHeader);
    REGISTER_MESSAGE_HEADER(OperationErrorHeader);

#ifdef PLATFORM_UNIX
    unicastTransport_->SetEventLoopReadDispatch(false);
    unicastTransport_->SetEventLoopWriteDispatch(false);
#endif
    unicastTransport_->DisableThrottle();
    unicastTransport_->DisableAllPerMessageTraces();

    // ReplicationTransport's root isn't a good root for the demuxer since the lifetime's don't match up
    this->demuxer_ = Common::make_unique<ReplicationDemuxer>(*this, unicastTransport_);
}

ReplicationTransport::~ReplicationTransport()
{
    Stop();
}

std::shared_ptr<ReplicationTransport> ReplicationTransport::Create(wstring const & address, wstring const& id)
{
    auto transportSPtr = shared_ptr<ReplicationTransport>(
        new ReplicationTransport(address, id));

    // all messages now pass through unreliable transport.
    // Which will be bypassed in case there is no unreliable transport behavior specified.    
    transportSPtr->unicastTransport_ = DatagramTransportFactory::CreateUnreliable(*transportSPtr, transportSPtr->unicastTransport_);   

    return transportSPtr;
}

ErrorCode ReplicationTransport::SetSecurity(SecuritySettings const& securitySettings)
{
    SecuritySettings securitySettingsCopy(securitySettings);
    securitySettingsCopy.EnablePeerToPeerMode();

    if (SecurityProvider::IsWindowsProvider(securitySettingsCopy.SecurityProvider())
        && securitySettingsCopy.RemoteSpn().empty())
    {
        TransportSecurity transportSecurity;
        if (!transportSecurity.RunningAsMachineAccount())
        {
            // Not running as machine account, thus all replicas must be running as the same account.
            // So, when RemoteSpn is not set, we can fall back on downlevel logon name of this process.
            securitySettingsCopy.SetRemoteSpn(transportSecurity.LocalWindowsIdentity());
        }
    }

    auto error = this->unicastTransport_->SetSecurity(securitySettingsCopy);
    if (error.IsSuccess())
    {
        // Remember the security settings 
        AcquireWriteLock grab(securitySettingsLock_);
        this->securitySettings_ = securitySettingsCopy;
    }

    return error;
}

bool ReplicationTransport::SecuritySettingsMatch(SecuritySettings const & securitySettings) const
{
    AcquireReadLock grab(securitySettingsLock_);
    return this->securitySettings_ == securitySettings;
}

ErrorCode ReplicationTransport::Start(
    __in wstring const & publishAddress,
    __in_opt SecuritySettings const & securitySettings)
{
    auto errorCode = this->SetSecurity(securitySettings);
    if (!errorCode.IsSuccess())
    {
        return errorCode;
    }

    errorCode = this->demuxer_->Open();
    if (!errorCode.IsSuccess())
    {
        return errorCode;
    }

    errorCode = this->unicastTransport_->Start();
    if (errorCode.IsSuccess())
    {
        this->endpoint_ = unicastTransport_->ListenAddress();

        publishEndpoint_ = L""; // In case Start is called multiple times on the same object
        Common::StringWriter writer(publishEndpoint_);
        USHORT port = TcpTransportUtility::ParsePortString(endpoint_);
        std::wstring host = TcpTransportUtility::ParseHostString(publishAddress);
        auto endpoint = TcpTransportUtility::ConstructAddressString(host, port);

        writer << endpoint;

        ReplicatorEventSource::Events->TransportStart(
            endpoint_,
            publishEndpoint_);
    }

    return errorCode;
}

void ReplicationTransport::Stop()
{
    ReplicatorEventSource::Events->TransportStop(endpoint_);
    
    this->demuxer_->Abort();
    this->unicastTransport_->Stop();
}

ISendTarget::SPtr ReplicationTransport::ResolveTarget(std::wstring const & endpoint, std::wstring const& id)
{
    return unicastTransport_->ResolveTarget(endpoint, id);
}

MessageUPtr ReplicationTransport::CreateAckMessage(
    FABRIC_SEQUENCE_NUMBER replicationReceivedLSN, 
    FABRIC_SEQUENCE_NUMBER replicationQuorumLSN,
    FABRIC_SEQUENCE_NUMBER copyReceivedLSN,
    FABRIC_SEQUENCE_NUMBER copyQuorumLSN,
    int errorCodeValue)
{
    MessageUPtr message = Common::make_unique<Transport::Message>(ReplicationAckMessageBody(
        replicationReceivedLSN, 
        replicationQuorumLSN, 
        copyReceivedLSN, 
        copyQuorumLSN));

    message->Headers.Add(MessageIdHeader());

    if (errorCodeValue != 0)
    {
        message->Headers.Add(OperationErrorHeader(errorCodeValue));
    }

    if (Constants::NonInitializedLSN == copyReceivedLSN &&
        Constants::NonInitializedLSN == copyQuorumLSN)
    {
        // If copy is done, use only replication LSN's in trace
        message->SetLocalTraceContext(move(wformatString("{0}:{1},{2}", TransportTraceTagPrefix::ReplicationAck, replicationReceivedLSN, replicationQuorumLSN)));
    }
    else
    {
        message->SetLocalTraceContext(move(wformatString("{0}:{1},{2}:{3},{4}", TransportTraceTagPrefix::ReplicationAck, replicationReceivedLSN, replicationQuorumLSN, copyReceivedLSN, copyQuorumLSN)));
    }

    return move(message);
}

void ReplicationTransport::GetAckFromMessage(
    __in Message & message, 
    __out FABRIC_SEQUENCE_NUMBER & replicationReceivedLSN, 
    __out FABRIC_SEQUENCE_NUMBER & replicationQuorumLSN,
    __out FABRIC_SEQUENCE_NUMBER & copyReceivedLSN,
    __out FABRIC_SEQUENCE_NUMBER & copyQuorumLSN,
    __out int & errorCodeValue)
{
    OperationErrorHeader header;
    if (message.Headers.TryReadFirst(header))
    {
        errorCodeValue = header.ErrorCodeValue;
    }
    else
    {
        errorCodeValue = 0;
    }
    
    ReplicationAckMessageBody body;
    bool isBodyValid = message.GetBody<ReplicationAckMessageBody>(body);
    ASSERT_IF(!isBodyValid, "GetBody() in GetAckFromMessage failed with message status {0}", message.Status);
    replicationReceivedLSN = body.ReplicationReceivedLSN;
    replicationQuorumLSN = body.ReplicationQuorumLSN;
    copyReceivedLSN = body.CopyReceivedLSN;
    copyQuorumLSN = body.CopyQuorumLSN;
}

Transport::MessageUPtr ReplicationTransport::CreateCopyContextAckMessage(
    FABRIC_SEQUENCE_NUMBER sequenceNumber,
    int errorCodeValue)
{
    MessageUPtr message = Common::make_unique<Transport::Message>(AckMessageBody(sequenceNumber));

    message->Headers.Add(MessageIdHeader());

    if (errorCodeValue != 0)
    {
        message->Headers.Add(OperationErrorHeader(errorCodeValue));
    }

    message->SetLocalTraceContext(move(wformatString("{0}:{1}", TransportTraceTagPrefix::CopyContextAck, sequenceNumber)));

    return move(message);
}

void ReplicationTransport::GetCopyContextAckFromMessage(
    __in Transport::Message & message, 
    __out FABRIC_SEQUENCE_NUMBER & sequenceNumber,
    __out int & errorCodeValue)
{
    OperationErrorHeader header;
    if (message.Headers.TryReadFirst(header))
    {
        errorCodeValue = header.ErrorCodeValue;
    }
    else
    {
        errorCodeValue = 0;
    }
    
    AckMessageBody ackBody;
    bool isBodyValid = message.GetBody(ackBody);
    ASSERT_IF(!isBodyValid, "GetBody() in GetCopyContextAckFromMessage failed with message status {0}", message.Status);
    sequenceNumber = ackBody.SequenceNumber;
}

MessageUPtr ReplicationTransport::CreateReplicationOperationMessage(
    ComOperationCPtr const & operation, 
    FABRIC_SEQUENCE_NUMBER lastSequenceNumberInBatch,
    FABRIC_EPOCH const & epoch,
    bool enableReplicationOperationHeaderInBody,
    FABRIC_SEQUENCE_NUMBER completedSequenceNumber)
{
    ASSERT_IFNOT(operation, "CreateReplicationOperationMessage: Null replication operation not allowed");

    ULONG bufferCount;
    FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = nullptr;
    operation->GetData(&bufferCount, &replicaBuffers);

    FABRIC_SEQUENCE_NUMBER sequenceNumber = operation->SequenceNumber;
    vector<Common::const_buffer> buffers;
    vector<ULONG> segmentSizes;
    vector<ULONG> bufferCounts(1, bufferCount);

    for (ULONG i = 0; i < bufferCount; ++i)
    {
        buffers.push_back(Common::const_buffer(replicaBuffers[i].Buffer, replicaBuffers[i].BufferSize));
        segmentSizes.push_back(replicaBuffers[i].BufferSize);
    }

    ReplicationOperationHeader opHeader(
        operation->Metadata,
        epoch,
        operation->Epoch,
        std::move(segmentSizes),
        sequenceNumber,
        sequenceNumber,
        lastSequenceNumberInBatch,
        std::move(bufferCounts),
        completedSequenceNumber);
    
    //serializing the replication operation header to bytes
    shared_ptr<vector<BYTE>> replicationOperationBodyHeaderBuffer = nullptr;
    if (enableReplicationOperationHeaderInBody)
    {
        replicationOperationBodyHeaderBuffer = make_shared<vector<BYTE>>(vector<BYTE>());
        FabricSerializer::Serialize(&opHeader, *replicationOperationBodyHeaderBuffer);
    
        //adding the replicationOperationHeader as the first buffer.
        buffers.insert(buffers.begin(), Common::const_buffer(&(*replicationOperationBodyHeaderBuffer)[0], replicationOperationBodyHeaderBuffer->size()));
    }

    void * state = nullptr;

    ComOperationCPtr copy(operation);
    MoveCPtr<ComOperation> mover(move(copy));
    MessageUPtr message = Common::make_unique<Message>(
        buffers,
        [mover, sequenceNumber, lastSequenceNumberInBatch, replicationOperationBodyHeaderBuffer] (vector<Common::const_buffer> const & buffers, void *)
        {
            //replicationOperationBodyHeaderBuffer is captured as its value is placed in the buffers that are sent out with the message
            //if not captures, it could be garbage collected and no correct message buffer will be sent.
            size_t size = 0;
            for (auto const & buffer : buffers)
            {
                size += buffer.size();
            }

            ReplicatorEventSource::Events->TransportMsgBatchCallback(
                Constants::ReplOperationTrace,
                sequenceNumber, 
                sequenceNumber, 
                lastSequenceNumberInBatch, 
                static_cast<uint64>(size));
        },
        state);

    //if allowed to add replication operation header in the body, then add the bodyHeader with the header size that is put in the body buffer
    //if not, add the operation header completely in the header
    if (enableReplicationOperationHeaderInBody)
    {
        ASSERT_IF(replicationOperationBodyHeaderBuffer == nullptr, "replication operation header buffer cannot be null");
        message->Headers.Add(ReplicationOperationBodyHeader(static_cast<ULONG>(replicationOperationBodyHeaderBuffer->size())));
    }
    else
    {
        message->Headers.Add(opHeader);
    }
    
    message->Headers.Add(MessageIdHeader());

    message->SetLocalTraceContext(move(wformatString("{0}:{1}", TransportTraceTagPrefix::ReplicationOperation, sequenceNumber)));

    return move(message);
}

bool ReplicationTransport::ReadOperationHeaderInBodyBytes(
    __in Transport::Message & message,
    __inout vector<Common::const_buffer> & msgBuffers,
    __out vector<byte> & bodyHeaderBytes)
{
    bool bodyHeaderDetected = false;
    ReplicationOperationBodyHeader bodyHeader;

    if (message.Headers.TryReadFirst(bodyHeader))
    {
        if (bodyHeader.ReplicationOperationBodyHeaderSize == 0)
        {
            Common::Assert::TestAssert("in-body operation header size cannot be 0");
            return false;
        }
        if (msgBuffers.size() > 0)
        {
            ULONG remainingBytes = bodyHeader.ReplicationOperationBodyHeaderSize;
            while (remainingBytes > 0 && msgBuffers.size() > 0)
            {
                ULONG bufferSize = static_cast<ULONG>(msgBuffers[0].size());

                if (remainingBytes >= bufferSize)
                {
                    bodyHeaderBytes.insert(bodyHeaderBytes.end(), msgBuffers[0].buf, msgBuffers[0].buf + bufferSize);
                    msgBuffers.erase(msgBuffers.begin());
                    remainingBytes -= bufferSize;
                }
                else
                {
                    bodyHeaderBytes.insert(bodyHeaderBytes.end(), msgBuffers[0].buf, msgBuffers[0].buf + remainingBytes);
                    msgBuffers[0] = const_buffer(&msgBuffers[0].buf[remainingBytes], msgBuffers[0].size() - remainingBytes);
                    remainingBytes = 0;
                }
            }

            //if there are still remaining bytes for the header to read but no more buffers are available, return false
            if (remainingBytes > 0)
            {
                ReplicatorEventSource::Events->ReplicationHeaderMissing(L"in-body operation header bytes are not received completely.");
                Common::Assert::TestAssert("in-body operation header bytes are not received completely.");
                return false;
            }

            bodyHeaderDetected = true;
        }
    }

    return bodyHeaderDetected;
}

bool ReplicationTransport::GetReplicationBatchOperationFromMessage(
    __in Message & message, 
    OperationAckCallback const & ackCallback,
    __out std::vector<ComOperationCPtr> & batchOperation, 
    __out FABRIC_EPOCH & epoch,
    __out FABRIC_SEQUENCE_NUMBER & completedSequenceNumber)
{
    //
    // Process ReplicationOperationHeader
    //
    vector<const_buffer> msgBuffers;
    bool isBodyValid = message.GetBody(msgBuffers);
    ASSERT_IF(!isBodyValid, "GetBody() in GetReplicationOperationFromMessage failed with message status {0}", message.Status);
    
    ReplicationOperationHeader header;
    bool bodyHeaderDetected = ReadInBodyOperationHeader(message, msgBuffers, header);

    if (bodyHeaderDetected || message.Headers.TryReadFirst(header))
    {
        vector<ULONG> segmentSizes = std::move(header.SegmentSizes);
        vector<ULONG> bufferCounts = std::move(header.BufferCounts);

        FABRIC_SEQUENCE_NUMBER firstSequenceNumber = header.FirstSequenceNumber;
        FABRIC_SEQUENCE_NUMBER lastSequenceNumber = header.LastSequenceNumber;
        FABRIC_SEQUENCE_NUMBER lastSequenceNumberInBatch = header.LastSequenceNumberInBatch;
        FABRIC_OPERATION_METADATA metadata = header.OperationMetadata;
        completedSequenceNumber = header.CompletedSequenceNumber;

        if (Constants::InvalidLSN == firstSequenceNumber) // received from V1 replicator
        {
            //
            // populate V1+ values, treated as batch of one
            //

            firstSequenceNumber = metadata.SequenceNumber;
            lastSequenceNumber = firstSequenceNumber;
            lastSequenceNumberInBatch = lastSequenceNumber;

            //
            // all segments belong to the same operation
            //

            bufferCounts.clear();
            bufferCounts.push_back(static_cast<ULONG>(segmentSizes.size()));
        }

        if (lastSequenceNumber < firstSequenceNumber || 
            (int)bufferCounts.size() != (lastSequenceNumber - firstSequenceNumber + 1))
        {
            return false;
        }

        if (metadata.SequenceNumber != firstSequenceNumber)
        {
            return false;
        }

        size_t start = 0;
        size_t bufferStart = 0;
        int bufferIndex = 0;

        for (FABRIC_SEQUENCE_NUMBER lsn = firstSequenceNumber; lsn <= lastSequenceNumber; lsn++)
        {
            ULONG bufferCount = bufferCounts[static_cast<ULONG>(lsn - firstSequenceNumber)];
            vector<ULONG> segment(bufferCount);
            if (start + bufferCount > segmentSizes.size())
            {
                return false;
            }

            std::swap_ranges(segmentSizes.begin()+start, segmentSizes.begin()+start+bufferCount, segment.begin());
            start += bufferCount;

            size_t totalSize = 0;
            for (auto it = segment.begin(); it != segment.end(); it++)
            {
                totalSize += *it;
            }

            vector<BYTE> buffer;
            while (totalSize > 0 && bufferIndex < (int)msgBuffers.size())
            {
                if ((msgBuffers[bufferIndex].len - bufferStart) > totalSize)
                {
                    buffer.insert(buffer.end(), msgBuffers[bufferIndex].buf + bufferStart, msgBuffers[bufferIndex].buf + bufferStart + totalSize);
                    bufferStart += totalSize;
                    totalSize = 0;
                }
                else
                {
                    buffer.insert(buffer.end(), msgBuffers[bufferIndex].buf + bufferStart, msgBuffers[bufferIndex].buf + msgBuffers[bufferIndex].len);
                    totalSize -= (msgBuffers[bufferIndex].len - bufferStart);
                    bufferStart = 0;
                    bufferIndex++;
                }
            }

            if (totalSize > 0)
            {
                return false; // no enough data in msgBuffers
            }

            metadata.SequenceNumber = lsn;
            ComOperationCPtr operation = make_com<ComFromBytesOperation, ComOperation>(
                std::move(buffer),
                segment,
                metadata,
                header.OperationEpoch,
                ackCallback,
                lastSequenceNumberInBatch);

            batchOperation.push_back(std::move(operation));
        } // end for

        epoch = header.PrimaryEpoch;

        return true;
    }
    
    ReplicatorEventSource::Events->ReplicationHeaderMissing(L"replication header is not found in neither the header nor the body.");
    return false;
}

MessageUPtr ReplicationTransport::CreateMessageFromCopyOperation(
    ComOperationCPtr const & operation,
    vector<ULONG> & segmentSizes)
{
    const wstring NullOperationTraceString(L"Null Operation");
    const wstring EmptyOperationTraceString(L"Empty Operation");
    MessageUPtr message;

    if (operation->IsEmpty())
    {
        message = Common::make_unique<Transport::Message>();
        // Though the operation is "IsEmpty()", it means here that the underlying operation pointer is NULL.
        ReplicatorEventSource::Events->TransportSendCopyOperation(NullOperationTraceString);
    }
    else
    {
        ULONG bufferCount;
        FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = nullptr;
        operation->GetData(&bufferCount, &replicaBuffers);

        if (bufferCount == 0)
        {
            message = Common::make_unique<Transport::Message>();
            ReplicatorEventSource::Events->TransportSendCopyOperation(EmptyOperationTraceString);
        }
        else
        {
            FABRIC_SEQUENCE_NUMBER sequenceNumber = operation->SequenceNumber;

            vector<Common::const_buffer> buffers;

            for (ULONG i = 0; i < bufferCount; ++i)
            {
                buffers.push_back(Common::const_buffer(replicaBuffers[i].Buffer, replicaBuffers[i].BufferSize));
                segmentSizes.push_back(replicaBuffers[i].BufferSize);
            }

            void * state = nullptr;
            ComOperationCPtr copy(operation);
            MoveCPtr<ComOperation> mover(move(copy));
            message = Common::make_unique<Message>(
                buffers,
                [mover, sequenceNumber] (vector<Common::const_buffer> const & buffers, void *)
            {
                size_t size = 0;
                for (auto const & buffer : buffers)
                {
                    size += buffer.size();
                }

                ReplicatorEventSource::Events->TransportMsgCallback(
                    Constants::CopyOperationTrace,
                    sequenceNumber,
                    static_cast<uint64>(size));
            },
                state);
        }
    }

    return move(message);
}

MessageUPtr ReplicationTransport::CreateMessageFromCopyOperationWithHeaderInBody(
    ComOperationCPtr const & operation,
    FABRIC_REPLICA_ID replicaId,
    FABRIC_EPOCH const & epoch,
    bool isLast)
{
    MessageUPtr message;
    
    shared_ptr<vector<BYTE>> copyOperationHeaderInBodyBuffer = nullptr;

    ULONG bufferCount;
    FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = nullptr;
    vector<Common::const_buffer> buffers;
    vector<ULONG> segmentSizes;
    
    FABRIC_SEQUENCE_NUMBER sequenceNumber;

    bool isNullOperation = false;
    bool isEmptyOperation = false;

    // Though the operation is "IsEmpty()", it means here that the underlying operation pointer is NULL.
    if (!operation->IsEmpty())
    {
        isNullOperation = true;

        operation->GetData(&bufferCount, &replicaBuffers);
        if (bufferCount != 0)
        {
            sequenceNumber = operation->SequenceNumber;

            for (ULONG i = 0; i < bufferCount; ++i)
            {
                buffers.push_back(Common::const_buffer(replicaBuffers[i].Buffer, replicaBuffers[i].BufferSize));
                segmentSizes.push_back(replicaBuffers[i].BufferSize);
            }
            
            isEmptyOperation = true;
            isNullOperation = false;
        }
    }

    CopyOperationHeader copyOperationHeader(replicaId, epoch, operation->Metadata, std::move(segmentSizes), isLast);
    copyOperationHeaderInBodyBuffer = make_shared<vector<BYTE>>(vector<BYTE>());
    
    //serializing the copy operation header to bytes
    FabricSerializer::Serialize(&copyOperationHeader, *copyOperationHeaderInBodyBuffer);

    //adding the copyOperationHeader as the first buffer.
    buffers.insert(buffers.begin(), Common::const_buffer(&(*copyOperationHeaderInBodyBuffer)[0], copyOperationHeaderInBodyBuffer->size()));

    void * state = nullptr;
    ComOperationCPtr copy(operation);
    MoveCPtr<ComOperation> mover(move(copy));
    message = Common::make_unique<Message>(
        buffers,
        [mover, sequenceNumber, isNullOperation, isEmptyOperation, copyOperationHeaderInBodyBuffer](vector<Common::const_buffer> const & buffers, void *)
    {
        //copyOperationHeaderInBodyBuffer is captured as its value is placed in the buffers that are sent out with the message
        //if not captured, it could be garbage collected and no correct message buffer will be sent.
        size_t size = 0;
        for (auto const & buffer : buffers)
        {
            size += buffer.size();
        }
        if (isNullOperation)
        {
            ReplicatorEventSource::Events->TransportSendCopyOperation(L"Null Operation");
        }
        else if (isEmptyOperation)
        {
            ReplicatorEventSource::Events->TransportSendCopyOperation(L"Empty Operation");
        }
        else
        {
            ReplicatorEventSource::Events->TransportMsgCallback(
                Constants::CopyOperationTrace,
                sequenceNumber,
                static_cast<uint64>(size));
        }
    },
        state);

    message->Headers.Add(MessageIdHeader());

    ASSERT_IF(copyOperationHeaderInBodyBuffer == nullptr, "copy operation header buffer cannot be null");
    message->Headers.Add(ReplicationOperationBodyHeader(static_cast<ULONG>(copyOperationHeaderInBodyBuffer->size())));

    return move(message);
}

MessageUPtr ReplicationTransport::CreateCopyOperationMessage(
    ComOperationCPtr const & operation,
    FABRIC_REPLICA_ID replicaId, 
    FABRIC_EPOCH const & epoch,
    bool isLast,
    bool enableReplicationOperationHeaderInBody)
{
    if (!enableReplicationOperationHeaderInBody)
    {
        std::vector<ULONG> segmentSizes;

        MessageUPtr message = CreateMessageFromCopyOperation(operation, segmentSizes);

        message->Headers.Add(MessageIdHeader());
        message->Headers.Add(CopyOperationHeader(
            replicaId,
            epoch,
            operation->Metadata,
            std::move(segmentSizes),
            isLast));

        message->SetLocalTraceContext(move(wformatString("{0}:{1}", TransportTraceTagPrefix::CopyOperation, operation->SequenceNumber)));

        return move(message);
    }
    else
    {
        MessageUPtr message = CreateMessageFromCopyOperationWithHeaderInBody(
            operation,
            replicaId,
            epoch,
            isLast);

        message->SetLocalTraceContext(move(wformatString("{0}:{1}", TransportTraceTagPrefix::CopyOperation, operation->SequenceNumber)));

        return move(message);
    }
}

bool ReplicationTransport::GetCopyOperationFromMessage(
    __in Transport::Message & message, 
    OperationAckCallback const & ackCallback,
    __out ComOperationCPtr & operation, 
    __out FABRIC_REPLICA_ID & replicaId, 
    __out FABRIC_EPOCH & epoch,
    __out bool & isLast)
{
    CopyOperationHeader header;
    isLast = false;

    vector<const_buffer> msgBuffers;
    bool isBodyValid = message.GetBody(msgBuffers);
    ASSERT_IF(!isBodyValid, "GetBody() in GetCopyOperationFromMessage failed with message status {0}", message.Status);

    bool bodyHeaderDetected = ReadInBodyOperationHeader(message, msgBuffers, header);
    
    if (bodyHeaderDetected || message.Headers.TryReadFirst(header))
    {
        FABRIC_OPERATION_METADATA metaData = header.OperationMetadata;
        if (header.IsLast)
        {
            metaData.Type = FABRIC_OPERATION_TYPE_END_OF_STREAM;
            isLast = true;
        }

        operation = make_com<ComFromBytesOperation, ComOperation>(msgBuffers, header.SegmentSizes, metaData, ackCallback, metaData.SequenceNumber);

        replicaId = header.ReplicaId;
        epoch = header.PrimaryEpoch;

        return true;
    }

    return false;
}

MessageUPtr ReplicationTransport::CreateCopyContextOperationMessage(
    ComOperationCPtr const & operation,
    bool isLast)
{
    std::vector<ULONG> segmentSizes;

    MessageUPtr message = CreateMessageFromCopyOperation(operation, segmentSizes);

    message->Headers.Add(MessageIdHeader());
    message->Headers.Add(CopyContextOperationHeader(operation->Metadata, std::move(segmentSizes), isLast));

    message->SetLocalTraceContext(move(wformatString("{0}:{1}", TransportTraceTagPrefix::CopyContextOperation, operation->SequenceNumber)));

    return move(message);
}

bool ReplicationTransport::GetCopyContextOperationFromMessage(
    __in Transport::Message & message, 
    __out ComOperationCPtr & operation, 
    __out bool & isLast)
{
    CopyContextOperationHeader header;
    isLast = false;

    if (message.Headers.TryReadFirst(header))
    {
        vector<const_buffer> msgBuffers;
        bool isBodyValid = message.GetBody(msgBuffers);
        ASSERT_IF(!isBodyValid, "GetBody() in GetCopyContextOperationFromMessage failed with message status {0}", message.Status);

        FABRIC_OPERATION_METADATA metaData = header.OperationMetadata;
        if (header.IsLast)
        {
            metaData.Type = FABRIC_OPERATION_TYPE_END_OF_STREAM;
            isLast = true;
        }

        operation = make_com<ComFromBytesOperation, ComOperation>(msgBuffers, header.SegmentSizes, metaData, nullptr, metaData.SequenceNumber);
        
        return true;
    }

    return false;
}

MessageUPtr ReplicationTransport::CreateStartCopyMessage(
    FABRIC_EPOCH const & epoch,
    FABRIC_REPLICA_ID replicaId,
    FABRIC_SEQUENCE_NUMBER replicationStartSequenceNumber)
{
    MessageUPtr message = Common::make_unique<Transport::Message>(StartCopyMessageBody(epoch, replicaId, replicationStartSequenceNumber));

    message->Headers.Add(MessageIdHeader());

    message->SetLocalTraceContext(move(wformatString("{0}:{1}", TransportTraceTagPrefix::StartCopy, replicationStartSequenceNumber)));

    return move(message);
}

void ReplicationTransport::GetStartCopyFromMessage(
    __in Transport::Message & message,
    __out FABRIC_EPOCH & epoch,
    __out FABRIC_REPLICA_ID & replicaId,
    __out FABRIC_SEQUENCE_NUMBER & replicationStartSequenceNumber)
{
    StartCopyMessageBody body;
    bool isBodyValid = message.GetBody(body);
    ASSERT_IF(!isBodyValid, "GetBody() in GetStartCopyFromMessage failed with message status {0}", message.Status);
    epoch = body.Epoch;
    replicaId = body.ReplicaId;
    replicationStartSequenceNumber = body.ReplicationStartSequenceNumber;
}

MessageUPtr ReplicationTransport::CreateRequestAckMessage()
{
    MessageUPtr message = Common::make_unique<Transport::Message>();

    message->Headers.Add(MessageIdHeader());

    message->SetLocalTraceContext(move(wformatString("{0}", TransportTraceTagPrefix::RequestAck)));

    return move(message);
}

MessageUPtr ReplicationTransport::CreateInduceFaultMessage(
    __in FABRIC_REPLICA_ID targetReplicaId,
    __in Common::Guid const & targetReplicaIncarnationId,
    __in wstring const & reason)
{
    MessageUPtr message = Common::make_unique<Transport::Message>(InduceFaultMessageBody(targetReplicaId, targetReplicaIncarnationId, reason));

    message->Headers.Add(MessageIdHeader());

    message->SetLocalTraceContext(move(wformatString("{0}", TransportTraceTagPrefix::InduceFault)));

    return move(message);
}
            
void ReplicationTransport::GetInduceFaultFromMessage(
    __in Transport::Message & message,
    __out FABRIC_REPLICA_ID & targetReplicaId,
    __out Common::Guid & targetReplicaIncarnationId,
    __out std::wstring & reason)
{
    InduceFaultMessageBody body;
    bool isBodyValid = message.GetBody(body);
    ASSERT_IF(!isBodyValid, "GetBody() in GetInduceFaultFromMessage failed with message status {0}", message.Status);
    targetReplicaId = body.TargetReplicaId;
    targetReplicaIncarnationId = body.TargetReplicaIncarnationId;
    reason = body.Reason;
}

void ReplicationTransport::GeneratePublishEndpoint(
    ReplicationEndpointId const & uniqueId,
    __out std::wstring & outEndpoint) const
{
    Common::StringWriter writer(outEndpoint);
    writer << publishEndpoint_ << L"/" << uniqueId;
}

ErrorCode ReplicationTransport::SendMessage(
    KBuffer const & buffer,
    Guid const & partitionId,
    ISendTarget::SPtr const & receiverTarget, 
    __in MessageUPtr && message, 
    TimeSpan const & messageExpiration) const
{
    ASSERT_IF(message == nullptr, "SendMessage to {0}: null message", receiverTarget->Address());
    message->Headers.Add(buffer, true);
    UnreliableTransport::AddPartitionIdToMessageProperty(*message, partitionId);
    auto errorCode = unicastTransport_->SendOneWay(receiverTarget, move(message), messageExpiration);

    return this->ReturnSendMessage(errorCode, receiverTarget);
}

ErrorCode ReplicationTransport::ReturnSendMessage(
    ErrorCode const & errorCode,
    ISendTarget::SPtr const & receiverTarget) const
{
    if (!errorCode.IsSuccess())
    {
        if (errorCode.ReadValue() == ErrorCodeValue::MessageTooLarge ||
            errorCode.ReadValue() == ErrorCodeValue::ObjectClosed)
        {
            // MessageTooLarge is handled at the upper layer
            // ObjectClosed is an expected error message if there was a racing close and a send
        }
        else 
        {
            ReplicatorEventSource::Events->TransportSendMessage(
                endpoint_,
                receiverTarget->Address(),
                errorCode.ReadValue());
        }
    }

    return errorCode;
}

KBuffer::SPtr ReplicationTransport::CreateSharedHeaders(
    ReplicationEndpointId const & sendActor,
    ReplicationEndpointId const & receiverActor,
    wstring const & action) const
{
    auto sharedStream = FabricSerializer::CreateSerializableStream();
    auto status = MessageHeaders::Serialize(*sharedStream, ReplicationFromHeader(publishEndpoint_,sendActor));
    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to serialize sendActor");
    status = MessageHeaders::Serialize(*sharedStream, ReplicationActorHeader(receiverActor));
    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to serialize receiverActor");
    status = MessageHeaders::Serialize(*sharedStream, ActionHeader(action));
    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to serialize action");
    
    KBuffer::SPtr buffer;
    status = FabricSerializer::CreateKBufferFromStream(*sharedStream, buffer);
    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create kbuffer from stream");

    return buffer;
}
