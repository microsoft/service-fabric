// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    class ITransactionalReplicatorFactory
        : public Common::FabricComponent
    {
        DENY_COPY(ITransactionalReplicatorFactory)

    public:

        ITransactionalReplicatorFactory();

        virtual HRESULT CreateReplicator(
            __in FABRIC_REPLICA_ID replicaId,
            __in Reliability::ReplicationComponent::IReplicatorFactory & replicatorFactory,
            __in IFabricStatefulServicePartition * partition,
            __in_opt FABRIC_REPLICATOR_SETTINGS const * replicatorSettings,
            __in_opt TRANSACTIONAL_REPLICATOR_SETTINGS const * transactionalReplicatorSettings,
            __in_opt KTLLOGGER_SHARED_LOG_SETTINGS const * ktlloggerSharedSettings,                                          
            __in IFabricCodePackageActivationContext & codePackage,
            __in BOOLEAN hasPersistedState,
            __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr && healthClient,
            __in IFabricStateProvider2Factory * factory,
            __in IFabricDataLossHandler * userDataLossHandler,
            __out IFabricPrimaryReplicator ** primaryReplicator,
            __out PHANDLE transactionalReplicatorHandle) = 0;

        virtual HRESULT CreateReplicator(
            __in FABRIC_REPLICA_ID replicaId,
            __in Reliability::ReplicationComponent::IReplicatorFactory & replicatorFactory,
            __in IFabricStatefulServicePartition * partition,
            __in_opt FABRIC_REPLICATOR_SETTINGS const * replicatorSettings,
            __in_opt TRANSACTIONAL_REPLICATOR_SETTINGS const * transactionalReplicatorSettings,
            __in_opt KTLLOGGER_SHARED_LOG_SETTINGS const * ktlloggerSharedSettings,                                          
            __in IFabricTransactionalReplicatorRuntimeConfigurations * runtimeConfigurations,
            __in BOOLEAN hasPersistedState,
            __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr && healthClient,
            __in IFabricStateProvider2Factory * factory,
            __in IFabricDataLossHandler * userDataLossHandler,
            __out IFabricPrimaryReplicator ** primaryReplicator,
            __out PHANDLE transactionalReplicatorHandle) = 0;

        virtual Common::ErrorCode Open(__in KtlSystem & ktlSystem, __in Data::Log::LogManager & logManager) = 0;

        virtual ~ITransactionalReplicatorFactory();
    };
}
