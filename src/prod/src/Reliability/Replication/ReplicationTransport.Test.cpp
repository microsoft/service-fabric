// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ComTestOperation.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
 
namespace ReplicationUnitTest
{
    using namespace Common;
    using namespace std;
    using namespace Reliability::ReplicationComponent;
    using namespace Transport;

    typedef std::shared_ptr<REConfig> REConfigSPtr;
    static Common::StringLiteral const TransportTestSource("TransportTest");
    
    class MessageProcessor;
    class TestReplicationTransport
    {
    protected:
        TestReplicationTransport() { BOOST_REQUIRE(Setup()); }
        TEST_CLASS_SETUP(Setup)

        wstring CreateEndpoint(int port, ReplicationEndpointId const & uniqueId);

        ReplicationTransportSPtr CreateTransport(int port);

        static void SendStartCopyMessage(
            LONGLONG configurationNumber,
            ReplicationEndpointId const & endpointUniqueId,
            FABRIC_SEQUENCE_NUMBER sequenceNumber,
            __in MessageProcessor & from,
            wstring const & toAddress);

        static void SendCopyContextMessage(
            FABRIC_SEQUENCE_NUMBER sequenceNumber, 
            ReplicationEndpointId const & toDemuxerAddress, 
            bool isLast, 
            __in MessageProcessor & from,
            wstring const & toAddress,
            __out ComOperationCPtr & operation);

        static void SendCopyMessage(
            FABRIC_SEQUENCE_NUMBER sequenceNumber,
            ReplicationEndpointId const & endpointUniqueId,
            bool isLast,
            __in MessageProcessor & from,
            wstring const & toAddress,
            __out ComOperationCPtr & operation);

        static void SendReplicationMessage(
            FABRIC_SEQUENCE_NUMBER sequenceNumber,
            LONGLONG configurationNumber,
            __in MessageProcessor & from,
            vector<ReplicationEndpointId> const & toActor,
            vector<wstring> const & toAddress,
            __out ComOperationCPtr & operation);

        static int GetNextPort()
        {
            USHORT basePort = 0;
            TestPortHelper::GetPorts(1, basePort);
            return static_cast<int>(basePort);
        }

        class DummyRoot : public Common::ComponentRoot
        {
        public:
            ~DummyRoot() {}
        };
        std::shared_ptr<DummyRoot> root_;

        static LONGLONG DefaultConfigurationNumber;
    };

    LONGLONG TestReplicationTransport::DefaultConfigurationNumber = 100;

    class MessageProcessor : public Common::ComponentRoot
    {
    public:
        MessageProcessor(
            wstring const & endpoint,
            ReplicationEndpointId const & endpointUniqueId,
            ReplicationTransportSPtr transport) 
            :
            endpoint_(endpoint),
            endpointUniqueId_(endpointUniqueId),
            transport_(transport),
            expectedMessages_(),
            receivedMessages_(),
            lock_()
        {
        }

        virtual ~MessageProcessor() {}
        
        void Open()
        {
            transport_->RegisterMessageProcessor(*this);
        }

        void Close()
        {
            transport_->UnregisterMessageProcessor(*this);
        }

        __declspec(property(get=get_ReplicationEndpoint)) wstring const & ReplicationEndpoint;
        wstring const & get_ReplicationEndpoint() const { return endpoint_;}

        __declspec(property(get=get_EndpointUniqueId)) ReplicationEndpointId const & EndpointUniqueId;
        ReplicationEndpointId const & get_EndpointUniqueId() const { return endpointUniqueId_; }

        ReplicationEndpointId const & get_ReplicationEndpointId() const { return endpointUniqueId_; }

