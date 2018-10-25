// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace Store;
using namespace ServiceModel;
using namespace Query;
using namespace Management::HealthManager;

QueryRequestContext::QueryRequestContext(
    Store::ReplicaActivityId const & replicaActivityId,
    __in ServiceModel::QueryArgumentMap & queryArgs)
    : ReplicaActivityTraceComponent(replicaActivityId)
    , queryArgs_(queryArgs)
    , contextKind_(RequestContextKind::Query)
    , applicationHealthPolicy_()
    , clusterHealthPolicy_()
    , applicationHealthPolicies_()
    , queryResult_()
    , filters_()
    , childrenKind_(HealthEntityKind::Unknown)
    , contextId_()
    , maxResults_(ServiceModel::QueryPagingDescription::MaxResultsDefaultValue)
    , canCacheResult_(true)
{
}

QueryRequestContext::QueryRequestContext(QueryRequestContext && other)
    : ReplicaActivityTraceComponent(move(other))
    , queryArgs_(other.queryArgs_)
    , contextKind_(move(other.contextKind_))
    , applicationHealthPolicy_(move(other.applicationHealthPolicy_))
    , clusterHealthPolicy_(move(other.clusterHealthPolicy_))
    , applicationHealthPolicies_(move(other.applicationHealthPolicies_))
    , queryResult_(move(other.queryResult_))
    , filters_(move(other.filters_))
    , childrenKind_(move(other.childrenKind_))
    , contextId_(move(other.contextId_))
    , maxResults_(move(other.maxResults_))
    , canCacheResult_(move(other.canCacheResult_))
{
}

QueryRequestContext & QueryRequestContext::operator = (QueryRequestContext && other)
{
    if (this != &other)
    {
        queryArgs_ = move(other.queryArgs_);
        contextKind_ = move(other.contextKind_);
        applicationHealthPolicy_ = move(other.applicationHealthPolicy_);
        clusterHealthPolicy_ = move(other.clusterHealthPolicy_);
        applicationHealthPolicies_ = move(other.applicationHealthPolicies_);
        queryResult_ = move(other.queryResult_);
        filters_ = move(other.filters_);
        childrenKind_ = move(other.childrenKind_);
        contextId_ = move(other.contextId_);
        maxResults_ = move(other.maxResults_);
        canCacheResult_ = move(other.canCacheResult_);
    }

    return *this;
}

QueryRequestContext::~QueryRequestContext()
{
}

RequestContextKind::Enum QueryRequestContext::get_ContextKind() const
{
    ASSERT_IF(contextKind_ == RequestContextKind::Query, "{0}: contextKind_ is not initialized for Query", this->ReplicaActivityId);
    return contextKind_;
}

bool QueryRequestContext::get_HealthPolicyConsiderWarningAsError() const
{
    if (applicationHealthPolicy_)
    {
        return applicationHealthPolicy_->ConsiderWarningAsError;
    }

    if (clusterHealthPolicy_)
    {
        return clusterHealthPolicy_->ConsiderWarningAsError;
    }

    Assert::CodingError("{0}: At least one health policy context should be set", *this);
}

