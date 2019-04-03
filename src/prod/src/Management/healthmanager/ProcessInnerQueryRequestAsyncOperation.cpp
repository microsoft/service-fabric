// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace Transport;
using namespace Store;
using namespace Query;
using namespace ServiceModel;
using namespace std;

using namespace Management::HealthManager;

StringLiteral const TraceComponent("ProcessInnerQuery");

ProcessInnerQueryRequestAsyncOperation::ProcessInnerQueryRequestAsyncOperation(
     __in EntityCacheManager & entityManager,
    Store::PartitionedReplicaId const & partitionedReplicaId,
    Common::ActivityId const & activityId,
    Common::ErrorCode const & passedThroughError,
    Query::QueryNames::Enum queryName,
    QueryArgumentMap const & queryArgs,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
    : TimedAsyncOperation(timeout, callback, parent)
    , ReplicaActivityTraceComponent(partitionedReplicaId, activityId)
    , passedThroughError_(passedThroughError)
    , entityManager_(entityManager)
    , queryName_(queryName)
    , queryArgs_(queryArgs)
    , reply_()
    , startTime_(Stopwatch::GetTimestamp())
{
}

ProcessInnerQueryRequestAsyncOperation::~ProcessInnerQueryRequestAsyncOperation()
{
}

ErrorCode ProcessInnerQueryRequestAsyncOperation::End(
    AsyncOperationSPtr const & asyncOperation,
    __out MessageUPtr & reply)
{
    auto casted = AsyncOperation::End<ProcessInnerQueryRequestAsyncOperation>(asyncOperation);

    if (casted->Error.IsSuccess())
    {
        if (casted->reply_)
        {
            reply = move(casted->reply_);
            if (reply->Action.empty())
            {
                TRACE_LEVEL_AND_TESTASSERT(
                    casted->WriteInfo,
                    TraceComponent,
                    "{0}: ProcessInnerQueryRequestAsyncOperation: Reply action is empty for {1}",
                    casted->TraceId,
                    reply->MessageId);
            }
        }
        else
        {
            TRACE_LEVEL_AND_TESTASSERT(
                casted->WriteInfo,
                TraceComponent,
                "{0}: ProcessInnerQueryRequestAsyncOperation: Reply not set on Success",
                casted->TraceId);

            reply = HealthManagerMessage::GetClientOperationSuccess(casted->ActivityId);
        }
    }
    else
    {
        reply = nullptr;
    }

    return casted->Error;
}

void ProcessInnerQueryRequestAsyncOperation::OnStart(Common::AsyncOperationSPtr const & thisSPtr)
{
    entityManager_.HealthManagerReplicaObj.HealthManagerCounters->OnHealthQueryReceived();
    if (!passedThroughError_.IsSuccess())
    {
        entityManager_.HealthManagerReplicaObj.HealthManagerCounters->OnHealthQueryDropped();
        this->TryComplete(thisSPtr, move(passedThroughError_));
        return;
    }

    TimedAsyncOperation::OnStart(thisSPtr);

    auto error = entityManager_.HealthManagerReplicaObj.AddQuery(thisSPtr, this);
    if (!error.IsSuccess())
    {
        HMEvents::Trace->DropQuery(
            this->TraceId,
            queryName_,
            error,
            error.Message);
        this->TryComplete(thisSPtr, move(error));
    }
}

// Started on query job queue thread
void ProcessInnerQueryRequestAsyncOperation::StartProcessQuery(Common::AsyncOperationSPtr const & thisSPtr)
{
    if (this->RemainingTime <= TimeSpan::Zero)
    {
        this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Timeout));
        return;
    }

    if (!entityManager_.IsReady())
    {
        // If the replica is not primary anymore, the cache manager is disabled.
        this->WriteWarning(TraceComponent, "{0}: {1} should not be processed when cache manager is not ready", this->TraceId, queryName_);
        this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::InvalidState));
        return;
    }

    ErrorCode error(ErrorCodeValue::Success);
    QueryRequestContext context(this->ReplicaActivityId, queryArgs_);
    switch (queryName_)
    {
    case QueryNames::GetClusterHealthChunk:
        error = CreateClusterHealthChunkQueryContext(context);
        break;
    case QueryNames::GetClusterHealth:
        error = CreateClusterQueryContext(context);
        break;
    case QueryNames::GetNodeHealth:
        error = CreateNodeQueryContext(context);
        break;
    case QueryNames::GetServicePartitionHealth:
        error = CreatePartitionQueryContext(context);
        break;
    case QueryNames::GetServicePartitionReplicaHealth:
        error = CreateReplicaQueryContext(context);
        break;
    case QueryNames::GetServiceHealth:
        error = CreateServiceQueryContext(context);
        break;
    case QueryNames::GetApplicationHealth:
        error = CreateApplicationQueryContext(context);
        break;
    case QueryNames::GetDeployedApplicationHealth:
        error = CreateDeployedApplicationQueryContext(context);
        break;
    case QueryNames::GetDeployedServicePackageHealth:
        error = CreateDeployedServicePackageQueryContext(context);
        break;
    case QueryNames::GetAggregatedNodeHealthList:
        error = CreateNodeQueryListContext(context);
        break;
    case QueryNames::GetAggregatedServicePartitionHealthList:
        error = CreatePartitionQueryListContext(context);
        break;
    case QueryNames::GetAggregatedServicePartitionReplicaHealthList:
        error = CreateReplicaQueryListContext(context);
        break;
    case QueryNames::GetAggregatedServiceHealthList:
        error = CreateServiceQueryListContext(context);
        break;
    case QueryNames::GetAggregatedApplicationHealthList:
        error = CreateApplicationQueryListContext(context);
        break;
    case QueryNames::GetApplicationUnhealthyEvaluation:
        error = CreateApplicationUnhealthyEvaluationContext(context);
        break;
    case QueryNames::GetAggregatedDeployedApplicationHealthList:
        error = CreateDeployedApplicationQueryListContext(context);
        break;
    case QueryNames::GetAggregatedDeployedServicePackageHealthList:
        error = CreateDeployedServicePackageQueryListContext(context);
        break;
    case QueryNames::GetAggregatedDeployedApplicationsOnNodeHealthPagedList:
        error = CreateDeployedApplicationsOnNodeQueryListContext(context);
        break;
    default:
        error = ErrorCode(
            ErrorCodeValue::InvalidConfiguration,
            wformatString("{0} {1}", Resources::GetResources().InvalidQueryName, queryName_));
        HMEvents::Trace->QueryInvalidConfig(this->TraceId, wformatString(queryName_), error);
        break;
    }

    if (error.IsError(ErrorCodeValue::HealthEntityNotFound) &&
        (context.ContextKind == RequestContextKind::QueryEntityChildren ||
         context.ContextKind == RequestContextKind::QueryEntityHealthState))
    {
        HMEvents::Trace->QueryFailed(context, error, error.Message);
        // Return empty list instead of error
        context.InitializeEmptyQueryResult();
        error = ErrorCode::Success();
    }

    if (error.IsSuccess())
    {
        entityManager_.HealthManagerCounters->OnSuccessfulHealthQuery(startTime_);

        if (this->TryStartComplete())
        {
            auto result = context.MoveQueryResult();
            reply_ = HealthManagerMessage::GetQueryReply(
                move(result),
                this->ActivityId);
            this->FinishComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
        }
    }
    else
    {
        HMEvents::Trace->QueryFailed(context, error, error.Message);
        this->TryComplete(thisSPtr, move(error));
    }
}