        void ProcessMessage(Transport::Message & message, Transport::ReceiverContextUPtr &)
        {
            wstring const & action = message.Action;

            ReplicationFromHeader fromHeader;
            if (!message.Headers.TryReadFirst(fromHeader))
            {
                VERIFY_FAIL(L"The message should contain \"from\" header");
            }

            if (action == ReplicationTransport::ReplicationAckAction)
            {
                ProcessAckMessage(message, fromHeader);
            } 
            else if (action == ReplicationTransport::ReplicationOperationAction)
            {
                ProcessReplicationMessage(message, fromHeader);
            }
            else if (action == ReplicationTransport::CopyOperationAction)
            {
                ProcessCopyMessage(message, fromHeader);
            }
            else if (action == ReplicationTransport::StartCopyAction)
            {
                ProcessStartCopyMessage(message, fromHeader);
            }
            else if (action == ReplicationTransport::CopyContextOperationAction)
            {
                ProcessCopyContextMessage(message, fromHeader);
            }
            else if (action == ReplicationTransport::CopyContextAckAction)
            {
                ProcessCopyContextAckMessage(message, fromHeader);
            }
            else
            {
                Trace.WriteInfo(TransportTestSource, "Received message with action unknown: {0}", action);
                VERIFY_FAIL(L"Action unknown");
            }
        }

        void AddExpectedMessage(wstring const & message)
        {
            Common::AcquireExclusiveLock lock(this->lock_);
            expectedMessages_.push_back(message);
        }

        void CheckExpectedMessages()
        {
            // Wait for the messages to be received
            int tryCount = 10;
            while(tryCount > 0)
            {
                {
                    Common::AcquireReadLock lock(this->lock_);
                    if (expectedMessages_.size() <= receivedMessages_.size())
                    {
                        break;
                    }
                }

                Sleep(500);
                --tryCount;
            }

            wstring expected;
            wstring received;
                
            {
                Common::AcquireExclusiveLock lock(this->lock_);
                std::sort(expectedMessages_.begin(), expectedMessages_.end());
                std::sort(receivedMessages_.begin(), receivedMessages_.end());

                StringWriter writer(expected);
                for (wstring s : expectedMessages_)
                {
                    writer << s << L";";
                }
            
                StringWriter writer2(received);
                for (wstring s : receivedMessages_)
                {
                    writer2 << s << L";";
                }
            }
            
            if (expected != received)
            {
                Trace.WriteInfo(TransportTestSource, "Expected messages: {0}", expected);
                Trace.WriteInfo(TransportTestSource, "Received messages: {0}", received);
                VERIFY_FAIL(L"Expected messages are not received");
            }
            else
            {
                Trace.WriteInfo(TransportTestSource, "Received expected messages: {0}", received);
            }
        }
        
        void SendMessage(ReplicationEndpointId const & toActor, wstring const & toAddress, MessageUPtr && message, wstring const & action)
        {
            auto headers = transport_->CreateSharedHeaders(endpointUniqueId_, toActor, action);
            transport_->SendMessage(*headers, endpointUniqueId_.PartitionId, transport_->ResolveTarget(toAddress), move(message), TimeSpan::MaxValue);
        }

    private:

        void ProcessReplicationMessage(Transport::Message & message, ReplicationFromHeader const & header)
        {
            wstring address = header.Address;
            ReplicationEndpointId fromUniqueAddress = header.DemuxerActor;
                
            std::vector<ComOperationCPtr> batchOperation;
            FABRIC_EPOCH epoch;
            Transport::MessageUPtr reply;
            FABRIC_SEQUENCE_NUMBER completedSequenceNumber;

            if (ReplicationTransport::GetReplicationBatchOperationFromMessage(
                    message,
                    nullptr,
                    batchOperation,
                    epoch,
                    completedSequenceNumber))
            {
                ComOperationCPtr operation = std::move(batchOperation[0]);
                Trace.WriteInfo(TransportTestSource, "{0}: Received replication operation from {1}. Send ACK.",
                    endpointUniqueId_, fromUniqueAddress);

                reply = ReplicationTransport::CreateAckMessage(
                    operation->SequenceNumber,
                    operation->SequenceNumber,
                    -1,
                    -1);

                wstring op;
                StringWriter writer(op);
                writer.Write("{0}:R:{1}", fromUniqueAddress.ReplicaId, operation->SequenceNumber);

                {
                    Common::AcquireExclusiveLock lock(this->lock_);
                    receivedMessages_.push_back(move(op));
                }
            }
            else
            {
                VERIFY_FAIL_FMT("{0}: Received INVALID replication operation from {1}.",
                    endpointUniqueId_, fromUniqueAddress);
            }

            Trace.WriteInfo(TransportTestSource, "{0}: Send reply to {1}.", endpointUniqueId_, fromUniqueAddress);
            SendMessage(fromUniqueAddress, address, move(reply), ReplicationTransport::ReplicationAckAction);
        }

