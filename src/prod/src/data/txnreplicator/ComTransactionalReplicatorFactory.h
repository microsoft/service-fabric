// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    //
    // Entry point to the replicator from failover subsystem and user service
    //
    class ComTransactionalReplicatorFactory
        : public Common::RootedObject
        , public ITransactionalReplicatorFactory
    {
    public:

        ComTransactionalReplicatorFactory(Common::ComponentRoot const & root);
        ComTransactionalReplicatorFactory(TransactionalReplicatorFactoryConstructorParameters & parameters);
        virtual ~ComTransactionalReplicatorFactory();

        HRESULT CreateReplicator(
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
            __out PHANDLE transactionalReplicatorHandle) override;

        HRESULT CreateReplicator(
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
            __out PHANDLE transactionalReplicatorHandle) override;

        Common::ErrorCode Open(
            __in KtlSystem & ktlSystem,
            __in Data::Log::LogManager & logManager) override;
 
    protected:
        virtual Common::ErrorCode OnOpen();
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();

    private:

        HRESULT CreateReplicatorPrivate(
            __in FABRIC_REPLICA_ID replicaId,
            __in Reliability::ReplicationComponent::IReplicatorFactory & replicatorFactory,
            __in IFabricStatefulServicePartition * partition,
            __in_opt FABRIC_REPLICATOR_SETTINGS const * replicatorSettings,
            __in IRuntimeFolders & runtimeFolders,
            __in BOOLEAN hasPersistedState,
            __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr && healthClient,
            __in IFabricStateProvider2Factory * factory,
            __in IFabricDataLossHandler * userDataLossHandler,
            __in_opt TRANSACTIONAL_REPLICATOR_SETTINGS const * transactionalReplicatorSettings,
            __in_opt KTLLOGGER_SHARED_LOG_SETTINGS const * ktlloggerSharedSettings,                                          
            __out IFabricPrimaryReplicator ** primaryReplicator,
            __out PHANDLE transactionalReplicatorHandle);

        KtlSystem * ktlSystem_;
        Data::Log::LogManager::SPtr logManager_;
    };
}
