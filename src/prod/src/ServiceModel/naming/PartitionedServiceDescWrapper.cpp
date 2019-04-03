// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Reliability;

StringLiteral const FabricClientSource("FabricClient");

PartitionedServiceDescWrapper::PartitionedServiceDescWrapper()
    : serviceKind_(FABRIC_SERVICE_KIND_INVALID) 
    , partitionDescription_()
    , instanceCount_(0)
    , targetReplicaSetSize_(0)
    , minReplicaSetSize_(0)
    , hasPersistedState_(false)
    , flags_(FABRIC_STATEFUL_SERVICE_SETTINGS_NONE)
    , isServiceGroup_(false)
    , defaultMoveCost_(FABRIC_MOVE_COST_LOW)
    , isDefaultMoveCostSpecified_(false)
    , servicePackageActivationMode_(ServicePackageActivationMode::SharedProcess)
    , scalingPolicies_()
{
}

PartitionedServiceDescWrapper::PartitionedServiceDescWrapper(
    FABRIC_SERVICE_KIND serviceKind,
    wstring const& applicationName,
    wstring const& serviceName,
    wstring const& serviceTypeName,
    ByteBuffer const& initializationData,
    PartitionSchemeDescription::Enum partScheme,
    uint partitionCount,
    __int64 lowKeyInt64,
    __int64 highKeyInt64,
    vector<wstring> const& partitionNames,
    LONG instanceCount,
    LONG targetReplicaSize,
    LONG minReplicaSize,
    bool hasPersistedState,
    wstring const& placementConstraints,
    vector<Reliability::ServiceCorrelationDescription> const& correlations,
    vector<Reliability::ServiceLoadMetricDescription> const& metrics,
    vector<ServicePlacementPolicyDescription> const& placementPolicies,
    FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS flags,
    DWORD replicaRestartWaitDurationSeconds,
    DWORD quorumLossWaitDurationSeconds,
    DWORD standByReplicaKeepDurationSeconds,
    FABRIC_MOVE_COST defaultMoveCost,
    ServicePackageActivationMode::Enum const servicePackageActivationMode,
    std::wstring const& serviceDnsName,
    std::vector<Reliability::ServiceScalingPolicyDescription> const & scalingPolicies)
    : serviceKind_(serviceKind)
    , applicationName_(applicationName)
    , serviceName_(serviceName)
    , serviceTypeName_(serviceTypeName)
    , initializationData_(initializationData)
    , partitionDescription_(partScheme, partitionCount, lowKeyInt64, highKeyInt64, partitionNames)
    , instanceCount_(instanceCount)
    , targetReplicaSetSize_(targetReplicaSize)
    , minReplicaSetSize_(minReplicaSize)
    , hasPersistedState_(hasPersistedState)
    , placementConstraints_(placementConstraints)
    , correlations_(correlations)
    , metrics_(metrics)
    , placementPolicies_(placementPolicies)
    , flags_(flags)
    , replicaRestartWaitDurationSeconds_(replicaRestartWaitDurationSeconds)
    , quorumLossWaitDurationSeconds_(quorumLossWaitDurationSeconds)
    , standByReplicaKeepDurationSeconds_(standByReplicaKeepDurationSeconds)
    , isServiceGroup_(false)      
    , defaultMoveCost_(defaultMoveCost)
    , isDefaultMoveCostSpecified_(true)
    , servicePackageActivationMode_(servicePackageActivationMode)
    , serviceDnsName_(serviceDnsName)
    , scalingPolicies_(scalingPolicies)
{
    StringUtility::ToLower(serviceDnsName_);
}