Common::ErrorCode QueryRequestContext::SetApplicationHealthPolicy(
    std::wstring const & healthPolicy)
{
    ApplicationHealthPolicy applicationHealthPolicy;
    auto error = ApplicationHealthPolicy::FromString(healthPolicy, applicationHealthPolicy);
    if (!error.IsSuccess())
    {
        return error;
    }

    applicationHealthPolicy_ = make_shared<ApplicationHealthPolicy>(std::move(applicationHealthPolicy));
    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::SetClusterHealthPolicy(
    std::wstring const & healthPolicy)
{
    ClusterHealthPolicy clusterHealthPolicy;
    auto error = ClusterHealthPolicy::FromString(healthPolicy, clusterHealthPolicy);
    if (!error.IsSuccess())
    {
        return error;
    }

    clusterHealthPolicy_ = make_shared<ClusterHealthPolicy>(std::move(clusterHealthPolicy));
    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::SetClusterHealthPolicy()
{
    wstring healthPolicyArg;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Health::ClusterHealthPolicy, healthPolicyArg))
    {
        auto error = SetClusterHealthPolicy(healthPolicyArg);
        if (!error.IsSuccess())
        {
            HMEvents::Trace->QueryParseArg(
                this->TraceId,
                QueryResourceProperties::Health::ClusterHealthPolicy,
                healthPolicyArg);
            return error;
        }

        // Can't cache result if policies are specified
        canCacheResult_ = false;
    }

    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::SetApplicationHealthPolicy()
{
    wstring healthPolicyArg;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Health::ApplicationHealthPolicy, healthPolicyArg))
    {
        auto error = SetApplicationHealthPolicy(healthPolicyArg);
        if (!error.IsSuccess())
        {
            HMEvents::Trace->QueryParseArg(
                this->TraceId,
                QueryResourceProperties::Health::ApplicationHealthPolicy,
                healthPolicyArg);
            return error;
        }

        // Can't cache result if policies are specified
        canCacheResult_ = false;
    }

    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::SetContinuationToken()
{
    wstring continuationToken;
    if (queryArgs_.TryGetValue(Query::QueryResourceProperties::QueryMetadata::ContinuationToken, continuationToken))
    {
        // Can't cache result if continuation token is specified
        canCacheResult_ = false;
        continuationToken_ = move(continuationToken);
    }

    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::ClearContinuationToken()
{
    continuationToken_.clear();

    // Since continuation token is now empty again, can cache results
    canCacheResult_ = true;

    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::ClearMaxResults()
{
    maxResults_ = ServiceModel::QueryPagingDescription::MaxResultsDefaultValue;

    // Since max results is set to default value again, can cache results
    canCacheResult_ = true;

    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::SetPagingStatus()
{
    auto error = SetContinuationToken();
    if (!error.IsSuccess())
    {
        return error;
    }

    return SetMaxResults();
}

Common::ErrorCode QueryRequestContext::SetMaxResults()
{
    // If the maxResults value exists, then set it.
    int64 maxResults;

    auto error = queryArgs_.GetInt64(Query::QueryResourceProperties::QueryMetadata::MaxResults, maxResults);
    if (error.IsSuccess() && maxResults > 0)
    {
        maxResults_ = maxResults;

        // Can't cache if max results is set
        canCacheResult_ = false;
    }

    if (error.IsError(ErrorCodeValue::InvalidArgument) || maxResults < 0)
    {
        wstring maxResultsString;
        queryArgs_.TryGetValue(Query::QueryResourceProperties::QueryMetadata::MaxResults, maxResultsString);

        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(GET_COMMON_RC(Invalid_Max_Results), maxResultsString));
    }

    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::SetApplicationHealthPolicyMap()
{
    wstring healthPolicyArg;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Health::ApplicationHealthPolicyMap, healthPolicyArg))
    {
        ApplicationHealthPolicyMap policies;
        auto error = ApplicationHealthPolicyMap::FromString(healthPolicyArg, policies);
        if (!error.IsSuccess())
        {
            HMEvents::Trace->QueryParseArg(
                this->TraceId,
                QueryResourceProperties::Health::ApplicationHealthPolicyMap,
                healthPolicyArg);
            return error;
        }

        applicationHealthPolicies_ = make_shared<ApplicationHealthPolicyMap>(move(policies));
        // Can't cache result if custom policies are specified
        canCacheResult_ = false;
    }

    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::GetPartitionHealthStatsFilter(
    __inout ServiceModel::PartitionHealthStatisticsFilterUPtr & statsFilter) const
{
    wstring filterString;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Health::HealthStatsFilter, filterString))
    {
        PartitionHealthStatisticsFilter filter;
        auto error = PartitionHealthStatisticsFilter::FromString(filterString, filter);
        if (!error.IsSuccess())
        {
            HMEvents::Trace->QueryParseArg(
                this->TraceId,
                *QueryResourceProperties::Health::HealthStatsFilter,
                filterString);
            return error;
        }

        statsFilter = make_unique<PartitionHealthStatisticsFilter>(move(filter));
    }

    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::GetServiceHealthStatsFilter(
    __inout ServiceModel::ServiceHealthStatisticsFilterUPtr & statsFilter) const
{
    wstring filterString;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Health::HealthStatsFilter, filterString))
    {
        ServiceHealthStatisticsFilter filter;
        auto error = ServiceHealthStatisticsFilter::FromString(filterString, filter);
        if (!error.IsSuccess())
        {
            HMEvents::Trace->QueryParseArg(
                this->TraceId,
                *QueryResourceProperties::Health::HealthStatsFilter,
                filterString);
            return error;
        }

        statsFilter = make_unique<ServiceHealthStatisticsFilter>(move(filter));
    }

    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::GetApplicationHealthStatsFilter(
    __inout ServiceModel::ApplicationHealthStatisticsFilterUPtr & statsFilter) const
{
    wstring filterString;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Health::HealthStatsFilter, filterString))
    {
        ApplicationHealthStatisticsFilter filter;
        auto error = ApplicationHealthStatisticsFilter::FromString(filterString, filter);
        if (!error.IsSuccess())
        {
            HMEvents::Trace->QueryParseArg(
                this->TraceId,
                *QueryResourceProperties::Health::HealthStatsFilter,
                filterString);
            return error;
        }

        statsFilter = make_unique<ApplicationHealthStatisticsFilter>(move(filter));
    }

    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::GetDeployedApplicationHealthStatsFilter(
    __inout ServiceModel::DeployedApplicationHealthStatisticsFilterUPtr & statsFilter) const
{
    wstring filterString;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Health::HealthStatsFilter, filterString))
    {
        DeployedApplicationHealthStatisticsFilter filter;
        auto error = DeployedApplicationHealthStatisticsFilter::FromString(filterString, filter);
        if (!error.IsSuccess())
        {
            HMEvents::Trace->QueryParseArg(
                this->TraceId,
                *QueryResourceProperties::Health::HealthStatsFilter,
                filterString);
            return error;
        }

        statsFilter = make_unique<DeployedApplicationHealthStatisticsFilter>(move(filter));
    }

    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::GetClusterHealthStatsFilter(
    __inout ServiceModel::ClusterHealthStatisticsFilterUPtr & statsFilter) const
{
    wstring filterString;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Health::HealthStatsFilter, filterString))
    {
        ClusterHealthStatisticsFilter filter;
        auto error = ClusterHealthStatisticsFilter::FromString(filterString, filter);
        if (!error.IsSuccess())
        {
            HMEvents::Trace->QueryParseArg(
                this->TraceId,
                *QueryResourceProperties::Health::HealthStatsFilter,
                filterString);
            return error;
        }

        if (filter.ExcludeHealthStatistics || filter.IncludeSystemApplicationHealthStatistics)
        {
            canCacheResult_ = false;
        }

        statsFilter = make_unique<ClusterHealthStatisticsFilter>(move(filter));
    }

    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::GetEventsFilter(__inout std::unique_ptr<ServiceModel::HealthEventsFilter> & eventsFilter) const
{
    wstring filterString;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Health::EventsFilter, filterString))
    {
        HealthEventsFilter filter;
        auto error = HealthEventsFilter::FromString(filterString, filter);
        if (!error.IsSuccess())
        {
            HMEvents::Trace->QueryParseArg(
                this->TraceId,
                *QueryResourceProperties::Health::EventsFilter,
                filterString);
            return error;
        }

        eventsFilter = make_unique<HealthEventsFilter>(move(filter));
    }

    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::GetServicesFilter(
    __inout std::unique_ptr<ServiceModel::ServiceHealthStatesFilter> & servicesFilter) const
{
    wstring filterString;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Health::ServicesFilter, filterString))
    {
        ServiceHealthStatesFilter filter;
        auto error = ServiceHealthStatesFilter::FromString(filterString, filter);
        if (!error.IsSuccess())
        {
            HMEvents::Trace->QueryParseArg(
                this->TraceId,
                *QueryResourceProperties::Health::ServicesFilter,
                filterString);
            return error;
        }

        servicesFilter = make_unique<ServiceHealthStatesFilter>(move(filter));
    }

    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::GetDeployedApplicationsFilter(
    __inout std::unique_ptr<ServiceModel::DeployedApplicationHealthStatesFilter> & deployedApplicationsFilter) const
{
    wstring filterString;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Health::DeployedApplicationsFilter, filterString))
    {
        DeployedApplicationHealthStatesFilter filter;
        auto error = DeployedApplicationHealthStatesFilter::FromString(filterString, filter);
        if (!error.IsSuccess())
        {
            HMEvents::Trace->QueryParseArg(
                this->TraceId,
                *QueryResourceProperties::Health::DeployedApplicationsFilter,
                filterString);
            return error;
        }

        deployedApplicationsFilter = make_unique<DeployedApplicationHealthStatesFilter>(move(filter));
    }

    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::GetDeployedServicePackagesFilter(
    __inout std::unique_ptr<ServiceModel::DeployedServicePackageHealthStatesFilter> & deployedServicePackagesFilter) const
{
    wstring filterString;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Health::DeployedServicePackagesFilter, filterString))
    {
        DeployedServicePackageHealthStatesFilter filter;
        auto error = DeployedServicePackageHealthStatesFilter::FromString(filterString, filter);
        if (!error.IsSuccess())
        {
            HMEvents::Trace->QueryParseArg(
                this->TraceId,
                QueryResourceProperties::Health::DeployedServicePackagesFilter,
                filterString);
            return error;
        }

        deployedServicePackagesFilter = make_unique<DeployedServicePackageHealthStatesFilter>(move(filter));
    }

    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::GetPartitionsFilter(
    __inout std::unique_ptr<ServiceModel::PartitionHealthStatesFilter> & partitionsFilter) const
{
    wstring filterString;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Health::PartitionsFilter, filterString))
    {
        PartitionHealthStatesFilter filter;
        auto error = PartitionHealthStatesFilter::FromString(filterString, filter);
        if (!error.IsSuccess())
        {
            HMEvents::Trace->QueryParseArg(
                this->TraceId,
                QueryResourceProperties::Health::PartitionsFilter,
                filterString);
            return error;
        }

        partitionsFilter = make_unique<PartitionHealthStatesFilter>(move(filter));
    }

    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::GetNodesFilter(
    __inout std::unique_ptr<ServiceModel::NodeHealthStatesFilter> & nodesFilter) const
{
    wstring filterString;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Health::NodesFilter, filterString))
    {
        NodeHealthStatesFilter filter;
        auto error = NodeHealthStatesFilter::FromString(filterString, filter);
        if (!error.IsSuccess())
        {
            HMEvents::Trace->QueryParseArg(
                this->TraceId,
                QueryResourceProperties::Health::NodesFilter,
                filterString);
            return error;
        }

        nodesFilter = make_unique<NodeHealthStatesFilter>(move(filter));
    }

    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::GetApplicationsFilter(
    __inout std::unique_ptr<ServiceModel::ApplicationHealthStatesFilter> & applicationsFilter) const
{
    wstring filterString;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Health::ApplicationsFilter, filterString))
    {
        ApplicationHealthStatesFilter filter;
        auto error = ApplicationHealthStatesFilter::FromString(filterString, filter);
        if (!error.IsSuccess())
        {
            HMEvents::Trace->QueryParseArg(
                this->TraceId,
                QueryResourceProperties::Health::ApplicationsFilter,
                filterString);
            return error;
        }

        applicationsFilter = make_unique<ApplicationHealthStatesFilter>(move(filter));
    }

    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::GetReplicasFilter(
    __inout std::unique_ptr<ServiceModel::ReplicaHealthStatesFilter> & replicasFilter) const
{
    wstring filterString;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Health::ReplicasFilter, filterString))
    {
        ReplicaHealthStatesFilter filter;
        auto error = ReplicaHealthStatesFilter::FromString(filterString, filter);
        if (!error.IsSuccess())
        {
            HMEvents::Trace->QueryParseArg(
                this->TraceId,
                QueryResourceProperties::Health::ReplicasFilter,
                filterString);
            return error;
        }

        replicasFilter = make_unique<ReplicaHealthStatesFilter>(move(filter));
    }

    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::GetNodesFilter(
    __inout ServiceModel::NodeHealthStateFilterListWrapper & nodesFilter) const
{
    wstring filterString;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Health::NodeFilters, filterString))
    {
        auto error = NodeHealthStateFilterListWrapper::FromJsonString(filterString, nodesFilter);
        if (!error.IsSuccess())
        {
            HMEvents::Trace->QueryParseArg(
                this->TraceId,
                QueryResourceProperties::Health::NodeFilters,
                filterString);
            return error;
        }
    }

    return ErrorCode::Success();
}

