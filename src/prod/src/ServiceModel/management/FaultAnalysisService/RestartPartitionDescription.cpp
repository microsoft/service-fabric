// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"
#include "RestartPartitionMode.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("RestartPartitionDescription");

RestartPartitionDescription::RestartPartitionDescription()
    : operationId_()
    , partitionSelector_()
    , restartPartitionMode_()
{
}

RestartPartitionDescription::RestartPartitionDescription(
    Common::Guid operationId, 
    std::shared_ptr<PartitionSelector> const& partitionSelector, 
    RestartPartitionMode::Enum restartPartitionMode)
    : operationId_(operationId)
    , partitionSelector_(partitionSelector)
    , restartPartitionMode_(restartPartitionMode)
{
}

RestartPartitionDescription::RestartPartitionDescription(RestartPartitionDescription const & other)
    : operationId_(other.Id)
    , partitionSelector_(other.Selector)
    , restartPartitionMode_(other.Mode)
{
}

RestartPartitionDescription::RestartPartitionDescription(RestartPartitionDescription && other)
    : operationId_(move(other.operationId_))
    , partitionSelector_(move(other.partitionSelector_))
    , restartPartitionMode_(move(other.restartPartitionMode_))
{
}

RestartPartitionDescription & RestartPartitionDescription::operator = (RestartPartitionDescription && other)
{
    if (this != &other)
    {
        this->operationId_ = move(other.operationId_);
        this->partitionSelector_ = move(other.partitionSelector_);
        this->restartPartitionMode_ = move(other.restartPartitionMode_);
    }

    return *this;
}

bool RestartPartitionDescription::operator == (RestartPartitionDescription const & other) const
{
    if (operationId_ != other.operationId_) { return false; }

    CHECK_EQUALS_IF_NON_NULL(partitionSelector_)

        if (restartPartitionMode_ != other.restartPartitionMode_) { return false; }

    return true;
}

bool RestartPartitionDescription::operator != (RestartPartitionDescription const & other) const
{
    return !(*this == other);
}

void RestartPartitionDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w << "[ " << operationId_;

    if (partitionSelector_) { w << ", " << *partitionSelector_; }

    w << ", " << restartPartitionMode_ << " ]";
}

Common::ErrorCode RestartPartitionDescription::FromPublicApi(
    FABRIC_START_PARTITION_RESTART_DESCRIPTION const & publicDescription)
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
    error = RestartPartitionMode::FromPublicApi(publicDescription.RestartPartitionMode, restartPartitionMode_);

    if (!error.IsSuccess())
    {
        Trace.WriteError(TraceComponent, "RestartPartitionMode::FromPublicApi failed, error: {0}.", error);
        return error;
    }

    return ErrorCodeValue::Success;
}

void RestartPartitionDescription::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_START_PARTITION_RESTART_DESCRIPTION & result) const
{
    result.OperationId = operationId_.AsGUID();

    auto publicPartitionSelectorPtr = heap.AddItem<FABRIC_PARTITION_SELECTOR>();
    partitionSelector_->ToPublicApi(heap, *publicPartitionSelectorPtr);
    result.PartitionSelector = publicPartitionSelectorPtr.GetRawPointer();

    result.RestartPartitionMode = RestartPartitionMode::ToPublicApi(restartPartitionMode_);
}
