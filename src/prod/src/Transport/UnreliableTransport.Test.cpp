// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TestCommon.h"

using std::shared_ptr;
using std::weak_ptr;

using namespace std;
using namespace std::placeholders;
using namespace Transport;
using namespace Common;

namespace TransportUnitTest
{

    class MessageReceiver
    {
    public:
        __declspec(property(get=get_MessageCount)) unsigned int MessageCount;

        explicit MessageReceiver(shared_ptr<IDatagramTransport> const & receiver) : receiver_(receiver), messageCount_(0)
        {
            ASSERT_IF(!this->receiver_, "invalid pointer");
        }

        Common::ErrorCode Start()
        {
            auto errorCode = this->receiver_->Start();

            if (errorCode.IsSuccess())
            {
                this->receiver_->SetMessageHandler(std::bind(&MessageReceiver::OnMessage, this, _1, _2));
            }

            return errorCode;
        }

        void OnMessage(MessageUPtr &, ISendTarget::SPtr const &)
        {
            ++messageCount_;
        }

        unsigned int get_MessageCount() const { return this->messageCount_.load(); }

    private:
        shared_ptr<IDatagramTransport> receiver_;
        Common::atomic_long messageCount_;
    };

    struct TestRoot: public ComponentRoot
    {
        IDatagramTransportSPtr sender;
    };

    class UnreliableTransportTest
    {
    protected:
        StringCollection InitializeTransportParams(
            wstring source = L"", 
            wstring destination = L"", 
            wstring action = L"", 
            wstring priority = L"", 
            wstring probability = L"", 
            wstring delay = L"", 
            wstring delaySpan = L"")
        {
            StringCollection params;
            if (source == L"")
            {
                return params;
            }
            params.push_back(source);
            if (destination == L"")
            {
                return params;
            }
            params.push_back(destination);
            if (action == L"")
            {
                return params;
            }
            params.push_back(action);
            if (priority == L"")
            {
                return params;
            }
            params.push_back(priority);
            if (probability == L"")
            {
                return params;
            }
            params.push_back(probability);
            if (delay == L"")
            {
                return params;
            }
            params.push_back(delay);
            if (source == L"")
            {
                return params;
            }
            params.push_back(delaySpan);
            return params;
        }
    };

    BOOST_FIXTURE_TEST_SUITE2(UnreliableTransportTestSuite,UnreliableTransportTest)

#ifndef PLATFORM_UNIX //LINUXTODO
    BOOST_AUTO_TEST_CASE(DestinationTest)
    {
        ENTER;

        wstring one(L"one");
        wstring two(L"two");
        wstring three(L"three");

        const int TotalMessageCount = 50;
        shared_ptr<TestRoot> root = make_shared<TestRoot>();
        root->sender = DatagramTransportFactory::CreateUnreliable(*root, DatagramTransportFactory::CreateMem(L"sender"));
        VERIFY_IS_TRUE(root->sender->Start().IsSuccess());
        
        MessageReceiver receiverOne(DatagramTransportFactory::CreateMem(one));
        VERIFY_IS_TRUE(receiverOne.Start().IsSuccess());
        MessageReceiver receiverTwo(DatagramTransportFactory::CreateMem(two));
        VERIFY_IS_TRUE(receiverTwo.Start().IsSuccess());
        MessageReceiver receiverThree(DatagramTransportFactory::CreateMem(three));
        VERIFY_IS_TRUE(receiverThree.Start().IsSuccess());

        VERIFY_IS_TRUE(UnreliableTransportConfig::GetConfig().AddSpecification(L"Test1", L"sender one * 0.5 10"));
        
        VERIFY_IS_TRUE(UnreliableTransportConfig::GetConfig().AddSpecification(L"Test2", L"sender two * 1.0 20"));

        ISendTarget::SPtr targetOne = root->sender->ResolveTarget(one);
        ISendTarget::SPtr targetTwo = root->sender->ResolveTarget(two);
        ISendTarget::SPtr targetThree = root->sender->ResolveTarget(three);

        for (int i = 0; i < TotalMessageCount; ++i)
        {
            root->sender->SendOneWay(targetOne, make_unique<Message>());
            root->sender->SendOneWay(targetTwo, make_unique<Message>());
            root->sender->SendOneWay(targetThree, make_unique<Message>());
        }

        Sleep(1000);

        // two shouldn't have anything yet
        VERIFY_IS_TRUE(receiverTwo.MessageCount == 0);

        Sleep(2000);

        // one should have half
        VERIFY_IS_TRUE(receiverOne.MessageCount > 5 && receiverOne.MessageCount < 45);

        // three should have everything
        VERIFY_IS_TRUE(receiverThree.MessageCount == TotalMessageCount);

        Sleep(10000);

        // one should have received everything
        VERIFY_IS_TRUE(receiverOne.MessageCount == TotalMessageCount);
            
        // two still has nothing
        VERIFY_IS_TRUE(receiverTwo.MessageCount == 0);

        Sleep(10000);

        // now two has everything
        VERIFY_IS_TRUE(receiverTwo.MessageCount == TotalMessageCount);

        root->sender->Stop();

        LEAVE;
    }
#endif

    BOOST_AUTO_TEST_SUITE_END()
}