ErrorCode ProcessInnerQueryRequestAsyncOperation::GetNodeIdFromQueryArguments(__out NodeHealthId & nodeId)
{
    wstring nodeIdArg;
    if (!queryArgs_.TryGetValue(QueryResourceProperties::Node::Id, nodeIdArg))
    {
        if (!queryArgs_.TryGetValue(QueryResourceProperties::Node::Name, nodeIdArg))
        {
            HMEvents::Trace->QueryNoArg(
                this->TraceId,
                QueryResourceProperties::Node::Id);
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString("{0} {1} {2}", Resources::GetResources().MissingQueryParameter, wformatString(queryName_), QueryResourceProperties::Node::Name));
        }

        NodeId id;
        ErrorCode error = Federation::NodeIdGenerator::GenerateFromString(nodeIdArg, id);
        if (!error.IsSuccess())
        {
            return error;
        }

        nodeId = id.IdValue;
    }
    // Parse and validate
    else if (!LargeInteger::TryParse(nodeIdArg, /*out*/ nodeId))
    {
        HMEvents::Trace->QueryParseArg(
            this->TraceId,
            QueryResourceProperties::Node::Id,
            nodeIdArg);
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString("{0} {1} {2}", Resources::GetResources().InvalidQueryParameter, QueryResourceProperties::Node::Id, nodeIdArg));
    }

    return ErrorCode::Success();
}

