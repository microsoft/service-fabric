// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace Data::StateManager;

NTSTATUS Metadata::Create(
    __in KUri const & name,
    __in KString const & typeString,
    __in TxnReplicator::IStateProvider2 & stateProvider,
    __in_opt Utilities::OperationData const * const initializationParameters,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in FABRIC_STATE_PROVIDER_ID parentId,
    __in MetadataMode::Enum metadataMode,
    __in bool transientCreate,
    __in KAllocator& allocator,
    __out SPtr & result) noexcept
{
    return Create(
        name, 
        typeString, 
        stateProvider, 
        initializationParameters, 
        stateProviderId, 
        parentId, 
        FABRIC_INVALID_SEQUENCE_NUMBER, 
        FABRIC_INVALID_SEQUENCE_NUMBER, 
        metadataMode, 
        transientCreate, 
        allocator, 
        result);
}

NTSTATUS Metadata::Create(
    __in KUri const & name,
    __in KString const & typeString,
    __in TxnReplicator::IStateProvider2 & stateProvider,
    __in_opt Utilities::OperationData const * const initializationParameters,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in FABRIC_STATE_PROVIDER_ID parentId,
    __in FABRIC_SEQUENCE_NUMBER createLSN,
    __in FABRIC_SEQUENCE_NUMBER deleteLSN,
    __in MetadataMode::Enum metadataMode,
    __in bool transientCreate,
    __in KAllocator& allocator, 
    __out SPtr & result) noexcept
{
    // Argument validation
    ASSERT_IFNOT(
        stateProviderId != Constants::EmptyStateProviderId,
        "Metadata: Create: StateProviderId should not be the same as StateManagerId. SPName: {0}, SPId: {1}",
        Utilities::ToStringLiteral(name),
        stateProviderId);

    result = _new(METADATA_TAG, allocator) Metadata(
        name, 
        typeString, 
        stateProvider, 
        initializationParameters, 
        stateProviderId, 
        parentId, 
        createLSN, 
        deleteLSN,
        metadataMode, 
        transientCreate);

    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        return (SPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

NTSTATUS Metadata::Create(
    __in KUri const & name,
    __in KString const & typeString,
    __in TxnReplicator::IStateProvider2 & stateProvider,
    __in_opt Utilities::OperationData const * const initializationParameters,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in FABRIC_STATE_PROVIDER_ID parentId,
    __in FABRIC_SEQUENCE_NUMBER createLSN,
    __in FABRIC_SEQUENCE_NUMBER deleteLSN,
    __in MetadataMode::Enum metadataMode,
    __in bool transientCreate,
    __in KAllocator& allocator,
    __out CSPtr & result) noexcept
{
    // Argument validation
    ASSERT_IFNOT(stateProviderId != Constants::EmptyStateProviderId, "Empty state provider id during metadata create");

    result = _new(METADATA_TAG, allocator) Metadata(
        name, 
        typeString, 
        stateProvider, 
        initializationParameters, 
        stateProviderId, 
        parentId, 
        createLSN, 
        deleteLSN, 
        metadataMode, 
        transientCreate);

    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        return (CSPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

NTSTATUS Metadata::Create(
    __in SerializableMetadata const & metadata,
    __in TxnReplicator::IStateProvider2 & stateProvider,
    __in bool transientCreate,
    __in KAllocator & allocator,
    __out SPtr & result) noexcept
{
    return Create(
        *metadata.Name,
        *metadata.TypeString,
        stateProvider,
        metadata.InitializationParameters.RawPtr(),
        metadata.StateProviderId,
        metadata.ParentStateProviderId,
        metadata.CreateLSN,
        metadata.DeleteLSN,
        metadata.MetadataMode,
        transientCreate,
        allocator,
        result);
}

KUri::CSPtr Metadata::get_Name() const
{
    return name_;
}

KString::CSPtr Metadata::get_TypeString() const
{
    return typeString_;
}

TxnReplicator::IStateProvider2::SPtr Metadata::get_StateProvider() const
{
    return stateProvider_;
}

Data::Utilities::OperationData::CSPtr Metadata::get_InitializationParameters() const
{
    return initializationParameters_;
}

FABRIC_STATE_PROVIDER_ID Metadata::get_StateProviderId() const
{
    return stateProviderId_;
}

LONG64 Metadata::get_TransactionId() const
{
    return transactionId_;
}

void Metadata::put_TransactionId(__in LONG64 value)
{
    transactionId_ = value;
}

FABRIC_STATE_PROVIDER_ID Metadata::get_ParentId() const
{
    return parentId_;
}

FABRIC_SEQUENCE_NUMBER Metadata::get_CreateLSN() const
{
    return createLSN_;
}

void Metadata::put_CreateLSN(__in FABRIC_SEQUENCE_NUMBER value)
{
    createLSN_ = value;
}

FABRIC_SEQUENCE_NUMBER Metadata::get_DeleteLSN() const
{
    return deleteLSN_;
}

void Metadata::put_DeleteLSN(__in FABRIC_SEQUENCE_NUMBER deleteLSN)
{
    deleteLSN_ = deleteLSN;
}

MetadataMode::Enum  Metadata::get_MetadataMode() const
{
    return metadataMode_;
}

void Metadata::put_MetadataMode(__in MetadataMode::Enum metadataMode)
{
    metadataMode_ = metadataMode;
}

bool Metadata::get_TransientCreate() const
{
    return transientCreate_;
}

void Metadata::put_TransientCreate(__in bool value)
{
    transientCreate_ = value;
}

bool Metadata::get_TransientDelete() const
{
    return transientDelete_;
}

void Metadata::put_TransientDelete(__in bool value)
{
    transientDelete_ = value;
}

Metadata::Metadata(
    __in KUri const & name,
    __in KString const & typeString,
    __in TxnReplicator::IStateProvider2 & stateProvider,
    __in_opt Utilities::OperationData const * const initializationParameters,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in FABRIC_STATE_PROVIDER_ID parentId,
    __in FABRIC_SEQUENCE_NUMBER createLSN,
    __in FABRIC_SEQUENCE_NUMBER deleteLSN,
    __in MetadataMode::Enum metadataMode,
    __in bool transientCreate) noexcept
: KObject()
, KShared()
, name_(&name)
, typeString_(&typeString)
, stateProvider_(&stateProvider)
, initializationParameters_(initializationParameters)
, stateProviderId_(stateProviderId)
, parentId_(parentId)
, metadataMode_(metadataMode)
, createLSN_(createLSN)
, deleteLSN_(deleteLSN)
, transientCreate_(transientCreate)
, transientDelete_(false)
{
}

Metadata::~Metadata()
{
}
