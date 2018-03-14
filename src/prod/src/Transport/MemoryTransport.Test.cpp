// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace Transport;
using namespace Common;
using namespace std;

namespace TransportUnitTest
{
    BOOST_AUTO_TEST_SUITE2(MemoryTransportTests)

    BOOST_AUTO_TEST_CASE(SimpleMessageTest)
    {
        ENTER;

        shared_ptr<IDatagramTransport> sender = DatagramTransportFactory::CreateMem(L"sender");
        VERIFY_IS_TRUE(sender);
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        AutoResetEvent replyReceived;
        sender->SetMessageHandler([&replyReceived](MessageUPtr &, ISendTarget::SPtr const &) -> void
        {
            Trace.WriteInfo(TraceType, "[sender] got a message");

            replyReceived.Set();
        });

        shared_ptr<IDatagramTransport> receiver = DatagramTransportFactory::CreateMem(L"receiver");
        VERIFY_IS_TRUE(receiver);
        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        AutoResetEvent messageReceived;
        receiver->SetMessageHandler([&receiver, &messageReceived](MessageUPtr &, ISendTarget::SPtr const & sender) -> void
        {
            Trace.WriteInfo(TraceType, "[receiver] got a message");

            messageReceived.Set();

            receiver->SendOneWay(sender, make_unique<Message>());
        });

        bool receivedReplyOne = replyReceived.WaitOne(TimeSpan::FromSeconds(1));
        VERIFY_IS_FALSE(receivedReplyOne);

        ISendTarget::SPtr target = sender->ResolveTarget(L"receiver");
        VERIFY_IS_TRUE(target);

        sender->SendOneWay(target, make_unique<Message>());

        bool receivedMessageOne = messageReceived.WaitOne(TimeSpan::FromSeconds(5));
        VERIFY_IS_TRUE(receivedMessageOne);

        receivedReplyOne = replyReceived.WaitOne(TimeSpan::FromSeconds(5));
        VERIFY_IS_TRUE(receivedReplyOne);

        receiver->Stop();
        sender->Stop();

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(InvalidTargetTest)
    {
        ENTER;

        shared_ptr<IDatagramTransport> sender = DatagramTransportFactory::CreateMem(L"invalidTargetTestSender");
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        auto target = sender->ResolveTarget(L"second banana");
        VERIFY_IS_TRUE(target);
        shared_ptr<IDatagramTransport> secondBanana = DatagramTransportFactory::CreateMem(L"second banana");
        VERIFY_IS_TRUE(secondBanana->Start().IsSuccess());

        sender->SendOneWay(target, make_unique<Message>());

        shared_ptr<IDatagramTransport> receiver = DatagramTransportFactory::CreateMem(L"invalidTargetTestSender");
        VERIFY_IS_TRUE(receiver);
        VERIFY_IS_FALSE(receiver->Start().IsSuccess());

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ManyMessagesTest)
    {
        ENTER;

        const int TotalMessageCount = 100;
        Common::atomic_long messageCount(0);

        shared_ptr<IDatagramTransport> sender = DatagramTransportFactory::CreateMem(L"manyMessagesTestSender");
        shared_ptr<IDatagramTransport> receiver = DatagramTransportFactory::CreateMem(L"manyMessagesTestReceiver");

        VERIFY_IS_TRUE(sender->Start().IsSuccess());
        VERIFY_IS_TRUE(receiver->Start().IsSuccess());

        AutoResetEvent messagesReceived;

        receiver->SetMessageHandler([&messageCount, &messagesReceived, TotalMessageCount](MessageUPtr & msg, ISendTarget::SPtr const & st) -> void
        {
            Trace.WriteInfo(TraceType, "received {0} from {1}", msg->TraceId(), st->Address());
            auto count = ++messageCount;

            if (count == TotalMessageCount)
            {
                messagesReceived.Set();
            }
        });

        ISendTarget::SPtr target = sender->ResolveTarget(L"manyMessagesTestReceiver");
        VERIFY_IS_TRUE(target);

        for (int i = 0; i < TotalMessageCount; ++i)
        {
            auto msg = make_unique<Message>();
            msg->Headers.Add(MessageIdHeader());
            Trace.WriteInfo(TraceType, "sending {0}", msg->TraceId());
            sender->SendOneWay(target, move(msg));
        }

        bool receivedMessages = messagesReceived.WaitOne(TimeSpan::FromSeconds(15));
        ASSERT_IFNOT(receivedMessages, "Need to debug this");

        VERIFY_IS_TRUE(receivedMessages);

        VERIFY_IS_TRUE(messageCount.load() == TotalMessageCount);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SendToSelfTest)
    {
        ENTER;

        shared_ptr<IDatagramTransport> sender = DatagramTransportFactory::CreateMem(L"loopback");
        VERIFY_IS_TRUE(sender);
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        AutoResetEvent messageReceived;
        sender->SetMessageHandler([&messageReceived](MessageUPtr &, ISendTarget::SPtr const &) -> void
        {
            Trace.WriteInfo(TraceType, "[sender] got a message");

            messageReceived.Set();
        });

        ISendTarget::SPtr target = sender->ResolveTarget(L"loopback");
        VERIFY_IS_TRUE(target);

        sender->SendOneWay(target, make_unique<Message>());

        bool receivedMessageOne = messageReceived.WaitOne(TimeSpan::FromSeconds(5));
        VERIFY_IS_TRUE(receivedMessageOne);

        LEAVE;
    }

    BOOST_AUTO_TEST_SUITE_END()
}
