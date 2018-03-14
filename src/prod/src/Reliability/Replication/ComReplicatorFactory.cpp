// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Reliability::ReplicationComponent;
using namespace Common;
using namespace std;

IReplicatorFactoryUPtr Reliability::ReplicationComponent::ReplicatorFactoryFactory(ReplicatorFactoryConstructorParameters & parameters)
{
    return make_unique<ComReplicatorFactory>(parameters);
}

ComReplicatorFactory::ComReplicatorFactory(ReplicatorFactoryConstructorParameters & parameters)
    :   RootedObject(*parameters.Root),
        transports_(),
        lock_(),
        nodeId_()
{
}

ComReplicatorFactory::ComReplicatorFactory(ComponentRoot const & root)
    :   RootedObject(root),
        transports_(),
        lock_(),
        nodeId_()
{
}

ComReplicatorFactory::~ComReplicatorFactory()
{
}

HRESULT ComReplicatorFactory::CreateReplicatorConfigAndTransport(
    __in Common::Guid const & partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in BOOLEAN hasPersistedState,
    __in_opt FABRIC_REPLICATOR_SETTINGS const * replicatorSettings,
    __out std::shared_ptr<REInternalSettings> & internalConfig,
    __out ReplicationTransportSPtr & transport)
{
    // Check that the state is opened and return error if not.
    // This is just an optimization, 
    // because the state may change at any time after this method is closed.
    if (State.Value != FabricComponentState::Opened)
    {
        return ErrorCode(Common::ErrorCodeValue::ObjectClosed).ToHResult();
    }
    
    // Load user settings and merge them with the defaults provided by user in clustermanifest/code
    {
        ReplicatorSettingsUPtr inputSettings;

        if (replicatorSettings != NULL)
        {
            // First verify the input settings are valid if provided
            auto error = ReplicatorSettings::FromPublicApi(
                *replicatorSettings,
                inputSettings);

            if (!error.IsSuccess())
            {
                return error.ToHResult();
            }
        }

        internalConfig = REInternalSettings::Create(move(inputSettings), make_shared<REConfig>());
    }

    // Make sure only one thread creates the transport object
    {
        AcquireExclusiveLock lock(lock_);
        
        // Get the transport in the map of transports (if it exists)
        auto it = transports_.find(internalConfig->ReplicatorListenAddress);
        if (it != transports_.end())
        {
            transport = it->second;
        }

        Transport::SecuritySettings securitySettings;
        if ((replicatorSettings != nullptr) && ((replicatorSettings->Flags & FABRIC_REPLICATOR_SECURITY) != 0))
        {
            if (nullptr == replicatorSettings->SecurityCredentials)
            {
                ReplicatorEventSource::Events->ReplicatorFactoryGeneralConfigError(
                    partitionId,
                    replicaId,
                    L"Security Settings is NULL, but FABRIC_REPLICATOR_SECURITY flag is enabled");
                return E_INVALIDARG;
            }

            auto error = Transport::SecuritySettings::FromPublicApi(*(replicatorSettings->SecurityCredentials), securitySettings);
            if (!error.IsSuccess())
            {
                return error.ToHResult();
            }
        }

        // After combining the default config values and the overrides given by the user, validate that the final configuration is valid
        ReplicatorSettingsUPtr finalConfig;
        auto error = ReplicatorSettings::FromConfig(
            *internalConfig,
            internalConfig->ReplicatorAddress,
            internalConfig->ReplicatorListenAddress,
            internalConfig->ReplicatorPublishAddress,
            securitySettings,
            finalConfig);

        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        if (!internalConfig->RequireServiceAck &&
            !hasPersistedState &&
            internalConfig->UseStreamFaultsAndEndOfStreamOperationAck)
        {
            // For volatile services which do not require service ack, setting UseStreamFaultsAndendofstreamoperationack is invalid

            ReplicatorEventSource::Events->ReplicatorFactoryGeneralConfigError(
                partitionId,
                replicaId,
                L"Volatile Services cannot enable end of stream operation ack and stream faults without requireServiceAck being enabled");
            return E_INVALIDARG;
        }

        if (internalConfig->QueueHealthWarningAtUsagePercent < 1 ||
            internalConfig->QueueHealthWarningAtUsagePercent > 100)
        {
            ReplicatorEventSource::Events->ReplicatorFactoryGeneralConfigError(
                partitionId,
                replicaId,
                L"QueueHealthWarningAtUsagePercent cannot be < 1 or > 100");
            return E_INVALIDARG;
        }

        if (internalConfig->SlowIdleRestartAtQueueUsagePercent < 1 ||
            internalConfig->SlowIdleRestartAtQueueUsagePercent > 100)
        {
            ReplicatorEventSource::Events->ReplicatorFactoryGeneralConfigError(
                partitionId,
                replicaId,
                L"SlowIdleRestartAtQueueUsagePercent cannot be < 1 or > 100");
            return E_INVALIDARG;
        }

        if (internalConfig->SlowActiveSecondaryRestartAtQueueUsagePercent < 1 ||
            internalConfig->SlowActiveSecondaryRestartAtQueueUsagePercent > 100)
        {
            ReplicatorEventSource::Events->ReplicatorFactoryGeneralConfigError(
                partitionId,
                replicaId,
                L"SlowActiveSecondaryRestartAtQueueUsagePercent cannot be < 1 or > 100");
            return E_INVALIDARG;
        }

        if (internalConfig->SecondaryProgressRateDecayFactor < 0.0 ||
            internalConfig->SecondaryProgressRateDecayFactor > 1.0)
        {
            ReplicatorEventSource::Events->ReplicatorFactoryGeneralConfigError(
                partitionId,
                replicaId,
                L"SecondaryProgressRateDecayFactor cannot be < 0 or > 1");
            return E_INVALIDARG;
        }

        if (transport)
        {
            if (!transport->SecuritySettingsMatch(securitySettings))
            {
                ReplicatorEventSource::Events->ReplicatorFactorySecuritySettingsError(
                    partitionId,
                    replicaId,
                    Common::wformatString(securitySettings.SecurityProvider()));

                return E_INVALIDARG;
            }

            ReplicatorEventSource::Events->ReplicatorFactoryShareTransport(
                partitionId,
                replicaId,
                transport->Endpoint, 
                nodeId_);
        }
        else
        {
            transport = ReplicationTransport::Create(internalConfig->ReplicatorListenAddress, nodeId_);
            error = transport->Start(internalConfig->ReplicatorPublishAddress, securitySettings);

            if (!error.IsSuccess())
            {
                return error.ToHResult();
            }

            ReplicatorEventSource::Events->ReplicatorFactory(
                partitionId,
                replicaId,
                transport->Endpoint, 
                nodeId_);

            // Add the transport to map
            transports_.insert(TransportPair(
                internalConfig->ReplicatorListenAddress,
                transport));
        }

        // IMP- Do NOT change the order of the below 2 assignments as the queue size must be greater than message size before setting
        // these values on transport
        transport->MaxPendingUnsentBytes = 
            internalConfig->MaxPrimaryReplicationQueueMemorySize > internalConfig->MaxSecondaryReplicationQueueMemorySize ?
            static_cast<ULONG>(internalConfig->MaxPrimaryReplicationQueueMemorySize) : 
            static_cast<ULONG>(internalConfig->MaxSecondaryReplicationQueueMemorySize);

        transport->MaxMessageSize = static_cast<ULONG>(internalConfig->MaxReplicationMessageSize);
    }

    ASSERT_IFNOT(transport, "ComReplicatorFactory: transport shouldn't be null");
    ASSERT_IFNOT(internalConfig, "ComReplicatorFactory: internalconfig shouldn't be null");
    
    return S_OK;
}