Common::ErrorCode QueryRequestContext::GetApplicationsFilter(
    __inout ServiceModel::ApplicationHealthStateFilterListWrapper & applicationsFilter) const
{
    wstring filterString;
    if (queryArgs_.TryGetValue(QueryResourceProperties::Health::ApplicationFilters, filterString))
    {
        auto error = ApplicationHealthStateFilterListWrapper::FromJsonString(filterString, applicationsFilter);
        if (!error.IsSuccess())
        {
            HMEvents::Trace->QueryParseArg(
                this->TraceId,
                QueryResourceProperties::Health::ApplicationFilters,
                filterString);
            return error;
        }
    }

    return ErrorCode::Success();
}

std::wstring QueryRequestContext::GetFilterDescription() const
{
    return wformatString(queryArgs_);
}

size_t QueryRequestContext::GetMaxAllowedSize()
{
    return static_cast<size_t>(
        static_cast<double>(ManagementConfig::GetConfig().MaxMessageSize) * ManagementConfig::GetConfig().MessageContentBufferRatio);
}

//
// Templated methods
//
template void QueryRequestContext::AddQueryListResults<NodeAggregatedHealthState>(ListPager<NodeAggregatedHealthState> && pager);
template void QueryRequestContext::AddQueryListResults<ApplicationAggregatedHealthState>(ListPager<ApplicationAggregatedHealthState> && pager);
template void QueryRequestContext::AddQueryListResults<ApplicationUnhealthyEvaluation>(ListPager<ApplicationUnhealthyEvaluation> && pager);
template void QueryRequestContext::AddQueryListResults<ServiceAggregatedHealthState>(ListPager<ServiceAggregatedHealthState> && pager);
template void QueryRequestContext::AddQueryListResults<PartitionAggregatedHealthState>(ListPager<PartitionAggregatedHealthState> && pager);
template void QueryRequestContext::AddQueryListResults<ReplicaAggregatedHealthState>(ListPager<ReplicaAggregatedHealthState> && pager);
template void QueryRequestContext::AddQueryListResults<DeployedServicePackageAggregatedHealthState>(ListPager<DeployedServicePackageAggregatedHealthState> && pager);
template void QueryRequestContext::AddQueryListResults<DeployedApplicationAggregatedHealthState>(ListPager<DeployedApplicationAggregatedHealthState> && pager);

