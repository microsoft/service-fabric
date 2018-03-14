// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Common::ApiMonitoring;
using namespace Infrastructure;

namespace
{
    CompatibleOperationTable CreateCompatibilityOperationTable()
    {
        CompatibleOperationTableBuilder builder(false);
        builder.MarkInvalid({ ApiName::GetNextCopyState, ApiName::GetNextCopyContext });

        builder.MarkTrue(ApiName::CatchupReplicaSet, ApiName::UpdateCatchupConfiguration);
        builder.MarkTrue(ApiName::CatchupReplicaSet, ApiName::UpdateCurrentConfiguration);
        builder.MarkTrue(ApiName::UpdateCatchupConfiguration, ApiName::CatchupReplicaSet);
        builder.MarkTrue(ApiName::UpdateCurrentConfiguration, ApiName::CatchupReplicaSet);

        builder.MarkTrue(ApiName::BuildReplica, ApiName::UpdateCatchupConfiguration);
        builder.MarkTrue(ApiName::BuildReplica, ApiName::UpdateCurrentConfiguration);
        builder.MarkTrue(ApiName::RemoveReplica, ApiName::UpdateCatchupConfiguration);
        builder.MarkTrue(ApiName::RemoveReplica, ApiName::UpdateCurrentConfiguration);

        builder.MarkTrue(ApiName::BuildReplica, ApiName::CatchupReplicaSet);
        builder.MarkTrue(ApiName::RemoveReplica, ApiName::CatchupReplicaSet);
        builder.MarkTrue(ApiName::CatchupReplicaSet, ApiName::BuildReplica);
        builder.MarkTrue(ApiName::CatchupReplicaSet, ApiName::RemoveReplica);

        builder.MarkTrue(ApiName::BuildReplica, ApiName::BuildReplica);
        builder.MarkTrue(ApiName::BuildReplica, ApiName::RemoveReplica);
        builder.MarkTrue(ApiName::RemoveReplica, ApiName::BuildReplica);

        // It is valid to query the replicator at any time, except during open,close,abort and an already existing 
        // query has been issued
        builder.MarkTrue(ApiName::BuildReplica, ApiName::GetQuery);
        builder.MarkTrue(ApiName::CatchupReplicaSet, ApiName::GetQuery);
        builder.MarkTrue(ApiName::ChangeRole, ApiName::GetQuery);
        builder.MarkTrue(ApiName::GetStatus, ApiName::GetQuery);
        builder.MarkTrue(ApiName::OnDataLoss, ApiName::GetQuery);
        builder.MarkTrue(ApiName::RemoveReplica, ApiName::GetQuery);
        builder.MarkTrue(ApiName::UpdateCatchupConfiguration, ApiName::GetQuery);
        builder.MarkTrue(ApiName::UpdateCurrentConfiguration, ApiName::GetQuery);
        builder.MarkTrue(ApiName::UpdateEpoch, ApiName::GetQuery);

        // While the query is running, it is valid for other operations to begin
        builder.MarkTrue(ApiName::GetQuery, ApiName::Abort);
        builder.MarkTrue(ApiName::GetQuery, ApiName::BuildReplica);
        builder.MarkTrue(ApiName::GetQuery, ApiName::CatchupReplicaSet);
        builder.MarkTrue(ApiName::GetQuery, ApiName::ChangeRole);
        builder.MarkTrue(ApiName::GetQuery, ApiName::Close);
        builder.MarkTrue(ApiName::GetQuery, ApiName::GetStatus);
        builder.MarkTrue(ApiName::GetQuery, ApiName::OnDataLoss);
        builder.MarkTrue(ApiName::GetQuery, ApiName::Open);
        builder.MarkTrue(ApiName::GetQuery, ApiName::RemoveReplica);
        builder.MarkTrue(ApiName::GetQuery, ApiName::UpdateCatchupConfiguration);
        builder.MarkTrue(ApiName::GetQuery, ApiName::UpdateCurrentConfiguration);
        builder.MarkTrue(ApiName::GetQuery, ApiName::UpdateEpoch);

        return builder.CreateTable();
    }
}