Common::ErrorCode ProcessInnerQueryRequestAsyncOperation::GetStringQueryArgument(
    std::wstring const & queryParameterName,
    bool acceptEmpty,
    __inout std::wstring & queryParameterValue)
{
    return this->GetStringQueryArgument(queryParameterName, acceptEmpty, false, queryParameterValue);
}

ErrorCode ProcessInnerQueryRequestAsyncOperation::GetStringQueryArgument(
    wstring const & queryParameterName,
    bool acceptEmpty,
    bool isOptionalArg,
    __inout std::wstring & queryParameterValue)
{
    if (!queryArgs_.TryGetValue(queryParameterName, queryParameterValue))
    {
        if (isOptionalArg)
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        HMEvents::Trace->QueryNoArg(
            this->TraceId,
            queryParameterName);
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString("{0} {1} {2}", Resources::GetResources().MissingQueryParameter, wformatString(queryName_), queryParameterName));
    }

    if (!acceptEmpty && queryParameterValue.empty())
    {
        this->WriteInfo(
            TraceComponent,
            "{0}: {1}: empty parameter not allowed",
            this->TraceId,
            queryParameterName);
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString("{0} {1} {2}", Resources::GetResources().MissingQueryParameter, wformatString(queryName_), queryParameterName));
    }

    return ErrorCode::Success();
}

ErrorCode ProcessInnerQueryRequestAsyncOperation::CreateClusterHealthChunkQueryContext(
    __in QueryRequestContext & context)
{
    context.ContextId = *Constants::StoreType_ClusterTypeString;
    context.ContextKind = RequestContextKind::QueryEntityHealthStateChunk;

    auto error = context.SetApplicationHealthPolicyMap();
    if (!error.IsSuccess())
    {
        return error;
    }

    error = context.SetClusterHealthPolicy();
    if (!error.IsSuccess())
    {
        return error;
    }

    error = entityManager_.Cluster.ProcessQuery(*Constants::StoreType_ClusterTypeString, context);
    return error;
}

ErrorCode ProcessInnerQueryRequestAsyncOperation::CreateClusterQueryContext(
    __in QueryRequestContext & context)
{
    context.ContextKind = RequestContextKind::QueryEntityDetail;
    context.ContextId = *Constants::StoreType_ClusterTypeString;

    auto error = context.SetApplicationHealthPolicyMap();
    if (!error.IsSuccess())
    {
        return error;
    }

    error = context.SetClusterHealthPolicy();
    if (!error.IsSuccess())
    {
        return error;
    }

    error = entityManager_.Cluster.ProcessQuery(*Constants::StoreType_ClusterTypeString, context);
    return error;
}

ErrorCode ProcessInnerQueryRequestAsyncOperation::CreateNodeQueryContext(
    __in QueryRequestContext & context)
{
    // Find the NodeId argument (required).
    NodeHealthId nodeId;
    ErrorCode error = GetNodeIdFromQueryArguments(nodeId);
    if (!error.IsSuccess())
    {
        return error;
    }

    context.ContextKind = RequestContextKind::QueryEntityDetail;
    context.ContextId = wformatString("{0}", nodeId);

    error = context.SetClusterHealthPolicy();
    if (!error.IsSuccess())
    {
        return error;
    }

    error = entityManager_.Nodes.ProcessQuery(nodeId, context);
    return error;
}