ErrorCode PartitionedServiceDescWrapper::FromPublicApi(
    __in FABRIC_SERVICE_DESCRIPTION const & serviceDescription)
{
    ServiceTypeIdentifier typeIdentifier;
    
    if (serviceDescription.Kind == FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL)
    {
        serviceKind_ = FABRIC_SERVICE_KIND_STATEFUL;
        auto stateful = reinterpret_cast<FABRIC_STATEFUL_SERVICE_DESCRIPTION*>(serviceDescription.Value);

        auto hr = StringUtility::LpcwstrToWstring(stateful->ServiceName, false /*acceptNull*/, serviceName_);
        if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

        hr = StringUtility::LpcwstrToWstring(stateful->ApplicationName, true /*acceptNull*/, applicationName_);
        if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

        hr = StringUtility::LpcwstrToWstring(stateful->ServiceTypeName, false /*acceptNull*/, serviceTypeName_);
        if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

        ErrorCode error = ServiceTypeIdentifier::FromString(serviceTypeName_, typeIdentifier);
        if (!error.IsSuccess())
        {
            Trace.WriteWarning(FabricClientSource, "Could not parse ServiceTypeName '{0}'", stateful->ServiceTypeName);
            return error;
        }

        if (stateful->CorrelationCount > 0 && stateful->Correlations == NULL) 
        { 
            return TraceNullArgumentAndGetErrorDetails(wformatString("{0} {1}", GET_CM_RC(Invalid_Null_Pointer), "FABRIC_STATEFUL_SERVICE_DESCRIPTION->Correlations"));
        }

        for(ULONG i = 0; i < stateful->CorrelationCount; i++)
        {
            wstring correlationServiceName;
            hr = StringUtility::LpcwstrToWstring(stateful->Correlations[i].ServiceName, true /*acceptNull*/, correlationServiceName);
            if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

            Reliability::ServiceCorrelationDescription correlationDescription(correlationServiceName, stateful->Correlations[i].Scheme);

            correlations_.push_back(correlationDescription);
        }

        hr = StringUtility::LpcwstrToWstring(stateful->PlacementConstraints, true /*acceptNull*/, placementConstraints_);
        if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

        if (stateful->MetricCount > 0 && stateful->Metrics == NULL) 
        { 
            return TraceNullArgumentAndGetErrorDetails(wformatString("{0} {1}", GET_CM_RC(Invalid_Null_Pointer), "FABRIC_STATEFUL_SERVICE_DESCRIPTION->Metrics"));
        }

        for(ULONG i = 0; i < stateful->MetricCount; i++)
        {
            wstring metricsName;
            hr = StringUtility::LpcwstrToWstring(stateful->Metrics[i].Name, true /*acceptNull*/, metricsName);
            if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }
            metrics_.push_back(Reliability::ServiceLoadMetricDescription(
                move(metricsName), 
                stateful->Metrics[i].Weight,
                stateful->Metrics[i].PrimaryDefaultLoad,
                stateful->Metrics[i].SecondaryDefaultLoad));
        }

        initializationData_ = std::vector<byte>(
            stateful->InitializationData,
            stateful->InitializationData + stateful->InitializationDataSize);
        targetReplicaSetSize_ = stateful->TargetReplicaSetSize;
        minReplicaSetSize_ = stateful->MinReplicaSetSize;
        hasPersistedState_ = stateful->HasPersistedState ? true : false;
        
        error = this->InitializePartitionDescription(stateful->PartitionScheme, stateful->PartitionSchemeDescription, typeIdentifier);
        if (!error.IsSuccess())
        {
            return error;
        }

        if (stateful->Reserved == NULL)
        {
            return ErrorCodeValue::Success;
        }

        auto statefulEx1 = reinterpret_cast<FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX1*>(stateful->Reserved);

        if (statefulEx1->PolicyList != NULL)
        {
            auto pList = reinterpret_cast<FABRIC_SERVICE_PLACEMENT_POLICY_LIST*>(statefulEx1->PolicyList);

            if (pList->PolicyCount > 0 && pList->Policies == NULL)
            {
                return TraceNullArgumentAndGetErrorDetails(wformatString("{0} {1}", GET_CM_RC(Invalid_Null_Pointer), "FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX1->PolicyList->Policies"));
            }

            for (ULONG i = 0; i < pList->PolicyCount; i++)
            {
                std::wstring domainName;
                FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION & policyDesc = pList->Policies[i];
                ServicePlacementPolicyHelper::PolicyDescriptionToDomainName(policyDesc, domainName);
                placementPolicies_.push_back(ServiceModel::ServicePlacementPolicyDescription(move(domainName), policyDesc.Type));
            }
        }

        if (statefulEx1->FailoverSettings != NULL)
        {
            auto failoverSettings = statefulEx1->FailoverSettings;

            if ((failoverSettings->Flags & FABRIC_STATEFUL_SERVICE_SETTINGS_REPLICA_RESTART_WAIT_DURATION) != 0)
            {
                flags_ = (FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS)(flags_ | FABRIC_STATEFUL_SERVICE_SETTINGS_REPLICA_RESTART_WAIT_DURATION);
                replicaRestartWaitDurationSeconds_ = failoverSettings->ReplicaRestartWaitDurationSeconds;
            }

            if ((failoverSettings->Flags & FABRIC_STATEFUL_SERVICE_SETTINGS_QUORUM_LOSS_WAIT_DURATION) != 0)
            {
                flags_ = (FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS)(flags_ | FABRIC_STATEFUL_SERVICE_SETTINGS_QUORUM_LOSS_WAIT_DURATION);
                quorumLossWaitDurationSeconds_ = failoverSettings->QuorumLossWaitDurationSeconds;
            }

            if (failoverSettings->Reserved != NULL)
            {
                auto failoverSettingsEx1 = reinterpret_cast<FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_EX1*>(failoverSettings->Reserved);
                if (failoverSettingsEx1 == NULL)
                {
                    return TraceNullArgumentAndGetErrorDetails(
                        wformatString("{0} {1}", GET_CM_RC(Invalid_Null_Pointer), "FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX1->FailoverSettings->FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_EX1"));
                }

                if ((failoverSettings->Flags & FABRIC_STATEFUL_SERVICE_SETTINGS_STANDBY_REPLICA_KEEP_DURATION) != 0)
                {
                    flags_ = (FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS)(flags_ | FABRIC_STATEFUL_SERVICE_SETTINGS_STANDBY_REPLICA_KEEP_DURATION);
                    standByReplicaKeepDurationSeconds_ = failoverSettingsEx1->StandByReplicaKeepDurationSeconds;
                }
            }
        }
        
        if (statefulEx1->Reserved == NULL)
        {
            isDefaultMoveCostSpecified_ = false;
            return ErrorCodeValue::Success;
        }

        auto statefulEx2 = reinterpret_cast<FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX2*>(statefulEx1->Reserved);
        isDefaultMoveCostSpecified_ = statefulEx2->IsDefaultMoveCostSpecified == TRUE ? true : false;
        defaultMoveCost_ = statefulEx2->DefaultMoveCost;

        if (statefulEx2->Reserved == NULL)
        {
            servicePackageActivationMode_ = ServicePackageActivationMode::SharedProcess;
            return ErrorCodeValue::Success;
        }

        auto statefulEx3 = reinterpret_cast<FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX3*>(statefulEx2->Reserved);

        auto err = ServicePackageActivationMode::FromPublicApi(statefulEx3->ServicePackageActivationMode, servicePackageActivationMode_);
        if (!err.IsSuccess())
        {
            return this->TraceAndGetErrorDetails(err.ReadValue(), L"Invalid value of FABRIC_ISOLATION_LEVEL provided.");;
        }

        hr = StringUtility::LpcwstrToWstring(statefulEx3->ServiceDnsName, true /*acceptNull*/, serviceDnsName_);
        if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

        StringUtility::ToLower(serviceDnsName_);

        if (statefulEx3->Reserved == NULL)
        {
            return ErrorCodeValue::Success;
        }

	auto statefulEx4 = reinterpret_cast<FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX4*>(statefulEx3->Reserved);

        if (statefulEx4->ScalingPolicyCount > 1)
        {
            // Currently, only one scaling policy is allowed per service.
            // Vector is there for future uses (when services could have multiple scaling policies).
            return TraceAndGetErrorDetails(ErrorCodeValue::InvalidServiceScalingPolicy, wformatString(GET_NS_RC(ScalingPolicy_Scaling_Count), statefulEx4->ScalingPolicyCount));
        }

        for (ULONG i = 0; i < statefulEx4->ScalingPolicyCount; i++)
        {
            Reliability::ServiceScalingPolicyDescription scalingDescription;
            auto scalingError = scalingDescription.FromPublicApi(statefulEx4->ServiceScalingPolicies[i]);
            if (!scalingError.IsSuccess())
            {
                return scalingError;
            }
            scalingPolicies_.push_back(move(scalingDescription));
        }
        
        return err;
    }
    else if (serviceDescription.Kind == FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS)
    {
        serviceKind_ = FABRIC_SERVICE_KIND_STATELESS;
        auto stateless = reinterpret_cast<FABRIC_STATELESS_SERVICE_DESCRIPTION*>(
            serviceDescription.Value);

        HRESULT hr = StringUtility::LpcwstrToWstring(stateless->ServiceName, false /*acceptNull*/, serviceName_);
        if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

        hr = StringUtility::LpcwstrToWstring(stateless->ApplicationName, true /*acceptNull*/, applicationName_);
        if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

        hr = StringUtility::LpcwstrToWstring(stateless->ServiceTypeName, false /*acceptNull*/, serviceTypeName_);
        if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

        ErrorCode error = ServiceTypeIdentifier::FromString(serviceTypeName_, typeIdentifier);
        if (!error.IsSuccess())
        {
            Trace.WriteWarning(FabricClientSource, "Could not parse ServiceTypeName '{0}'", stateless->ServiceTypeName);
            return error;
        }

        if (stateless->CorrelationCount > 0 && stateless->Correlations == NULL)
        {
            return TraceNullArgumentAndGetErrorDetails(
                wformatString("{0} {1}", GET_CM_RC(Invalid_Null_Pointer), "FABRIC_STATELESS_SERVICE_DESCRIPTION->Correlation"));
        }
        for (ULONG i = 0; i < stateless->CorrelationCount; i++)
        {
            wstring correlationServiceName;
            hr = StringUtility::LpcwstrToWstring(stateless->Correlations[i].ServiceName, true /*acceptNull*/, correlationServiceName);
            if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

            Reliability::ServiceCorrelationDescription correlationDescription(correlationServiceName, stateless->Correlations[i].Scheme);

            correlations_.push_back(correlationDescription);
        }

        hr = StringUtility::LpcwstrToWstring(stateless->PlacementConstraints, true /*acceptNull*/, placementConstraints_);
        if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

        if (stateless->MetricCount > 0 && stateless->Metrics == NULL)
        {
            return TraceNullArgumentAndGetErrorDetails(
                wformatString("{0} {1}", GET_CM_RC(Invalid_Null_Pointer), "FABRIC_STATEFUL_SERVICE_DESCRIPTION->Metrics"));
        }

        for (ULONG i = 0; i < stateless->MetricCount; i++)
        {
            wstring metricsName;
            hr = StringUtility::LpcwstrToWstring(stateless->Metrics[i].Name, true /*acceptNull*/, metricsName);
            if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }
            metrics_.push_back(Reliability::ServiceLoadMetricDescription(
                move(metricsName),
                stateless->Metrics[i].Weight,
                stateless->Metrics[i].PrimaryDefaultLoad,
                stateless->Metrics[i].SecondaryDefaultLoad));
        }

        initializationData_ = std::vector<byte>(
            stateless->InitializationData,
            stateless->InitializationData + stateless->InitializationDataSize);
        instanceCount_ = stateless->InstanceCount;

        error = this->InitializePartitionDescription(stateless->PartitionScheme, stateless->PartitionSchemeDescription, typeIdentifier);
        if (!error.IsSuccess())
        {
            return error;
        }

        if (stateless->Reserved == NULL)
        {
            return ErrorCodeValue::Success;
        }

        auto statelessEx1 = reinterpret_cast<FABRIC_STATELESS_SERVICE_DESCRIPTION_EX1*>(stateless->Reserved);

        if (statelessEx1->PolicyList != NULL)
        {
            auto pList = reinterpret_cast<FABRIC_SERVICE_PLACEMENT_POLICY_LIST*>(statelessEx1->PolicyList);

            if (pList->PolicyCount > 0 && pList->Policies == NULL)
            {
                return TraceNullArgumentAndGetErrorDetails(
                    wformatString("{0} {1}", GET_CM_RC(Invalid_Null_Pointer), "FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX1->PolicyList->Policies"));
            }

            for (ULONG i = 0; i < pList->PolicyCount; i++)
            {
                std::wstring domainName;
                FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION & policyDesc = pList->Policies[i];
                ServicePlacementPolicyHelper::PolicyDescriptionToDomainName(policyDesc, domainName);
                placementPolicies_.push_back(ServiceModel::ServicePlacementPolicyDescription(move(domainName), policyDesc.Type));
            }

        }

        if (statelessEx1->Reserved == NULL)
        {
            isDefaultMoveCostSpecified_ = false;
            return ErrorCodeValue::Success;
        }

        auto statelessEx2 = reinterpret_cast<FABRIC_STATELESS_SERVICE_DESCRIPTION_EX2*>(statelessEx1->Reserved);
        isDefaultMoveCostSpecified_ = statelessEx2->IsDefaultMoveCostSpecified == TRUE ? true : false;
        defaultMoveCost_ = statelessEx2->DefaultMoveCost;

        if (statelessEx2->Reserved == NULL)
        {
            servicePackageActivationMode_ = ServicePackageActivationMode::SharedProcess;
            return ErrorCodeValue::Success;
        }

        auto statelessEx3 = reinterpret_cast<FABRIC_STATELESS_SERVICE_DESCRIPTION_EX3*>(statelessEx2->Reserved);

        auto err = ServicePackageActivationMode::FromPublicApi(statelessEx3->ServicePackageActivationMode, servicePackageActivationMode_);
        if (!err.IsSuccess())
        {
            return this->TraceAndGetErrorDetails(err.ReadValue(), L"Invalid value of FABRIC_ISOLATION_LEVEL provided.");;
        }

        hr = StringUtility::LpcwstrToWstring(statelessEx3->ServiceDnsName, true /*acceptNull*/, serviceDnsName_);
        if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

        StringUtility::ToLower(serviceDnsName_);

        if (statelessEx3->Reserved == NULL)
        {
            return ErrorCodeValue::Success;
        }

        auto statelessEx4 = reinterpret_cast<FABRIC_STATELESS_SERVICE_DESCRIPTION_EX4*>(statelessEx3->Reserved);

        if (statelessEx4->ScalingPolicyCount > 1)
        {
            // Currently, only one scaling policy is allowed per service.
            // Vector is there for future uses (when services could have multiple scaling policies).
            return TraceAndGetErrorDetails(ErrorCodeValue::InvalidServiceScalingPolicy, wformatString(GET_NS_RC(ScalingPolicy_Scaling_Count), statelessEx4->ScalingPolicyCount));
        }

        for (ULONG i = 0; i < statelessEx4->ScalingPolicyCount; i++)
        {
            Reliability::ServiceScalingPolicyDescription scalingDescription;
            auto scalingError = scalingDescription.FromPublicApi(statelessEx4->ServiceScalingPolicies[i]);
            if (!scalingError.IsSuccess())
            {
                return scalingError;
            }
            scalingPolicies_.push_back(move(scalingDescription));
        }

        return err;
    }

    return ErrorCodeValue::InvalidArgument;
}

