// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class JoinLockManager : public Common::TextTraceComponent<Common::TraceTaskCodes::JoinLock>
    {
        DENY_COPY(JoinLockManager);

    public:
        JoinLockManager(SiteNode & site);

        void LockRequestHandler(__in Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);
        void LockRenewHandler(__in Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);
        void LockReleaseHandler(__in Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);
        void UnlockHandler(__in Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);

        void AddThrottleHeaderIfNeeded(__in Transport::Message & message, NodeId id);
        void ResetLock();

    private:
        bool ReleaseLock(int64 lockId, NodeInstance const & from);

        void AddCompeting(NodeId id, Common::TimeSpan timeout);
        void RemoveCompeting(NodeId id);
        void RefreshCompeting(NodeIdRange const & range);
        void CheckWaitingList();
        void ScheduleNextCheck();
        bool ProcessWaitingList();

        SiteNode & site_;
        RWLOCK(Federation.JoinLockManager, lock_);

        NodeInstance owner_;
        int64 lockId_;
        std::map<NodeId, Common::StopwatchTime> competing_;
        std::map<NodeId, Common::StopwatchTime> waiting_;
    };
}