ErrorCode ProcessInnerQueryRequestAsyncOperation::CreateNodeQueryListContext(
    __in QueryRequestContext & context)
{
    // Go to cluster then return all Nodes children
    wstring const & clusterId = *Constants::StoreType_ClusterTypeString;

    AttributesMap filters;

    wstring nodeName;
    // Filter for specific node name
    if (queryArgs_.TryGetValue(QueryResourceProperties::Node::Name, nodeName))
    {
        filters.insert(make_pair(HealthAttributeNames::NodeName, move(nodeName)));
    }

    context.ContextKind = RequestContextKind::QueryEntityChildren;
    context.ContextId = wformatString("{0}:Get{1}List", clusterId, HealthEntityKind::Node);
    context.ChildrenKind = HealthEntityKind::Node;
    context.Filters = move(filters);
    context.SetPagingStatus();

    auto error = context.SetClusterHealthPolicy();
    if (!error.IsSuccess())
    {
        return error;
    }

    error = entityManager_.Cluster.ProcessQuery(clusterId, context);
    return error;
}

ErrorCode ProcessInnerQueryRequestAsyncOperation::CreatePartitionQueryContext(
    __in QueryRequestContext & context)
{
    // Find the PartitionId argument (required).
    wstring partitionIdArg;
    auto error = GetStringQueryArgument(QueryResourceProperties::Partition::PartitionId, false, /*out*/ partitionIdArg);
    if (!error.IsSuccess())
    {
        return error;
    }

    PartitionHealthId partitionId(partitionIdArg);

    context.ContextKind = RequestContextKind::QueryEntityDetail;
    context.ContextId = move(partitionIdArg);

    error = context.SetApplicationHealthPolicy();
    if (!error.IsSuccess())
    {
        return error;
    }

    error = entityManager_.Partitions.ProcessQuery(partitionId, context);
    return error;
}

ErrorCode ProcessInnerQueryRequestAsyncOperation::CreatePartitionQueryListContext(
    __in QueryRequestContext & context)
{
    // Find the service name
    wstring serviceName;
    bool serviceNameFound = queryArgs_.TryGetValue(QueryResourceProperties::Service::ServiceName, serviceName);

    wstring partitionIdArg;
    bool partitionIdFound = queryArgs_.TryGetValue(QueryResourceProperties::Partition::PartitionId, partitionIdArg);

    context.SetContinuationToken();

    auto error = context.SetApplicationHealthPolicy();
    if (!error.IsSuccess()) { return error; }

    if (partitionIdFound)
    {
        // Go to partition and get only the information about the partition.
        // If service name is specified, add it to attributes to be checked.
        if (serviceNameFound)
        {
            AttributesMap filters;
            filters.insert(make_pair(HealthAttributeNames::ServiceName, move(serviceName)));
            context.Filters = move(filters);
        }

        PartitionHealthId partitionId(partitionIdArg);
        context.ContextKind = RequestContextKind::QueryEntityHealthState;
        context.ContextId = move(partitionIdArg);
        context.ChildrenKind = HealthEntityKind::Partition;
        return entityManager_.Partitions.ProcessQuery(partitionId, context);
    }
    else if (serviceNameFound)
    {
        // Find parent service entity, and return all Partition children
        context.ContextKind = RequestContextKind::QueryEntityChildren;
        context.ContextId = wformatString("{0}:Get{1}List", serviceName, HealthEntityKind::Partition);
        context.ChildrenKind = HealthEntityKind::Partition;
        ServiceHealthId id(move(serviceName));
        return entityManager_.Services.ProcessQuery(id, context);
    }
    else
    {
        // Invalid arguments: at least one of the service name or partition id must be specified
        error = ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString("{0} {1} {2}", Resources::GetResources().MissingQueryParameter, wformatString(queryName_), QueryResourceProperties::Service::ServiceName));
        HMEvents::Trace->QueryInvalidConfig(this->TraceId, wformatString(queryName_), error);
        return move(error);
    }
}

