// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class MulticastOperation : public Common::AsyncOperation
    {
    public:
        MulticastOperation(Common::AsyncCallback const & callback, Common::AsyncOperationSPtr const & parent)
            : Common::AsyncOperation(callback, parent)
        {
        }

        void OnStart(Common::AsyncOperationSPtr const &)
        {
        }

        void MoveResults(__out std::vector<NodeInstance> & failedTargets, __out std::vector<NodeInstance> & pendingTargets)
        {
            failedTargets = std::move(failedTargets_);
            pendingTargets = std::move(pendingTargets_);
        }

        void SetResults(std::vector<NodeInstance> const & failedTargets, std::vector<NodeInstance> const & pendingTargets)
        {
            failedTargets_ = failedTargets;
            pendingTargets_ = pendingTargets;
        }

    private:
        std::vector<NodeInstance> failedTargets_;
        std::vector<NodeInstance> pendingTargets_;
    };

    typedef std::shared_ptr<MulticastOperation> MulticastOperationSPtr;

    class MulticastForwardContext
    {
        DENY_COPY(MulticastForwardContext);

    public:
        struct SubTree
        {
            NodeInstance Root;
            std::vector<NodeInstance> Targets;
        };

        MulticastForwardContext();

        MulticastForwardContext(
            NodeId nodeId,
            Transport::MessageUPtr && message,
            Transport::MessageId const & multicastId,
            std::vector<SubTree> && pending,
            MulticastOperationSPtr const & operation,
            RequestReceiverContextUPtr && requestContext);

        MulticastForwardContext(MulticastForwardContext && other);

        void Complete(NodeInstance const & target);

        bool RemoveDuplicateTargets(std::vector<NodeInstance> & targets) const;

        void AddSubTrees(
            Transport::MessageUPtr & message,
            std::vector<SubTree> const & targets,
            RequestReceiverContextUPtr && requestContext);  
        
        void SetError(Common::ErrorCode error);

        void AddFailedTarget(NodeInstance const & target);
        void AddUnknownTarget(NodeInstance const & target);

        Transport::MessageUPtr FailTarget(NodeInstance const & target, __out std::vector<SubTree> & subTrees);

        Transport::MessageUPtr CreateAckMessage() const;

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const;

    private:
        void CompletIfNeeded();

        NodeId nodeId_;
        Transport::MessageId multicastId_;
        Common::ErrorCode error_;
        std::set<NodeInstance> acked_;
        std::vector<NodeInstance> failed_;
        std::vector<NodeInstance> unknown_;
        std::vector<SubTree> pending_;
        MulticastOperationSPtr operation_;
        std::vector<RequestReceiverContextUPtr> requestContexts_;
        Transport::MessageUPtr message_;
    };
}
