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

StringLiteral const TraceComponent("InvokeDataLossDescription");

InvokeDataLossDescription::InvokeDataLossDescription()
    : operationId_()
    , partitionSelector_()
    , dataLossMode_()
{
}

InvokeDataLossDescription::InvokeDataLossDescription(
    Common::Guid operationId, 
    std::shared_ptr<PartitionSelector> const& partitionSelector, 
    DataLossMode::Enum mode)
    : operationId_(operationId)
    , partitionSelector_(partitionSelector)
    , dataLossMode_(mode)
{
}

InvokeDataLossDescription::InvokeDataLossDescription(InvokeDataLossDescription const & other)
    : operationId_(other.Id)
    , partitionSelector_(other.Selector)
    , dataLossMode_(other.Mode)
{
}

InvokeDataLossDescription::InvokeDataLossDescription(InvokeDataLossDescription && other)
    : operationId_(move(other.operationId_))
    , partitionSelector_(move(other.partitionSelector_))
    , dataLossMode_(move(other.dataLossMode_))
{
}

InvokeDataLossDescription & InvokeDataLossDescription::operator = (InvokeDataLossDescription && other)
{
    if (this != &other)
    {
        this->operationId_ = move(other.operationId_);
        this->partitionSelector_ = move(other.partitionSelector_);
        this->dataLossMode_ = move(other.dataLossMode_);
    }

    return *this;
}

bool InvokeDataLossDescription::operator == (InvokeDataLossDescription const & other) const
{
    if (operationId_ != other.operationId_) { return false; }

    CHECK_EQUALS_IF_NON_NULL(partitionSelector_)

        if (dataLossMode_ != other.dataLossMode_) { return false; }

    return true;
}

bool InvokeDataLossDescription::operator != (InvokeDataLossDescription const & other) const
{
    return !(*this == other);
}

void InvokeDataLossDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w << "[ " << operationId_;

    if (partitionSelector_) { w << ", " << *partitionSelector_; }

    w << ", " << dataLossMode_ << " ]";
}

Common::ErrorCode InvokeDataLossDescription::FromPublicApi(
    FABRIC_START_PARTITION_DATA_LOSS_DESCRIPTION const & publicDescription)
{
    operationId_ = Guid(publicDescription.OperationId);
    PartitionSelector partitionSelector;
    auto error = partitionSelector.FromPublicApi(*publicDescription.PartitionSelector);
    if (!error.IsSuccess())
    {
        Trace.WriteError(TraceComponent, "partitionSelector.FromPublicApi failed to parse PartitionSelector, error: {0}.", error);
        return error;
    }

    partitionSelector_ = make_shared<PartitionSelector>(partitionSelector);
    error = DataLossMode::FromPublicApi(publicDescription.DataLossMode, dataLossMode_);
    
    if (!error.IsSuccess())
    {
        Trace.WriteError(TraceComponent, "DataLossMode::FromPublicApi failed to parse DataLossMode, error: {0}.", error);
        return error;
    }

    return ErrorCodeValue::Success;
}

void InvokeDataLossDescription::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_START_PARTITION_DATA_LOSS_DESCRIPTION & result) const
{
    result.OperationId = operationId_.AsGUID();

    auto publicPartitionSelectorPtr = heap.AddItem<FABRIC_PARTITION_SELECTOR>();
    partitionSelector_->ToPublicApi(heap, *publicPartitionSelectorPtr);
    result.PartitionSelector = publicPartitionSelectorPtr.GetRawPointer();

    result.DataLossMode = DataLossMode::ToPublicApi(dataLossMode_);
}
