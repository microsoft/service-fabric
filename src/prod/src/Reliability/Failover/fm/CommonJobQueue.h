// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class FailoverManager::OneWayMessageHandlerJobItem : public Common::CommonTimedJobItem<FailoverManager>
        {
            DENY_COPY(OneWayMessageHandlerJobItem);

        public:

            OneWayMessageHandlerJobItem(
                Transport::MessageUPtr && message,
                Federation::OneWayReceiverContextUPtr && context,
                Common::TimeSpan timeout);

            virtual void Process(FailoverManager & fm) override;

            virtual void OnTimeout(FailoverManager & fm) override;

            virtual void OnQueueFull(FailoverManager & fm, size_t actualQueueSize) override;

        private:

            Transport::MessageUPtr message_;
            Federation::OneWayReceiverContextUPtr context_;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class FailoverManager::RequestReplyMessageHandlerJobItem : public Common::CommonTimedJobItem<FailoverManager>
        {
            DENY_COPY(RequestReplyMessageHandlerJobItem)

        public:

            RequestReplyMessageHandlerJobItem(
                Transport::MessageUPtr && message,
                TimedRequestReceiverContextUPtr && context,
                Common::TimeSpan timeout);

            virtual void Process(FailoverManager & fm) override;

            virtual void OnTimeout(FailoverManager & fm) override;

            virtual void OnQueueFull(FailoverManager & fm, size_t actualQueueSize) override;

        private:

            Transport::MessageUPtr message_;
            TimedRequestReceiverContextUPtr context_;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class FailoverManager::NodeSequenceNumberCommitJobItem : public Common::CommonTimedJobItem<FailoverManager>
        {
            DENY_COPY(NodeSequenceNumberCommitJobItem);

        public:

            NodeSequenceNumberCommitJobItem(
                ServiceTypeUpdateKind::Enum serviceTypeUpdateEvent,
                vector<ServiceModel::ServiceTypeIdentifier> && serviceTypeIds,
                uint64 sequenceNumber,
                Transport::MessageId const& messageId,
                Federation::NodeInstance const& from);

            void Process(FailoverManager & fm) override;

            void OnTimeout(FailoverManager & fm) override;

            virtual void OnQueueFull(FailoverManager & fm, size_t actualQueueSize) override;

        private:

            ServiceTypeUpdateKind::Enum serviceTypeUpdateEvent_;
            vector<ServiceModel::ServiceTypeIdentifier> serviceTypeIds_;
            uint64 sequenceNumber_;
            Transport::MessageId messageId_;
            Federation::NodeInstance from_;
        };
    }
}
