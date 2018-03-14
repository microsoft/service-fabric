// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace TransportUnitTest
{
    using namespace std;
    using namespace Common;
    using namespace Transport;

    class TestClient : public enable_shared_from_this<TestClient>
    {
        DENY_COPY(TestClient)

    public:

        TestClient()
        {
        }

        shared_ptr<AsyncOperation> BeginSendRequest(
            MessageUPtr && message,
            wstring const &,
            TimeSpan const & timeout,
            AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
        {
            MessageId id = message->MessageId;

            auto operation = make_shared<RequestAsyncOperation>(
                this->table_,
                id,
                timeout,
                callback,
                parent);
            operation->Start(operation);

            auto error = this->table_.TryInsertEntry(id, operation);
            if (error.IsSuccess())
            {
                if (operation->IsCompleted)
                {
                    // If there was a racing and we added to the table after OnComplete ran,
                    // we must remove it again since it will not itself.
                    this->table_.TryRemoveEntry(id, operation);
                }
            }
            else
            {
                operation->TryComplete(operation, error);
            }

            return operation;
        }

        ErrorCode EndSendRequest(shared_ptr<AsyncOperation> const & operation, MessageUPtr & message)
        {
            return RequestAsyncOperation::End(operation, message);
        }

        void CompleteRequest(MessageId const & messageId)
        {
            RequestAsyncOperationSPtr operation;
            if (this->table_.TryRemoveEntry(messageId, operation))
            {
                // we removed it from the table so complete with a reply message
                VERIFY_IS_TRUE(operation);

                operation->SetReply(make_unique<Message>());
                operation->TryComplete(operation, ErrorCode());
            }
            else
            {
                VERIFY_IS_TRUE(!operation);
            }
        }

        void Close()
        {
            this->table_.Close();
        }

    private:
        RequestTable table_;
    };

    BOOST_AUTO_TEST_SUITE2(RequestTableTests)

    BOOST_AUTO_TEST_CASE(BasicTest)
    {
        TestClient siteNode;
        vector<MessageId> messageIds;

        for (int i = 0; i < 10; ++i)
        {
            auto request = make_unique<Message>();
            request->Headers.Add(MessageIdHeader());

            messageIds.push_back(request->MessageId);

            auto result = siteNode.BeginSendRequest(std::move(request), L"localhost", TimeSpan::FromSeconds(5), 
                [&siteNode](AsyncOperationSPtr contextSPtr) -> void
                {
                    MessageUPtr resultSPtr;
                    ErrorCode error = siteNode.EndSendRequest(contextSPtr, resultSPtr);

                    Trace.WriteInfo(TraceType, "Success = {0}", error.IsSuccess());
                    Trace.WriteInfo(TraceType, "result = '{0}'", (resultSPtr ? "no message" : "message"));
                },
                AsyncOperationSPtr());
        }

        for(MessageId const & id : messageIds)
        {
            siteNode.CompleteRequest(id);
        }
    }

    BOOST_AUTO_TEST_CASE(BasicTest2)
    {
        auto siteNode = make_shared<TestClient>();
        auto request = make_unique<Message>();

        // create a message
        request->Headers.Add(MessageIdHeader());

        auto result = siteNode->BeginSendRequest(std::move(request), L"localhost", TimeSpan::FromSeconds(5),
            [](AsyncOperationSPtr contextSPtr) -> void
            {
                auto callbackSiteNode = AsyncOperationRoot<std::shared_ptr<TestClient> >::Get(contextSPtr->Parent);

                MessageUPtr resultSPtr;
                ErrorCode error = callbackSiteNode->EndSendRequest(contextSPtr, resultSPtr);

                Trace.WriteInfo(TraceType, "Success = {0}", error.IsSuccess());
                Trace.WriteInfo(TraceType, "result = '{0}'", (resultSPtr ? "no message" : "message"));
            }, 
            AsyncOperationRoot<std::shared_ptr<TestClient> >::Create(siteNode)
            );

        siteNode->CompleteRequest(request->MessageId);
    }

    BOOST_AUTO_TEST_CASE(BasicAsyncTest)
    {
        TestClient siteNode;
        auto request = make_unique<Message>();

        // create a message
        request->Headers.Add(MessageIdHeader());
        auto id = request->MessageId;

        ManualResetEvent callbackFired(false);

        auto result = siteNode.BeginSendRequest(std::move(request), L"localhost", TimeSpan::FromSeconds(5), 
            [&siteNode, &callbackFired](AsyncOperationSPtr contextSPtr) -> void
            {
                MessageUPtr resultSPtr;
                ErrorCode error = siteNode.EndSendRequest(contextSPtr, resultSPtr);

                Trace.WriteInfo(TraceType, "Success = {0}", error.IsSuccess());
                Trace.WriteInfo(TraceType, "result = '{0}'", (resultSPtr ? "no message" : "message"));

                callbackFired.Set();
            }, AsyncOperationSPtr());

        Threadpool::Post(
            [&siteNode, id]() -> void
            {
                siteNode.CompleteRequest(id);
            });

        VERIFY_IS_TRUE(callbackFired.WaitOne(TimeSpan::FromSeconds(10)));
    }

    BOOST_AUTO_TEST_CASE(TimeoutTest)
    {
        TestClient siteNode;
        auto request = make_unique<Message>();

        // create a message
        request->Headers.Add(MessageIdHeader());

        ManualResetEvent callbackFired(false);
        Stopwatch timer;
        timer.Start();

        auto result = siteNode.BeginSendRequest(std::move(request), L"localhost", TimeSpan::FromSeconds(2),
            [&siteNode, &callbackFired](AsyncOperationSPtr contextSPtr) -> void
            {
                MessageUPtr resultSPtr;
                ErrorCode error = siteNode.EndSendRequest(contextSPtr, resultSPtr);

                VERIFY_IS_TRUE(error.ReadValue() == ErrorCodeValue::Timeout);
                Trace.WriteInfo(TraceType, "result = '{0}'", (resultSPtr ? "no message" : "message"));

                callbackFired.Set();
            }, AsyncOperationSPtr());

        VERIFY_IS_TRUE(callbackFired.WaitOne(TimeSpan::FromSeconds(10)));
        timer.Stop();

        Trace.WriteInfo(TraceType, "Elapesed: {0}", timer.Elapsed.TotalMilliseconds());
        VERIFY_IS_TRUE(timer.Elapsed > TimeSpan::FromSeconds(1.5));
    }

    BOOST_AUTO_TEST_CASE(CloseTest)
    {
        TestClient siteNode;
        auto request = make_unique<Message>();

        // create a message
        request->Headers.Add(MessageIdHeader());

        ManualResetEvent callbackFired(false);

        siteNode.Close();

        auto result = siteNode.BeginSendRequest(std::move(request), L"localhost", TimeSpan::FromSeconds(2),
            [&siteNode, &callbackFired](AsyncOperationSPtr contextSPtr) -> void
            {
                MessageUPtr resultSPtr;
                ErrorCode error = siteNode.EndSendRequest(contextSPtr, resultSPtr);

                VERIFY_IS_TRUE(error.ReadValue() == ErrorCodeValue::ObjectClosed);
                Trace.WriteInfo(TraceType, "result = '{0}'", (resultSPtr ? "no message" : "message"));

                callbackFired.Set();
            }, AsyncOperationSPtr());

        VERIFY_IS_TRUE(callbackFired.WaitOne(TimeSpan::FromSeconds(5)));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
