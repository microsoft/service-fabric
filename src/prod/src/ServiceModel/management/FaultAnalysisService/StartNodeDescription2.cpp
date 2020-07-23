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

StringLiteral const TraceComponent("StartNodeDescription2");

StartNodeDescription2::StartNodeDescription2()
: kind_(StartNodeDescriptionKind::None)
, value_()
{
}

StartNodeDescription2::StartNodeDescription2(StartNodeDescription2 const & other)
: kind_(other.Kind)
, value_(other.Value)
{
}

StartNodeDescription2::StartNodeDescription2(StartNodeDescription2 && other)
: kind_(move(other.kind_))
, value_(move(other.value_))
{
}

Common::ErrorCode StartNodeDescription2::FromPublicApi(
    FABRIC_START_NODE_DESCRIPTION2 const & publicDescription)
{
    switch (publicDescription.Kind)
    {
        case FABRIC_START_NODE_DESCRIPTION_KIND_USING_NODE_NAME:
            kind_ = StartNodeDescriptionKind::Enum::UsingNodeName;
            break;
        case FABRIC_START_NODE_DESCRIPTION_KIND_USING_REPLICA_SELECTOR:
            kind_ = StartNodeDescriptionKind::Enum::UsingReplicaSelector;
            break;
    }

    if (value_)
    {
        Trace.WriteWarning(TraceComponent, "before assignment value_ is null");
    }

    value_ = publicDescription.Value;

    if (value_)
    {
        Trace.WriteWarning(TraceComponent, "after assignment value_ is null");
    }

    return ErrorCodeValue::Success;
}

void StartNodeDescription2::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_START_NODE_DESCRIPTION2 & result) const
{
    switch (kind_)
    {
    case StartNodeDescriptionKind::Enum::UsingNodeName:
        result.Kind = FABRIC_START_NODE_DESCRIPTION_KIND_USING_NODE_NAME;
        break;
    case StartNodeDescriptionKind::Enum::UsingReplicaSelector:
        result.Kind = FABRIC_START_NODE_DESCRIPTION_KIND_USING_REPLICA_SELECTOR;
        break;
    }

    result.Value = value_;
}
