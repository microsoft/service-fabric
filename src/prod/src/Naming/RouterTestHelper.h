// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Transport/Message.h"
#include "Common/AsyncOperation.h"
#include "Common/RwLock.h"
#include "Federation/IRouter.h"
#include "Federation/NodeIdRange.h"
#include "Naming/INamingStoreServiceRouter.h"

namespace Naming
{
    namespace TestHelper
    {
        class RouteAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY( RouteAsyncOperation );

        public:
            RouteAsyncOperation(
                Transport::MessageUPtr &&, 
                __in INamingStoreServiceRouter &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            __declspec(property(get=get_MessageId)) Transport::MessageId MessageId;
            inline Transport::MessageId get_MessageId() { return messageId_; }

            static Common::ErrorCode End(Common::AsyncOperationSPtr const &, Transport::MessageUPtr &);

        private:
            Transport::MessageUPtr request_;
            INamingStoreServiceRouter & messageProcessor_;
            Common::TimeSpan timeout_;
            Transport::MessageId messageId_;
            Transport::MessageUPtr reply_;

            void OnStart(Common::AsyncOperationSPtr const &);

            void OnProcessRequestComplete(Common::AsyncOperationSPtr const &);
        };

        class RouterTestHelper : public Federation::IRouter
        {
            DENY_COPY( RouterTestHelper );

        public:
            RouterTestHelper();

            void Initialize(__in INamingStoreServiceRouter &, Federation::NodeId, Federation::NodeId);

            Common::ErrorCode EndRouteRequest(Common::AsyncOperationSPtr const &, Transport::MessageUPtr &) const;

            void BlockNextRequest(Federation::NodeId const receiver);

            void BlockAllRequests();
            void UnblockAllRequests();

            Common::ErrorCode Close();

        protected:
            Common::AsyncOperationSPtr OnBeginRoute(
                Transport::MessageUPtr &&,
                Federation::NodeId,
                uint64,
                std::wstring const &,
                bool,
                Common::TimeSpan,
                Common::TimeSpan,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            Common::AsyncOperationSPtr OnBeginRouteRequest(
                Transport::MessageUPtr &&,
                Federation::NodeId,
                uint64,
                std::wstring const &,
                bool,
                Common::TimeSpan,
                Common::TimeSpan,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

        private:
            struct ServiceNodeEntry
            {
                ServiceNodeEntry(Federation::NodeIdRange range, INamingStoreServiceRouter & messageProcessor)
                    : nodeIdRange_(range)
                    , messageProcessor_(messageProcessor)
                {
                }

                ServiceNodeEntry & operator=(ServiceNodeEntry const & rhs)
                {
                    nodeIdRange_ = rhs.Range;
                    messageProcessor_ = rhs.MessageProcessor;

                    return *this;
                }

                __declspec(property(get=get_NodeIdRange)) Federation::NodeIdRange Range;
                __declspec(property(get=get_MessageProcessor)) INamingStoreServiceRouter & MessageProcessor;

                Federation::NodeIdRange get_NodeIdRange() const { return nodeIdRange_; }
                INamingStoreServiceRouter & get_MessageProcessor() const { return messageProcessor_; }                

            private:
                Federation::NodeIdRange nodeIdRange_;
                INamingStoreServiceRouter & messageProcessor_;
            };           

            // Unused member functions from IRouter.h
            
            Common::ErrorCode EndRoute(Common::AsyncOperationSPtr const &);

            mutable Common::ExclusiveLock lock_;
            mutable std::map<Transport::MessageId, Common::AsyncOperationSPtr> pendingRequests_;
            std::vector<ServiceNodeEntry> serviceNodes_;
            std::vector<Federation::NodeId> blocking_;
            bool blockAll_;
            bool closed_;
            mutable Common::ManualResetEvent pendingOperationsEmptyEvent_;
        };
    }
}
