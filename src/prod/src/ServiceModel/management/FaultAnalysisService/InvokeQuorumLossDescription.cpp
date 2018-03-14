// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("InvokeQuorumLossDescription");

InvokeQuorumLossDescription::InvokeQuorumLossDescription()
    : operationId_()
    , partitionSelector_()
    , quorumLossMode_()
    , quorumLossDuration_(TimeSpan::Zero)
{
}

InvokeQuorumLossDescription::InvokeQuorumLossDescription(
     Common::Guid operationId, 
     std::shared_ptr<PartitionSelector> const& partitionSelector, 
     QuorumLossMode::Enum mode,
     Common::TimeSpan & quorumLossDuration)
    : operationId_(operationId)
    , partitionSelector_(partitionSelector)
    , quorumLossMode_(mode)
    , quorumLossDuration_(quorumLossDuration)
{
}

InvokeQuorumLossDescription::InvokeQuorumLossDescription(InvokeQuorumLossDescription const & other)
    : operationId_(other.Id)
    , partitionSelector_(other.Selector)
    , quorumLossMode_(other.Mode)
    , quorumLossDuration_(other.QuorumLossDuration)
{
}

InvokeQuorumLossDescription::InvokeQuorumLossDescription(InvokeQuorumLossDescription && other)
    : operationId_(move(other.operationId_))
    , partitionSelector_(move(other.partitionSelector_))
    , quorumLossMode_(move(other.quorumLossMode_))
    , quorumLossDuration_(move(other.quorumLossDuration_))
{
}

InvokeQuorumLossDescription & InvokeQuorumLossDescription::operator = (InvokeQuorumLossDescription && other)
{
    if (this != &other)
    {
        this->operationId_ = move(other.operationId_);
        this->partitionSelector_ = move(other.partitionSelector_);
        this->quorumLossMode_ = move(other.quorumLossMode_);
        this->quorumLossDuration_ = move(other.quorumLossDuration_);
    }

    return *this;
}

bool InvokeQuorumLossDescription::operator == (InvokeQuorumLossDescription const & other) const
{
    if (operationId_ != other.operationId_) { return false; }

    CHECK_EQUALS_IF_NON_NULL(partitionSelector_)

        if (quorumLossMode_ != other.quorumLossMode_) { return false; }

    if (quorumLossDuration_ != other.quorumLossDuration_) { return false; }

    return true;
}

bool InvokeQuorumLossDescription::operator != (InvokeQuorumLossDescription const & other) const
{
    return !(*this == other);
}

void InvokeQuorumLossDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w << "[ " << operationId_;

    if (partitionSelector_) { w << ", " << *partitionSelector_; }

    w << ", " << quorumLossMode_ << "," << quorumLossDuration_ << " ]";
}

Common::ErrorCode InvokeQuorumLossDescription::FromPublicApi(
    FABRIC_START_PARTITION_QUORUM_LOSS_DESCRIPTION const & publicDescription)
{
    operationId_ = Guid(publicDescription.OperationId);
    PartitionSelector partitionSelector;
    auto error = partitionSelector.FromPublicApi(*publicDescription.PartitionSelector);

    if (!error.IsSuccess())
    {
        Trace.WriteError(TraceComponent, "partitionSelector.FromPublicApi failed, error: {0}.", error);
        return error;
    }

    partitionSelector_ = make_shared<PartitionSelector>(partitionSelector);
    error = QuorumLossMode::FromPublicApi(publicDescription.QuorumLossMode, quorumLossMode_);

    if (!error.IsSuccess())
    {
        Trace.WriteError(TraceComponent, "QuorumLossMode::FromPublicApi failed, error: {0}.", error);
        return error;
    }

    quorumLossDuration_ = TimeSpan::FromMilliseconds(publicDescription.QuorumLossDurationInMilliSeconds);

    return ErrorCodeValue::Success;
}

void InvokeQuorumLossDescription::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_START_PARTITION_QUORUM_LOSS_DESCRIPTION & result) const
{
    result.OperationId = operationId_.AsGUID();

    auto publicPartitionSelectorPtr = heap.AddItem<FABRIC_PARTITION_SELECTOR>();
    partitionSelector_->ToPublicApi(heap, *publicPartitionSelectorPtr);
    result.PartitionSelector = publicPartitionSelectorPtr.GetRawPointer();

    result.QuorumLossMode = QuorumLossMode::ToPublicApi(quorumLossMode_);
    result.QuorumLossDurationInMilliSeconds = static_cast<LONG>(quorumLossDuration_.TotalMilliseconds());
}
