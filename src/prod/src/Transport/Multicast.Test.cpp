// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace Common;
using namespace std;

namespace TransportUnitTest
{
    using namespace Transport;

    BOOST_AUTO_TEST_SUITE2(MulticastTransportTests)

    class Receiver;
    class MulticastSender;

    typedef shared_ptr<Receiver> ReceiverSPtr;
    typedef shared_ptr<MulticastSender> MulticastSenderSPtr;

    class TestMessageBody : public Serialization::FabricSerializable
    {
    public:
        TestMessageBody() {}

        TestMessageBody(wstring&& str) 
            : str_(std::move(str))
        {
        }

        wstring const & Value() const 
        { 
            return str_; 
        }

        FABRIC_FIELDS_01(str_);

    private:
        wstring str_;
    };

    struct TestHeader : MessageHeader<MessageHeaderId::MulticastTest_TargetSpecificHeader>, public Serialization::FabricSerializable
    {
        TestHeader() {}

        TestHeader(std::wstring && key, std::wstring && value)
            : key_(std::move(key)), value_(std::move(value)) {}
        
        FABRIC_FIELDS_02(key_, value_);

        std::wstring const & Key() const { return key_; }
        std::wstring const & Value() const { return value_; }

    private:
        std::wstring key_;
        std::wstring value_;
    };

    struct Utility
    {
        static IDatagramTransportSPtr CreateTransport(wstring const & address);

        static void CreateReceivers(
            size_t numReceivers, 
            __out shared_ptr<vector<wstring>> & addresses, 
            __out shared_ptr<vector<ReceiverSPtr>> & receivers,
            bool verifyContent);

        static MulticastSenderSPtr CreateMulticastSender(__in shared_ptr<vector<wstring>> const & receiverAddresses);

        static void VerifySent(int numExpected, MulticastSenderSPtr const & sender);

        static void VerifyReceived(
            MulticastSenderSPtr const & sender,
            shared_ptr<vector<ReceiverSPtr>> const & receivers, 
            bool verifyBody = false,
            bool verifyHeaders = false);

    private:
        static void VerifyReceivedBody(TestMessageBody const & expectedBody, MessageUPtr const & receivedMessage);
        
        static void VerifyReceivedHeaders(MessageHeaders & expected, MessageUPtr const & receviedMessage);

        static void VerifyReceivedMessages(
            MulticastSenderSPtr const & sender, 
            ReceiverSPtr const & receiver, 
            bool verifyBody, 
            bool verifyHeaders);
    };

    struct Tests
    {
        static void TestNoHeaders(size_t numReceivers, int numMessages, bool verifyContent = false);
        static void TestWithHeaders(size_t numReceivers, int numMessages, bool verifyContent = false);

    private:
        static void Run(size_t numReceivers, int numMessages, bool headers, bool verifyBody, bool verifyHeaders);
    };
    
    class MulticastSender
    {
        DENY_COPY(MulticastSender)

    public:
        __declspec(property(get=get_SentMessageCount)) LONG SentMessageCount;
        __declspec(property(get=get_TestBody)) TestMessageBody const & TestBody;
        __declspec(property(get=get_TestHeaders)) MessageHeadersCollection & TestHeaders;

        MulticastSender(wstring const & myaddress, shared_ptr<vector<wstring>> const & targets)
            :   transport_(Utility::CreateTransport(myaddress)),
                sender_(make_shared<MulticastDatagramSender>(transport_)),
                sentMessageCount_(0),
                testBody_((wstring)L"This is a test message."),
                testHeaders_((size_t)(targets->end() - targets->begin()))
        {
            vector<NamedAddress> namedTargets;
            for(wstring const & target : *targets)
            {
                namedTargets.push_back(NamedAddress(target));
            }

            size_t failureCount = 0;
            target_ = sender_->Resolve(namedTargets.begin(), namedTargets.end(), failureCount);

            VERIFY_IS_TRUE(failureCount == 0);

            for(size_t i = 0; i < testHeaders_.size(); i++)
            {
                AddCommonHeaders(testHeaders_[i]);
            }

            AddTargetSpecificHeaders(testHeaders_);
        }

        void SendMessage_Headers()
        {
            wstring str = L"This is a test message.";
            TestMessageBody body(std::move(str));
            auto message = make_unique<Message>(body);

            // add a common header
            AddCommonHeaders(message->Headers);

            // add target specific headers
            MessageHeadersCollection targetHeaders(target_->size());
            AddTargetSpecificHeaders(targetHeaders);

            sender_->Send(
                target_,
                std::move(message),
                std::move(targetHeaders));
                
            sentMessageCount_++;
        }

        void SendMessage_NoHeaders()
        {
            wstring str = L"This is a test message.";
            TestMessageBody body(std::move(str));
            sender_->Send(
                target_, 
                make_unique<Message>(body));
            sentMessageCount_++;
        }

        LONG get_SentMessageCount() const
        {
            return sentMessageCount_.load();
        }