ErrorCode ProcessInnerQueryRequestAsyncOperation::CreateReplicaQueryContext(
    __in QueryRequestContext & context)
{
    // Find the PartitionId and Replica Id arguments (required).
    wstring partitionIdArg;
    auto error = GetStringQueryArgument(QueryResourceProperties::Partition::PartitionId, false, /*out*/ partitionIdArg);
    if (!error.IsSuccess())
    {
        return error;
    }

    wstring replicaIdArg;
    error = GetStringQueryArgument(QueryResourceProperties::Replica::ReplicaId, false, /*out*/ replicaIdArg);
    if (!error.IsSuccess())
    {
        return error;
    }

    FABRIC_REPLICA_ID parsedReplicaId;
    if (!TryParseInt64(replicaIdArg, parsedReplicaId))
    {
        HMEvents::Trace->QueryParseArg(
            this->TraceId,
            QueryResourceProperties::Replica::ReplicaId,
            replicaIdArg);
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString("{0} {1} {2}", Resources::GetResources().InvalidQueryParameter, QueryResourceProperties::Replica::ReplicaId, replicaIdArg));
    }

    ReplicaHealthId replicaId(
        PartitionHealthId(partitionIdArg),
        parsedReplicaId);

    context.ContextKind = RequestContextKind::QueryEntityDetail;
    context.ContextId = wformatString(replicaId);

    error = context.SetApplicationHealthPolicy();
    if (!error.IsSuccess()) { return error; }

    return entityManager_.Replicas.ProcessQuery(replicaId, context);
}

ErrorCode ProcessInnerQueryRequestAsyncOperation::CreateReplicaQueryListContext(
    __in QueryRequestContext & context)
{
    wstring partitionIdArg;
    auto error = GetStringQueryArgument(QueryResourceProperties::Partition::PartitionId, false, /*out*/ partitionIdArg);
    if (!error.IsSuccess())
    {
        return error;
    }

    context.ChildrenKind = HealthEntityKind::Replica;
    context.SetContinuationToken();

    AttributesMap filters;

    wstring replicaIdArg;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Replica::ReplicaId, replicaIdArg) ||
        queryArgs_.TryGetValue(QueryResourceProperties::Replica::InstanceId, replicaIdArg))
    {
        filters.insert(make_pair(QueryResourceProperties::Replica::ReplicaId, move(replicaIdArg)));
    }

    // Go to parent partition and get all replica children
    context.ContextKind = RequestContextKind::QueryEntityChildren;
    context.ContextId = wformatString("{0}:Get{1}List", partitionIdArg, HealthEntityKind::Replica);
    context.Filters = move(filters);
    error = context.SetApplicationHealthPolicy();
    if (!error.IsSuccess()) { return error; }

    PartitionHealthId id(partitionIdArg);
    return entityManager_.Partitions.ProcessQuery(id, context);
}

ErrorCode ProcessInnerQueryRequestAsyncOperation::CreateServiceQueryContext(
    __in QueryRequestContext & context)
{
    // Find the service name (required)
    wstring serviceName;
    auto error = GetStringQueryArgument(QueryResourceProperties::Service::ServiceName, true, /*out*/ serviceName);
    if (!error.IsSuccess())
    {
        return error;
    }

    context.ContextKind = RequestContextKind::QueryEntityDetail;
    context.ContextId = serviceName;

    error = context.SetApplicationHealthPolicy();
    if (!error.IsSuccess()) { return error; }

    ServiceHealthId id(move(serviceName));
    return entityManager_.Services.ProcessQuery(id, context);
}

