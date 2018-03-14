// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class GenerationStateManager
        {
            DENY_COPY(GenerationStateManager);

        public:
            GenerationStateManager(ReconfigurationAgent & ra);

#pragma region Generation Header            
            void AddGenerationHeader(
                Transport::Message & message, 
                bool isForFMReplica);

            bool CheckGenerationHeader(
                GenerationHeader const & generationHeader, 
                std::wstring const & action, 
                Federation::NodeInstance const & from);

#pragma endregion

            GenerationProposalReplyMessageBody ProcessGenerationProposal(
                Transport::Message & request, 
                bool & isGfumUploadOut);

            void ProcessLFUMUploadReply(
                Reliability::GenerationHeader const & generationHeader);

            void ProcessGenerationUpdate(
                std::wstring const & activityId, 
                GenerationHeader const & generationHeader, 
                GenerationUpdateMessageBody const & body, 
                Federation::NodeInstance const& from);

            void ProcessNodeUpAck(
                std::wstring const & activityId, 
                GenerationHeader const & header);

            GenerationState & Test_GetGenerationState(bool isForFMReplica)
            {
                return GetGenerationState(isForFMReplica);
            }

            bool GetLfumProcessor(
                Infrastructure::HandlerParameters & handlerParameters, 
                GetLfumJobItemContext & context);

            void FinishGetLfumProcessing(
                Infrastructure::MultipleEntityWork const & work);

            bool GenerationUpdateProcessor(
                Infrastructure::HandlerParameters & handlerParameters, 
                GenerationUpdateJobItemContext & context);

        private:
            class PreProcessGenerationUpdateFromFMResult
            {
            public:
                enum Enum
                {
                    UploadLfum = 0,
                    EnqueueFMReplicaJobItem = 1,
                    StaleMessage = 2,
                };
            };

            bool TryUpdateReceiveGenerationFromGenerationUpdateMessage(
                std::wstring const & activityId, 
                GenerationState & generationState, 
                GenerationHeader const & header);

            bool TryValidateGenerationUpdateMessageCallerHoldsLock(
                std::wstring const & activityId, 
                GenerationState & generationState, 
                GenerationHeader const & header);

            void UpdateReceiveGenerationFromGenerationUpdateMessageCallerHoldsLock(
                std::wstring const & activityId, 
                GenerationState & generationState, 
                GenerationHeader const & header);

            PreProcessGenerationUpdateFromFMResult::Enum PreProcessGenerationUpdateFromFM(
                std::wstring const & activityId, 
                GenerationHeader const & generationHeader, 
                Federation::NodeInstance const & from, 
                Infrastructure::EntityEntryBaseSPtr & fmFailoverUnitEntry);

            void ProcessGenerationUpdateFromFmm(
                std::wstring const & activityId, 
                GenerationHeader const & generationHeader, 
                GenerationUpdateMessageBody const & body, 
                Federation::NodeInstance const& from);

            void ProcessGenerationUpdateFromFM(
                std::wstring const & activityId, 
                GenerationHeader const & generationHeader, 
                GenerationUpdateMessageBody const & body, 
                Federation::NodeInstance const& from);

            void StartUploadLfum(
                std::wstring const & activityId, 
                GenerationHeader const & header);
            
            GenerationState & GetGenerationState(
                bool isForFMReplica);

            ReconfigurationAgent & ra_;

            Common::RwLock lock_;
            GenerationState commonGenerations_;
            GenerationState fmReplicaGenerations_;
        };
    }
}