        TestMessageBody const & get_TestBody() const
        {
            return testBody_;
        }

        MessageHeadersCollection & get_TestHeaders()
        {
            return testHeaders_;
        }

        void Stop()
        {
            transport_->Stop();
        }

    private:
        IDatagramTransportSPtr transport_;
        MulticastSendTargetSPtr target_;
        MulticastDatagramSenderSPtr sender_;
        TestMessageBody testBody_;
        MessageHeadersCollection testHeaders_;
        Common::atomic_long sentMessageCount_;

        void AddCommonHeaders(MessageHeaders & headers)
        {
            headers.Add(ActionHeader(L"Test Action"));
        }

        void AddTargetSpecificHeaders(MessageHeadersCollection & targetHeaders)
        {
            for(size_t i = 0; i < targetHeaders.size(); i ++)
            {
                targetHeaders[i].Add(TestHeader(wformatString("Key{0}",i), wformatString("Value{0}",i)));
            }
        }
    };

    class Receiver
    {
        DENY_COPY(Receiver)

    public:
        __declspec(property(get=get_Address)) wstring const & Address;
        __declspec(property(get=get_Index)) size_t Index;
        __declspec(property(get=get_ReceivedMessageCount)) LONG ReceivedMessageCount;
        __declspec(property(get=get_ReceivedMessages)) vector<MessageUPtr> const & ReceivedMessages;

        Receiver(wstring const & address, size_t index = 0, bool storeMessages = false)
            : address_(address),
            index_(index),
            storeMessages_(storeMessages),
            transport_(Utility::CreateTransport(address)),
            receivedMessageCount_(0)
        {
            transport_->SetMessageHandler(
                [&](MessageUPtr & message,
                ISendTarget::SPtr const & sender){ this->OnMessageReceived(*message, sender); });
        };

        void OnMessageReceived(Message & message, ISendTarget::SPtr const & )
        {
            if (storeMessages_)
            {
                auto clone = message.Clone();
                receivedMessages_.push_back(std::move(clone));
            }

            receivedMessageCount_++;
        }

        LONG get_ReceivedMessageCount() const
        {
            return receivedMessageCount_.load();
        }

        wstring const & get_Address() const
        {
            return address_;
        }

        size_t get_Index() const
        {
            return index_;
        };

        vector<MessageUPtr> const & get_ReceivedMessages() const
        {
            return (receivedMessages_);
        }

        void Stop()
        {
            transport_->Stop();
        }

    private:
        wstring address_;
        size_t index_;
        bool storeMessages_;
        IDatagramTransportSPtr transport_;
        Common::atomic_long receivedMessageCount_;
        vector<MessageUPtr> receivedMessages_;
    };

    //
    // Utility Implementation
    // 
    IDatagramTransportSPtr Utility::CreateTransport(wstring const & address)
    {
        auto transport = DatagramTransportFactory::CreateMem(address);

        auto errorCode = transport->Start();
        ASSERT_IFNOT(errorCode.IsSuccess(), "Failed to start: {0}", errorCode);

        return transport;
    }

    void Utility::CreateReceivers(
            size_t numReceivers, 
            __out shared_ptr<vector<wstring>> & addresses, 
            __out shared_ptr<vector<ReceiverSPtr>> & receivers,
            bool verifyContent)
    {
        Trace.WriteInfo(TraceType, "Creating {0} Receivers", numReceivers);

        addresses = make_shared<vector<wstring>>(numReceivers);
        receivers = make_shared<vector<ReceiverSPtr>>(numReceivers);

        for(size_t i = 0; i < numReceivers; i++)
        {
            (*addresses)[i] = Common::wformatString("MemoryTransport.Receiver#{0}", i);
            (*receivers)[i] = make_shared<Receiver>((*addresses)[i], i, verifyContent);
        }
    }

    MulticastSenderSPtr Utility::CreateMulticastSender(__in shared_ptr<vector<wstring>> const & receiverAddresses)
    {
        Trace.WriteInfo(TraceType, "Creating a MulticastSender.");
        return make_shared<MulticastSender>(L"MemoryTransport/MulticastSender", receiverAddresses);
    }

    void Utility::VerifySent(int numExpected, MulticastSenderSPtr const & sender)
    {
        bool result = (sender->SentMessageCount == numExpected);
        Trace.WriteInfo(TraceType, "Verifying that {0} messages were sent: {1}", numExpected, result);
        VERIFY_IS_TRUE(result);
    }

    void Utility::VerifyReceived(
        MulticastSenderSPtr const & sender,
        shared_ptr<vector<ReceiverSPtr>> const & receivers, 
        bool verifyBody,
        bool verifyHeaders)
    {
        vector<ReceiverSPtr>::iterator iter;

        for(iter = receivers->begin(); iter < receivers->end(); iter++)
        {
            Utility::VerifyReceivedMessages(sender, *iter, verifyBody, verifyHeaders);
        }
    }