        void ProcessStartCopyMessage(Transport::Message & message, ReplicationFromHeader const & header)
        {
            wstring address = header.Address;
            ReplicationEndpointId fromUniqueAddress = header.DemuxerActor;
            FABRIC_EPOCH epoch;
            FABRIC_REPLICA_ID replicaId;
            FABRIC_SEQUENCE_NUMBER firstReplicationSequenceNumber;
            Transport::MessageUPtr reply;

            ReplicationTransport::GetStartCopyFromMessage(message, epoch, replicaId, firstReplicationSequenceNumber);
            Trace.WriteInfo(TransportTestSource, "{0}: Received START COPY from {1}. Send ACK.",
                endpointUniqueId_, fromUniqueAddress);

            reply = ReplicationTransport::CreateAckMessage(firstReplicationSequenceNumber - 1, 0, -1, -1);

            wstring op;
            StringWriter writer(op);
            writer.Write("{0}:SC:{1}", fromUniqueAddress.ReplicaId, firstReplicationSequenceNumber);
            {
                Common::AcquireExclusiveLock lock(this->lock_);
                receivedMessages_.push_back(move(op));
            }
        
            Trace.WriteInfo(TransportTestSource, "{0}: Send ACK reply to {1}.", endpointUniqueId_, fromUniqueAddress);
            SendMessage(fromUniqueAddress, address, move(reply), ReplicationTransport::ReplicationAckAction);
        }

        void ProcessCopyMessage(Transport::Message & message, ReplicationFromHeader const & header)
        {
            wstring address = header.Address;
            ReplicationEndpointId fromUniqueAddress = header.DemuxerActor;
            
            ComOperationCPtr operation;
            FABRIC_REPLICA_ID replicaId;
            FABRIC_EPOCH epoch;
            bool isLast;
            Transport::MessageUPtr reply;

            if (ReplicationTransport::GetCopyOperationFromMessage(message, nullptr, operation, replicaId, epoch, isLast))
            {
                Trace.WriteInfo(TransportTestSource, "{0}: Received copy operation from {1}. Send ACK.",
                    endpointUniqueId_, fromUniqueAddress);

                reply = ReplicationTransport::CreateAckMessage(
                    -1,
                    -1,
                    operation->SequenceNumber,
                    operation->SequenceNumber);

                wstring op;
                StringWriter writer(op);
                writer.Write("{0}:C:{1}", fromUniqueAddress.ReplicaId, operation->SequenceNumber);
                {
                    Common::AcquireExclusiveLock lock(this->lock_);
                    receivedMessages_.push_back(move(op));
                }
            }
            else
            {
                VERIFY_FAIL_FMT("{0}: Received INVALID copy operation from {1}.",
                    endpointUniqueId_, fromUniqueAddress);
            }

            Trace.WriteInfo(TransportTestSource, "{0}: Send ACK reply to {1}.", endpointUniqueId_, fromUniqueAddress);
            SendMessage(fromUniqueAddress, address, move(reply), ReplicationTransport::ReplicationAckAction);
        }
        