HRESULT ComReplicatorFactory::CreateReplicator(
    FABRIC_REPLICA_ID replicaId,
    __in IFabricStatefulServicePartition * partition,
    __in IFabricStateProvider * stateProvider,
    __in_opt FABRIC_REPLICATOR_SETTINGS const * replicatorSettings,
    BOOLEAN hasPersistedState, 
    __in IReplicatorHealthClientSPtr && healthClient,
    __out IFabricStateReplicator **stateReplicator)
{
    if (replicatorSettings != NULL)
    {
        FABRIC_REPLICATOR_SETTINGS_EX1 * replicatorSettingsEx1 = (FABRIC_REPLICATOR_SETTINGS_EX1 *) replicatorSettings->Reserved;

        if (replicatorSettingsEx1 != NULL)
        {
            FABRIC_REPLICATOR_SETTINGS_EX2 * replicatorSettingsEx2 = (FABRIC_REPLICATOR_SETTINGS_EX2 *)replicatorSettingsEx1->Reserved;
            
            if (replicatorSettingsEx2 != NULL)
            {
                FABRIC_REPLICATOR_SETTINGS_EX3 * replicatorSettingsEx3 = (FABRIC_REPLICATOR_SETTINGS_EX3 *)replicatorSettingsEx2->Reserved;

                if (replicatorSettingsEx3 != NULL)
                {
                    FABRIC_REPLICATOR_SETTINGS_EX4 * replicatorSettingsEx4 = (FABRIC_REPLICATOR_SETTINGS_EX4 *)replicatorSettingsEx3->Reserved;
                    FAIL_IF_OPTIONAL_PARAM_RESERVED_FIELD_NOT_NULL(replicatorSettingsEx4);
                }
            }
        }
    }

    ReplicationTransportSPtr transport;
    std::shared_ptr<REInternalSettings> internalConfig;
    FABRIC_SERVICE_PARTITION_INFORMATION const *partitionInfo;
            
    HRESULT hr = partition->GetPartitionInfo(&partitionInfo);

    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    Common::Guid partitionId(Common::ServiceInfoUtility::GetPartitionId(partitionInfo));

    hr = CreateReplicatorConfigAndTransport(
        partitionId,
        replicaId, hasPersistedState,
        replicatorSettings, internalConfig, transport);

    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    return ComReplicator::CreateReplicator(
        replicaId,
        partitionId,
        partition, 
        stateProvider,
        hasPersistedState,
        move(internalConfig),
        move(transport),
        move(healthClient),
        stateReplicator);
}