    void Utility::VerifyReceivedMessages(
        MulticastSenderSPtr const & sender, 
        ReceiverSPtr const & receiver, 
        bool verifyBody,
        bool verifyHeaders)
    {
         bool result = receiver->ReceivedMessageCount == sender->SentMessageCount;

         Trace.WriteInfo(TraceType,
            "Verifying that receiver {0} received {1} messages: {2} ({3}/{1})",
            receiver->Address,
            sender->SentMessageCount,
            result,
            receiver->ReceivedMessageCount);

         VERIFY_IS_TRUE(result);

         if (verifyBody || verifyHeaders)
         {
             Trace.WriteInfo(TraceType, "\tVerifing the contents of the received messages.");

             for(auto iter = receiver->ReceivedMessages.begin(); iter < receiver->ReceivedMessages.end(); iter++)
             {
                 if (verifyBody)
                 {
                    VerifyReceivedBody(sender->TestBody, *iter);
                 }
                 if (verifyHeaders)
                 {
                    VerifyReceivedHeaders(sender->get_TestHeaders()[receiver->Index], *iter);
                 }
             }
         }
    }
   
    void Utility::VerifyReceivedBody(TestMessageBody const & expectedBody, MessageUPtr const & receivedMessage)
    {
        TestMessageBody receivedBody;
        receivedMessage->GetBody<TestMessageBody>(receivedBody);

        VERIFY_IS_TRUE(expectedBody.Value() == receivedBody.Value());
    }

    void Utility::VerifyReceivedHeaders(MessageHeaders & expected, MessageUPtr const & receivedMessage)
    {
        // right now verify action and TestHeaders, later expand to be more generic
        VERIFY_IS_TRUE(expected.Action == receivedMessage->Headers.Action);

        TestHeader expectedTestHeader;
        if (expected.TryReadFirst(expectedTestHeader))
        {
            TestHeader receivedHeader;
            VERIFY_IS_TRUE(receivedMessage->Headers.TryReadFirst(receivedHeader));
            VERIFY_IS_TRUE(receivedHeader.Key() == expectedTestHeader.Key());
            VERIFY_IS_TRUE(receivedHeader.Value() == expectedTestHeader.Value());
        }
    }

    void Tests::TestNoHeaders(size_t numReceivers, int numMessages, bool verifyContent)
    {
        Tests::Run(numReceivers, numMessages, false, verifyContent, false);
    }

    void Tests::TestWithHeaders(size_t numReceivers, int numMessages, bool verifyContent)
    {
        Tests::Run(numReceivers, numMessages, true, verifyContent, verifyContent);
    }

    void Tests::Run(size_t numReceivers, int numMessages, bool headers, bool verifyBody, bool verifyHeaders)
    {
        Trace.WriteInfo(TraceType, "Testing multicast one-way messages: NumReceivers {0}, NumMessages {1}", numReceivers, numMessages);

        shared_ptr<vector<wstring>> receiverAddresses;
        shared_ptr<vector<ReceiverSPtr>> receivers;

        // Create two receivers
        Utility::CreateReceivers(numReceivers, receiverAddresses, receivers, verifyBody || verifyHeaders);

        // Create multicast sender
        MulticastSenderSPtr sender = Utility::CreateMulticastSender(receiverAddresses);

        Trace.WriteInfo(TraceType, "Sending {0} messages to a multicast target of {1} receivers.", numMessages, numReceivers);
        // send a message
        for(int i = 0; i < numMessages; i++)
        {
            if (headers)
            {
                sender->SendMessage_Headers();
            }
            else
            {
                sender->SendMessage_NoHeaders();
            }
        }

        // verify that one message was sent by sender
        Utility::VerifySent(numMessages, sender);

        // wait for message to drain
        // TODO: do not use timeout - use request-response in the test.
        Sleep(1000);

        // verify that one message was received by all receivers.
        Utility::VerifyReceived(sender, receivers, verifyBody, verifyHeaders);

        for (ReceiverSPtr const & receiver : *receivers)
        {
            receiver->Stop();
        }

        sender->Stop();
    }

    BOOST_AUTO_TEST_CASE(BasicScenarioTest)
    {
        Config config;
        // TestNoHeaders(size_t numReceivers, int numMessages, bool verifyContent = false);
        Tests::TestNoHeaders(2, 1, true);
        Tests::TestNoHeaders(16, 10);
        Tests::TestNoHeaders(64, 100);
        // LINUXTODO uncomment when thread pool is fixed
        //Tests::TestNoHeaders(128, 1000);

        // TestWithHeaders(size_t numReceivers, int numMessages, bool verifyContent = false);
        Tests::TestWithHeaders(2, 1, true);
        Tests::TestWithHeaders(16, 10);
        Tests::TestWithHeaders(64, 100);
        // LINUXTODO uncomment when thread pool is fixed
        //Tests::TestWithHeaders(128, 1000);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