        void ProcessAckMessage(Transport::Message & message, ReplicationFromHeader const & header)
        {
            wstring address = header.Address;
            ReplicationEndpointId fromUniqueAddress = header.DemuxerActor;

            FABRIC_SEQUENCE_NUMBER replicationRLSN;
            FABRIC_SEQUENCE_NUMBER replicationQLSN;
            FABRIC_SEQUENCE_NUMBER copyRLSN;
            FABRIC_SEQUENCE_NUMBER copyQLSN;
            int errorCodeValue;
            ReplicationTransport::GetAckFromMessage(message, replicationRLSN, replicationQLSN, copyRLSN, copyQLSN, errorCodeValue);
            VERIFY_ARE_EQUAL(errorCodeValue, 0, L"Error code value = 0");

            wstring op;
            StringWriter writer(op);
            writer.Write("{0}:ACK:{1}:{2}", fromUniqueAddress.ReplicaId, replicationRLSN, copyRLSN);
            Trace.WriteInfo(TransportTestSource, "{0}: ACK processed", op);
            {
                Common::AcquireExclusiveLock lock(this->lock_);
                receivedMessages_.push_back(move(op));
            }
        }

        void ProcessCopyContextMessage(Transport::Message & message, ReplicationFromHeader const & header)
        {
            wstring address = header.Address;
            ReplicationEndpointId fromUniqueAddress = header.DemuxerActor;

            ComOperationCPtr operation;
            bool isLast;
            Transport::MessageUPtr reply;

            if (ReplicationTransport::GetCopyContextOperationFromMessage(message, operation, isLast))
            {
                Trace.WriteInfo(TransportTestSource, "{0}: Received copy CONTEXT operation from {1}. Send ACK.",
                    endpointUniqueId_, fromUniqueAddress);

                reply = ReplicationTransport::CreateCopyContextAckMessage(operation->SequenceNumber, 0);

                wstring op;
                StringWriter writer(op);
                writer.Write("{0}:CC:{1}", fromUniqueAddress.ReplicaId, operation->SequenceNumber);
                {
                    Common::AcquireExclusiveLock lock(this->lock_);
                    receivedMessages_.push_back(move(op));
                }
            }
            else
            {
                VERIFY_FAIL_FMT("{0}: Received INVALID copy context operation from {1}",
                    endpointUniqueId_, fromUniqueAddress);
            }

            Trace.WriteInfo(TransportTestSource, "{0}: Send ACK reply to {1}.", endpointUniqueId_, fromUniqueAddress);
            SendMessage(fromUniqueAddress, address, move(reply), ReplicationTransport::CopyContextAckAction);
        }

        void ProcessCopyContextAckMessage(Transport::Message & message, ReplicationFromHeader const & header)
        {
            wstring address = header.Address;
            ReplicationEndpointId fromUniqueAddress = header.DemuxerActor;

            FABRIC_SEQUENCE_NUMBER lsn;
            int errorCodeValue;
            ReplicationTransport::GetCopyContextAckFromMessage(message, lsn, errorCodeValue);
            VERIFY_ARE_EQUAL(errorCodeValue, 0, L"ProcessCopyContextAckMessage: ErrorCodeValue should be 0");

            wstring op;
            StringWriter writer(op);
            writer.Write("{0}:CCACK:{1}", fromUniqueAddress.ReplicaId, lsn);
            {
                Common::AcquireExclusiveLock lock(this->lock_);
                receivedMessages_.push_back(move(op));
            }
        }

        wstring endpoint_;
        ReplicationEndpointId endpointUniqueId_;
        ReplicationTransportSPtr transport_;
        vector<wstring> expectedMessages_;
        vector<wstring> receivedMessages_;
        Common::ExclusiveLock lock_;
    };

    typedef std::shared_ptr<MessageProcessor> MessageProcessorSPtr;

    /***********************************
    * TestReplicationTransport methods
    **********************************/

    BOOST_FIXTURE_TEST_SUITE(TestReplicationTransportSuite,TestReplicationTransport)