ErrorCode ProcessInnerQueryRequestAsyncOperation::CreateServiceQueryListContext(
    __in QueryRequestContext & context)
{
    wstring applicationName;
    // Application name can be null for ad-hoc services
    auto error = GetStringQueryArgument(QueryResourceProperties::Application::ApplicationName, true, /*out*/ applicationName);
    if (!error.IsSuccess())
    {
        return error;
    }

    context.SetPagingStatus();

    AttributesMap filters;
    wstring serviceName;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Service::ServiceName, serviceName))
    {
        filters.insert(make_pair(HealthAttributeNames::ServiceName, move(serviceName)));
    }

    wstring serviceTypeName;
    if (queryArgs_.TryGetValue(QueryResourceProperties::ServiceType::ServiceTypeName, serviceTypeName))
    {
        filters.insert(make_pair(HealthAttributeNames::ServiceTypeName, move(serviceTypeName)));
    }

    context.ChildrenKind = HealthEntityKind::Service;

    // Go to application cache, find parent entity and return the aggregated health of Service children
    context.ContextKind = RequestContextKind::QueryEntityChildren;
    context.ContextId = wformatString("{0}:Get{1}List", applicationName, HealthEntityKind::Service);
    context.Filters = move(filters);

    error = context.SetApplicationHealthPolicy();
    if (!error.IsSuccess()) { return error; }

    ApplicationHealthId id(move(applicationName));
    return entityManager_.Applications.ProcessQuery(id, context);
}

ErrorCode ProcessInnerQueryRequestAsyncOperation::CreateApplicationQueryContext(
    __in QueryRequestContext & context)
{
    wstring applicationName;
    auto error = GetStringQueryArgument(QueryResourceProperties::Application::ApplicationName, false, /*out*/ applicationName);
    if (!error.IsSuccess())
    {
        return error;
    }

    context.ContextKind = RequestContextKind::QueryEntityDetail;
    context.ContextId = applicationName;

    error = context.SetApplicationHealthPolicy();
    if (!error.IsSuccess()) { return error; }

    ApplicationHealthId id(move(applicationName));
    return entityManager_.Applications.ProcessQuery(id, context);
}

ErrorCode ProcessInnerQueryRequestAsyncOperation::CreateApplicationQueryListContext(
    __in QueryRequestContext & context)
{
    // Go to cluster then return all Applications children
    AttributesMap filters;
    wstring applicationName;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Application::ApplicationName, applicationName))
    {
        filters.insert(make_pair(HealthAttributeNames::ApplicationName, move(applicationName)));
    }

    wstring applicationTypeName;
    if (queryArgs_.TryGetValue(QueryResourceProperties::ApplicationType::ApplicationTypeName, applicationTypeName))
    {
        filters.insert(make_pair(HealthAttributeNames::ApplicationTypeName, move(applicationTypeName)));
    }

    wstring applicationDefinitionKindFilter;
    DWORD applicationDefinitionKindFilterDW;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Deployment::ApplicationDefinitionKindFilter, applicationDefinitionKindFilter))
    {
        if (StringUtility::TryFromWString<DWORD>(applicationDefinitionKindFilter, applicationDefinitionKindFilterDW)
            && applicationDefinitionKindFilterDW != FABRIC_APPLICATION_DEFINITION_KIND_FILTER_DEFAULT)
        {
            filters.insert(make_pair(HealthAttributeNames::ApplicationDefinitionKind, applicationDefinitionKindFilter));
        }
    }

    context.ContextKind = RequestContextKind::QueryEntityChildren;
    context.ContextId = wformatString("{0}:Get{1}List", *Constants::StoreType_ClusterTypeString, HealthEntityKind::Application);
    context.ChildrenKind = HealthEntityKind::Application;
    context.SetPagingStatus();
    context.Filters = move(filters);

    // TODO: add application health policy map

    return entityManager_.Cluster.ProcessQuery(*Constants::StoreType_ClusterTypeString, context);
}

