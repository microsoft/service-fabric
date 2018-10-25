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

StringLiteral const TraceComponent("ChaosTargetFilter");

ChaosTargetFilter::ChaosTargetFilter()
: nodeTypeInclusionList_()
, applicationInclusionList_()
{
}

ErrorCode ChaosTargetFilter::FromPublicApi(
    FABRIC_CHAOS_TARGET_FILTER const & publicFilter)
{
    if(publicFilter.NodeTypeInclusionList != nullptr)
    {
        // NodeTypeInclusionList
        StringList::FromPublicApi(*publicFilter.NodeTypeInclusionList, nodeTypeInclusionList_);
    }

    if(publicFilter.ApplicationInclusionList != nullptr)
    {
        // ApplicationInclusionList
        StringList::FromPublicApi(*publicFilter.ApplicationInclusionList, applicationInclusionList_);
    }

    return ErrorCode::Success();
}

ErrorCode ChaosTargetFilter::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_CHAOS_TARGET_FILTER & result) const
{
    auto publicNodeTypeInclusionList = heap.AddItem<FABRIC_STRING_LIST>();
    StringList::ToPublicAPI(heap, nodeTypeInclusionList_, publicNodeTypeInclusionList);
    result.NodeTypeInclusionList = publicNodeTypeInclusionList.GetRawPointer();

    auto publicApplicationInclusionList = heap.AddItem<FABRIC_STRING_LIST>();
    StringList::ToPublicAPI(heap, applicationInclusionList_, publicApplicationInclusionList);
    result.ApplicationInclusionList = publicApplicationInclusionList.GetRawPointer();

    return ErrorCode::Success();
}

ErrorCode ChaosTargetFilter::Validate() const
{
    if (nodeTypeInclusionList_.empty() && applicationInclusionList_.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("{0}", FASResource::GetResources().ChaosTargetFilterSpecificationNotFound));
    }

    int maximumNumberOfNodeTypesInChaosTargetFilter = ServiceModelConfig::GetConfig().MaxNumberOfNodeTypesInChaosTargetFilter;

    if (!nodeTypeInclusionList_.empty() && nodeTypeInclusionList_.size() > maximumNumberOfNodeTypesInChaosTargetFilter)
    {
        return ErrorCode(ErrorCodeValue::EntryTooLarge, wformatString("{0}", FASResource::GetResources().ChaosNodeTypeInclusionListTooLong));
    }

    int maximumNumberOfApplicationsInChaosTargetFilter = ServiceModelConfig::GetConfig().MaxNumberOfApplicationsInChaosTargetFilter;

    if (!applicationInclusionList_.empty() && applicationInclusionList_.size() > maximumNumberOfApplicationsInChaosTargetFilter)
    {
        return ErrorCode(ErrorCodeValue::EntryTooLarge, wformatString("{0}", FASResource::GetResources().ChaosApplicationInclusionListTooLong));
    }

    return ErrorCode::Success();
}