template<class TAggregatedChild>
void QueryRequestContext::AddQueryListResults(ServiceModel::ListPager<TAggregatedChild> && pager)
{
    HMEvents::Trace->List_QueryCompleted(
        *this,
        static_cast<uint64>(pager.Entries.size()),
        continuationToken_);

    this->SetQueryResult(QueryResult(move(pager)));
}

void QueryRequestContext::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    // Do not print the continuation token and the max results, since they are not needed on all traces.
    // The query completion trace prints the incoming query arguments, which have the same information.
    w.Write(
        "{0}:{1}({2})",
        this->ActivityId,
        this->contextKind_,
        this->ContextId);
}

void QueryRequestContext::WriteToEtw(uint16 contextSequenceId) const
{
    HMCommonEvents::Trace->QueryRequestContextTrace(
        contextSequenceId,
        this->ActivityId,
        this->contextKind_,
        this->ContextId);
}

void QueryRequestContext::InitializeEmptyQueryResult()
{
    switch (childrenKind_)
    {
    case HealthEntityKind::Replica:
        this->SetQueryResult(QueryResult(vector<ReplicaAggregatedHealthState>()));
        break;
    case HealthEntityKind::Partition:
        this->SetQueryResult(QueryResult(vector<PartitionAggregatedHealthState>()));
        break;
    case HealthEntityKind::Service:
        this->SetQueryResult(QueryResult(vector<ServiceAggregatedHealthState>()));
        break;
    case HealthEntityKind::DeployedApplication:
        this->SetQueryResult(QueryResult(vector<DeployedApplicationAggregatedHealthState>()));
        break;
    case HealthEntityKind::DeployedServicePackage:
        this->SetQueryResult(QueryResult(vector<DeployedServicePackageAggregatedHealthState>()));
        break;
    case HealthEntityKind::Application:
        this->SetQueryResult(QueryResult(vector<ApplicationAggregatedHealthState>()));
        break;
    case HealthEntityKind::Node:
        this->SetQueryResult(QueryResult(vector<NodeAggregatedHealthState>()));
        break;
    default:
        break;
    }
}