ErrorCode ProcessInnerQueryRequestAsyncOperation::CreateApplicationUnhealthyEvaluationContext(__in QueryRequestContext & context)
{
    AttributesMap filters;
    wstring applicationName;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Application::ApplicationName, applicationName))
    {
        filters.insert(make_pair(HealthAttributeNames::ApplicationName, move(applicationName)));
    }

    context.ContextKind = RequestContextKind::QueryEntityUnhealthyChildren;
    context.ContextId = wformatString("{0}:Get{1}UnhealthyList", *Constants::StoreType_ClusterTypeString, HealthEntityKind::Application);
    context.ChildrenKind = HealthEntityKind::Application;
    context.SetPagingStatus();
    context.Filters = move(filters);

    return entityManager_.Cluster.ProcessQuery(*Constants::StoreType_ClusterTypeString, context);
}

ErrorCode ProcessInnerQueryRequestAsyncOperation::CreateDeployedApplicationQueryContext(
    __in QueryRequestContext & context)
{
    wstring applicationName;
    auto error = GetStringQueryArgument(QueryResourceProperties::Application::ApplicationName, false, /*out*/ applicationName);
    if (!error.IsSuccess())
    {
        return error;
    }

    // Find the NodeId argument (required).
    NodeHealthId nodeId;
    error = GetNodeIdFromQueryArguments(nodeId);
    if (!error.IsSuccess())
    {
        return error;
    }

    AttributesMap filters;
    wstring deployedServicePackagesFilterString;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Health::DeployedServicePackagesFilter, deployedServicePackagesFilterString))
    {
        filters.insert(make_pair(QueryResourceProperties::Health::DeployedServicePackagesFilter, deployedServicePackagesFilterString));
    }

    context.ContextKind = RequestContextKind::QueryEntityDetail;
    DeployedApplicationHealthId id(move(applicationName), nodeId);
    context.ContextId = wformatString("{0}", id);
    context.Filters = move(filters);

    error = context.SetApplicationHealthPolicy();
    if (!error.IsSuccess()) { return error; }

    return entityManager_.DeployedApplications.ProcessQuery(id, context);
}

ErrorCode ProcessInnerQueryRequestAsyncOperation::CreateDeployedApplicationQueryListContext(
    __in QueryRequestContext & context)
{
    wstring applicationName;
    auto error = GetStringQueryArgument(QueryResourceProperties::Application::ApplicationName, false, /*out*/ applicationName);
    if (!error.IsSuccess())
    {
        return error;
    }

    context.ChildrenKind = HealthEntityKind::DeployedApplication;
    context.SetContinuationToken();
    context.SetMaxResults();

    // Go to application cache, find parent entity and return the aggregated health of DeployedApplication children
    // ( return all deployed applications of a given application name )
    context.ContextKind = RequestContextKind::QueryEntityChildren;
    context.ContextId = wformatString("{0}:Get{1}List", applicationName, HealthEntityKind::DeployedApplication);

    error = context.SetApplicationHealthPolicy();
    if (!error.IsSuccess()) { return error; }

    ApplicationHealthId id(move(applicationName));
    return entityManager_.Applications.ProcessQuery(id, context);
}

