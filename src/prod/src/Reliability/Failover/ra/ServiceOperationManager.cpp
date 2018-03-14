// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Common::ApiMonitoring;

namespace
{
    CompatibleOperationTable CreateCompatibilityOperationTable()
    {
        CompatibleOperationTableBuilder builder(false);

        builder.MarkInvalid(
        {
            ApiName::GetNextCopyState,
            ApiName::GetNextCopyState,
            ApiName::GetNextCopyContext,
            ApiName::UpdateEpoch,
            ApiName::OnDataLoss,
            ApiName::CatchupReplicaSet,
            ApiName::BuildReplica,
            ApiName::RemoveReplica,
            ApiName::GetStatus,
            ApiName::UpdateCatchupConfiguration,
            ApiName::UpdateCurrentConfiguration,
            ApiName::GetQuery
        });

        return builder.CreateTable();
    }
}

Global<CompatibleOperationTable> ServiceOperationManager::CompatibilityTable = make_global<CompatibleOperationTable>(CreateCompatibilityOperationTable());

ServiceOperationManager::ServiceOperationManager(bool isStateful, FailoverUnitProxy & owner) :
    FailoverUnitProxyOperationManagerBase(owner, isStateful ? InterfaceName::IStatefulServiceReplica : InterfaceName::IStatelessServiceInstance, *CompatibilityTable)
{
}

void ServiceOperationManager::GetQueryInformation(
    ::FABRIC_QUERY_SERVICE_OPERATION_NAME & operationNameOut,  
    DateTime & startTimeOut) const
{
    vector<ApiName::Enum> operations;

    // Init defaults
    operationNameOut = FABRIC_QUERY_SERVICE_OPERATION_NAME_NONE;    
    opMgr_.GetDetailsForQuery(operations, startTimeOut);

    TESTASSERT_IF(operations.size() > 1, "Cannot have more than one operation. If so then query breaks");
    if (!operations.empty())
    {
        operationNameOut = GetPublicApiOperationName(*operations.cbegin());
    }
}

FABRIC_QUERY_SERVICE_OPERATION_NAME ServiceOperationManager::GetPublicApiOperationName(ApiName::Enum operation)
{
    switch (operation)
    {
    case ApiName::Open:
        return FABRIC_QUERY_SERVICE_OPERATION_NAME_OPEN;
    case ApiName::ChangeRole:
        return FABRIC_QUERY_SERVICE_OPERATION_NAME_CHANGEROLE;
    case ApiName::Close:
        return FABRIC_QUERY_SERVICE_OPERATION_NAME_CLOSE;
    case ApiName::Abort:
        return FABRIC_QUERY_SERVICE_OPERATION_NAME_ABORT;
    default:
        Assert::CodingError("Unknown operation {0}", static_cast<int>(operation));
    }
}
