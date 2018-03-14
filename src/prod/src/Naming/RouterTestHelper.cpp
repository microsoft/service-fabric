// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "RouterTestHelper.h"

using namespace Transport;
using namespace Common;
using namespace Federation;
using namespace std;

namespace Naming
{
    namespace TestHelper
    {
        RouteAsyncOperation::RouteAsyncOperation(
            MessageUPtr && request, 
            __in INamingStoreServiceRouter & messageProcessor,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & root)
            : AsyncOperation(callback, root)
            , request_(std::move(request))
            , messageProcessor_(messageProcessor)
            , timeout_(timeout)
        {
            messageId_ = request_->MessageId;
        }

        ErrorCode RouteAsyncOperation::End(AsyncOperationSPtr const & asyncOperation, MessageUPtr & result)
        {
            auto casted = AsyncOperation::End<RouteAsyncOperation>(asyncOperation);
            swap(casted->reply_, result);

            return casted->Error;
        }

        void RouteAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            messageProcessor_.BeginProcessRequest(
                std::move(request_),
                timeout_,
                [&](AsyncOperationSPtr const & operation) { OnProcessRequestComplete(operation); },
                thisSPtr);
        }

        void RouteAsyncOperation::OnProcessRequestComplete(AsyncOperationSPtr const & operation)
        {
            auto error = messageProcessor_.EndProcessRequest(operation, reply_);

            this->TryComplete(operation->Parent, error);
        }

        //
        // *** RouterTestHelper
        //

        RouterTestHelper::RouterTestHelper()
            : lock_()
            , pendingRequests_()
            , serviceNodes_()
            , blocking_()
            , blockAll_(false)
            , closed_(false)
            , pendingOperationsEmptyEvent_()
        {
        }

        void RouterTestHelper::Initialize(__in INamingStoreServiceRouter & messageProcessor, NodeId predNodeId, NodeId succNodeId)
        {
            NodeId thisNodeId = messageProcessor.Id;
            NodeIdRange range(
                (thisNodeId == predNodeId && thisNodeId == succNodeId)
                ? NodeIdRange::Full
                : NodeIdRange(thisNodeId.GetPredMidPoint(predNodeId), thisNodeId.GetSuccMidPoint(succNodeId)));

            Trace.WriteNoise(Constants::TestSource, "{0} owns range {1}", thisNodeId, range);

            AcquireExclusiveLock lock(lock_);

            auto iter = find_if(serviceNodes_.begin(), serviceNodes_.end(),
                [&](ServiceNodeEntry const & entry) -> bool { return entry.MessageProcessor.Id == thisNodeId; });
            CODING_ERROR_ASSERT(iter == serviceNodes_.end());

            serviceNodes_.push_back(ServiceNodeEntry(range, messageProcessor));
        }

        ErrorCode RouterTestHelper::Close()
        {
            // Mark that the object is closed, so no more operations can be started
            {
                AcquireExclusiveLock lock(lock_);
                closed_ = true;
                if (pendingRequests_.empty())
                {
                    return ErrorCode(ErrorCodeValue::Success);
                }
            }

            // Wait for all operations to finish
            bool operationsFinished = pendingOperationsEmptyEvent_.WaitOne(10000);
            if (operationsFinished)
            {
                return ErrorCode(ErrorCodeValue::Success);
            }
            else
            {
                return ErrorCode(ErrorCodeValue::Timeout);
            }
        }

        void RouterTestHelper::BlockNextRequest(NodeId const receiver)
        {
            AcquireExclusiveLock lock(lock_);
            blocking_.push_back(receiver);
        }

