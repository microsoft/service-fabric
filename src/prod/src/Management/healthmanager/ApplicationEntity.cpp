// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace ServiceModel;
using namespace Store;
using namespace Query;
using namespace Management::HealthManager;

StringLiteral const TraceComponent("ApplicationEntity");
StringLiteral const TraceUpgrade("Upgrade");

ApplicationEntity::ApplicationEntity(
    AttributesStoreDataSPtr && attributes,
    __in HealthManagerReplica & healthManagerReplica,
    HealthEntityState::Enum entityState)
    : HealthEntity(
        move(attributes),
        healthManagerReplica,
        false /*considerSystemErrorUnhealthy*/,
        true /*expectSystemReports*/,
        entityState)
    , entityId_()
    , services_(healthManagerReplica.PartitionedReplicaId, this->EntityIdString)
    , deployedApplications_(healthManagerReplica.PartitionedReplicaId, this->EntityIdString)
    , expectSystemReports_(true)
    , isSystemApp_(false)
{
    auto & castedAttributes = GetCastedAttributes(this->InternalAttributes);
    entityId_ = castedAttributes.EntityId;

    isSystemApp_ = (this->EntityIdString == SystemServiceApplicationNameHelper::SystemServiceApplicationName);

    // do not expect system reports for ad-hoc services or system services
    // They do not have the app type name set.
    if (this->EntityIdString.empty() || isSystemApp_)
    {
        // Save to local variable so it doesn't need the lock to access the attributes
        expectSystemReports_ = false;
        this->InternalAttributes->ExpectSystemReports = false;

        // Set default app health policy, since the attributes are not reported on
        // TODO: system app policy should be read from config instead
        castedAttributes.SetAppHealthPolicy(ApplicationHealthPolicy());
    }
}

ApplicationEntity::~ApplicationEntity()
{
}

HEALTH_ENTITY_TEMPLATED_METHODS_DEFINITIONS( ApplicationAttributesStoreData, ApplicationEntity, ApplicationHealthId )

void ApplicationEntity::AddService(ServiceEntitySPtr const & service)
{
    services_.AddChild(service);
}

std::set<ServiceEntitySPtr> ApplicationEntity::GetServices()
{
    return services_.GetChildren();
}

void ApplicationEntity::AddDeployedApplication(
    DeployedApplicationEntitySPtr const & deployedApplication)
{
    deployedApplications_.AddChild(deployedApplication);
}

set<DeployedApplicationEntitySPtr> ApplicationEntity::GetDeployedApplications()
{
    return deployedApplications_.GetChildren();
}

Common::ErrorCode ApplicationEntity::GetApplicationHealthPolicy(
    __inout std::shared_ptr<ServiceModel::ApplicationHealthPolicy> & appHealthPolicy)
{
    auto attributes = this->GetAttributesCopy();
    if (attributes->IsCleanedUp || attributes->IsMarkedForDeletion || !attributes->HasSystemReport)
    {
        return ErrorCode(
            ErrorCodeValue::HealthEntityNotFound,
            wformatString("{0} {1}", Resources::GetResources().ApplicationNotFound, this->EntityIdString));
    }

    auto & castedAttributes = ApplicationEntity::GetCastedAttributes(attributes);
    auto error = castedAttributes.GetAppHealthPolicy(appHealthPolicy);
    if (!error.IsSuccess())
    {
        return ErrorCode(
            ErrorCodeValue::HealthEntityNotFound,
            wformatString("{0} {1}", Resources::GetResources().ApplicationPolicyNotSet, this->EntityIdString));
    }

    return error;
}