    BOOST_AUTO_TEST_CASE(TestSendMessagesShareTransport)
    {
        ComTestOperation::WriteInfo(
            TransportTestSource,
            "Start TestSendMessagesShareTransport");

        int port = GetNextPort();
        ReplicationTransportSPtr transport = CreateTransport(port);

        // Share transport
        ReplicationEndpointId uniqueIdPrimary(Guid::NewGuid(), 0); // Primary

        MessageProcessorSPtr primaryPtr = make_shared<MessageProcessor>(CreateEndpoint(port, uniqueIdPrimary), uniqueIdPrimary, transport);
        primaryPtr->Open();
        MessageProcessor & primary = *(primaryPtr.get());

        ReplicationEndpointId uniqueIdSecondary1(Guid::NewGuid(), 1); // Secondary
        MessageProcessorSPtr secondary1Ptr = make_shared<MessageProcessor>(CreateEndpoint(port, uniqueIdSecondary1), uniqueIdSecondary1, transport);

        secondary1Ptr->Open();
        MessageProcessor & secondary1 = *(secondary1Ptr.get());

        Trace.WriteInfo(TransportTestSource, "Primary: Send copy operation to Secondary1 ({0})", secondary1.ReplicationEndpoint);
        ComOperationCPtr op1;
        SendCopyMessage(1, uniqueIdSecondary1, false, primary, secondary1.ReplicationEndpoint, op1);
        secondary1.AddExpectedMessage(L"0:C:1");
        primary.AddExpectedMessage(L"1:ACK:-1:1");

        // Send empty copy operation
        ComOperationCPtr op2;
        SendCopyMessage(2, uniqueIdSecondary1, true, primary, secondary1.ReplicationEndpoint, op2);
        secondary1.AddExpectedMessage(L"0:C:2");
        primary.AddExpectedMessage(L"1:ACK:-1:2");
                
        primary.CheckExpectedMessages();
        primary.Close();
        secondary1.CheckExpectedMessages();
        secondary1.Close();

        transport->Stop();
    }

