// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ClusterHealthChunkQueryDescription");

ClusterHealthChunkQueryDescription::ClusterHealthChunkQueryDescription()
    : healthPolicy_()
    , applicationHealthPolicies_()
    , applicationFilters_()
    , nodeFilters_()
{
}

ClusterHealthChunkQueryDescription::~ClusterHealthChunkQueryDescription()
{
}

ErrorCode ClusterHealthChunkQueryDescription::FromPublicApi(FABRIC_CLUSTER_HEALTH_CHUNK_QUERY_DESCRIPTION const & publicClusterHealthChunkQueryDescription)
{
    // ClusterHealthPolicy
    if (publicClusterHealthChunkQueryDescription.ClusterHealthPolicy != NULL)
    {
        ClusterHealthPolicy policy;
        auto error = policy.FromPublicApi(*publicClusterHealthChunkQueryDescription.ClusterHealthPolicy);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "ClusterHealthPolicy::FromPublicApi error: {0}", error);
            return error;
        }

        healthPolicy_ = make_unique<ClusterHealthPolicy>(move(policy));
    }

    // ApplicationHealthPolicyMap
    if (publicClusterHealthChunkQueryDescription.ApplicationHealthPolicyMap != NULL)
    {
        auto error = applicationHealthPolicies_.FromPublicApi(*publicClusterHealthChunkQueryDescription.ApplicationHealthPolicyMap);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "ApplicationHealthPolicyMap::FromPublicApi error: {0}", error);
            return error;
        }
    }

    // ApplicationFilters
    {
        auto error = PublicApiHelper::FromPublicApiList<ApplicationHealthStateFilter, FABRIC_APPLICATION_HEALTH_STATE_FILTER_LIST>(publicClusterHealthChunkQueryDescription.ApplicationFilters, applicationFilters_);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "ApplicationFilters::FromPublicApi error: {0}", error);
            return error;
        }
    }

    // NodeFilters
    {
        auto error = PublicApiHelper::FromPublicApiList<NodeHealthStateFilter, FABRIC_NODE_HEALTH_STATE_FILTER_LIST>(publicClusterHealthChunkQueryDescription.NodeFilters, nodeFilters_);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "NodeFilters::FromPublicApi error: {0}", error);
            return error;
        }
    }

    return ValidateFilters();
}

Common::ErrorCode ClusterHealthChunkQueryDescription::ValidateNodeFilters(NodeHealthStateFilterList const & filters)
{
    if (!filters.empty())
    {
        unordered_set<wstring> keys;
        for (auto const & filter : filters)
        {
            auto insertResult = keys.insert(filter.NodeNameFilter);
            if (!insertResult.second)
            {
                // There is already a filter for the node name
                wstring msg;
                StringWriter writer(msg);
                writer.Write("{0}", HMResource::GetResources().DuplicateNodeFilters);
                if (filter.NodeNameFilter.empty())
                {
                    writer.Write(" global.");
                }
                else
                {
                    writer.Write(" NodeNameFilter: {0}.", filter.NodeNameFilter);
                }

                ErrorCode error(ErrorCodeValue::InvalidArgument, move(msg));
                Trace.WriteInfo(TraceSource, "ValidateFilters failed with {0} {1}", error, error.Message);
                return error;
            }
        }
    }

    return ErrorCode::Success();
}

Common::ErrorCode ClusterHealthChunkQueryDescription::ValidateApplicationFilters(ApplicationHealthStateFilterList const & filters)
{
    if (!filters.empty())
    {
        unordered_set<wstring> keys;
        for (auto const & filter : filters)
        {
            bool added;
            if (!filter.ApplicationNameFilter.empty())
            {
                added = keys.insert(filter.ApplicationNameFilter).second;
            }
            else
            {
                added = keys.insert(filter.ApplicationTypeNameFilter).second;
            }

            if (!added)
            {
                // There is already a filter for the application
                wstring msg;
                StringWriter writer(msg);
                writer.Write("{0}", HMResource::GetResources().DuplicateApplicationFilters);

                if (!filter.ApplicationNameFilter.empty())
                {
                    writer.Write(" ApplicationNameFilter: {0}.", filter.ApplicationNameFilter);
                }
                else if (!filter.ApplicationTypeNameFilter.empty())
                {
                    writer.Write(" ApplicationTypeNameFilter: {0}.", filter.ApplicationTypeNameFilter);
                }
                else
                {
                    writer.Write(" global.");
                }

                ErrorCode error(ErrorCodeValue::InvalidArgument, move(msg));
                Trace.WriteInfo(TraceSource, "ValidateFilters failed with {0} {1}", error, error.Message);
                return error;
            }

            // Check services filters
            auto error = ValidateServiceFilters(filter.ServiceFilters, filter.ApplicationNameFilter);
            if (!error.IsSuccess())
            {
                return error;
            }

            // Check deployed application filters
            error = ValidateDeployedApplicationFilters(filter.DeployedApplicationFilters, filter.ApplicationNameFilter);
            if (!error.IsSuccess())
            {
                return error;
            }
        }
    }

    return ErrorCode::Success();
}

