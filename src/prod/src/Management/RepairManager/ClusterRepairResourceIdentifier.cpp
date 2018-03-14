// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::RepairManager;

ClusterRepairScopeIdentifier::ClusterRepairScopeIdentifier()
    : RepairScopeIdentifierBase(RepairScopeIdentifierKind::Cluster)
{
}

ClusterRepairScopeIdentifier::ClusterRepairScopeIdentifier(ClusterRepairScopeIdentifier const & other)
    : RepairScopeIdentifierBase(other)
{
}

ClusterRepairScopeIdentifier::ClusterRepairScopeIdentifier(ClusterRepairScopeIdentifier && other)
    : RepairScopeIdentifierBase(move(other))
{
}

ErrorCode ClusterRepairScopeIdentifier::FromPublicApi(FABRIC_REPAIR_SCOPE_IDENTIFIER const & publicDescription)
{
    if (publicDescription.Value)
    {
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCode::Success();
}

void ClusterRepairScopeIdentifier::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("ClusterRepairScopeIdentifier");
}

bool ClusterRepairScopeIdentifier::IsVisibleTo(RepairScopeIdentifierBase const & scopeBoundary) const
{
    return (scopeBoundary.Kind == RepairScopeIdentifierKind::Cluster);
}