// From cluster, get all applications and then get all deployed applications.
// Do the above recursively, and have each recursive child function call return result items for us to add to the list pager.
// Because of the structure of the health store, nodes do not directly have children, so we can't query a node.
ErrorCode ProcessInnerQueryRequestAsyncOperation::CreateDeployedApplicationsOnNodeQueryListContext(
    __in QueryRequestContext & context)
{
    // Get the node name
    // queryArgs_ is where the following method tries to get the node name from
    wstring nodeName;

    // This configuration doesn't allow empty and requires a parameter.
    auto error = GetStringQueryArgument(QueryResourceProperties::Node::Name, false, /*out*/ nodeName);
    if (!error.IsSuccess())
    {
        if (error.IsError(ErrorCodeValue::NotFound))
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(GET_QUERY_RC(Query_Missing_Required_Argument), queryName_, QueryResourceProperties::Node::Name));
        }
        return error;
    }

    // Set the query filter in the attribute map.
    AttributesMap filters;
    filters.insert(make_pair(QueryResourceProperties::Node::Name, nodeName));

    // See if there is an application name provided. If so, then get only the deployed node matching that name.
    // This is an optional parameter. Do not allow empty.
    wstring applicationName;
    bool hasApplicationName = false;
    error = GetStringQueryArgument(QueryResourceProperties::Application::ApplicationName, false, true, /*out*/ applicationName);
    if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NotFound))
    {
        return error;
    }
    else if (error.IsSuccess() && !applicationName.empty())
    {
        // If there is an error, the string will be empty, so we check here in the if statement.
        hasApplicationName = true;
    }

    if (hasApplicationName)
    {
        filters.insert(make_pair(HealthAttributeNames::ApplicationName, applicationName));
    }
    else
    {
        context.SetPagingStatus();
    }

    // Set context so that the query knows how to process the information
    context.ChildrenKind = HealthEntityKind::DeployedApplication;
    context.ContextKind = RequestContextKind::QueryEntityChildren;
    context.ContextId = wformatString("{0}:Get{1}List", nodeName, HealthEntityKind::DeployedApplication);
    context.Filters = move(filters);

    // Set the entityID
    return entityManager_.Cluster.ProcessQuery(*Constants::StoreType_ClusterTypeString, context);
}

ErrorCode ProcessInnerQueryRequestAsyncOperation::CreateDeployedServicePackageQueryContext(
    __in QueryRequestContext & context)
{
    wstring applicationName;
    auto error = GetStringQueryArgument(QueryResourceProperties::Application::ApplicationName, false, /*out*/ applicationName);
    if (!error.IsSuccess())
    {
        return error;
    }

    // Find the service package name (required)
    wstring serviceManifestName;
    error = GetStringQueryArgument(QueryResourceProperties::ServicePackage::ServiceManifestName, false, /*out*/ serviceManifestName);
    if (!error.IsSuccess())
    {
        return error;
    }

    // Find ServicePackageActivationId (optional)
    wstring servicePackageActivationId;
    error = GetStringQueryArgument(QueryResourceProperties::ServicePackage::ServicePackageActivationId, true, true, /*out*/ servicePackageActivationId);
    if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NotFound))
    {
        return error;
    }

    // Find the NodeId argument (required).
    NodeHealthId nodeId;
    error = GetNodeIdFromQueryArguments(nodeId);
    if (!error.IsSuccess())
    {
        return error;
    }

    context.ContextKind = RequestContextKind::QueryEntityDetail;

    DeployedServicePackageHealthId id(move(applicationName), move(serviceManifestName), servicePackageActivationId, nodeId);
    context.ContextId = wformatString("{0}", id);

    error = context.SetApplicationHealthPolicy();
    if (!error.IsSuccess()) { return error; }

    return entityManager_.DeployedServicePackages.ProcessQuery(id, context);
}

ErrorCode ProcessInnerQueryRequestAsyncOperation::CreateDeployedServicePackageQueryListContext(
    __in QueryRequestContext & context)
{
    wstring applicationName;
    auto error = GetStringQueryArgument(QueryResourceProperties::Application::ApplicationName, false, /*out*/ applicationName);
    if (!error.IsSuccess())
    {
        return error;
    }

    // Find the NodeId argument (required).
    NodeHealthId nodeId;
    error = GetNodeIdFromQueryArguments(nodeId);
    if (!error.IsSuccess())
    {
        return error;
    }

    context.ContextKind = RequestContextKind::QueryEntityChildren;
    context.ChildrenKind = HealthEntityKind::DeployedServicePackage;
    context.SetContinuationToken();

    AttributesMap filters;
    // Go to deployed application cache, find parent entity and return the aggregated health of DeployedServicePackage children
    DeployedApplicationHealthId id(move(applicationName), nodeId);
    context.ContextId = wformatString("{0}:Get{1}List", id, HealthEntityKind::DeployedServicePackage);
    context.Filters = move(filters);

    error = context.SetApplicationHealthPolicy();
    if (!error.IsSuccess()) { return error; }

    return entityManager_.DeployedApplications.ProcessQuery(id, context);
}