Common::ErrorCode ApplicationEntity::UpdateContextHealthPoliciesCallerHoldsLock(
    __in QueryRequestContext & context)
{
    if (!context.ApplicationPolicy)
    {
        auto & castedAttributes = GetCastedAttributes(this->InternalAttributes);
        std::shared_ptr<ServiceModel::ApplicationHealthPolicy> appHealthPolicy;
        auto error = castedAttributes.GetAppHealthPolicy(appHealthPolicy);
        if (!error.IsSuccess())
        {
            return ErrorCode(
                ErrorCodeValue::HealthEntityNotFound,
                wformatString("{0} {1}", Resources::GetResources().ApplicationPolicyNotSet, this->EntityIdString));
        }

        // Save the app health policy set in attributes into the context, to be used for processing
        context.ApplicationPolicy = move(appHealthPolicy);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

Common::ErrorCode ApplicationEntity::GetApplicationTypeName(__inout std::wstring & appTypeName) const
{
    if (this->IsAdHocApp() || isSystemApp_)
    {
        // Ad-hoc and System applications don't have app type set
        HealthManagerReplica::WriteInfo(TraceComponent, this->EntityIdString, "{0}: app type not set", this->PartitionedReplicaId.TraceId);
        return ErrorCode(ErrorCodeValue::ApplicationTypeNotFound);
    }

    auto appAttributes = this->GetAttributesCopy();
    if (appAttributes->IsMarkedForDeletion || appAttributes->IsCleanedUp)
    {
        return ErrorCode(ErrorCodeValue::HealthEntityDeleted);
    }

    if (!appAttributes->HasSystemReport)
    {
        return ErrorCode(ErrorCodeValue::HealthEntityNotFound);
    }

    auto & castedAttributes = GetCastedAttributes(appAttributes);
    if (!castedAttributes.AttributeSetFlags.IsApplicationTypeNameSet())
    {
        HealthManagerReplica::WriteInfo(TraceComponent, "{0}: {1}: app type not set", this->PartitionedReplicaId.TraceId, castedAttributes);
        return ErrorCode(ErrorCodeValue::ApplicationTypeNotFound);
    }

    appTypeName = castedAttributes.ApplicationTypeName;
    HealthManagerReplica::WriteNoise(TraceComponent, "{0}: {1}: app type {2}", this->PartitionedReplicaId.TraceId, this->EntityIdString, appTypeName);
    return ErrorCode::Success();
}

bool ApplicationEntity::NeedsApplicationTypeName() const
{
    if (this->IsAdHocApp() || isSystemApp_)
    {
        // Ad-hoc and System applications don't have app type set
        return false;
    }

    auto appAttributes = this->GetAttributesCopy();
    if (appAttributes->IsMarkedForDeletion || appAttributes->IsCleanedUp)
    {
        return false;
    }

    auto & castedAttributes = GetCastedAttributes(appAttributes);
    return !castedAttributes.AttributeSetFlags.IsApplicationTypeNameSet();
}

Common::ErrorCode ApplicationEntity::IsApplicationHealthy(
    Common::ActivityId const & activityId,
    ApplicationHealthPolicy const * policy,
    vector<wstring> const & upgradeDomains,
    __inout bool & healthy,
    __inout ServiceModel::HealthEvaluationList & unhealthyEvaluations)
{
    ErrorCode error(ErrorCodeValue::Success);
    healthy = false;
    shared_ptr<ApplicationHealthPolicy> appHealthPolicy;
    if (policy)
    {
        appHealthPolicy = make_shared<ApplicationHealthPolicy>(*policy);
    }

    FABRIC_HEALTH_STATE aggregatedHealthState;
    error = EvaluateHealth(activityId, appHealthPolicy, upgradeDomains, nullptr, aggregatedHealthState, unhealthyEvaluations);
    if (!error.IsSuccess())
    {
        return error;
    }

    if (aggregatedHealthState == FABRIC_HEALTH_STATE_ERROR)
    {
        TESTASSERT_IF(unhealthyEvaluations.empty(), "{0}:{1} app is not healthy but evaluation reasons are not set", this->PartitionedReplicaId.TraceId, activityId);
        healthy = false;

        size_t currentSize = 0;
        error = HealthEvaluation::GenerateDescriptionAndTrimIfNeeded(
            unhealthyEvaluations,
            ManagementConfig::GetConfig().MaxUnhealthyEvaluationsSizeBytes,
            currentSize);

        if (error.IsSuccess())
        {
            HMEvents::Trace->UpgradeAppUnhealthy(
                this->EntityIdString,
                activityId,
                *unhealthyEvaluations[0].Evaluation);
        }
        else
        {
            // Trace, but do not return error.
            // Upgrade can continue even if the unhealthy evaluations are empty.
            HealthManagerReplica::WriteWarning(
                TraceUpgrade,
                this->EntityIdString,
                "{0}: {1}: generating description for the unhealthy evaluations failed: {2}. ",
                activityId,
                aggregatedHealthState,
                error);
        }
    }
    else
    {
        HMEvents::Trace->AppHealthy(
            this->EntityIdString,
            activityId,
            wformatString(upgradeDomains),
            wformatString(aggregatedHealthState));

        // Consider OK and Warning as Healthy
        healthy = true;
        unhealthyEvaluations.clear();
    }

    return ErrorCode::Success();
}

Common::ErrorCode ApplicationEntity::EvaluateHealth(
    Common::ActivityId const & activityId,
    ServiceModel::ApplicationHealthPolicySPtr const & appHealthPolicy,
    vector<wstring> const & upgradeDomains,
    HealthStatisticsUPtr const & healthStats,
    __out FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout ServiceModel::HealthEvaluationList & unhealthyEvaluations,
    bool isDeployedChildrenPrecedent)
{
    aggregatedHealthState = FABRIC_HEALTH_STATE_UNKNOWN;

    ServiceModel::ApplicationHealthPolicySPtr healthPolicy = appHealthPolicy;
    if (!healthPolicy)
    {
        auto error = this->GetApplicationHealthPolicy(healthPolicy);
        if (!error.IsSuccess()) { return error; }
    }

    auto error = GetEventsHealthState(activityId, healthPolicy->ConsiderWarningAsError, aggregatedHealthState, unhealthyEvaluations);
    if (!error.IsSuccess()) { return error; }

    auto servicesFilter = make_unique<ServiceHealthStatesFilter>(FABRIC_HEALTH_STATE_FILTER_NONE);
    std::vector<ServiceAggregatedHealthState> serviceHealthStates;

    std::vector<DeployedApplicationAggregatedHealthState> deployedApplicationHealthStates;
    auto deployedApplicationsFilter = make_unique<DeployedApplicationHealthStatesFilter>(FABRIC_HEALTH_STATE_FILTER_NONE);

    if (isDeployedChildrenPrecedent)
    {
        EvaluateDeployedApplications(activityId, *healthPolicy, upgradeDomains, deployedApplicationsFilter, healthStats, aggregatedHealthState, unhealthyEvaluations, deployedApplicationHealthStates);
        EvaluateServices(activityId, *healthPolicy, servicesFilter, healthStats, aggregatedHealthState, unhealthyEvaluations, serviceHealthStates);
    }
    else
    {
        EvaluateServices(activityId, *healthPolicy, servicesFilter, healthStats, aggregatedHealthState, unhealthyEvaluations, serviceHealthStates);
        EvaluateDeployedApplications(activityId, *healthPolicy, upgradeDomains, deployedApplicationsFilter, healthStats, aggregatedHealthState, unhealthyEvaluations, deployedApplicationHealthStates);
    }
    return ErrorCode::Success();
}

Common::ErrorCode ApplicationEntity::GetServicesAggregatedHealthStates(
    __in QueryRequestContext & context)
{
    ASSERT_IFNOT(context.ApplicationPolicy, "{0}: health policy not set", context);

    std::set<ServiceEntitySPtr> unsortedServices = GetServices();

    wstring const & lastServiceName = context.ContinuationToken;
    bool checkContinuationToken = !lastServiceName.empty();

    // Sort the services by the service name
    map<wstring, ServiceEntitySPtr> services;
    for (auto it = unsortedServices.begin(); it != unsortedServices.end(); ++it)
    {
        if (!checkContinuationToken || (*it)->EntityId.ServiceName > lastServiceName)
        {
            if ((*it)->Match(context.Filters))
            {
                wstring serviceName = (*it)->EntityId.ServiceName;
                services.insert(make_pair(move(serviceName), move(*it)));
            }
        }
    }

    // For each service, compute the aggregated health using the application policy:
    // - ConsiderWarningAsError
    // - ServiceType specific health policy
    ListPager<ServiceAggregatedHealthState> pager;
    pager.SetMaxResults(context.MaxResults);
    for (auto it = services.begin(); it != services.end(); ++it)
    {
        auto & service = it->second;

        FABRIC_HEALTH_STATE serviceHealthState;
        vector<HealthEvaluation> unhealthyEvaluations;
        auto error = service->EvaluateHealth(context.ActivityId, *context.ApplicationPolicy, nullptr, /*out*/serviceHealthState, /*out*/unhealthyEvaluations);
        if (error.IsSuccess())
        {
            error = pager.TryAdd(ServiceAggregatedHealthState(move(it->first), serviceHealthState));

            bool benignError;
            bool hasError;
            CheckListPagerErrorAndTrace(error, this->EntityIdString, context.ActivityId, service->EntityIdString, hasError, benignError);
            if (hasError && benignError)
            {
                break;
            }
            else if (hasError && !benignError)
            {
                return error;
            }
        }
        else
        {
            HealthManagerReplica::WriteNoise(
                TraceComponent,
                "{0}: {1}: Service {2} EvaluateHealth returned {3}. Skip child",
                this->PartitionedReplicaId.TraceId,
                this->EntityIdString,
                it->second->EntityIdString,
                error);
        }
    }

    context.AddQueryListResults<ServiceAggregatedHealthState>(move(pager));
    return ErrorCode::Success();
}

Common::ErrorCode ApplicationEntity::EvaluateFilteredHealth(
    Common::ActivityId const & activityId,
    ServiceModel::ApplicationHealthPolicySPtr const & appHealthPolicy,
    std::vector<std::wstring> const & upgradeDomains,
    ServiceModel::ServiceHealthStateFilterList const & serviceFilters,
    ServiceModel::DeployedApplicationHealthStateFilterList const & deployedApplicationFilters,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout ServiceModel::ServiceHealthStateChunkList & services,
    __inout ServiceModel::DeployedApplicationHealthStateChunkList & deployedApplications)
{
    aggregatedHealthState = FABRIC_HEALTH_STATE_UNKNOWN;
    ErrorCode error(ErrorCodeValue::Success);

    ServiceModel::ApplicationHealthPolicySPtr healthPolicy = appHealthPolicy;
    if (!healthPolicy)
    {
        error = this->GetApplicationHealthPolicy(healthPolicy);
        if (!error.IsSuccess()) { return error; }
    }

    error = GetEventsHealthState(activityId, healthPolicy->ConsiderWarningAsError, aggregatedHealthState);
    if (!error.IsSuccess()) { return error; }

    EvaluateServiceChunks(activityId, *healthPolicy, serviceFilters, aggregatedHealthState, services);

    EvaluateDeployedApplicationChunks(activityId, *healthPolicy, upgradeDomains, deployedApplicationFilters, aggregatedHealthState, deployedApplications);

    return error;
}

void ApplicationEntity::EvaluateServiceChunks(
    Common::ActivityId const & activityId,
    ServiceModel::ApplicationHealthPolicy const & healthPolicy,
    ServiceModel::ServiceHealthStateFilterList const & serviceFilters,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout ServiceModel::ServiceHealthStateChunkList & childrenHealthStates)
{
    map<wstring, GroupHealthStateCount> servicesPerServiceType;

    bool canImpactAggregatedHealth = (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR);

    std::set<ServiceEntitySPtr> services = GetServices();
    ULONG totalCount = 0;
    for (auto it = services.begin(); it != services.end(); ++it)
    {
        ErrorCode error(ErrorCodeValue::Success);
        auto & service = *it;

        auto serviceAttributes = service->GetAttributesCopy();
        auto & castedServiceAttributes = ServiceEntity::GetCastedAttributes(serviceAttributes);
        if (!castedServiceAttributes.AttributeSetFlags.IsServiceTypeNameSet())
        {
            HealthManagerReplica::WriteNoise(
                TraceComponent,
                "{0}: {1}: Service {2} doesn't have service type name set. Skip child",
                this->PartitionedReplicaId.TraceId,
                this->EntityIdString,
                service->EntityIdString);
            continue;
        }

        // Find the filter that applies to this service
        auto itServiceFilter = serviceFilters.cend();
        for (auto itFilter = serviceFilters.cbegin(); itFilter != serviceFilters.cend(); ++itFilter)
        {
            if (itFilter->ServiceNameFilter == service->EntityId.ServiceName)
            {
                // Found the most specific filter, use it
                itServiceFilter = itFilter;
                break;
            }

            if (itFilter->ServiceNameFilter.empty())
            {
                itServiceFilter = itFilter;
            }
        }

        FABRIC_HEALTH_STATE healthState;
        PartitionHealthStateChunkList partitions;
        if (itServiceFilter != serviceFilters.cend())
        {
            error = service->EvaluateFilteredHealth(activityId, healthPolicy, itServiceFilter->PartitionFilters, /*out*/healthState, /*out*/partitions);
        }
        else
        {
            PartitionHealthStateFilterList partitionFilters;
            error = service->EvaluateFilteredHealth(activityId, healthPolicy, partitionFilters, /*out*/healthState, /*out*/partitions);
        }

        if (error.IsSuccess())
        {
            if (itServiceFilter != serviceFilters.cend() && itServiceFilter->IsRespected(healthState))
            {
                ++totalCount;
                childrenHealthStates.Add(ServiceHealthStateChunk(service->EntityId.ServiceName, healthState, move(partitions)));
            }

            if (canImpactAggregatedHealth)
            {
                auto itServicePerServiceType = servicesPerServiceType.find(castedServiceAttributes.ServiceTypeName);
                if (itServicePerServiceType == servicesPerServiceType.end())
                {
                    auto maxUnhealthy = healthPolicy.GetServiceTypeHealthPolicy(castedServiceAttributes.ServiceTypeName).MaxPercentUnhealthyServices;
                    itServicePerServiceType = (servicesPerServiceType.insert(pair<wstring, GroupHealthStateCount>(
                        castedServiceAttributes.ServiceTypeName,
                        GroupHealthStateCount(maxUnhealthy)))).first;
                }

                itServicePerServiceType->second.Add(healthState, nullptr);
            }
        }
        else
        {
            HealthManagerReplica::WriteNoise(
                TraceComponent,
                "{0}: {1}: Service {2} GetAggregatedHealth returned {3}. Skip child",
                this->PartitionedReplicaId.TraceId,
                this->EntityIdString,
                (*it)->EntityIdString,
                error);
        }
    }

    childrenHealthStates.TotalCount = totalCount;

    // If not error, evaluate services health per service type - if any service type is unhealthy, consider error
    if (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR)
    {
        for (auto it = servicesPerServiceType.begin(); it != servicesPerServiceType.end(); ++it)
        {
            auto serviceTypeAggregatedHealthState = it->second.GetHealthState();
            HMEvents::Trace->ServiceType_ChildrenState(
                EntityIdString,
                this->PartitionedReplicaId.TraceId,
                activityId,
                it->first,
                HealthEntityKind::Service,
                it->second.Count,
                wformatString(serviceTypeAggregatedHealthState),
                it->second.MaxPercentUnhealthy);

            if (aggregatedHealthState < serviceTypeAggregatedHealthState)
            {
                aggregatedHealthState = serviceTypeAggregatedHealthState;
            }
        }
    }
}

void ApplicationEntity::EvaluateServices(
    Common::ActivityId const & activityId,
    ServiceModel::ApplicationHealthPolicy const & healthPolicy,
    std::unique_ptr<ServiceModel::ServiceHealthStatesFilter> const & servicesFilter,
    HealthStatisticsUPtr const & healthStats,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout ServiceModel::HealthEvaluationList & unhealthyEvaluations,
    __inout std::vector<ServiceModel::ServiceAggregatedHealthState> & childrenHealthStates)
{
    map<wstring, GroupHealthStateCount> servicesPerServiceType;

    bool canImpactAggregatedHealth = (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR);
    std::vector<HealthEvaluation> childUnhealthyEvaluationsList;

    HealthCount totalServiceCount;
    std::set<ServiceEntitySPtr> services = GetServices();
    for (auto it = services.begin(); it != services.end(); ++it)
    {
        auto & service = *it;

        auto serviceAttributes = service->GetAttributesCopy();
        auto & castedServiceAttributes = ServiceEntity::GetCastedAttributes(serviceAttributes);
        if (!castedServiceAttributes.AttributeSetFlags.IsServiceTypeNameSet())
        {
            HealthManagerReplica::WriteNoise(
                TraceComponent,
                "{0}: {1}: Service {2} doesn't have service type name set. Skip child",
                this->PartitionedReplicaId.TraceId,
                this->EntityIdString,
                service->EntityIdString);
            continue;
        }

        FABRIC_HEALTH_STATE healthState;
        std::vector<HealthEvaluation> childUnhealthyEvaluations;
        auto error = service->EvaluateHealth(activityId, healthPolicy, healthStats, /*out*/healthState, /*out*/childUnhealthyEvaluations);
        if (error.IsSuccess())
        {
            totalServiceCount.AddResult(healthState);

            if (!servicesFilter || servicesFilter->IsRespected(healthState))
            {
                childrenHealthStates.push_back(ServiceAggregatedHealthState(service->EntityId.ServiceName, healthState));
            }

            if (canImpactAggregatedHealth)
            {
                auto itServicePerServiceType = servicesPerServiceType.find(castedServiceAttributes.ServiceTypeName);
                if (itServicePerServiceType == servicesPerServiceType.end())
                {
                    auto maxUnhealthy = healthPolicy.GetServiceTypeHealthPolicy(castedServiceAttributes.ServiceTypeName).MaxPercentUnhealthyServices;
                    itServicePerServiceType = (servicesPerServiceType.insert(pair<wstring, GroupHealthStateCount>(
                        castedServiceAttributes.ServiceTypeName,
                        GroupHealthStateCount(maxUnhealthy)))).first;
                }

                HealthEvaluationBaseSPtr childEvaluation;
                if (!childUnhealthyEvaluations.empty())
                {
                    childEvaluation = make_shared<ServiceHealthEvaluation>(
                        service->EntityId.ServiceName,
                        healthState,
                        move(childUnhealthyEvaluations));

                    childUnhealthyEvaluationsList.push_back(HealthEvaluation(childEvaluation));
                }

                itServicePerServiceType->second.Add(healthState, move(childEvaluation));
            }
        }
        else
        {
            HealthManagerReplica::WriteNoise(
                TraceComponent,
                "{0}: {1}: Service {2} GetAggregatedHealth returned {3}. Skip child",
                this->PartitionedReplicaId.TraceId,
                this->EntityIdString,
                (*it)->EntityIdString,
                error);
        }
    }

    // If not error, evaluate services health per service type - if any service type is unhealthy, consider error
    if (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR)
    {
        map<wstring, GroupHealthStateCount>::const_iterator itServiceType = servicesPerServiceType.end();

        for (auto it = servicesPerServiceType.begin(); it != servicesPerServiceType.end(); ++it)
        {
            auto serviceTypeAggregatedHealthState = it->second.GetHealthState();
            HMEvents::Trace->ServiceType_ChildrenState(
                EntityIdString,
                this->PartitionedReplicaId.TraceId,
                activityId,
                it->first,
                HealthEntityKind::Service,
                it->second.Count,
                wformatString(serviceTypeAggregatedHealthState),
                it->second.MaxPercentUnhealthy);

            if (aggregatedHealthState < serviceTypeAggregatedHealthState)
            {
                aggregatedHealthState = serviceTypeAggregatedHealthState;
                itServiceType = it;
            }
        }

        if (itServiceType != servicesPerServiceType.end())
        {
            auto const & serviceTypeHealthInfo = itServiceType->second;
            unhealthyEvaluations.clear();
            unhealthyEvaluations.push_back(
                HealthEvaluation(make_shared<ServicesHealthEvaluation>(
                aggregatedHealthState,
                itServiceType->first,
                serviceTypeHealthInfo.GetUnhealthy(),
                itServiceType->second.Count.TotalCount,
                serviceTypeHealthInfo.MaxPercentUnhealthy)));
        }
    }

    if (healthStats)
    {
        healthStats->Add(EntityKind::Service, totalServiceCount.GetHealthStateCount());

        if (totalServiceCount.TotalCount == 0)
        {
            // Initialize children of the 1st degree children, becasue there are no 1st degree children that can initialize them
            healthStats->Add(EntityKind::Replica);
            healthStats->Add(EntityKind::Partition);
        }
    }
}

Common::ErrorCode ApplicationEntity::GetDeployedApplicationsAggregatedHealthStates(
    __in QueryRequestContext & context)
{
    ASSERT_IFNOT(context.ApplicationPolicy, "{0}: health policy not set", context);

    std::set<DeployedApplicationEntitySPtr> unsortedDeployedApplications = GetDeployedApplications();

    wstring const & lastNodeName = context.ContinuationToken;
    bool checkContinuationToken = !lastNodeName.empty();

    // Sort the deployedApplications by the node name
    map<wstring, DeployedApplicationEntitySPtr> deployedApplications;
    for (auto it = unsortedDeployedApplications.begin(); it != unsortedDeployedApplications.end(); ++it)
    {
        auto attributes = (*it)->GetAttributesCopy();
        auto & castedAttributes = DeployedApplicationEntity::GetCastedAttributes(attributes);
        wstring nodeName = castedAttributes.NodeName;
        if (!checkContinuationToken || nodeName > lastNodeName)
        {
            deployedApplications.insert(make_pair(move(nodeName), move(*it)));
        }
    }

    ListPager<DeployedApplicationAggregatedHealthState> pager;

    // If the maxResults is not valid (<= 0), then list pager will simply ignore the set command
    pager.SetMaxResults(context.MaxResults);

    for (auto & deployedApp : deployedApplications)
    {
        auto & deployedApplication = deployedApp.second;

        if (!deployedApplication->Match(context.Filters))
        {
            continue;
        }

        FABRIC_HEALTH_STATE deployedApplicationHealthState;
        vector<HealthEvaluation> unhealthyEvaluations;
        auto error = deployedApplication->EvaluateHealth(context.ActivityId, *context.ApplicationPolicy, nullptr, /*out*/deployedApplicationHealthState, unhealthyEvaluations);
        if (error.IsSuccess())
        {
            error = pager.TryAdd(
                DeployedApplicationAggregatedHealthState(
                    deployedApplication->EntityId.ApplicationName,
                    deployedApp.first,
                    deployedApplicationHealthState));
            bool benignError;
            bool hasError;
            CheckListPagerErrorAndTrace(error, this->EntityIdString, context.ActivityId, deployedApplication->EntityIdString, hasError, benignError);
            if (hasError && benignError)
            {
                break;
            }
            else if (hasError && !benignError)
            {
                return error;
            }
        }
        else
        {
            HMEvents::Trace->InternalMethodFailed(
                this->EntityIdString,
                context.ActivityId,
                L"EvaluateHealth",
                error,
                error.Message);
        }
    }

    context.AddQueryListResults<DeployedApplicationAggregatedHealthState>(move(pager));
    return ErrorCode::Success();
}

void ApplicationEntity::EvaluateDeployedApplicationChunks(
    Common::ActivityId const & activityId,
    ServiceModel::ApplicationHealthPolicy const & healthPolicy,
    std::vector<std::wstring> const & upgradeDomains,
    ServiceModel::DeployedApplicationHealthStateFilterList const & deployedApplicationFilters,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout ServiceModel::DeployedApplicationHealthStateChunkList & childrenHealthStates)
{
    std::map<wstring, GroupHealthStateCount> deployedApplicationsPerUd;

    for (wstring const & domain : upgradeDomains)
    {
        deployedApplicationsPerUd.insert(make_pair(domain, GroupHealthStateCount(healthPolicy.MaxPercentUnhealthyDeployedApplications)));
    }

    bool canImpactAggregatedHealth = (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR);
    HealthCount deployedApplicationHealthCount;

    std::set<DeployedApplicationEntitySPtr> deployedApplications = GetDeployedApplications();
    ULONG totalCount = 0;
    for (auto it = deployedApplications.begin(); it != deployedApplications.end(); ++it)
    {
        ErrorCode error(ErrorCodeValue::Success);
        auto & deployedApplication = *it;
        FABRIC_HEALTH_STATE healthState;

        auto attribute = deployedApplication->GetAttributesCopy();
        auto & castedAttributes = DeployedApplicationEntity::GetCastedAttributes(attribute);

        // Find the filter that applies to this deployedApplication
        auto itDeployedApplicationFilter = deployedApplicationFilters.cend();
        for (auto itFilter = deployedApplicationFilters.cbegin(); itFilter != deployedApplicationFilters.cend(); ++itFilter)
        {
            if (castedAttributes.AttributeSetFlags.IsNodeNameSet() && itFilter->NodeNameFilter == castedAttributes.NodeName)
            {
                // Found the most specific filter, use it
                itDeployedApplicationFilter = itFilter;
                break;
            }

            if (itFilter->NodeNameFilter.empty())
            {
                // Default filter
                itDeployedApplicationFilter = itFilter;
            }
        }

        DeployedServicePackageHealthStateChunkList deployedServicePackages;

        if (itDeployedApplicationFilter != deployedApplicationFilters.cend())
        {
            error = deployedApplication->EvaluateFilteredHealth(activityId, healthPolicy, itDeployedApplicationFilter->DeployedServicePackageFilters, /*out*/healthState, /*out*/deployedServicePackages);
        }
        else
        {
            DeployedServicePackageHealthStateFilterList deployedServicePackageFilters;
            error = deployedApplication->EvaluateFilteredHealth(activityId, healthPolicy, deployedServicePackageFilters, /*out*/healthState, /*out*/deployedServicePackages);
        }

        if (error.IsSuccess())
        {
            if (itDeployedApplicationFilter != deployedApplicationFilters.cend() && itDeployedApplicationFilter->IsRespected(healthState))
            {
                ++totalCount;
                childrenHealthStates.Add(DeployedApplicationHealthStateChunk(
                    castedAttributes.NodeName,
                    healthState,
                    move(deployedServicePackages)));
            }

            if (canImpactAggregatedHealth)
            {
                deployedApplicationHealthCount.AddResult(healthState);

                // Keep track of per-UD deployed applications, if needed
                if (!upgradeDomains.empty())
                {
                    wstring upgradeDomain;
                    error = deployedApplication->GetUpgradeDomain(activityId, upgradeDomain);
                    if (error.IsSuccess())
                    {
                        auto itDeployedAppPerUd = deployedApplicationsPerUd.find(upgradeDomain);
                        if (itDeployedAppPerUd != deployedApplicationsPerUd.end())
                        {
                            itDeployedAppPerUd->second.Add(healthState, nullptr);
                        }
                    }
                }
            }
        }
    }

    childrenHealthStates.TotalCount = totalCount;

    UpdateHealthState(
        activityId,
        HealthEntityKind::DeployedApplication,
        deployedApplicationHealthCount,
        healthPolicy.MaxPercentUnhealthyDeployedApplications,
        aggregatedHealthState);

    // If not error, evaluate deployed application health per UD
    if (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR)
    {
        // Compute health per UD
        for (auto it = deployedApplicationsPerUd.begin(); it != deployedApplicationsPerUd.end(); ++it)
        {
            auto udHealthState = it->second.GetHealthState();
            if (udHealthState == FABRIC_HEALTH_STATE_ERROR)
            {
                HMEvents::Trace->UnhealthyChildrenPerUd(
                    this->EntityIdString,
                    this->PartitionedReplicaId.TraceId,
                    activityId,
                    HealthEntityKind::DeployedApplication,
                    it->first,
                    it->second.Count,
                    wformatString(udHealthState),
                    healthPolicy.MaxPercentUnhealthyDeployedApplications);

                aggregatedHealthState = FABRIC_HEALTH_STATE_ERROR;
                break;
            }
            else if (it->second.Count.TotalCount > 0)
            {
                HMEvents::Trace->ChildrenPerUd(
                    this->EntityIdString,
                    this->PartitionedReplicaId.TraceId,
                    activityId,
                    HealthEntityKind::DeployedApplication,
                    it->first,
                    it->second.Count,
                    wformatString(udHealthState),
                    healthPolicy.MaxPercentUnhealthyDeployedApplications);
            }
        }
    }
}

void ApplicationEntity::EvaluateDeployedApplications(
    Common::ActivityId const & activityId,
    ServiceModel::ApplicationHealthPolicy const & healthPolicy,
    std::vector<std::wstring> const & upgradeDomains,
    std::unique_ptr<ServiceModel::DeployedApplicationHealthStatesFilter> const & deployedApplicationsFilter,
    HealthStatisticsUPtr const & healthStats,
    __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
    __inout ServiceModel::HealthEvaluationList & unhealthyEvaluations,
    __inout std::vector<ServiceModel::DeployedApplicationAggregatedHealthState> & childrenHealthStates)
{
    std::map<wstring, GroupHealthStateCount> deployedApplicationsPerUd;

    for (wstring const & domain : upgradeDomains)
    {
        deployedApplicationsPerUd.insert(make_pair(domain, GroupHealthStateCount(healthPolicy.MaxPercentUnhealthyDeployedApplications)));
    }

    bool canImpactAggregatedHealth = (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR);
    std::vector<HealthEvaluation> childUnhealthyEvaluationsList;
    HealthCount totalDeployedApplicationCount;
    HealthCount deployedApplicationHealthCount;

    std::set<DeployedApplicationEntitySPtr> deployedApplications = GetDeployedApplications();
    for (auto it = deployedApplications.begin(); it != deployedApplications.end(); ++it)
    {
        auto & deployedApplication = *it;
        FABRIC_HEALTH_STATE healthState;
        std::vector<HealthEvaluation> childUnhealthyEvaluations;
        auto error = deployedApplication->EvaluateHealth(activityId, healthPolicy, healthStats, /*out*/healthState, childUnhealthyEvaluations);
        if (error.IsSuccess())
        {
            totalDeployedApplicationCount.AddResult(healthState);

            auto attribute = deployedApplication->GetAttributesCopy();
            auto & castedAttributes = DeployedApplicationEntity::GetCastedAttributes(attribute);

            if (!deployedApplicationsFilter || deployedApplicationsFilter->IsRespected(healthState))
            {
                childrenHealthStates.push_back(DeployedApplicationAggregatedHealthState(
                    deployedApplication->EntityId.ApplicationName,
                    castedAttributes.NodeName,
                    healthState));
            }

            if (canImpactAggregatedHealth)
            {
                deployedApplicationHealthCount.AddResult(healthState);

                HealthEvaluationBaseSPtr childEvaluation;
                if (!childUnhealthyEvaluations.empty())
                {
                    childEvaluation = make_shared<DeployedApplicationHealthEvaluation>(
                        deployedApplication->EntityId.ApplicationName,
                        castedAttributes.NodeName,
                        healthState,
                        move(childUnhealthyEvaluations));

                    childUnhealthyEvaluationsList.push_back(HealthEvaluation(childEvaluation));
                }

                // Keep track of per-UD deployed applications, if needed
                if (!upgradeDomains.empty())
                {
                    wstring upgradeDomain;
                    error = deployedApplication->GetUpgradeDomain(activityId, upgradeDomain);
                    if (error.IsSuccess())
                    {
                        auto itDeployedAppPerUd = deployedApplicationsPerUd.find(upgradeDomain);
                        if (itDeployedAppPerUd != deployedApplicationsPerUd.end())
                        {
                            itDeployedAppPerUd->second.Add(healthState, move(childEvaluation));
                        }
                    }
                }
            }
        }
    }

    if (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR)
    {
        map<wstring, GroupHealthStateCount>::const_iterator itDeployedApplicationsUd = deployedApplicationsPerUd.end();
        FABRIC_HEALTH_EVALUATION_KIND evaluationKind = FABRIC_HEALTH_EVALUATION_KIND_INVALID;

        // Deployed applications global health evaluation
        auto deployedApplicationAggregatedHealthState = deployedApplicationHealthCount.GetHealthState(healthPolicy.MaxPercentUnhealthyDeployedApplications);
        HMEvents::Trace->ChildrenState(
            this->EntityIdString,
            this->PartitionedReplicaId.TraceId,
            activityId,
            HealthEntityKind::DeployedApplication,
            deployedApplicationHealthCount,
            wformatString(deployedApplicationAggregatedHealthState),
            healthPolicy.MaxPercentUnhealthyDeployedApplications);

        if (aggregatedHealthState < deployedApplicationAggregatedHealthState)
        {
            aggregatedHealthState = deployedApplicationAggregatedHealthState;
            evaluationKind = FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATIONS;
        }

        // If not error, evaluate deployed application health per UD
        if (aggregatedHealthState != FABRIC_HEALTH_STATE_ERROR)
        {
            // Compute health per UD
            for (auto it = deployedApplicationsPerUd.begin(); it != deployedApplicationsPerUd.end(); ++it)
            {
                auto udHealthState = it->second.GetHealthState();
                if (udHealthState == FABRIC_HEALTH_STATE_ERROR)
                {
                    HMEvents::Trace->UnhealthyChildrenPerUd(
                        this->EntityIdString,
                        this->PartitionedReplicaId.TraceId,
                        activityId,
                        HealthEntityKind::DeployedApplication,
                        it->first,
                        it->second.Count,
                        wformatString(udHealthState),
                        healthPolicy.MaxPercentUnhealthyDeployedApplications);

                    aggregatedHealthState = FABRIC_HEALTH_STATE_ERROR;
                    evaluationKind = FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS;
                    itDeployedApplicationsUd = it;
                    break;
                }
                else if (it->second.Count.TotalCount > 0)
                {
                    HMEvents::Trace->ChildrenPerUd(
                        this->EntityIdString,
                        this->PartitionedReplicaId.TraceId,
                        activityId,
                        HealthEntityKind::DeployedApplication,
                        it->first,
                        it->second.Count,
                        wformatString(udHealthState),
                        healthPolicy.MaxPercentUnhealthyDeployedApplications);
                }
            }
        }

        if (evaluationKind == FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATIONS)
        {
            unhealthyEvaluations.clear();
            unhealthyEvaluations.push_back(
                HealthEvaluation(make_shared<DeployedApplicationsHealthEvaluation>(
                aggregatedHealthState,
                HealthCount::FilterUnhealthy(childUnhealthyEvaluationsList, aggregatedHealthState),
                deployedApplicationHealthCount.TotalCount,
                healthPolicy.MaxPercentUnhealthyDeployedApplications)));
        }
        else if (evaluationKind == FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS)
        {
            auto const & perUdHealthInfo = itDeployedApplicationsUd->second;
            unhealthyEvaluations.clear();
            unhealthyEvaluations.push_back(
                HealthEvaluation(make_shared<UDDeployedApplicationsHealthEvaluation>(
                aggregatedHealthState,
                itDeployedApplicationsUd->first /*UD*/,
                perUdHealthInfo.GetUnhealthy(),
                perUdHealthInfo.Count.TotalCount,
                perUdHealthInfo.MaxPercentUnhealthy)));
        }
    }

    if (healthStats)
    {
        healthStats->Add(EntityKind::DeployedApplication, totalDeployedApplicationCount.GetHealthStateCount());
        if (totalDeployedApplicationCount.TotalCount == 0)
        {
            // No children of 1st degree, no grandchildren
            healthStats->Add(EntityKind::DeployedServicePackage);
        }
    }
}

bool ApplicationEntity::CleanupChildren()
{
    return (services_.CleanupChildren() && deployedApplications_.CleanupChildren());
}

bool ApplicationEntity::ShouldBeDeletedDueToChildrenState()
{
    if (expectSystemReports_)
    {
        return false;
    }

    // Check first that the children caches are loaded completely
    // Otherwise, after fail-over we will get that there are no children, when in fact the children are not yet loaded.
    if (!this->EntityManager->Services.IsReady() ||
        !this->EntityManager->DeployedApplications.IsReady())
    {
        return false;
    }

    return CleanupChildren();
}

Common::ErrorCode ApplicationEntity::SetDetailQueryResult(
    __in QueryRequestContext & context,
    FABRIC_HEALTH_STATE entityEventsState,
    std::vector<ServiceModel::HealthEvent> && queryEvents,
    ServiceModel::HealthEvaluationList && unhealthyEvaluations)
{
    ASSERT_IFNOT(context.ApplicationPolicy, "{0}: application health policy not set", context);
    ASSERT_IF(context.ContextKind != RequestContextKind::QueryEntityDetail, "{0}: context kind not supported", context);

    ErrorCode error(ErrorCodeValue::Success);

    std::unique_ptr<ServiceHealthStatesFilter> servicesFilter;
    error = context.GetServicesFilter(servicesFilter);
    if (!error.IsSuccess())
    {
        return error;
    }

    std::unique_ptr<DeployedApplicationHealthStatesFilter> deployedApplicationsFilter;
    error = context.GetDeployedApplicationsFilter(deployedApplicationsFilter);
    if (!error.IsSuccess())
    {
        return error;
    }

    ApplicationHealthStatisticsFilterUPtr healthStatsFilter;
    error = context.GetApplicationHealthStatsFilter(healthStatsFilter);
    if (!error.IsSuccess())
    {
        return error;
    }

    HealthStatisticsUPtr healthStats;
    if (!healthStatsFilter || !healthStatsFilter->ExcludeHealthStatistics)
    {
        // Include healthStats
        healthStats = make_unique<HealthStatistics>();
    }

    std::vector<ServiceAggregatedHealthState> serviceHealthStates;
    std::vector<DeployedApplicationAggregatedHealthState> deployedApplicationHealthStates;
    auto aggregatedHealthState = entityEventsState;
    std::vector<std::wstring> upgradeDomains;

    // First, populate the list of services and deployed applications
    EvaluateServices(context.ActivityId, *context.ApplicationPolicy, servicesFilter, healthStats, aggregatedHealthState, unhealthyEvaluations, serviceHealthStates);

    EvaluateDeployedApplications(context.ActivityId, *context.ApplicationPolicy, upgradeDomains, deployedApplicationsFilter, healthStats, aggregatedHealthState, unhealthyEvaluations, deployedApplicationHealthStates);

    auto health = make_unique<ApplicationHealth>(
        this->EntityId.ApplicationName,
        move(queryEvents),
        aggregatedHealthState,
        move(unhealthyEvaluations),
        move(serviceHealthStates),
        move(deployedApplicationHealthStates),
        move(healthStats));

    error = health->UpdateUnhealthyEvalautionsPerMaxAllowedSize(QueryRequestContext::GetMaxAllowedSize());
    if (!error.IsSuccess())
    {
        return error;
    }

    HMEvents::Trace->Entity_QueryCompleted(
        EntityIdString,
        context,
        *health,
        context.GetFilterDescription());

    context.SetQueryResult(QueryResult(std::move(health)));
    return ErrorCode::Success();
}

Common::ErrorCode ApplicationEntity::SetChildrenAggregatedHealthQueryResult(
    __in QueryRequestContext & context)
{
    ASSERT_IFNOT(context.ApplicationPolicy, "{0}: application health policy not set", context);

    switch (context.ChildrenKind)
    {
    case HealthEntityKind::Service:
        return GetServicesAggregatedHealthStates(context);
    case HealthEntityKind::DeployedApplication:
        return GetDeployedApplicationsAggregatedHealthStates(context);

    default:
        Assert::CodingError(
            "{0}: {1}: SetChildrenAggregatedHealthQueryResult: can't return children of type {2}",
            this->PartitionedReplicaId.TraceId,
            this->EntityIdString,
            context.ChildrenKind);
    }
}
