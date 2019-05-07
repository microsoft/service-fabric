// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        // Helper struct for passing data to the message processor
        template<typename TBody, typename TEntity>
        class MessageContext 
        {
            DENY_COPY(MessageContext);

        public:
            typedef typename Infrastructure::EntityTraits<TEntity>::HandlerParametersType HandlerParametersType;
            typedef typename Infrastructure::EntityTraits<TEntity>::DataType DataType;

            static const bool HasCorrelatedTrace = true;
            static const bool HasTraceMetadata = true;

            // Not a std::function for simplicity as capturing is not needed
            typedef void (ReconfigurationAgent::*MessageProcessorFunctionPtr)(HandlerParametersType &, MessageContext<TBody, TEntity> &);

            MessageContext(
                Communication::MessageMetadata const * metadata,
                std::wstring const & action,
                TBody && body,
                GenerationHeader const & generationHeader,
                Federation::NodeInstance const & from,
                MessageProcessorFunctionPtr messageProcessor)
            : action_(action),
              body_(std::move(body)),
              from_(from),
              messageProcessor_(messageProcessor),
              generationHeader_(generationHeader),
              metadata_(metadata)
            {
                ASSERT_IF(messageProcessor_ == nullptr, "Message processor cannot be null");
                ASSERT_IF(metadata == nullptr, "Metadata cannot be null");
            }

            MessageContext(MessageContext && other)
            : action_(std::move(other.action_)),
              body_(std::move(other.body_)),
              from_(std::move(other.from_)),
              generationHeader_(std::move(other.generationHeader_)),
              messageProcessor_(std::move(other.messageProcessor_)),
              metadata_(std::move(other.metadata_))
            {
            }

            __declspec(property(get=get_From)) Federation::NodeInstance const & From;
            Federation::NodeInstance const & get_From() const { return from_; }       

            __declspec(property(get=get_Body)) TBody const & Body;
            TBody const & get_Body() const { return body_; }

            __declspec(property(get=get_Action)) std::wstring const & Action;
            std::wstring const & get_Action() const { return action_; }

            Infrastructure::JobItemCheck::Enum GetJobItemCheck() const
            {
                int check = Infrastructure::JobItemCheck::None;
                
                if (metadata_->ProcessDuringNodeClose)
                {
                    check |= Infrastructure::JobItemCheck::RAIsOpenOrClosing;
                }
                else
                {
                    check |= Infrastructure::JobItemCheck::RAIsOpen;
                }
                     
                if (!metadata_->CreatesEntity)
                {
                    check |= Infrastructure::JobItemCheck::FTIsNotNull;
                }
                
                return static_cast<Infrastructure::JobItemCheck::Enum>(check);
            }

            std::wstring const & GetTraceMetadata() const
            {
                return action_;
            }

            void Trace(
                std::string const & id,
                Federation::NodeInstance const & nodeInstance,
                std::wstring const & activityId) const
            {
                RAEventSource::Events->MessageAction(id, nodeInstance, activityId, action_, Common::TraceCorrelatedEvent<TBody>(body_));
            }

            bool Process(HandlerParametersType & jobItemHandlerParameters)
            {
                TraceMessageProcessStart(jobItemHandlerParameters);

                // If the entity is null then create it
                // This is possible if this message created the ft
                ASSERT_IF(!jobItemHandlerParameters.Entity && !metadata_->CreatesEntity, "Entity cannot be null if the inner handler does not support handling null FT");

                if (!CheckGenerationHeader(jobItemHandlerParameters))
                {
                    return false;
                }

                if (!CheckStaleness(jobItemHandlerParameters.Entity.Current))
                {
                    return false;
                }

                if (!CheckStaleness(jobItemHandlerParameters.Entity.Current))
                {
                    return false;
                }

                InvokeProcessor(jobItemHandlerParameters);

                return true;
            }

        private:
            bool CheckStaleness(DataType const * dataType) const
            {
                return EntityTraits<TEntity>::CheckStaleness(
                    metadata_->StalenessCheckType,
                    action_,
                    body_,
                    dataType);
            }

            bool CheckGenerationHeader(HandlerParametersType & jobItemHandlerParameters) const
            {
                if (generationHeader_.Generation == Reliability::GenerationNumber::Zero)
                {
                    return true;
                }

                // The generation header needs to be validated inside the FT lock
                // This is to ensure that the state of the LFUM does not change after having processed a generation update message
                // and uploaded the LFUM to the FM
                return jobItemHandlerParameters.RA.GenerationStateManagerObj.CheckGenerationHeader(generationHeader_, action_, from_);
            }

            void InvokeProcessor(HandlerParametersType & jobItemHandlerParameters)
            {
                ((jobItemHandlerParameters.RA).*(messageProcessor_))(jobItemHandlerParameters, *this);
            }

            void TraceMessageProcessStart(HandlerParametersType & jobItemHandlerParameters) const
            {
                if (!jobItemHandlerParameters.Entity)
                {
                    return;
                }

                auto const & entity = *jobItemHandlerParameters.get_Entity();
                Infrastructure::RAEventSource::Events->MessageProcessStart(
                    jobItemHandlerParameters.TraceId,
                    jobItemHandlerParameters.RA.NodeInstance,
                    jobItemHandlerParameters.ActivityId,
                    action_,
                    Common::TraceCorrelatedEvent<TBody>(body_),
                    entity);
            }

            std::wstring const action_;
            TBody body_;
            Federation::NodeInstance const from_;
            GenerationHeader const generationHeader_;
            MessageProcessorFunctionPtr messageProcessor_ ;
            Communication::MessageMetadata const * metadata_;
        };
    }
}