    BOOST_AUTO_TEST_CASE(TestSendMessages)
    {
        ComTestOperation::WriteInfo(
            TransportTestSource,
            "Start TestSendMessages");

        int port = GetNextPort();

        ReplicationEndpointId uniqueIdPrimary(Guid::NewGuid(), 0); // Primary

        ReplicationTransportSPtr transportPrimary = CreateTransport(port);
        MessageProcessorSPtr primaryPtr = make_shared<MessageProcessor>(CreateEndpoint(port, uniqueIdPrimary), uniqueIdPrimary, transportPrimary);
        primaryPtr->Open();
        MessageProcessor & primary = *(primaryPtr.get());

        port = GetNextPort();
        ReplicationEndpointId uniqueIdSecondary1(Guid::NewGuid(), 1); // Secondary
        ReplicationTransportSPtr transportSecondary1 = CreateTransport(port);
        MessageProcessorSPtr secondary1Ptr = make_shared<MessageProcessor>(CreateEndpoint(port, uniqueIdSecondary1), uniqueIdSecondary1, transportSecondary1);

        secondary1Ptr->Open();
        MessageProcessor & secondary1 = *(secondary1Ptr.get());

        port = GetNextPort();
        ReplicationEndpointId uniqueIdSecondary2(Guid::NewGuid(), 2); // Secondary
        ReplicationTransportSPtr transportSecondary2 = CreateTransport(port);
        MessageProcessorSPtr secondary2Ptr = make_shared<MessageProcessor>(CreateEndpoint(port, uniqueIdSecondary2), uniqueIdSecondary2, transportSecondary2);
        secondary2Ptr->Open();
        MessageProcessor & secondary2 = *(secondary2Ptr.get());

        // Send start copy message to S1
        Trace.WriteInfo(TransportTestSource, "Primary: Send copy operation to Secondary1 ({0})", secondary1.ReplicationEndpoint);
        SendStartCopyMessage(242, uniqueIdSecondary1, 34, primary, secondary1.ReplicationEndpoint);
        secondary1.AddExpectedMessage(L"0:SC:34");

        primary.AddExpectedMessage(L"1:ACK:33:-1");

        // Secondary sends copy context message to primary
        Trace.WriteInfo(TransportTestSource, "2: Send copy CONTEXT operation to Primary ({0})", secondary2.ReplicationEndpoint);
        ComOperationCPtr opCC1;
        SendCopyContextMessage(242, uniqueIdPrimary, false, secondary2, primary.ReplicationEndpoint, opCC1);
        primary.AddExpectedMessage(L"2:CC:242");
        secondary2.AddExpectedMessage(L"0:CCACK:242");

        // Send copy message to S1
        Trace.WriteInfo(TransportTestSource, "Primary: Send copy operation to Secondary1 ({0})", secondary1.ReplicationEndpoint);
        ComOperationCPtr op1;
        SendCopyMessage(201, uniqueIdSecondary1, false, primary, secondary1.ReplicationEndpoint, op1);
        secondary1.AddExpectedMessage(L"0:C:201");
        primary.AddExpectedMessage(L"1:ACK:-1:201");

        // Send copy message to S2
        Trace.WriteInfo(TransportTestSource, "Primary: Send copy operation to Secondary2");
        ComOperationCPtr op2;
        SendCopyMessage(202, uniqueIdSecondary2, false, primary, secondary2.ReplicationEndpoint, op2);
        secondary2.AddExpectedMessage(L"0:C:202");
        primary.AddExpectedMessage(L"2:ACK:-1:202");

        // Multicast replication message to S1 and S2
        Trace.WriteInfo(TransportTestSource, "Primary: Send replication operation to Secondary1 and Secondary2");
        vector<wstring> toAddress;
        toAddress.push_back(secondary1.ReplicationEndpoint);
        toAddress.push_back(secondary2.ReplicationEndpoint);
        vector<ReplicationEndpointId> toActor;
        toActor.push_back(uniqueIdSecondary1);
        toActor.push_back(uniqueIdSecondary2);

        ComOperationCPtr op3;
        SendReplicationMessage(8, 100, primary, toActor, toAddress, op3);
        secondary1.AddExpectedMessage(L"0:R:8");
        secondary2.AddExpectedMessage(L"0:R:8");
        primary.AddExpectedMessage(L"1:ACK:8:-1");
        primary.AddExpectedMessage(L"2:ACK:8:-1");

        // Unicast replication message to S1
        Trace.WriteInfo(TransportTestSource, "Primary: Send replication operation to Secondary2");
        toAddress.clear();
        toAddress.push_back(secondary2.ReplicationEndpoint);
        toActor.clear();
        toActor.push_back(uniqueIdSecondary2);

        ComOperationCPtr op4;
        SendReplicationMessage(9, 100, primary, toActor, toAddress, op4);
        secondary2.AddExpectedMessage(L"0:R:9");
        primary.AddExpectedMessage(L"2:ACK:9:-1");

        // Check that all messages have been received correctly
        primary.CheckExpectedMessages();
        primary.Close();
        secondary1.CheckExpectedMessages();
        secondary1.Close();
        secondary2.CheckExpectedMessages();
        secondary2.Close();

        transportPrimary->Stop();
        transportSecondary1->Stop();
        transportSecondary2->Stop();
    }

