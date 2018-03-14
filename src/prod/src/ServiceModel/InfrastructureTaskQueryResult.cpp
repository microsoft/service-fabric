// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::ClusterManager;

InfrastructureTaskQueryResult::InfrastructureTaskQueryResult()
    : description_()
    , state_()
{
}

InfrastructureTaskQueryResult::InfrastructureTaskQueryResult(
    shared_ptr<InfrastructureTaskDescription> && description,
    InfrastructureTaskState::Enum state)
    : description_(move(description))
    , state_(state)
{ 
}

void InfrastructureTaskQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_INFRASTRUCTURE_TASK_QUERY_RESULT_ITEM & result) const 
{
    result.Description = heap.AddItem<FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION>().GetRawPointer();
    description_->ToPublicApi(heap, *result.Description); 

    result.State = InfrastructureTaskState::ToPublicApi(state_);
}

ErrorCode InfrastructureTaskQueryResult::FromPublicApi(__in FABRIC_INFRASTRUCTURE_TASK_QUERY_RESULT_ITEM const& item)
{
    auto error = description_->FromPublicApi(*item.Description);
    if (!error.IsSuccess()) { return error; }

    state_ = InfrastructureTaskState::FromPublicApi(item.State);

    return error;
}

void InfrastructureTaskQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring InfrastructureTaskQueryResult::ToString() const
{
    return wformatString("Description = '{0}', State = '{1}'", description_, state_);
}