ErrorCode PartitionedServiceDescWrapper::InitializePartitionDescription(
    ::FABRIC_PARTITION_SCHEME partitionScheme, 
    void * partitionDescription,
    ServiceTypeIdentifier const & typeIdentifier)
{
    switch (partitionScheme)
    {
    case FABRIC_PARTITION_SCHEME_SINGLETON:
        partitionDescription_.Scheme = PartitionSchemeDescription::Singleton;
        partitionDescription_.PartitionCount = 1;
        break;

    case FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE:
    {
        if (partitionDescription == NULL)
        {
            Trace.WriteWarning(
                FabricClientSource,
                "Partition description cannot be NULL for service: type = {0} name = {1}",
                typeIdentifier,
                serviceName_);

            return TraceNullArgumentAndGetErrorDetails(
                wformatString("{0} {1}", GET_CM_RC(Invalid_Null_Pointer), "PartitionDescription"));
        }

        auto d = reinterpret_cast<FABRIC_UNIFORM_INT64_RANGE_PARTITION_SCHEME_DESCRIPTION*>(partitionDescription);
        partitionDescription_.Scheme = PartitionSchemeDescription::UniformInt64Range;
        partitionDescription_.PartitionCount = d->PartitionCount;
        partitionDescription_.LowKey = d->LowKey;
        partitionDescription_.HighKey = d->HighKey;
        break;
    }

    case FABRIC_PARTITION_SCHEME_NAMED:
    {
        if (partitionDescription == NULL)
        {
            Trace.WriteWarning(
                FabricClientSource,
                "Partition description cannot be NULL for service: type = {0} name = {1}",
                typeIdentifier,
                serviceName_);
        
            return TraceNullArgumentAndGetErrorDetails(
                wformatString("{0} {1}", GET_CM_RC(Invalid_Null_Pointer), "PartitionDescription"));
        }
        
        auto d = reinterpret_cast<FABRIC_NAMED_PARTITION_SCHEME_DESCRIPTION*>(partitionDescription);
        partitionDescription_.Scheme = PartitionSchemeDescription::Named;
        partitionDescription_.PartitionCount = d->PartitionCount;
        auto hr = StringUtility::FromLPCWSTRArray(d->PartitionCount, d->Names, partitionDescription_.PartitionNames);
        if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }
        break;
    }

    default:
        if (partitionDescription == NULL)
        {
            return TraceNullArgumentAndGetErrorDetails(
                wformatString("{0} {1}", GET_CM_RC(Invalid_Null_Pointer), "PartitionDescription"));
        }
        else
        {
            return ErrorCode(ErrorCodeValue::InvalidArgument);
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

void PartitionedServiceDescWrapper::ToPublicApi(__in ScopedHeap &heap, __in FABRIC_SERVICE_DESCRIPTION & serviceDescription) const
{
    ULONG const initDataSize = static_cast<const ULONG>(initializationData_.size());
    auto initData = heap.AddArray<BYTE>(initDataSize);

    for (size_t i = 0; i < initDataSize; i++)
    {
        initData[i] = initializationData_[i];
    }

    if (serviceKind_ == FABRIC_SERVICE_KIND_STATEFUL)
    {
        auto statefulDescription = heap.AddItem<FABRIC_STATEFUL_SERVICE_DESCRIPTION>();

        serviceDescription.Kind = FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
        serviceDescription.Value = statefulDescription.GetRawPointer();
        statefulDescription->ApplicationName = heap.AddString(applicationName_);
        statefulDescription->ServiceName = heap.AddString(serviceName_);
        statefulDescription->ServiceTypeName = heap.AddString(serviceTypeName_);
        statefulDescription->InitializationData = initData.GetRawArray();
        statefulDescription->InitializationDataSize = initDataSize;
        statefulDescription->TargetReplicaSetSize = targetReplicaSetSize_;
        statefulDescription->MinReplicaSetSize = minReplicaSetSize_;
        statefulDescription->PlacementConstraints = heap.AddString(placementConstraints_);


        size_t correlationCount = correlations_.size();
        statefulDescription->CorrelationCount = static_cast<ULONG>(correlationCount);
        if (correlationCount > 0)
        {
            auto correlations = heap.AddArray<FABRIC_SERVICE_CORRELATION_DESCRIPTION>(correlationCount);
            statefulDescription->Correlations = correlations.GetRawArray();
            for (size_t correlationIndex = 0; correlationIndex < correlationCount; ++correlationIndex)
            {
                correlations[correlationIndex].ServiceName = heap.AddString(correlations_[correlationIndex].ServiceName);
                correlations[correlationIndex].Scheme = correlations_[correlationIndex].Scheme;
            }
        }
        else
        {
            statefulDescription->Correlations = nullptr;
        }

        statefulDescription->HasPersistedState = hasPersistedState_;

        if (partitionDescription_.Scheme == PartitionSchemeDescription::UniformInt64Range)
        {
            statefulDescription->PartitionScheme = FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE;
            auto partitionDescription = heap.AddItem<FABRIC_UNIFORM_INT64_RANGE_PARTITION_SCHEME_DESCRIPTION>();
            partitionDescription->PartitionCount = partitionDescription_.PartitionCount;
            partitionDescription->LowKey = partitionDescription_.LowKey;
            partitionDescription->HighKey = partitionDescription_.HighKey;
            partitionDescription->Reserved = NULL;
            statefulDescription->PartitionSchemeDescription = partitionDescription.GetRawPointer();
        }
        else if (partitionDescription_.Scheme == PartitionSchemeDescription::Named)
        {
            auto partitionDescription = heap.AddItem<FABRIC_NAMED_PARTITION_SCHEME_DESCRIPTION>();
            statefulDescription->PartitionScheme = FABRIC_PARTITION_SCHEME_NAMED;
            partitionDescription->PartitionCount = partitionDescription_.PartitionCount;
            auto pNames = heap.AddArray<LPCWSTR>(partitionDescription_.PartitionCount);
            for (size_t i = 0; i < static_cast<size_t>(partitionDescription_.PartitionCount); ++i)
            {
                pNames[i] = heap.AddString(partitionDescription_.PartitionNames[i]);
            }

            partitionDescription->Names = pNames.GetRawArray();
            partitionDescription->Reserved = NULL;
            statefulDescription->PartitionSchemeDescription = partitionDescription.GetRawPointer();
        }
        else
        {
            statefulDescription->PartitionScheme = FABRIC_PARTITION_SCHEME_SINGLETON;
            statefulDescription->PartitionSchemeDescription = NULL;
        }

        size_t metricCount = metrics_.size();
        statefulDescription->MetricCount = static_cast<ULONG>(metricCount);
        if (metricCount > 0)
        {
            auto metrics = heap.AddArray<FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION>(metricCount);
            statefulDescription->Metrics = metrics.GetRawArray();
            for (size_t metricIndex = 0; metricIndex < metricCount; ++metricIndex)
            {
                metrics[metricIndex].Name = heap.AddString(metrics_[metricIndex].Name);
                metrics[metricIndex].Weight = metrics_[metricIndex].Weight;
                metrics[metricIndex].PrimaryDefaultLoad = metrics_[metricIndex].PrimaryDefaultLoad;
                metrics[metricIndex].SecondaryDefaultLoad = metrics_[metricIndex].SecondaryDefaultLoad;
            }
        }
        else
        {
            statefulDescription->Metrics = nullptr;
        }

        auto statefulDescriptionEx1 = heap.AddItem<FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX1>();
        statefulDescription->Reserved = statefulDescriptionEx1.GetRawPointer();

        // policy description
        size_t policyCount = placementPolicies_.size();
        if (policyCount > 0)
        {
            auto policyDescription = heap.AddItem<FABRIC_SERVICE_PLACEMENT_POLICY_LIST>();
            statefulDescriptionEx1->PolicyList = policyDescription.GetRawPointer();
            policyDescription->PolicyCount = static_cast<ULONG>(policyCount);

            auto policies = heap.AddArray<FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION>(policyCount);
            policyDescription->Policies = policies.GetRawArray();
            for (size_t policyIndex = 0; policyIndex < policyCount; ++policyIndex)
            {
                placementPolicies_[policyIndex].ToPublicApi(heap, policies[policyIndex]);
            }
        }
        else
        {
            statefulDescriptionEx1->PolicyList = nullptr;
        }

        auto failoverSettings = heap.AddItem<FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS>();
        auto failoverSettingsEx1 = heap.AddItem<FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_EX1>();
        failoverSettings->Reserved = failoverSettingsEx1.GetRawPointer();

        statefulDescriptionEx1->FailoverSettings = failoverSettings.GetRawPointer();

        failoverSettings->Flags |= FABRIC_STATEFUL_SERVICE_SETTINGS_REPLICA_RESTART_WAIT_DURATION;
        failoverSettings->ReplicaRestartWaitDurationSeconds = replicaRestartWaitDurationSeconds_;

        failoverSettings->Flags |= FABRIC_STATEFUL_SERVICE_SETTINGS_QUORUM_LOSS_WAIT_DURATION;
        failoverSettings->QuorumLossWaitDurationSeconds = quorumLossWaitDurationSeconds_;

        failoverSettings->Flags |= FABRIC_STATEFUL_SERVICE_SETTINGS_STANDBY_REPLICA_KEEP_DURATION;
        failoverSettingsEx1->StandByReplicaKeepDurationSeconds = standByReplicaKeepDurationSeconds_;

        auto statefulDescriptionEx2 = heap.AddItem<FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX2>();
        statefulDescriptionEx1->Reserved = statefulDescriptionEx2.GetRawPointer();
        statefulDescriptionEx2->IsDefaultMoveCostSpecified = isDefaultMoveCostSpecified_;
        statefulDescriptionEx2->DefaultMoveCost = defaultMoveCost_;

        auto statefulDescriptionEx3 = heap.AddItem<FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX3>();
        statefulDescriptionEx2->Reserved = statefulDescriptionEx3.GetRawPointer();
        statefulDescriptionEx3->ServicePackageActivationMode = ServicePackageActivationMode::ToPublicApi(servicePackageActivationMode_);
        statefulDescriptionEx3->ServiceDnsName = heap.AddString(serviceDnsName_);

	auto statefulDescriptionEx4 = heap.AddItem<FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX4>();
        statefulDescriptionEx3->Reserved = statefulDescriptionEx4.GetRawPointer();
        size_t scalingPolicyCount = scalingPolicies_.size();
        if (scalingPolicyCount > 0)
        {
            statefulDescriptionEx4->ScalingPolicyCount = static_cast<ULONG>(scalingPolicyCount);
            auto spArray = heap.AddArray<FABRIC_SERVICE_SCALING_POLICY>(scalingPolicyCount);
            statefulDescriptionEx4->ServiceScalingPolicies = spArray.GetRawArray();
            for (size_t spIndex = 0; spIndex < scalingPolicyCount; ++spIndex)
            {
                scalingPolicies_[spIndex].ToPublicApi(heap, spArray[spIndex]);
            }
        }
        else
        {
            statefulDescriptionEx4->ScalingPolicyCount = 0;
            statefulDescriptionEx4->ServiceScalingPolicies = nullptr;
	}
    }
    else
    {
        auto statelessDescription = heap.AddItem<FABRIC_STATELESS_SERVICE_DESCRIPTION>();

        serviceDescription.Kind = FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS;
        serviceDescription.Value = statelessDescription.GetRawPointer();
        statelessDescription->ApplicationName = heap.AddString(applicationName_);
        statelessDescription->ServiceName = heap.AddString(serviceName_);
        statelessDescription->ServiceTypeName = heap.AddString(serviceTypeName_);
        statelessDescription->InitializationData = initData.GetRawArray();
        statelessDescription->InitializationDataSize = initDataSize;
        statelessDescription->InstanceCount = instanceCount_;
        statelessDescription->PlacementConstraints = heap.AddString(placementConstraints_);
        statelessDescription->Reserved = NULL;

        size_t correlationCount = correlations_.size();
        statelessDescription->CorrelationCount = static_cast<ULONG>(correlationCount);
        if (correlationCount > 0)
        {
            auto correlations = heap.AddArray<FABRIC_SERVICE_CORRELATION_DESCRIPTION>(correlationCount);
            statelessDescription->Correlations = correlations.GetRawArray();
            for (size_t correlationIndex = 0; correlationIndex < correlationCount; ++correlationIndex)
            {
                correlations[correlationIndex].ServiceName = heap.AddString(correlations_[correlationIndex].ServiceName);
                correlations[correlationIndex].Scheme = correlations_[correlationIndex].Scheme;
            }
        }
        else
        {
            statelessDescription->Correlations = nullptr;
        }

        if (partitionDescription_.Scheme == PartitionSchemeDescription::UniformInt64Range)
        {
            statelessDescription->PartitionScheme = FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE;
            auto partitionDescription = heap.AddItem<FABRIC_UNIFORM_INT64_RANGE_PARTITION_SCHEME_DESCRIPTION>();
            partitionDescription->PartitionCount = partitionDescription_.PartitionCount;
            partitionDescription->LowKey = partitionDescription_.LowKey;
            partitionDescription->HighKey = partitionDescription_.HighKey;
            statelessDescription->PartitionSchemeDescription = partitionDescription.GetRawPointer();
        }
        else if (partitionDescription_.Scheme == PartitionSchemeDescription::Named)
        {
            statelessDescription->PartitionScheme = FABRIC_PARTITION_SCHEME_NAMED;
            auto partitionDescription = heap.AddItem<FABRIC_NAMED_PARTITION_SCHEME_DESCRIPTION>();
            partitionDescription->PartitionCount = partitionDescription_.PartitionCount;
            auto pNames = heap.AddArray<LPCWSTR>(partitionDescription_.PartitionCount);
            for (size_t i = 0; i < static_cast<size_t>(partitionDescription_.PartitionCount); ++i)
            {
                pNames[i] = heap.AddString(partitionDescription_.PartitionNames[i]);
            }

            partitionDescription->Names = pNames.GetRawArray();
            statelessDescription->PartitionSchemeDescription = partitionDescription.GetRawPointer();
        }
        else
        {
            statelessDescription->PartitionScheme = FABRIC_PARTITION_SCHEME_SINGLETON;
            statelessDescription->PartitionSchemeDescription = NULL;
        }

        size_t metricCount = metrics_.size();
        statelessDescription->MetricCount = static_cast<ULONG>(metricCount);
        if (metricCount > 0)
        {
            auto metrics = heap.AddArray<FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION>(metricCount);
            statelessDescription->Metrics = metrics.GetRawArray();
            for (size_t metricIndex = 0; metricIndex < metricCount; ++metricIndex)
            {
                metrics[metricIndex].Name = heap.AddString(metrics_[metricIndex].Name);
                metrics[metricIndex].Weight = metrics_[metricIndex].Weight;
                metrics[metricIndex].PrimaryDefaultLoad = metrics_[metricIndex].PrimaryDefaultLoad;
                metrics[metricIndex].SecondaryDefaultLoad = metrics_[metricIndex].SecondaryDefaultLoad;
            }
        }
        else
        {
            statelessDescription->Metrics = nullptr;
        }

        auto statelessDescriptionEx1 = heap.AddItem<FABRIC_STATELESS_SERVICE_DESCRIPTION_EX1>();
        statelessDescription->Reserved = statelessDescriptionEx1.GetRawPointer();

        // policy description
        size_t policyCount = placementPolicies_.size();
        if (policyCount > 0)
        {
            auto policyDescription = heap.AddItem<FABRIC_SERVICE_PLACEMENT_POLICY_LIST>();
            statelessDescriptionEx1->PolicyList = policyDescription.GetRawPointer();
            policyDescription->PolicyCount = static_cast<ULONG>(policyCount);

            auto policies = heap.AddArray<FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION>(policyCount);
            policyDescription->Policies = policies.GetRawArray();
            for (size_t policyIndex = 0; policyIndex < policyCount; ++policyIndex)
            {
                placementPolicies_[policyIndex].ToPublicApi(heap, policies[policyIndex]);
            }
        }
        else
        {
            statelessDescriptionEx1->PolicyList = nullptr;
        }

        auto statelessDescriptionEx2 = heap.AddItem<FABRIC_STATELESS_SERVICE_DESCRIPTION_EX2>();
        statelessDescriptionEx1->Reserved = statelessDescriptionEx2.GetRawPointer();
        statelessDescriptionEx2->IsDefaultMoveCostSpecified = isDefaultMoveCostSpecified_;
        statelessDescriptionEx2->DefaultMoveCost = defaultMoveCost_;

        auto statelessDescriptionEx3 = heap.AddItem<FABRIC_STATELESS_SERVICE_DESCRIPTION_EX3>();
        statelessDescriptionEx2->Reserved = statelessDescriptionEx3.GetRawPointer();
        statelessDescriptionEx3->ServicePackageActivationMode = ServicePackageActivationMode::ToPublicApi(servicePackageActivationMode_);
        statelessDescriptionEx3->ServiceDnsName = heap.AddString(serviceDnsName_);

	auto statelessDescriptionEx4 = heap.AddItem<FABRIC_STATELESS_SERVICE_DESCRIPTION_EX4>();
        statelessDescriptionEx3->Reserved = statelessDescriptionEx4.GetRawPointer();

        size_t scalingPolicyCount = scalingPolicies_.size();
        if (scalingPolicyCount > 0)
        {
            statelessDescriptionEx4->ScalingPolicyCount = static_cast<ULONG>(scalingPolicyCount);
            auto spArray = heap.AddArray<FABRIC_SERVICE_SCALING_POLICY> (scalingPolicyCount);
            statelessDescriptionEx4->ServiceScalingPolicies = spArray.GetRawArray();
            for (size_t spIndex = 0; spIndex < scalingPolicyCount; ++spIndex)
            {
                scalingPolicies_[spIndex].ToPublicApi(heap, spArray[spIndex]);
            }
        }
        else
        {
            statelessDescriptionEx4->ScalingPolicyCount = 0;
            statelessDescriptionEx4->ServiceScalingPolicies = nullptr;
	}
    }
}

ErrorCode PartitionedServiceDescWrapper::TraceAndGetErrorDetails(ErrorCodeValue::Enum errorCode, std::wstring && msg)
{
    Trace.WriteWarning(FabricClientSource, "{0}", msg);

    return ErrorCode(errorCode, move(msg));
}

ErrorCode PartitionedServiceDescWrapper::TraceNullArgumentAndGetErrorDetails(std::wstring && msg)
{
    return TraceAndGetErrorDetails(ErrorCode::FromHResult(E_POINTER).ReadValue(), move(msg));
}