    BOOST_AUTO_TEST_CASE(TestDropMessageToNotMatch)
    {
        ComTestOperation::WriteInfo(
            TransportTestSource,
            "Start TestDropMessageToNotMatch");

        int port = GetNextPort();

        auto partitionId = Guid::NewGuid();
        
        ReplicationEndpointId uniqueIdPrimary(partitionId, 0, Guid::NewGuid()); // Primary
        ReplicationEndpointId wrongIdPrimary(partitionId, 0, Guid::NewGuid()); 

        ReplicationTransportSPtr transportPrimary = CreateTransport(port);
        MessageProcessorSPtr primaryPtr = make_shared<MessageProcessor>(CreateEndpoint(port, uniqueIdPrimary), uniqueIdPrimary, transportPrimary);
        primaryPtr->Open();
        MessageProcessor & primary = *(primaryPtr.get());

        port = GetNextPort();
        ReplicationEndpointId uniqueIdSecondary1(partitionId, 1, Guid::NewGuid()); // Secondary
        ReplicationEndpointId wrongIdSecondary1(partitionId, 1, Guid::NewGuid());
        ReplicationTransportSPtr transportSecondary1 = CreateTransport(port);
        MessageProcessorSPtr secondary1Ptr = make_shared<MessageProcessor>(CreateEndpoint(port, uniqueIdSecondary1), uniqueIdSecondary1, transportSecondary1);

        secondary1Ptr->Open();
        MessageProcessor & secondary1 = *(secondary1Ptr.get());

        // Send a message from primary to wrong secondary and one from secondary to wrong primary.
        // Both will be dropped
        Trace.WriteInfo(TransportTestSource, "Primary: Send start copy operation to wrong Secondary1 ({0})", wrongIdSecondary1);
        SendStartCopyMessage(242, wrongIdSecondary1, 34, primary, secondary1.ReplicationEndpoint);
        Trace.WriteInfo(TransportTestSource, "2: Send copy CONTEXT operation to Wrong Primary ({0})", wrongIdPrimary);
        ComOperationCPtr opCC1;
        SendCopyContextMessage(242, wrongIdPrimary, false, secondary1, primary.ReplicationEndpoint, opCC1);
        
        Sleep(100);

        // Check that all messages have been received correctly
        primary.CheckExpectedMessages();
        primary.Close();
        secondary1.CheckExpectedMessages();
        secondary1.Close();
        
        transportPrimary->Stop();
        transportSecondary1->Stop();
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool TestReplicationTransport::Setup()
    {
        root_ = std::make_shared<DummyRoot>();
        return true;
    }

      
    ReplicationTransportSPtr TestReplicationTransport::CreateTransport(int port)
    {
        wstring baseEndpoint;
        StringWriter writer(baseEndpoint);
        writer.Write("127.0.0.1:{0}", port);
        REConfigSPtr config = std::make_shared<REConfig>();
        config->ReplicatorListenAddress = baseEndpoint;
        config->ReplicatorPublishAddress = baseEndpoint;
        config->SecondaryClearAcknowledgedOperations = (Random().NextDouble() > 0.5)? true : false;
        config->QueueHealthMonitoringInterval = TimeSpan::Zero;
        auto transport = ReplicationTransport::Create(config->ReplicatorListenAddress);

        Trace.WriteInfo(TransportTestSource, "Starting transport: {0}", baseEndpoint);
        auto errorCode = transport->Start(L"localhost:0");

        ASSERT_IFNOT(errorCode.IsSuccess(), "Failed to start: {0} with {1}", errorCode, baseEndpoint);

        return transport;
    }

    wstring TestReplicationTransport::CreateEndpoint(int port, ReplicationEndpointId const & uniqueId)
    {
        wstring endpoint;
        StringWriter writer(endpoint);
        writer.Write("127.0.0.1:{0}/{1}", port, uniqueId);
        return endpoint;
    }
        
    void TestReplicationTransport::SendStartCopyMessage(
        LONGLONG configurationNumber,
        ReplicationEndpointId const & endpointUniqueId,
        FABRIC_SEQUENCE_NUMBER replicationStartSeq,
        __in MessageProcessor & from,
        wstring const & toAddress)
    {
        FABRIC_EPOCH epoch;
        epoch.ConfigurationNumber = configurationNumber;
        epoch.DataLossNumber = 342525;
        epoch.Reserved = NULL;
        MessageUPtr message = ReplicationTransport::CreateStartCopyMessage(epoch, endpointUniqueId.ReplicaId, replicationStartSeq);

        from.SendMessage(endpointUniqueId, toAddress, move(message), ReplicationTransport::StartCopyAction);
    }

    void TestReplicationTransport::SendCopyContextMessage(
        FABRIC_SEQUENCE_NUMBER sequenceNumber,
        ReplicationEndpointId const & toDemuxerAddress,
        bool isLast,
        __in MessageProcessor & from,
        wstring const & toAddress,
        __out ComOperationCPtr & operation)
    {
        static wstring const opContent = L"TransportTest Copy Context Operation";
        ComPointer<IFabricOperationData> opDataPointer;
        if (!isLast)
        {
            opDataPointer = make_com<ComTestOperation,IFabricOperationData>(opContent);
        }

        FABRIC_OPERATION_METADATA metadata;
        metadata.Type = FABRIC_OPERATION_TYPE_NORMAL;
        metadata.SequenceNumber = sequenceNumber;
        metadata.Reserved = NULL;
        operation = make_com<ComUserDataOperation,ComOperation>(
            move(opDataPointer), 
            metadata);

        MessageUPtr message = ReplicationTransport::CreateCopyContextOperationMessage(operation, isLast);

        from.SendMessage(toDemuxerAddress, toAddress, move(message), ReplicationTransport::CopyContextOperationAction);
    }

    void TestReplicationTransport::SendCopyMessage(
        FABRIC_SEQUENCE_NUMBER sequenceNumber,
        ReplicationEndpointId const & replicaEndpoint,
        bool isLast,
        __in MessageProcessor & from,
        wstring const & toAddress,
        __out ComOperationCPtr & operation)
    {
        static wstring const opContent = L"TransportTest Copy Operation";
        ComPointer<IFabricOperationData> opDataPointer;
        if (!isLast)
        {
            opDataPointer = make_com<ComTestOperation,IFabricOperationData>(opContent);
        }

        FABRIC_OPERATION_METADATA metadata;
        metadata.Type = FABRIC_OPERATION_TYPE_NORMAL;
        metadata.SequenceNumber = sequenceNumber;
        metadata.Reserved = NULL;
        operation = make_com<ComUserDataOperation,ComOperation>(
            move(opDataPointer), 
            metadata);

        FABRIC_EPOCH epoch;
        epoch.ConfigurationNumber = DefaultConfigurationNumber;
        epoch.DataLossNumber = 1;
        epoch.Reserved = NULL;

        MessageUPtr message = ReplicationTransport::CreateCopyOperationMessage(operation, replicaEndpoint.ReplicaId, epoch, isLast, true);

        from.SendMessage(replicaEndpoint, toAddress, move(message), ReplicationTransport::CopyOperationAction);
    }

    void TestReplicationTransport::SendReplicationMessage(
        FABRIC_SEQUENCE_NUMBER sequenceNumber,
        LONGLONG configurationVersion,
        __in MessageProcessor & from,
        vector<ReplicationEndpointId> const & toActor,
        vector<wstring> const & toAddress,
        __out ComOperationCPtr & operation)
    {
        static wstring const opContent = L"TransportTest Replication Operation";
        FABRIC_OPERATION_METADATA metadata;
        metadata.Type = FABRIC_OPERATION_TYPE_NORMAL;
        metadata.SequenceNumber = sequenceNumber;
        metadata.Reserved = NULL;

        operation = make_com<ComUserDataOperation,ComOperation>(
            make_com<ComTestOperation,IFabricOperationData>(opContent),
            metadata);

        FABRIC_EPOCH epoch;
        epoch.ConfigurationNumber = configurationVersion;
        epoch.DataLossNumber = 1;
        epoch.Reserved = NULL;

        for(size_t i = 0; i < toActor.size(); ++i)
        {
            MessageUPtr message = ReplicationTransport::CreateReplicationOperationMessage(operation, operation->SequenceNumber, epoch, true, Reliability::ReplicationComponent::Constants::InvalidLSN);

            from.SendMessage(toActor[i], toAddress[i], move(message), ReplicationTransport::ReplicationOperationAction);
        }
    }
}
