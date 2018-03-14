// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class GlobalTimeManager
    {
        DENY_COPY(GlobalTimeManager);

    public:
        GlobalTimeManager(SiteNode & site, Common::RwLock & lock);

        int64 GetEpoch() const { return epoch_; }

        void Start();

        void BecomeLeader();

        void UpdateRange(GlobalTimeExchangeHeader const & header);

        int64 GetInfo(int64 & lowerLimit, int64 & upperLimit, bool isAuthority);

        void AddGlobalTimeExchangeHeader(Transport::Message & message, PartnerNodeSPtr const & target);
        void UpdatePartnerNodeUpperLimit(GlobalTimeExchangeHeader const & header, PartnerNodeSPtr const & target) const;

        void CheckHealth();

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

    private:
        class UpdateEpochAsyncOperation;

        void Refresh();

        bool IsLeader() const;
        int64 GetUpperLimit(Common::StopwatchTime time);
        int64 GetLowerLimit(Common::StopwatchTime time);

        int64 GenerateEpoch(int64 currentEpoch);
        Common::TimeSpan IncreaseEpoch(int64 epoch);
        void OnUpdateCompleted(Common::AsyncOperationSPtr const & operation);

        SiteNode & site_;
        Common::RwLock & lock_;
        int64 epoch_;
        Common::TimeSpan upperLimit_;
        Common::TimeSpan lowerLimit_;
        Common::StopwatchTime lastRefresh_;
        bool isAuthority_;
        bool isUpdatingEpoch_;
        Common::StopwatchTime leaderStartTime_;
        Common::StopwatchTime lastTraceTime_;
    };
}
