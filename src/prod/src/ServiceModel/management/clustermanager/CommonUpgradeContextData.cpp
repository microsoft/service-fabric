// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::ClusterManager;

CommonUpgradeContextData::CommonUpgradeContextData()
    : isPostUpgradeHealthCheckComplete_(false)
    , startTime_(DateTime::Zero)
    , failureTime_(DateTime::Zero)
    , failureReason_(UpgradeFailureReason::None)
    , upgradeProgressAtFailure_()
    , upgradeStatusDetails_(L"")
    , upgradeStatusDetailsAtFailure_(L"")
    , isRollbackAllowed_(true)
    , isSkipRollbackUD_(false)
{
}

CommonUpgradeContextData::CommonUpgradeContextData(CommonUpgradeContextData && other)
    : isPostUpgradeHealthCheckComplete_(move(other.isPostUpgradeHealthCheckComplete_))
    , startTime_(move(other.startTime_))
    , failureTime_(move(other.failureTime_))
    , failureReason_(move(other.failureReason_))
    , upgradeProgressAtFailure_(move(other.upgradeProgressAtFailure_))
    , upgradeStatusDetails_(move(other.upgradeStatusDetails_))
    , upgradeStatusDetailsAtFailure_(move(other.UpgradeStatusDetailsAtFailure))
    , isRollbackAllowed_(move(other.isRollbackAllowed_))
    , isSkipRollbackUD_(move(other.isSkipRollbackUD_))
{
}

CommonUpgradeContextData & CommonUpgradeContextData::operator=(CommonUpgradeContextData && other)
{
    if (this != &other)
    {
        isPostUpgradeHealthCheckComplete_ = move(other.isPostUpgradeHealthCheckComplete_);
        startTime_ = move(other.startTime_);
        failureTime_ = move(other.failureTime_);
        failureReason_ = move(other.failureReason_);
        upgradeProgressAtFailure_ = move(other.upgradeProgressAtFailure_);
        upgradeStatusDetails_ = move(other.upgradeStatusDetails_);
        upgradeStatusDetailsAtFailure_ = move(other.upgradeStatusDetailsAtFailure_);
        isRollbackAllowed_ = move(other.isRollbackAllowed_);
        isSkipRollbackUD_ = move(other.isSkipRollbackUD_);
    }

    return *this;
}

void CommonUpgradeContextData::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(
        "postHealthCheckComplete={0} start.utc={1} failure[utc={2} reason={3} progress={4}] details={5} isRollbackAllowed={6} isSkipRollbackUD={7}",
        isPostUpgradeHealthCheckComplete_,
        startTime_,
        failureTime_,
        failureReason_,
        upgradeProgressAtFailure_,
        upgradeStatusDetails_,
        isRollbackAllowed_,
        isSkipRollbackUD_);
}

void CommonUpgradeContextData::ClearFailureData()
{
    failureTime_ = DateTime::Zero;
    failureReason_ = UpgradeFailureReason::None;
    upgradeProgressAtFailure_ = Reliability::UpgradeDomainProgress();
}

void CommonUpgradeContextData::Reset()
{
    isPostUpgradeHealthCheckComplete_ = false;
    startTime_ = DateTime::Zero;
    upgradeStatusDetails_.clear();
    upgradeStatusDetailsAtFailure_ = L"";
    isRollbackAllowed_ = true;
    isSkipRollbackUD_ = false;

    ClearFailureData();
}