HRESULT ComReplicatorFactory::CreateReplicatorV1Plus(
    FABRIC_REPLICA_ID replicaId,
    __in IFabricStatefulServicePartition * partition,
    __in IFabricStateProvider * stateProvider,
    __in_opt FABRIC_REPLICATOR_SETTINGS const * replicatorSettings,
    BOOLEAN hasPersistedState,
    BOOLEAN batchEnabled,
    __in IReplicatorHealthClientSPtr && healthClient,
    __out IFabricStateReplicator **stateReplicator)
{
    ReplicationTransportSPtr transport;
    std::shared_ptr<REInternalSettings> internalConfig;
    FABRIC_SERVICE_PARTITION_INFORMATION const *partitionInfo;
            
    HRESULT hr = partition->GetPartitionInfo(&partitionInfo);

    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    Common::Guid partitionId(Common::ServiceInfoUtility::GetPartitionId(partitionInfo));

    hr = CreateReplicatorConfigAndTransport(
        partitionId,
        replicaId, hasPersistedState,
        replicatorSettings, internalConfig, transport);

    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    return ComReplicator::CreateReplicatorV1Plus(
        replicaId,
        partitionId,
        partition, 
        stateProvider,
        hasPersistedState,
        move(internalConfig),
        batchEnabled,
        move(transport),
        move(healthClient),
        stateReplicator);
}

ErrorCode ComReplicatorFactory::Open(wstring const & nodeId)
{
    nodeId_ = nodeId;
    return FabricComponent::Open();
}
  
ErrorCode ComReplicatorFactory::OnOpen()
{
    return ErrorCode(Common::ErrorCodeValue::Success);
}

ErrorCode ComReplicatorFactory::OnClose()
{
    // Release the shared ptr;
    // when all replicators using the transport are closed
    // the transport destructor will first call Stop on the IDatagramTransport
    Common::AcquireExclusiveLock lock(lock_);

    for (auto item : transports_)
    {
        item.second->Stop();
    }

    transports_.clear();

    return ErrorCode(Common::ErrorCodeValue::Success);
}

void ComReplicatorFactory::OnAbort()
{
    Common::AcquireExclusiveLock lock(lock_);

    for (auto item : transports_)
    {
        item.second->Stop();
    }

    transports_.clear();
}

void ComReplicatorFactory::Test_GetFactoryDetails(__out std::vector<std::wstring> & replicatorAddresses)
{
    replicatorAddresses.clear();
    AcquireExclusiveLock lock(lock_);
    for(auto it = transports_.begin(); it != transports_.end(); ++it)
    {
        replicatorAddresses.push_back(it->first);
    }
}
