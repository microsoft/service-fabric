// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace Data::StateManager;

NTSTATUS SerializableMetadata::Create(
    __in KUri const & name,
    __in KString const & typeString,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in FABRIC_STATE_PROVIDER_ID parentStateProviderId,
    __in FABRIC_SEQUENCE_NUMBER createLSN,
    __in FABRIC_SEQUENCE_NUMBER deleteLSN,
    __in MetadataMode::Enum metadataMode,
    __in KAllocator & allocator,
    __in_opt Utilities::OperationData const * const initializationParameters,
    __out CSPtr & result) noexcept
{
    // EmptyStateProviderId is state manager id, thus state provider id can't be EmptyStateProviderId
    ASSERT_IFNOT(stateProviderId != Constants::EmptyStateProviderId, "Empty state provider id specified");

    result = _new(SERIALIZABLEMETADATA_TAG, allocator) SerializableMetadata(name, typeString, stateProviderId, parentStateProviderId, createLSN, deleteLSN, metadataMode, initializationParameters);

    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        // null Result while fetching failure status with no extra AddRefs or Releases; should opt very well
        return (CSPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

NTSTATUS SerializableMetadata::Create(
    __in Metadata const & metadata, 
    __in KAllocator & allocator,
    __out CSPtr & result) noexcept
{
    return Create(
        *metadata.Name,
        *metadata.TypeString,
        metadata.StateProviderId,
        metadata.ParentId,
        metadata.CreateLSN,
        metadata.DeleteLSN,
        metadata.MetadataMode,
        allocator,
        metadata.InitializationParameters.RawPtr(),
        result);
}

// Property
KUri::CSPtr SerializableMetadata::get_Name() const { return name_; }

KString::CSPtr SerializableMetadata::get_TypeString() const { return typeString_; }

Utilities::OperationData::CSPtr SerializableMetadata::get_InitializationParameters() const { return initializationParameters_; }

FABRIC_STATE_PROVIDER_ID SerializableMetadata::get_StateProviderId() const { return stateProviderId_; }

FABRIC_STATE_PROVIDER_ID SerializableMetadata::get_ParentStateProviderId() const { return parentStateProviderId_; }

FABRIC_SEQUENCE_NUMBER SerializableMetadata::get_CreateLSN() const { return createLSN_; }

FABRIC_SEQUENCE_NUMBER SerializableMetadata::get_DeleteLSN() const { return deleteLSN_; }

MetadataMode::Enum SerializableMetadata::get_MetadataMode() const { return metadataMode_; }

SerializableMetadata::SerializableMetadata(
    __in KUri const & name,
    __in KString const & typeString,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in FABRIC_STATE_PROVIDER_ID parentStateProviderId,
    __in FABRIC_SEQUENCE_NUMBER createLSN,
    __in FABRIC_SEQUENCE_NUMBER deleteLSN,
    __in MetadataMode::Enum metadataMode,
    __in_opt Utilities::OperationData const * const initializationParameters) noexcept
    : KObject()
    , KShared()
    , name_(&name)
    , typeString_(&typeString)
    , stateProviderId_(stateProviderId)
    , parentStateProviderId_(parentStateProviderId)
    , createLSN_(createLSN)
    , deleteLSN_(deleteLSN)
    , metadataMode_(metadataMode) 
    , initializationParameters_(initializationParameters)
{
}

SerializableMetadata::~SerializableMetadata()
{
}
