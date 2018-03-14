// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            /*
                RA Wrapper around threadpool for testing
            */
            class IThreadpool
            {
                DENY_COPY(IThreadpool);

            public:

                virtual ~IThreadpool();

                virtual void ExecuteOnThreadpool(Common::ThreadpoolCallback callback) = 0;

                virtual bool EnqueueIntoMessageQueue(MessageProcessingJobItem<ReconfigurationAgent> &) = 0;

                virtual Common::AsyncOperationSPtr BeginScheduleEntity(
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent) = 0;

                virtual Common::ErrorCode EndScheduleEntity(
                    Common::AsyncOperationSPtr const & op) = 0;

                virtual Common::AsyncOperationSPtr BeginScheduleCommitCallback(
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent) = 0;

                virtual Common::ErrorCode EndScheduleCommitCallback(
                    Common::AsyncOperationSPtr const & op) = 0;

                virtual void Close() = 0;

            protected:
                IThreadpool();
            };

            class Threadpool : public IThreadpool
            {
                DENY_COPY(Threadpool);
            public:
                Threadpool(ReconfigurationAgent & root);

                void ExecuteOnThreadpool(Common::ThreadpoolCallback callback) override;

                bool EnqueueIntoMessageQueue(MessageProcessingJobItem<ReconfigurationAgent> &) override;

                Common::AsyncOperationSPtr BeginScheduleEntity(
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent) override;

                Common::ErrorCode EndScheduleEntity(
                    Common::AsyncOperationSPtr const & op) override;

                Common::AsyncOperationSPtr BeginScheduleCommitCallback(
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent) override;

                Common::ErrorCode EndScheduleCommitCallback(
                    Common::AsyncOperationSPtr const & op) override;

                void Close() override;

                static Infrastructure::IThreadpoolUPtr Factory(ReconfigurationAgent & ra)
                {
                    return Common::make_unique<Infrastructure::Threadpool>(ra);
                }

            private:
                typedef Common::JobQueue<MessageProcessingJobItem<ReconfigurationAgent>, ReconfigurationAgent> RAJobQueue;
                RAJobQueue messageQueue_;

                AsyncJobQueue entityJobQueue_;
                AsyncJobQueue commitCallbackJobQueue_;
            };
        }
    }
}