Common::ErrorCode ClusterHealthChunkQueryDescription::ValidateServiceFilters(
    ServiceHealthStateFilterList const & filters, 
    std::wstring const & applicationNameFilter)
{
    if (!filters.empty())
    {
        unordered_set<wstring> keys;
        for (auto const & filter : filters)
        {
            auto insertResult = keys.insert(filter.ServiceNameFilter);
            if (!insertResult.second)
            {
                // There is already a filter for the service
                wstring msg;
                StringWriter writer(msg);
                writer.Write("{0}", HMResource::GetResources().DuplicateServiceFilters);

                if (applicationNameFilter.empty())
                {
                    writer.Write(" global,");
                }
                else
                {
                    writer.Write(" ApplicationNameFilter: {0},", applicationNameFilter);
                }

                if (filter.ServiceNameFilter.empty())
                {
                    writer.Write(" global.");
                }
                else
                {
                    writer.Write(" ServiceNameFilter: {0}.", filter.ServiceNameFilter);
                }

                ErrorCode error(ErrorCodeValue::InvalidArgument, move(msg));
                Trace.WriteInfo(TraceSource, "ValidateFilters failed with {0} {1}", error, error.Message);
                return error;
            }

            // Check partitions filter
            auto error = ValidatePartitionFilters(filter.PartitionFilters, applicationNameFilter, filter.ServiceNameFilter);
            if (!error.IsSuccess())
            {
                return error;
            }
        }
    }

    return ErrorCode::Success();
}

Common::ErrorCode ClusterHealthChunkQueryDescription::ValidatePartitionFilters(
    PartitionHealthStateFilterList const & filters,
    std::wstring const & applicationNameFilter,
    std::wstring const & serviceNameFilter)
{
    if (!filters.empty())
    {
        set<Guid> keys;
        for (auto const & filter : filters)
        {
            auto insertResult = keys.insert(filter.PartitionIdFilter);
            if (!insertResult.second)
            {
                // There is already a filter for the partition
                wstring msg;
                StringWriter writer(msg);
                writer.Write("{0}", HMResource::GetResources().DuplicatePartitionFilters);

                if (applicationNameFilter.empty())
                {
                    writer.Write(" global,");
                }
                else
                {
                    writer.Write(" ApplicationNameFilter: {0},", applicationNameFilter);
                }

                if (serviceNameFilter.empty())
                {
                    writer.Write(" global,");
                }
                else
                {
                    writer.Write(" ServiceNameFilter: {0},", serviceNameFilter);
                }

                if (filter.PartitionIdFilter == Guid::Empty())
                {
                    writer.Write(" global.");
                }
                else
                {
                    writer.Write(" PartitionIdFilter: {0}.", filter.PartitionIdFilter);
                }

                ErrorCode error(ErrorCodeValue::InvalidArgument, move(msg));
                Trace.WriteInfo(TraceSource, "ValidateFilters failed with {0} {1}", error, error.Message);
                return error;
            }

            // Check replica filters
            auto error = ValidateReplicaFilters(filter.ReplicaFilters, applicationNameFilter, serviceNameFilter, filter.PartitionIdFilter);
            if (!error.IsSuccess())
            {
                return error;
            }
        }
    }

    return ErrorCode::Success();
}

Common::ErrorCode ClusterHealthChunkQueryDescription::ValidateReplicaFilters(
    ReplicaHealthStateFilterList const & filters,
    std::wstring const & applicationNameFilter,
    std::wstring const & serviceNameFilter,
    Common::Guid const & partitionIdFilter)
{
    if (!filters.empty())
    {
        unordered_set<FABRIC_REPLICA_ID> keys;
        for (auto const & filter : filters)
        {
            auto insertResult = keys.insert(filter.ReplicaOrInstanceIdFilter);
            if (!insertResult.second)
            {
                // There is already a filter for the replica
                wstring msg;
                StringWriter writer(msg);
                writer.Write("{0}", HMResource::GetResources().DuplicateReplicaFilters);

                if (applicationNameFilter.empty())
                {
                    writer.Write(" global,");
                }
                else
                {
                    writer.Write(" ApplicationNameFilter: {0},", applicationNameFilter);
                }

                if (serviceNameFilter.empty())
                {
                    writer.Write(" global,");
                }
                else
                {
                    writer.Write(" ServiceNameFilter: {0},", serviceNameFilter);
                }

                if (partitionIdFilter == Guid::Empty())
                {
                    writer.Write(" global,");
                }
                else
                {
                    writer.Write(" PartitionIdFilter: {0},", partitionIdFilter);
                }

                if (filter.ReplicaOrInstanceIdFilter == FABRIC_INVALID_REPLICA_ID)
                {
                    writer.Write(" global.");
                }
                else
                {
                    writer.Write(" ReplicaOrInstanceIdFilter: {0}.", filter.ReplicaOrInstanceIdFilter);
                }

                ErrorCode error(ErrorCodeValue::InvalidArgument, move(msg));
                Trace.WriteInfo(TraceSource, "ValidateFilters failed with {0} {1}", error, error.Message);
                return error;
            }
        }
    }

    return ErrorCode::Success();
}