        AsyncOperationSPtr RouterTestHelper::OnBeginRoute(
            MessageUPtr && message,
            NodeId nodeId,
            uint64 instance,
            std::wstring const & ringName,
            bool useExactRouting,
            TimeSpan retryTimeout,
            TimeSpan timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
        {
            return BeginRouteRequest(std::move(message), nodeId, instance, ringName, useExactRouting, retryTimeout, timeout, callback, parent);
        }

        ErrorCode RouterTestHelper::EndRoute(AsyncOperationSPtr const & operation)
        {
            MessageUPtr blankReply;
            return EndRouteRequest(operation, blankReply);
        }

        void RouterTestHelper::BlockAllRequests() 
        { 
            AcquireExclusiveLock lock(lock_);
            blockAll_ = true; 
        }
        
        void RouterTestHelper::UnblockAllRequests() 
        {
            AcquireExclusiveLock lock(lock_);
            blockAll_ = false; 
        }

        AsyncOperationSPtr RouterTestHelper::OnBeginRouteRequest(
            MessageUPtr && request,
            NodeId nodeId,
            uint64,
            wstring const &,
            bool,
            TimeSpan,
            TimeSpan timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
        {
            // Routing destination is retrieved based on node ID coverage
            INamingStoreServiceRouter * messageProcessor = NULL;
            bool shouldRoute = false;

            AsyncOperationSPtr routeOperation;

            {
                AcquireExclusiveLock lock(lock_);
                if (closed_)
                {
                    Trace.WriteNoise(Constants::TestSource, "Blocking request from {0}: object closed", nodeId);

                    routeOperation = make_shared<CompletedAsyncOperation>(
                        ErrorCode(ErrorCodeValue::ObjectClosed),
                        callback,
                        parent);
                }
                else
                {
                    auto iter = find_if(serviceNodes_.begin(), serviceNodes_.end(), 
                        [&](ServiceNodeEntry const & entry) -> bool { return entry.Range.Contains(nodeId); });
                    VERIFY_IS_TRUE(iter != serviceNodes_.end());
                    messageProcessor = &(iter->MessageProcessor);
                           
                    if (find(blocking_.begin(), blocking_.end(), nodeId) == blocking_.end())
                    {
                        shouldRoute = true;
                    }
                    else
                    {
                        blocking_.erase(find(blocking_.begin(), blocking_.end(), nodeId));
                        shouldRoute = false;
                    }

                    if (shouldRoute && !blockAll_)
                    {
                        request->Headers.Add(MessageIdHeader());
                        request->Headers.Add(ExpectsReplyHeader(true));

                        auto messageId = request->MessageId;
                        Trace.WriteNoise(Constants::TestSource, "Routing request {0} for {1} to {2}", *request, nodeId, messageProcessor->Id);

                        routeOperation = make_shared<RouteAsyncOperation>(
                            std::move(request),
                            *messageProcessor, 
                            timeout,
                            callback,
                            parent);
                        pendingRequests_[messageId] = routeOperation;
                    }
                    else
                    {
                        Trace.WriteNoise(Constants::TestSource, "Blocking request for {0} to {1}", nodeId, messageProcessor->Id);

                        routeOperation = make_shared<CompletedAsyncOperation>(
                            ErrorCode(ErrorCodeValue::Timeout),
                            callback,
                            parent);
                    }
                }
            }
                                   
            routeOperation->Start(routeOperation);
            return routeOperation;
        }

        ErrorCode RouterTestHelper::EndRouteRequest(AsyncOperationSPtr const & asyncOperation, MessageUPtr & reply) const
        {
            ErrorCode error;
            if (asyncOperation->Error.IsError(ErrorCodeValue::Timeout) || asyncOperation->Error.IsError(ErrorCodeValue::ObjectClosed))
            {
                // operation was blocked by RouterTestHelper
                error = CompletedAsyncOperation::End(asyncOperation);
            }
            else
            {
                error = RouteAsyncOperation::End(asyncOperation, reply);

                {
                    AcquireExclusiveLock lock(lock_);
                
                    auto iter = pendingRequests_.find(AsyncOperation::Get<RouteAsyncOperation>(asyncOperation)->MessageId);
                
                    VERIFY_IS_TRUE(iter != pendingRequests_.end());
                    pendingRequests_.erase(iter);

                    if (closed_ && pendingRequests_.empty())
                    {
                        pendingOperationsEmptyEvent_.Set();
                    }
                }
            }

            return error;
        }
    }
}
