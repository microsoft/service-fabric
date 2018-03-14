// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        // Message handling logic for the RA
        class MessageHandler : public IMessageHandler
        {
            DENY_COPY(MessageHandler);
        public:
            MessageHandler(ReconfigurationAgent & ra);

#pragma region Functions invoked on transport thread
            void ProcessTransportRequest(
                Transport::MessageUPtr & message, 
                Federation::OneWayReceiverContextUPtr && context);

            void ProcessTransportIpcRequest(
                Transport::MessageUPtr & message, 
                Transport::IpcReceiverContextUPtr && context);

            void ProcessTransportRequestResponseRequest(
                Transport::MessageUPtr & message, 
                Federation::RequestReceiverContextUPtr && context);

#pragma endregion

            void ProcessRequest(
                IMessageMetadata const * metadata,
                Transport::MessageUPtr && requestPtr,
                Federation::OneWayReceiverContextUPtr && context) override;

            void ProcessIpcMessage(
                IMessageMetadata const * metadata,
                Transport::MessageUPtr && messagePtr,
                Transport::IpcReceiverContextUPtr && context) override;

            void ProcessRequestResponseRequest(
                IMessageMetadata const * metadata,
                Transport::MessageUPtr && requestPtr,
                Federation::RequestReceiverContextUPtr && context) override;

            Infrastructure::EntityJobItemBaseSPtr Test_CreateFTJobItem(
                Communication::MessageMetadata const * metadata,
                Transport::MessageUPtr && requestPtr,
                GenerationHeader const & generationHeader,
                Federation::NodeInstance const & from)
            {
                return CreateJobItemForFTMessage(metadata, std::move(requestPtr), generationHeader, from);
            }

            bool CanProcessMessageBasedOnNodeLifeCycle(
                Communication::MessageMetadata const & metadata) const;

        private:
            template<typename TContext>
            void Reject(TContext & context);

            void ProcessFTMessage(
                Communication::MessageMetadata const * metadata,
                Transport::MessageUPtr && requestPtr,
                GenerationHeader const & generationHeader,
                Federation::NodeInstance const & from);

            template<typename TContext> 
            void RejectAndTrace(
                TContext & context,
                Common::StringLiteral const & reason,
                wstring const & action,
                Transport::MessageId const & id);

            template<typename TContext>
            void ProcessRequestHelper(
                bool assertOnUnknownMessage,
                Common::PerformanceCounterData* perfCounter,
                Transport::MessageUPtr & message,
                std::unique_ptr<TContext> && context);

            Infrastructure::EntityJobItemBaseSPtr CreateJobItemForFTMessage(
                Communication::MessageMetadata const * metadata,
                Transport::MessageUPtr && requestPtr,
                GenerationHeader const & generationHeader,
                Federation::NodeInstance const & from);

            bool MessageHandler::ReadAndCheckGenerationHeader(
                Transport::Message & message,
                Federation::NodeInstance const & from,
                __out GenerationHeader & generationHeader);

            ReconfigurationAgent & ra_;
        };
    }
}