Common::ErrorCode ClusterHealthChunkQueryDescription::ValidateDeployedApplicationFilters(
    DeployedApplicationHealthStateFilterList const & filters,
    std::wstring const & applicationNameFilter)
{
    if (!filters.empty())
    {
        unordered_set<std::wstring> keys;
        for (auto const & filter : filters)
        {
            auto insertResult = keys.insert(filter.NodeNameFilter);
            if (!insertResult.second)
            {
                // There is already a filter for the deployed application
                wstring msg;
                StringWriter writer(msg);
                writer.Write("{0}", HMResource::GetResources().DuplicateDeployedApplicationFilters);

                if (applicationNameFilter.empty())
                {
                    writer.Write(" global,");
                }
                else
                {
                    writer.Write(" ApplicationNameFilter: {0},", applicationNameFilter);
                }

                if (filter.NodeNameFilter.empty())
                {
                    writer.Write(" global.");
                }
                else
                {
                    writer.Write(" NodeNameFilter: {0}.", filter.NodeNameFilter);
                }

                ErrorCode error(ErrorCodeValue::InvalidArgument, move(msg));

                Trace.WriteInfo(TraceSource, "ValidateFilters failed with {0} {1}", error, error.Message);
                return error;
            }

            auto error = ValidateDeployedServicePackageFilters(filter.DeployedServicePackageFilters, applicationNameFilter, filter.NodeNameFilter);
            if (!error.IsSuccess())
            {
                return error;
            }
        }
    }

    return ErrorCode::Success();
}


Common::ErrorCode ClusterHealthChunkQueryDescription::ValidateDeployedServicePackageFilters(
    DeployedServicePackageHealthStateFilterList const & filters,
    std::wstring const & applicationNameFilter,
    std::wstring const & nodeNameFilter)
{
    if (!filters.empty())
    {
        unordered_set<wstring> keys;
        for (auto const & filter : filters)
        {
            auto insertResult = keys.insert(filter.ServiceManifestNameFilter);
            if (!insertResult.second)
            {
                // There is already a filter for the deployed service package
                wstring msg;
                StringWriter writer(msg);
                writer.Write("{0}", HMResource::GetResources().DuplicateDeployedServicePackageFilters);

                if (applicationNameFilter.empty())
                {
                    writer.Write(" global,");
                }
                else
                {
                    writer.Write(" ApplicationNameFilter: {0},", applicationNameFilter);
                }

                if (nodeNameFilter.empty())
                {
                    writer.Write(" global,");
                }
                else
                {
                    writer.Write(" NodeNameFilter: {0},", nodeNameFilter);
                }

                if (filter.ServiceManifestNameFilter.empty())
                {
                    writer.Write(" global.");
                }
                else
                {
                    writer.Write(" ServiceManifestNameFilter: {0}.", filter.ServiceManifestNameFilter);
                }

                ErrorCode error(ErrorCodeValue::InvalidArgument, move(msg));
                Trace.WriteInfo(TraceSource, "ValidateFilters failed with {0} {1}", error, error.Message);
                return error;
            }
        }
    }

    return ErrorCode::Success();
}

Common::ErrorCode ClusterHealthChunkQueryDescription::ValidateFilters() const
{
    auto error = ValidateNodeFilters(nodeFilters_);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = ValidateApplicationFilters(applicationFilters_);
    return error;
}

void ClusterHealthChunkQueryDescription::MoveToQueryArguments(__inout QueryArgumentMap & argMap)
{
    if (healthPolicy_)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::ClusterHealthPolicy,
            healthPolicy_->ToString());
    }

    if (!nodeFilters_.empty())
    {
        NodeHealthStateFilterListWrapper wrapper(move(nodeFilters_));
        argMap.Insert(
            Query::QueryResourceProperties::Health::NodeFilters,
            wrapper.ToJsonString());
    }

    if (!applicationFilters_.empty())
    {
        ApplicationHealthStateFilterListWrapper wrapper(move(applicationFilters_));
        argMap.Insert(
            Query::QueryResourceProperties::Health::ApplicationFilters,
            wrapper.ToJsonString());
    }

    if (!applicationHealthPolicies_.empty())
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::ApplicationHealthPolicyMap,
            applicationHealthPolicies_.ToString());
    }
}

wstring ClusterHealthChunkQueryDescription::ToJsonString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<ClusterHealthChunkQueryDescription&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

ErrorCode ClusterHealthChunkQueryDescription::FromJsonString(
    wstring const & str, 
    __inout ClusterHealthChunkQueryDescription & queryDescription)
{
    return JsonHelper::Deserialize(queryDescription, str);
}
