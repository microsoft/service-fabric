// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace TxnReplicator;
using namespace Data::Utilities;

Common::StringLiteral const TraceComponent("ComTransactionalReplicatorFactory");

ComTransactionalReplicatorFactory::ComTransactionalReplicatorFactory(TransactionalReplicatorFactoryConstructorParameters & parameters)
    : ITransactionalReplicatorFactory()
    , Common::RootedObject(*parameters.Root)
    , ktlSystem_()
{
}

ComTransactionalReplicatorFactory::ComTransactionalReplicatorFactory(Common::ComponentRoot const & root)
    : ITransactionalReplicatorFactory()
    , Common::RootedObject(root)
    , ktlSystem_()
{
}

ComTransactionalReplicatorFactory::~ComTransactionalReplicatorFactory()
{
}

ITransactionalReplicatorFactoryUPtr TxnReplicator::TransactionalReplicatorFactoryFactory(TransactionalReplicatorFactoryConstructorParameters & parameters)
{
    return std::make_unique<ComTransactionalReplicatorFactory>(parameters);
}

Common::ErrorCode ComTransactionalReplicatorFactory::Open(
    __in KtlSystem & ktlSystem,
    __in Data::Log::LogManager & logManager)
{
    Common::ErrorCode ret = Common::FabricComponent::Open();

    if (!ret.IsSuccess())
    {
        return ret;
    }

    ktlSystem_ = &ktlSystem;
    logManager_ = &logManager;

    ASSERT_IF(
        ktlSystem_ == nullptr,
        "KtlSystem cannot be nullptr");

    ASSERT_IF(
        logManager_ == nullptr,
        "LogManager cannot be nullptr");

   return ret;
}

Common::ErrorCode ComTransactionalReplicatorFactory::OnOpen()
{
    return Common::ErrorCode(Common::ErrorCodeValue::Success);
}

Common::ErrorCode ComTransactionalReplicatorFactory::OnClose()
{
    logManager_ = nullptr;

    return Common::ErrorCode(Common::ErrorCodeValue::Success);
}

void ComTransactionalReplicatorFactory::OnAbort()
{
    logManager_ = nullptr;

    return;
}

HRESULT ComTransactionalReplicatorFactory::CreateReplicator(
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
    __out PHANDLE transactionalReplicatorHandle)
{
    IRuntimeFolders::SPtr runtimeFolders = RuntimeFolders::Create(
        codePackage, 
        ktlSystem_->PagedAllocator());

    return CreateReplicatorPrivate(
        replicaId,
        replicatorFactory,
        partition,
        replicatorSettings,
        *runtimeFolders,
        hasPersistedState,
        move(healthClient),
        factory,
        userDataLossHandler,
        transactionalReplicatorSettings,
        ktlloggerSharedSettings,
        primaryReplicator,
        transactionalReplicatorHandle);
}

HRESULT ComTransactionalReplicatorFactory::CreateReplicator(
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
    __out PHANDLE transactionalReplicatorHandle)
{
    if (!runtimeConfigurations)
    {
        return STATUS_ACCESS_VIOLATION;
    }

    IRuntimeFolders::SPtr runtimeFolders = RuntimeFolders::Create(
        *runtimeConfigurations, 
        ktlSystem_->PagedAllocator());

    return CreateReplicatorPrivate(
        replicaId,
        replicatorFactory,
        partition,
        replicatorSettings,
        *runtimeFolders,
        hasPersistedState,
        move(healthClient),
        factory,
        userDataLossHandler,
        transactionalReplicatorSettings,
        ktlloggerSharedSettings,
        primaryReplicator,
        transactionalReplicatorHandle);
}

HRESULT ComTransactionalReplicatorFactory::CreateReplicatorPrivate(
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
    __out PHANDLE transactionalReplicatorHandle)
{
    if (!transactionalReplicatorHandle)
    {
        return STATUS_ACCESS_VIOLATION;
    }

    if (!primaryReplicator)
    {
        return STATUS_ACCESS_VIOLATION;
    }

    if (!factory)
    {
        return STATUS_ACCESS_VIOLATION;
    }

    // DataLossHandler passed in as a pointer, need to unreference it when creating TransactionalReplicator.
    // So fail if it is nullptr here.
    if (!userDataLossHandler)
    {
        return STATUS_ACCESS_VIOLATION;
    }

    if (State.Value != Common::FabricComponentState::Opened)
    {
        return SF_STATUS_OBJECT_CLOSED;
    }

    std::shared_ptr<TRInternalSettings> comTxnReplicatorFactoryInternalSettings;
    TransactionalReplicatorSettingsUPtr comTxnReplicatorFactoryInputSettings;
    
    if (transactionalReplicatorSettings != NULL)
    {
        // Verify input settings are valid
        auto error = TransactionalReplicatorSettings::FromPublicApi(
            *transactionalReplicatorSettings,
            comTxnReplicatorFactoryInputSettings);

        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }
    }

    comTxnReplicatorFactoryInternalSettings = TRInternalSettings::Create(
        move(comTxnReplicatorFactoryInputSettings),
        make_shared<TransactionalReplicatorConfig>());


    std::shared_ptr<SLInternalSettings> comTxnReplicatorFactorySLInternalSettings;
    KtlLoggerSharedLogSettingsUPtr comTxnReplicatorFactorySLInputSettings;
    if (ktlloggerSharedSettings != NULL)
    {
        // Verify input settings are valid
        auto error = KtlLoggerSharedLogSettings::FromPublicApi(
            *ktlloggerSharedSettings,
            comTxnReplicatorFactorySLInputSettings);

        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }
    }

    comTxnReplicatorFactorySLInternalSettings = SLInternalSettings::Create(
        move(comTxnReplicatorFactorySLInputSettings));

    ITransactionalReplicator::SPtr transactionalReplicator;

    NTSTATUS status = ComTransactionalReplicator::Create(
        replicaId,
        replicatorFactory,
        *partition,
        replicatorSettings,
        runtimeFolders,
        hasPersistedState,
        move(healthClient),
        *factory,
        move(comTxnReplicatorFactoryInternalSettings),
        move(comTxnReplicatorFactorySLInternalSettings),
        *logManager_,
        *userDataLossHandler,
        ktlSystem_->PagedAllocator(),
        primaryReplicator,
        transactionalReplicator);

    if (NT_SUCCESS(status))
    {
        *transactionalReplicatorHandle = transactionalReplicator.Detach();
    }

    return StatusConverter::ToHResult(status);
}