Global<CompatibleOperationTable> ReplicatorOperationManager::CompatibilityTable = make_global<CompatibleOperationTable>(CreateCompatibilityOperationTable());

ReplicatorOperationManager::ReplicatorOperationManager(FailoverUnitProxy & owner) : 
    FailoverUnitProxyOperationManagerBase(owner, ApiMonitoring::InterfaceName::IReplicator, *CompatibilityTable)
{
}

void ReplicatorOperationManager::CancelOrMarkOperationForCancel(ApiName::Enum operation)
{
    opMgr_.CancelOrMarkOperationForCancel(operation);
}

void ReplicatorOperationManager::RemoveOperationForCancel(ApiName::Enum operation)
{
    opMgr_.RemoveOperationForCancel(operation);
}

FABRIC_QUERY_REPLICATOR_OPERATION_NAME ReplicatorOperationManager::GetNameForQuery() const
{
    vector<ApiName::Enum> currentApis;
    DateTime value;

    opMgr_.GetDetailsForQuery(currentApis, value);

    return GetNameForQuery(currentApis);
}

void ReplicatorOperationManager::GetApiCallDescriptionProperties(
    Common::ApiMonitoring::ApiName::Enum operation,
    __out bool & isSlowHealthReportRequired,
    __out bool & traceServiceType) const 
{
    switch (operation)
    {
    case ApiName::RemoveReplica:
        isSlowHealthReportRequired = false;
        traceServiceType = false;
        break;

    case ApiName::GetStatus:
        isSlowHealthReportRequired = false;
        break;

    case ApiName::GetQuery:
        isSlowHealthReportRequired = false;
        traceServiceType = false;
        break;
    }
}

FABRIC_QUERY_REPLICATOR_OPERATION_NAME ReplicatorOperationManager::GetNameForQuery(vector<ApiName::Enum> const & apis)
{
    if (apis.empty())
    {
        return FABRIC_QUERY_REPLICATOR_OPERATION_NAME_NONE;
    }

    int rv = FABRIC_QUERY_REPLICATOR_OPERATION_NAME_INVALID;

    
    for(auto it = apis.cbegin(); it != apis.cend(); ++it)
    {
        FABRIC_QUERY_REPLICATOR_OPERATION_NAME current = FABRIC_QUERY_REPLICATOR_OPERATION_NAME_INVALID;

        switch (*it)
        {
        case ApiName::Open:
            current = FABRIC_QUERY_REPLICATOR_OPERATION_NAME_OPEN;
            break;

        case ApiName::Close:
            current = FABRIC_QUERY_REPLICATOR_OPERATION_NAME_CLOSE;
            break;

        case ApiName::ChangeRole:
            current = FABRIC_QUERY_REPLICATOR_OPERATION_NAME_CHANGEROLE;
            break;

        case ApiName::OnDataLoss:
            current = FABRIC_QUERY_REPLICATOR_OPERATION_NAME_ONDATALOSS;
            break;

        case ApiName::CatchupReplicaSet:
            current = FABRIC_QUERY_REPLICATOR_OPERATION_NAME_WAITFORCATCHUP;
            break;

        case ApiName::BuildReplica:
            current = FABRIC_QUERY_REPLICATOR_OPERATION_NAME_BUILD;
            break;

        case ApiName::UpdateEpoch:
            current = FABRIC_QUERY_REPLICATOR_OPERATION_NAME_UPDATEEPOCH;
            break;

        case ApiName::Abort:
            current = FABRIC_QUERY_REPLICATOR_OPERATION_NAME_ABORT;
            break;
        
        // The other operations are unmapped as of now
        default:
            current = FABRIC_QUERY_REPLICATOR_OPERATION_NAME_INVALID;
            break;
        }

        rv |= current;
    }

    if (FABRIC_QUERY_REPLICATOR_OPERATION_NAME_INVALID == rv)
    {
        // None of the mapped actions are running.
        return FABRIC_QUERY_REPLICATOR_OPERATION_NAME_NONE;
    }
    else
    {
        return static_cast<FABRIC_QUERY_REPLICATOR_OPERATION_NAME>(rv);
    }
}
