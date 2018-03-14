// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability {

    template <typename R>
    class MessageProcessingJobItem
    {
    public:

        MessageProcessingJobItem() = default; //default constructor necessary for limitations on jobqueue.h.
                                              //This can be removed in the event of a future refactoring.

        MessageProcessingJobItem(
            IMessageMetadata const * metadata,
            Transport::MessageUPtr && message,
            Transport::IpcReceiverContextUPtr && context) :
            metadata_(metadata),
            message_(std::move(message)),
            ipcContext_(std::move(context))
        {
        }

        MessageProcessingJobItem(
            IMessageMetadata const * metadata,
            Transport::MessageUPtr && message,
            Federation::OneWayReceiverContextUPtr && context) :
            metadata_(metadata),
            message_(std::move(message)),
            oneWayReceiverContext_(std::move(context))
        {
        }

        MessageProcessingJobItem(
            IMessageMetadata const * metadata,
            Transport::MessageUPtr && message,
            Federation::RequestReceiverContextUPtr && context) :
            metadata_(metadata), 
            message_(std::move(message)), 
            requestReceiverContext_(std::move(context))
        {
        }

        MessageProcessingJobItem(MessageProcessingJobItem && other) : 
            metadata_(std::move(other.metadata_)),
            message_(std::move(other.message_)), 
            ipcContext_(std::move(other.ipcContext_)), 
            requestReceiverContext_(std::move(other.requestReceiverContext_)), 
            oneWayReceiverContext_(std::move(other.oneWayReceiverContext_))
        {
        }

        MessageProcessingJobItem & operator=(MessageProcessingJobItem && other)
        {
            if (this != &other)
            {
                message_ = std::move(other.message_);
                requestReceiverContext_ = std::move(other.requestReceiverContext_);
                ipcContext_ = std::move(other.ipcContext_);
                oneWayReceiverContext_ = std::move(other.oneWayReceiverContext_);
                metadata_ = std::move(other.metadata_);
            }

            return *this;
        }

        ~MessageProcessingJobItem() = default;

        bool ProcessJob(R & root) //requirement from jobqueue.h
        {
            Process(root);
            return true;
        }

        //R should be a class that implements IMessageHandler
        void Process(R & root)
        {            
            if (ipcContext_ != nullptr)
            {
                root.ProcessIpcMessage(metadata_, std::move(message_), std::move(ipcContext_));
            }
            else if (requestReceiverContext_ != nullptr)
            {
                root.ProcessRequestResponseRequest(metadata_, std::move(message_), std::move(requestReceiverContext_));
            }
            else
            {
                root.ProcessRequest(metadata_, std::move(message_), std::move(oneWayReceiverContext_));
            }
        }

    private:
        IMessageMetadata const * metadata_;
        Transport::MessageUPtr message_;
        Transport::IpcReceiverContextUPtr  ipcContext_;
        Federation::RequestReceiverContextUPtr  requestReceiverContext_;
        Federation::OneWayReceiverContextUPtr oneWayReceiverContext_;
    };
}
