// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class RebuildContext
        {
            DENY_COPY(RebuildContext);

        public:
            RebuildContext(FailoverManager & fm, Common::RwLock & lock);

            void Start();
            void Stop();

            void CheckRecovery();
            void NodeDown(Federation::NodeInstance const & nodeInstance);

            void ProcessGenerationUpdateReject(Transport::Message & message, Federation::NodeInstance const & from);
            Transport::MessageUPtr ProcessLFUMUpload(LFUMMessageBody const& body, Federation::NodeInstance const & from);

        private:
            __declspec (property(get=get_Generation, put=set_Generation)) Reliability::GenerationNumber const& Generation;
            Reliability::GenerationNumber const& get_Generation() const;

            bool IsActive() const;

            void StartProposal();
            void PostReceiveForProposalReply(bool canProcessSynchronously);
            void ProcessProposalReply(Common::AsyncOperationSPtr const & operation, Federation::IMultipleReplyContextSPtr const & proposeContext);
            void ProcessProposalReplyCallerHoldingLock(Common::AsyncOperationSPtr const & operation, Federation::IMultipleReplyContextSPtr const & proposeContext);

            void StartUpdate();
            void NodeUp(Federation::NodeInstance const & nodeInstance);
            Transport::MessageUPtr CreateGenerationUpdateMessage();
            void OnTimer();

            void Complete();

            void CloseProposalContext();

            FailoverManager & fm_;

            bool updateCompleted_;
            bool recoverCompleted_;
            Federation::IMultipleReplyContextSPtr proposeContext_;
            std::map<Federation::NodeId, Federation::NodeInstance> pendingNodes_;

            std::unique_ptr<GenerationNumber> generationNumber_;

            Common::TimerSPtr timer_;

            Common::RwLock & lock_;
        };

        typedef std::unique_ptr<RebuildContext> RebuildContextUPtr;
    }
}
