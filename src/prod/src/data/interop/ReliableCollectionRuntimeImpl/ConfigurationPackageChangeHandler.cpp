// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Data;
using namespace Data::Interop;

HRESULT ConfigurationPackageChangeHandler::Create(
    __in Utilities::PartitionedReplicaId const & traceType,
    __in IFabricStatefulServicePartition* statefulServicePartition,
    __in IFabricPrimaryReplicator* fabricPrimaryReplicator,
    __in wstring const & configPackageName,
    __in wstring const & replicatorSettingsSectionName,
    __in wstring const & replicatorSecuritySectionName,
    __in KAllocator & allocator,
    __out ComPointer<IFabricConfigurationPackageChangeHandler>& result)
{
    ConfigurationPackageChangeHandler * pointer = _new(RELIABLECOLLECTIONRUNTIME_TAG, allocator) ConfigurationPackageChangeHandler(
        traceType,
        statefulServicePartition,
        fabricPrimaryReplicator,
        configPackageName,
        replicatorSettingsSectionName,
        replicatorSecuritySectionName);
    if (!pointer)
    {
        RCREventSource::Events->ExceptionError(
            traceType.TracePartitionId,
            traceType.ReplicaId,
            L"Failed to Create ConfigurationPackageChangeHandler due to insufficient memory in ConfigurationPackageChangeHandler::Create",
            E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    result.SetAndAddRef(pointer);
    return S_OK;
}

void ConfigurationPackageChangeHandler::OnPackageAdded(
    __in IFabricCodePackageActivationContext *source,
    __in IFabricConfigurationPackage *configPackage)
{
    UNREFERENCED_PARAMETER(source);

    HRESULT hr;
    Common::ScopedHeap heap;
    FABRIC_REPLICATOR_SETTINGS const * fabricReplicatorSettings = nullptr;
    ReplicatorConfigSettingsResult configSettingsResult;

    const FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION * packageDescription = configPackage->get_Description();
    ASSERT_IF(NULL == packageDescription, "Unexpected configuration package description");

    RCREventSource::Events->ConfigurationPackageChangeHandlerEvent(
        TracePartitionId,
        ReplicaId,
        configPackageName_,
        packageDescription->Name);

    if (!Common::StringUtility::AreEqualCaseInsensitive(configPackageName_, packageDescription->Name))
    {
        // this config package does not contain replicator settings
        return;
    }

    IFabricTransactionalReplicator* fabricTransactionalReplicator = dynamic_cast<IFabricTransactionalReplicator*>(fabricPrimaryReplicator_.GetRawPointer());
    if (fabricTransactionalReplicator == nullptr)
    {
        RCREventSource::Events->ExceptionError(
            TracePartitionId,
            ReplicaId,
            L"ConfigurationPackageChangeHandler: Dynamic cast to IFabricTransactionalReplicator failed", 
            STATUS_OBJECT_TYPE_MISMATCH);
        statefulServicePartition_->ReportFault(FABRIC_FAULT_TYPE_TRANSIENT);
        return;
    }

    hr = LoadReplicatorSettingsFromConfigPackage(
        *PartitionedReplicaIdentifier,
        heap,
        configSettingsResult,
        configPackageName_,
        replicatorSettingsSectionName_,
        replicatorSecuritySectionName_,
        &fabricReplicatorSettings,
        nullptr,
        nullptr);

    if (FAILED(hr))
    {
        RCREventSource::Events->ConfigPackageLoadFailed(TracePartitionId, ReplicaId, configPackageName_, replicatorSettingsSectionName_, replicatorSecuritySectionName_, hr);
        statefulServicePartition_->ReportFault(FABRIC_FAULT_TYPE_TRANSIENT);
        return;
    }

    hr = fabricTransactionalReplicator->UpdateReplicatorSettings(fabricReplicatorSettings);
    if (FAILED(hr))
    {
        RCREventSource::Events->ExceptionError(
            TracePartitionId,
            ReplicaId,
            L"ConfigurationPackageChangeHandler: UpdateReplicatorSettings failed",
            hr);
        statefulServicePartition_->ReportFault(FABRIC_FAULT_TYPE_TRANSIENT);
        return;
    }
}

void ConfigurationPackageChangeHandler::OnPackageRemoved(
    __in IFabricCodePackageActivationContext *source,
    __in IFabricConfigurationPackage *configPackage)
{
    UNREFERENCED_PARAMETER(source);
    UNREFERENCED_PARAMETER(configPackage);
}

void ConfigurationPackageChangeHandler::OnPackageModified(
    __in IFabricCodePackageActivationContext *source,
    __in IFabricConfigurationPackage *previousConfigPackage,
    __in IFabricConfigurationPackage *configPackage)
{
    UNREFERENCED_PARAMETER(previousConfigPackage);

    OnPackageAdded(source, configPackage);
}

ConfigurationPackageChangeHandler::ConfigurationPackageChangeHandler(
    __in Utilities::PartitionedReplicaId const & partitionedReplicaId,
    __in IFabricStatefulServicePartition* statefulServicePartition,
    __in IFabricPrimaryReplicator* fabricPrimaryReplicator,
    __in wstring const & configPackageName,
    __in wstring const & replicatorSettingsSectionName,
    __in wstring const & replicatorSecuritySectionName)
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(partitionedReplicaId)
    , configPackageName_(configPackageName)
    , replicatorSettingsSectionName_(replicatorSettingsSectionName)
    , replicatorSecuritySectionName_(replicatorSecuritySectionName)
{
    RCREventSource::Events->ConfigurationPackageChangeHandlerCtor(
        TracePartitionId,
        ReplicaId,
        reinterpret_cast<uintptr_t>(fabricPrimaryReplicator),
        configPackageName,
        replicatorSettingsSectionName,
        replicatorSecuritySectionName);
    fabricPrimaryReplicator_.SetAndAddRef(fabricPrimaryReplicator);
    statefulServicePartition_.SetAndAddRef(statefulServicePartition);
}

ConfigurationPackageChangeHandler::~ConfigurationPackageChangeHandler()
{
}
