// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability::ReplicationComponent;
using namespace ServiceModel;

typedef Common::ComPointer<IFabricCodePackageActivationContext> IFabricCodePackageActivationContextCPtr;
typedef Common::ComPointer<IFabricConfigurationPackage> IFabricConfigurationPackageCPtr;

ReplicatorSettings::ReplicatorSettings()
    : flags_(0)
    , retryInterval_()
    , batchAckInterval_()
    , replicatorAddress_()
    , replicatorListenAddress_()
    , replicatorPublishAddress_()
    , requireServiceAck_(false)
    , initialReplicationQueueSize_(0)
    , maxReplicationQueueSize_(0)
    , initialCopyQueueSize_(0)
    , maxCopyQueueSize_(0)
    , maxReplicationQueueMemorySize_(0)
    , secondaryClearAcknowledgedOperations_(false)
    , maxReplicationMessageSize_(0)
    , useStreamFaultsAndEndOfStreamOperationAck_(false)
    , securitySettings_()
    , initialSecondaryReplicationQueueSize_(0)
    , maxSecondaryReplicationQueueSize_(0)
    , maxSecondaryReplicationQueueMemorySize_(0)
    , initialPrimaryReplicationQueueSize_(0)
    , maxPrimaryReplicationQueueSize_(0)
    , maxPrimaryReplicationQueueMemorySize_(0)
    , primaryWaitForPendingQuorumsTimeout_()
{
    flags_ = FABRIC_REPLICATOR_SETTINGS_NONE;
}

bool ReplicatorSettings::get_RequireServiceAck() const
{
    return requireServiceAck_;
}

bool ReplicatorSettings::get_SecondaryClearAcknowledgedOperations() const
{
    return secondaryClearAcknowledgedOperations_;
}

bool ReplicatorSettings::get_UseStreamFaultsAndEndOfStreamOperationAck() const
{
    return useStreamFaultsAndEndOfStreamOperationAck_;
}

void ReplicatorSettings::set_UseStreamFaultsAndEndOfStreamOperationAck(bool value)
{
    useStreamFaultsAndEndOfStreamOperationAck_ = value;
    flags_ |= FABRIC_REPLICATOR_USE_STREAMFAULTS_AND_ENDOFSTREAM_OPERATIONACK;
}

Common::TimeSpan ReplicatorSettings::get_RetryInterval() const
{
    return retryInterval_;
}

Common::TimeSpan ReplicatorSettings::get_BatchAckInterval() const
{
    return batchAckInterval_;
}

Common::TimeSpan ReplicatorSettings::get_PrimaryWaitForPendingQuorumsTimeout() const
{
    return primaryWaitForPendingQuorumsTimeout_;
}

int64 ReplicatorSettings::get_InitialReplicationQueueSize() const
{
    return initialReplicationQueueSize_;
}

int64 ReplicatorSettings::get_MaxReplicationQueueSize() const
{
    return maxReplicationQueueSize_;
}

int64 ReplicatorSettings::get_InitialCopyQueueSize() const
{
    return initialCopyQueueSize_;
}

int64 ReplicatorSettings::get_MaxCopyQueueSize() const
{
    return maxCopyQueueSize_;
}

int64 ReplicatorSettings::get_MaxReplicationQueueMemorySize() const
{
    return maxReplicationQueueMemorySize_;
}

int64 ReplicatorSettings::get_MaxReplicationMessageSize() const
{
    return maxReplicationMessageSize_;
}

int64 ReplicatorSettings::get_InitialPrimaryReplicationQueueSize() const
{
    return initialPrimaryReplicationQueueSize_;
}

int64 ReplicatorSettings::get_MaxPrimaryReplicationQueueSize() const
{
    return maxPrimaryReplicationQueueSize_;
}

int64 ReplicatorSettings::get_MaxPrimaryReplicationQueueMemorySize() const
{
    return maxPrimaryReplicationQueueMemorySize_;
}

int64 ReplicatorSettings::get_InitialSecondaryReplicationQueueSize() const
{
    return initialSecondaryReplicationQueueSize_;
}

int64 ReplicatorSettings::get_MaxSecondaryReplicationQueueSize() const
{
    return maxSecondaryReplicationQueueSize_;
}

int64 ReplicatorSettings::get_MaxSecondaryReplicationQueueMemorySize() const
{
    return maxSecondaryReplicationQueueMemorySize_;
}

std::wstring ReplicatorSettings::get_ReplicatorAddress() const
{
    return replicatorAddress_;
}

std::wstring ReplicatorSettings::get_ReplicatorListenAddress() const
{
    return replicatorListenAddress_;
}

std::wstring ReplicatorSettings::get_ReplicatorPublishAddress() const
{
    return replicatorPublishAddress_;
}

ErrorCode ReplicatorSettings::FromSecuritySettings(
    __in Transport::SecuritySettings const& securitySettings,
    __out ReplicatorSettingsUPtr & output)
{
    auto created = unique_ptr<ReplicatorSettings>(new ReplicatorSettings());

    created->securitySettings_ = securitySettings;
    created->flags_ = FABRIC_REPLICATOR_SECURITY;
    
    return MoveIfValid(
        move(created),
        output);
}

ErrorCode ReplicatorSettings::FromPublicApi(
    __in FABRIC_REPLICATOR_SETTINGS const& replicatorSettings,
    __out ReplicatorSettingsUPtr & output)
{
    auto created = unique_ptr<ReplicatorSettings>(new ReplicatorSettings());

    ReferencePointer<FABRIC_REPLICATOR_SETTINGS_EX1> ex1Settings;
    ReferencePointer<FABRIC_REPLICATOR_SETTINGS_EX2> ex2Settings;
    ReferencePointer<FABRIC_REPLICATOR_SETTINGS_EX3> ex3Settings;
    ReferencePointer<FABRIC_REPLICATOR_SETTINGS_EX4> ex4Settings;

    // validate ex1 settings 
    if (IsReplicatorEx1FlagsUsed(replicatorSettings.Flags))
    {
        if (replicatorSettings.Reserved == NULL)
        {
            ServiceModelEventSource::Trace->ReplicatorSettingsError(L"Reserved Field is NULL with one of the ex1 settings flags enabled");
            return ErrorCodeValue::InvalidArgument;
        }
        ex1Settings = static_cast<FABRIC_REPLICATOR_SETTINGS_EX1*>(replicatorSettings.Reserved);
    }

    // validate ex2 settings 
    if (IsReplicatorEx2FlagsUsed(replicatorSettings.Flags))
    {
        if (replicatorSettings.Reserved == NULL)
        {
            ServiceModelEventSource::Trace->ReplicatorSettingsError(L"ReplicatorSettings Reserved Field is NULL with one of the ex2 settings flags enabled");
            return ErrorCodeValue::InvalidArgument;
        }
        if (!ex1Settings)
        {
            ex1Settings = static_cast<FABRIC_REPLICATOR_SETTINGS_EX1*>(replicatorSettings.Reserved);
        }

        if (ex1Settings->Reserved == NULL)
        {
            ServiceModelEventSource::Trace->ReplicatorSettingsError(L"ReplicatorSettingsEx1 Reserved Field is NULL with one of the ex2 settings flags enabled");
            return ErrorCodeValue::InvalidArgument;
        }
        ex2Settings = static_cast<FABRIC_REPLICATOR_SETTINGS_EX2*>(ex1Settings->Reserved);
    }

    // validate ex3 settings
    if (IsReplicatorEx3FlagsUsed(replicatorSettings.Flags))
    {
        if (replicatorSettings.Reserved == NULL)
        {
            ServiceModelEventSource::Trace->ReplicatorSettingsError(L"ReplicatorSettings Reserved Field is NULL with one of the ex3 settings flags enabled");
            return ErrorCodeValue::InvalidArgument;
        }
        if (!ex1Settings)
        {
            ex1Settings = static_cast<FABRIC_REPLICATOR_SETTINGS_EX1*>(replicatorSettings.Reserved);
        }

        if (ex1Settings->Reserved == NULL)
        {
            ServiceModelEventSource::Trace->ReplicatorSettingsError(L"ReplicatorSettingsEx1 Reserved Field is NULL with one of the ex3 settings flags enabled");
            return ErrorCodeValue::InvalidArgument;
        }
        if (!ex2Settings)
        {
            ex2Settings = static_cast<FABRIC_REPLICATOR_SETTINGS_EX2*>(ex1Settings->Reserved);
        }

        if (ex2Settings->Reserved == NULL)
        {
            ServiceModelEventSource::Trace->ReplicatorSettingsError(L"ReplicatorSettingsEx2 Reserved Field is NULL with one of the ex3 settings flags enabled");
            return ErrorCodeValue::InvalidArgument;
        }
        ex3Settings = static_cast<FABRIC_REPLICATOR_SETTINGS_EX3*>(ex2Settings->Reserved);
    }

    //validate ex4 settings
    if (IsReplicatorEx4FlagsUsed(replicatorSettings.Flags))
    {
        if (replicatorSettings.Reserved == NULL)
        {
            ServiceModelEventSource::Trace->ReplicatorSettingsError(L"ReplicatorSettings Reserved Field is NULL with one of the ex4 settings flags enabled");
            return ErrorCodeValue::InvalidArgument;
        }
        if (!ex1Settings)
        {
            ex1Settings = static_cast<FABRIC_REPLICATOR_SETTINGS_EX1*>(replicatorSettings.Reserved);
        }

        if (ex1Settings->Reserved == NULL)
        {
            ServiceModelEventSource::Trace->ReplicatorSettingsError(L"ReplicatorSettingsEx1 Reserved Field is NULL with one of the ex4 settings flags enabled");
            return ErrorCodeValue::InvalidArgument;
        }
        if (!ex2Settings)
        {
            ex2Settings = static_cast<FABRIC_REPLICATOR_SETTINGS_EX2*>(ex1Settings->Reserved);
        }

        if (ex2Settings->Reserved == NULL)
        {
            ServiceModelEventSource::Trace->ReplicatorSettingsError(L"ReplicatorSettingsEx2 Reserved Field is NULL with one of the ex4 settings flags enabled");
            return ErrorCodeValue::InvalidArgument;
        }
        if (!ex3Settings)
        {
            ex3Settings = static_cast<FABRIC_REPLICATOR_SETTINGS_EX3*>(ex2Settings->Reserved);
        }

        if (ex3Settings->Reserved == NULL)
        {
            ServiceModelEventSource::Trace->ReplicatorSettingsError(L"ReplicatorSettingsEx3 Reserved Field is NULL with one of the ex4 settings flags enabled");
            return ErrorCodeValue::InvalidArgument;
        }
        ex4Settings = static_cast<FABRIC_REPLICATOR_SETTINGS_EX4*>(ex3Settings->Reserved);
    }

    // Store the transport security settings if it exists
    if (replicatorSettings.Flags & FABRIC_REPLICATOR_SECURITY)
    {
        auto error = Transport::SecuritySettings::FromPublicApi(*replicatorSettings.SecurityCredentials, created->securitySettings_);

        if (!error.IsSuccess())
        {
            return error;
        }
    }

    // Populate all the member variables
    created->flags_ = replicatorSettings.Flags;
    if (created->flags_ & FABRIC_REPLICATOR_RETRY_INTERVAL)
    {
        created->retryInterval_ = TimeSpan::FromMilliseconds(replicatorSettings.RetryIntervalMilliseconds);
    }
    if (created->flags_ & FABRIC_REPLICATOR_BATCH_ACKNOWLEDGEMENT_INTERVAL)
    {
        created->batchAckInterval_ = TimeSpan::FromMilliseconds(replicatorSettings.BatchAcknowledgementIntervalMilliseconds);
    }
    if (created->flags_ & FABRIC_REPLICATOR_ADDRESS)
    {
        if (StringUtility::LpcwstrToWstring(replicatorSettings.ReplicatorAddress, false, created->replicatorAddress_) != S_OK)
        {
            ServiceModelEventSource::Trace->ReplicatorSettingsError(L"Failed to parse ReplicatorAddress config");
            return ErrorCodeValue::InvalidArgument;
        }
        if (created->replicatorAddress_ == L"")
        {
            ServiceModelEventSource::Trace->ReplicatorSettingsError(L"Empty Replicator Address is not allowed if FABRIC_REPLICATOR_ADDRESS flag is specified");
            return ErrorCodeValue::InvalidArgument;
        }
    }
    if (created->flags_ & FABRIC_REPLICATOR_REQUIRE_SERVICE_ACK)
    {
        created->requireServiceAck_ = replicatorSettings.RequireServiceAck ? true : false;
    }
    if (created->flags_ & FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE)
    {
        created->initialReplicationQueueSize_ = replicatorSettings.InitialReplicationQueueSize;
    }
    if (created->flags_ & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE)
    {
        created->maxReplicationQueueSize_ = replicatorSettings.MaxReplicationQueueSize;
    }
    if (created->flags_ & FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE)
    {
        created->initialCopyQueueSize_ = replicatorSettings.InitialCopyQueueSize;
    }
    if (created->flags_ & FABRIC_REPLICATOR_COPY_QUEUE_MAX_SIZE)
    {
        created->maxCopyQueueSize_ = replicatorSettings.MaxCopyQueueSize;
    }

    // ex1 settings
    if (created->flags_ & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE)
    {
        created->maxReplicationQueueMemorySize_ = ex1Settings->MaxReplicationQueueMemorySize;
    }
    if (created->flags_ & FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE)
    {
        created->maxReplicationMessageSize_ = ex1Settings->MaxReplicationMessageSize;
    }
    if (created->flags_ & FABRIC_REPLICATOR_SECONDARY_CLEAR_ACKNOWLEDGED_OPERATIONS)
    {
        created->secondaryClearAcknowledgedOperations_ = ex1Settings->SecondaryClearAcknowledgedOperations ? true : false;
    }

    // ex2 settings
    if (created->flags_ & FABRIC_REPLICATOR_USE_STREAMFAULTS_AND_ENDOFSTREAM_OPERATIONACK)
    {
        created->useStreamFaultsAndEndOfStreamOperationAck_ = ex2Settings->UseStreamFaultsAndEndOfStreamOperationAck ? true : false;
    }

    // ex3 settings
    if (created->flags_ & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE)
    {
        created->initialSecondaryReplicationQueueSize_ = ex3Settings->InitialSecondaryReplicationQueueSize;
    }
    if (created->flags_ & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE)
    {
        created->maxSecondaryReplicationQueueSize_ = ex3Settings->MaxSecondaryReplicationQueueSize;
    }
    if (created->flags_ & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE)
    {
        created->maxSecondaryReplicationQueueMemorySize_ = ex3Settings->MaxSecondaryReplicationQueueMemorySize;
    }
    if (created->flags_ & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE)
    {
        created->initialPrimaryReplicationQueueSize_ = ex3Settings->InitialPrimaryReplicationQueueSize;
    }
    if (created->flags_ & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE)
    {
        created->maxPrimaryReplicationQueueSize_ = ex3Settings->MaxPrimaryReplicationQueueSize;
    }
    if (created->flags_ & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE)
    {
        created->maxPrimaryReplicationQueueMemorySize_ = ex3Settings->MaxPrimaryReplicationQueueMemorySize;
    }
    if (created->flags_ & FABRIC_REPLICATOR_PRIMARY_WAIT_FOR_PENDING_QUORUMS_TIMEOUT)
    {
        created->primaryWaitForPendingQuorumsTimeout_ = Common::TimeSpan::FromMilliseconds(ex3Settings->PrimaryWaitForPendingQuorumsTimeoutMilliseconds);
    }

    //ex4 settings
    if (created->flags_ & FABRIC_REPLICATOR_LISTEN_ADDRESS)
    {
        if (StringUtility::LpcwstrToWstring(ex4Settings->ReplicatorListenAddress, false, created->replicatorListenAddress_) != S_OK)
        {
            ServiceModelEventSource::Trace->ReplicatorSettingsError(L"Failed to parse ReplicatorListenAddress config");
            return ErrorCodeValue::InvalidArgument;
        }
        if (created->replicatorListenAddress_ == L"")
        {
            ServiceModelEventSource::Trace->ReplicatorSettingsError(L"Empty Replicator Listen Address is not allowed if FABRIC_REPLICATOR_LISTEN_ADDRESS flag is specified");
            return ErrorCodeValue::InvalidArgument;
        }
    }
    if (created->flags_ & FABRIC_REPLICATOR_PUBLISH_ADDRESS)
    {
        if (StringUtility::LpcwstrToWstring(ex4Settings->ReplicatorPublishAddress, false, created->replicatorPublishAddress_) != S_OK)
        {
            ServiceModelEventSource::Trace->ReplicatorSettingsError(L"Failed to parse ReplicatorPublishAddress config");
            return ErrorCodeValue::InvalidArgument;
        }
        if (created->replicatorPublishAddress_ == L"")
        {
            ServiceModelEventSource::Trace->ReplicatorSettingsError(L"Empty Replicator Publish Address is not allowed if FABRIC_REPLICATOR_PUBLISH_ADDRESS flag is specified");
            return ErrorCodeValue::InvalidArgument;
        }
    }

    return MoveIfValid(
        move(created),
        output);
}

ErrorCode ReplicatorSettings::FromConfig(
    __in REConfigBase const& reConfig,
    __in std::wstring const& replicatorAddress,
    __in std::wstring const& replicatorListenAddress,
    __in std::wstring const& replicatorPublishAddress,
    __in Transport::SecuritySettings const& securitySettings,
    __out ReplicatorSettingsUPtr & output)
{ 
    auto created = unique_ptr<ReplicatorSettings>(new ReplicatorSettings());

    REConfigValues config;

    reConfig.GetReplicatorSettingsStructValues(config);

    created->flags_ = FABRIC_REPLICATOR_SETTINGS_NONE;

    // The replicator address is picked from the input parameter and the config value is ignored
    created->replicatorAddress_ = replicatorAddress;
    created->flags_ |= FABRIC_REPLICATOR_ADDRESS;

    created->replicatorListenAddress_ = replicatorListenAddress;
    created->flags_ |= FABRIC_REPLICATOR_LISTEN_ADDRESS;

    created->replicatorPublishAddress_ = replicatorPublishAddress;
    created->flags_ |= FABRIC_REPLICATOR_PUBLISH_ADDRESS;

    if (securitySettings.SecurityProvider() != Transport::SecurityProvider::None)
    {
        // Apply the security settings only if it is not none
        created->securitySettings_ = securitySettings;
        created->flags_ |= FABRIC_REPLICATOR_SECURITY;
    }

    created->retryInterval_ = config.RetryInterval;
    created->flags_ |= FABRIC_REPLICATOR_RETRY_INTERVAL;

    created->batchAckInterval_ = config.BatchAcknowledgementInterval;
    created->flags_ |= FABRIC_REPLICATOR_BATCH_ACKNOWLEDGEMENT_INTERVAL;
    
    created->requireServiceAck_ = config.RequireServiceAck;
    created->flags_ |= FABRIC_REPLICATOR_REQUIRE_SERVICE_ACK;
 
    created->initialReplicationQueueSize_ = config.InitialReplicationQueueSize;
    created->flags_ |= FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE;
    
    created->maxReplicationQueueSize_ = config.MaxReplicationQueueSize;
    created->flags_ |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE;
    
    created->initialCopyQueueSize_ = config.InitialCopyQueueSize;
    created->flags_ |= FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE;
    
    created->maxCopyQueueSize_ = config.MaxCopyQueueSize;
    created->flags_ |= FABRIC_REPLICATOR_COPY_QUEUE_MAX_SIZE;
    
    created->maxReplicationMessageSize_ = config.MaxReplicationMessageSize;
    created->flags_ |= FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE;
    
    created->maxReplicationQueueMemorySize_ = config.MaxReplicationQueueMemorySize;
    created->flags_ |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
    
    created->secondaryClearAcknowledgedOperations_ = config.SecondaryClearAcknowledgedOperations;
    created->flags_ |= FABRIC_REPLICATOR_SECONDARY_CLEAR_ACKNOWLEDGED_OPERATIONS;

    created->useStreamFaultsAndEndOfStreamOperationAck_ = config.UseStreamFaultsAndEndOfStreamOperationAck;
    created->flags_ |= FABRIC_REPLICATOR_USE_STREAMFAULTS_AND_ENDOFSTREAM_OPERATIONACK;

    created->initialSecondaryReplicationQueueSize_ = config.InitialSecondaryReplicationQueueSize;
    created->flags_ |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE;

    created->maxSecondaryReplicationQueueSize_ = config.MaxSecondaryReplicationQueueSize;
    created->flags_ |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE;

    created->maxSecondaryReplicationQueueMemorySize_ = config.MaxSecondaryReplicationQueueMemorySize;
    created->flags_ |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;

    created->initialPrimaryReplicationQueueSize_ = config.InitialPrimaryReplicationQueueSize;
    created->flags_ |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE;

    created->maxPrimaryReplicationQueueSize_ = config.MaxPrimaryReplicationQueueSize;
    created->flags_ |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE;

    created->maxPrimaryReplicationQueueMemorySize_ = config.MaxPrimaryReplicationQueueMemorySize;
    created->flags_ |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;

    created->primaryWaitForPendingQuorumsTimeout_ = config.PrimaryWaitForPendingQuorumsTimeout;
    created->flags_ |= FABRIC_REPLICATOR_PRIMARY_WAIT_FOR_PENDING_QUORUMS_TIMEOUT;

    return MoveIfValid(
        move(created),
        output);
}

void ReplicatorSettings::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_REPLICATOR_SETTINGS & replicatorSettings) const
{
    ReferencePointer<FABRIC_REPLICATOR_SETTINGS_EX1> ex1Settings;
    ReferencePointer<FABRIC_REPLICATOR_SETTINGS_EX2> ex2Settings;
    ReferencePointer<FABRIC_REPLICATOR_SETTINGS_EX3> ex3Settings;
    ReferencePointer<FABRIC_REPLICATOR_SETTINGS_EX4> ex4Settings;
    ReferencePointer<FABRIC_SECURITY_CREDENTIALS> securitySettings;

    replicatorSettings.Reserved = NULL;

    replicatorSettings.Flags = flags_;

    if (flags_ & FABRIC_REPLICATOR_RETRY_INTERVAL)
    {
        replicatorSettings.RetryIntervalMilliseconds = static_cast<DWORD>(retryInterval_.TotalMilliseconds());
    }
    if (flags_ & FABRIC_REPLICATOR_BATCH_ACKNOWLEDGEMENT_INTERVAL)
    {
        replicatorSettings.BatchAcknowledgementIntervalMilliseconds = static_cast<DWORD>(batchAckInterval_.TotalMilliseconds());
    }
    if (flags_ & FABRIC_REPLICATOR_ADDRESS)
    {
        replicatorSettings.ReplicatorAddress = heap.AddString(replicatorAddress_);
    }
    if (flags_ & FABRIC_REPLICATOR_REQUIRE_SERVICE_ACK)
    {
        replicatorSettings.RequireServiceAck = requireServiceAck_;
    }
    if (flags_ & FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE)
    {
        replicatorSettings.InitialReplicationQueueSize = static_cast<DWORD>(initialReplicationQueueSize_);
    }
    if (flags_ & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE)
    {
        replicatorSettings.MaxReplicationQueueSize = static_cast<DWORD>(maxReplicationQueueSize_);
    }
    if (flags_ & FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE)
    {
        replicatorSettings.InitialCopyQueueSize = static_cast<DWORD>(initialCopyQueueSize_);
    }
    if (flags_ & FABRIC_REPLICATOR_COPY_QUEUE_MAX_SIZE)
    {
        replicatorSettings.MaxCopyQueueSize = static_cast<DWORD>(maxCopyQueueSize_);
    }
    if (flags_ & FABRIC_REPLICATOR_SECURITY)
    {
        securitySettings = heap.AddItem<FABRIC_SECURITY_CREDENTIALS>();
        replicatorSettings.SecurityCredentials = securitySettings.GetRawPointer();
        securitySettings_.ToPublicApi(heap, *securitySettings);
    }
    
    // ex1 settings
    if (IsReplicatorEx1FlagsUsed(replicatorSettings.Flags))
    {
        ex1Settings = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX1>();
        ex1Settings->Reserved = NULL;
        replicatorSettings.Reserved = ex1Settings.GetRawPointer();

        if (flags_ & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE)
        {
            ex1Settings->MaxReplicationQueueMemorySize = static_cast<DWORD>(maxReplicationQueueMemorySize_);
        }
        if (flags_ & FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE)
        {
            ex1Settings->MaxReplicationMessageSize = static_cast<DWORD>(maxReplicationMessageSize_);
        }
        if (flags_ & FABRIC_REPLICATOR_SECONDARY_CLEAR_ACKNOWLEDGED_OPERATIONS)
        {
            ex1Settings->SecondaryClearAcknowledgedOperations = secondaryClearAcknowledgedOperations_;
        }
    }

    // ex2 settings
    if (IsReplicatorEx2FlagsUsed(replicatorSettings.Flags))
    {
        if (!ex1Settings)
        {
            ex1Settings = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX1>();
            ex1Settings->Reserved = NULL;
            replicatorSettings.Reserved = ex1Settings.GetRawPointer();
        }

        ex2Settings = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX2>();
        ex2Settings->Reserved = NULL;
        ex1Settings->Reserved = ex2Settings.GetRawPointer();

        ex2Settings->UseStreamFaultsAndEndOfStreamOperationAck = useStreamFaultsAndEndOfStreamOperationAck_;
    }
    
    // ex3 settings
    if (IsReplicatorEx3FlagsUsed(replicatorSettings.Flags))
    {
        if (!ex1Settings)
        {
            ex1Settings = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX1>();
            ex1Settings->Reserved = NULL;
            replicatorSettings.Reserved = ex1Settings.GetRawPointer();
        }

        if (!ex2Settings)
        {
            ex2Settings = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX2>();
            ex2Settings->Reserved = NULL;
            ex1Settings->Reserved = ex2Settings.GetRawPointer();
        }

        ex3Settings = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX3>();
        ex3Settings->Reserved = NULL;
        ex2Settings->Reserved = ex3Settings.GetRawPointer();

        if (flags_ & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE)
        {
            ex3Settings->InitialSecondaryReplicationQueueSize = static_cast<DWORD>(initialSecondaryReplicationQueueSize_);
        }
        if (flags_ & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE)
        {
            ex3Settings->MaxSecondaryReplicationQueueSize = static_cast<DWORD>(maxSecondaryReplicationQueueSize_);
        }
        if (flags_ & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE)
        {
            ex3Settings->MaxSecondaryReplicationQueueMemorySize = static_cast<DWORD>(maxSecondaryReplicationQueueMemorySize_);
        }
        if (flags_ & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE)
        {
            ex3Settings->InitialPrimaryReplicationQueueSize = static_cast<DWORD>(initialPrimaryReplicationQueueSize_);
        }
        if (flags_ & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE)
        {
            ex3Settings->MaxPrimaryReplicationQueueSize = static_cast<DWORD>(maxPrimaryReplicationQueueSize_);
        }
        if (flags_ & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE)
        {
            ex3Settings->MaxPrimaryReplicationQueueMemorySize = static_cast<DWORD>(maxPrimaryReplicationQueueMemorySize_);
        }
        if (flags_ & FABRIC_REPLICATOR_PRIMARY_WAIT_FOR_PENDING_QUORUMS_TIMEOUT)
        {
            ex3Settings->PrimaryWaitForPendingQuorumsTimeoutMilliseconds = static_cast<DWORD>(primaryWaitForPendingQuorumsTimeout_.TotalMilliseconds());
        }
    }

    //ex4 settings
    if (IsReplicatorEx4FlagsUsed(replicatorSettings.Flags))
    {
        if (!ex1Settings)
        {
            ex1Settings = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX1>();
            ex1Settings->Reserved = NULL;
            replicatorSettings.Reserved = ex1Settings.GetRawPointer();
        }

        if (!ex2Settings)
        {
            ex2Settings = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX2>();
            ex2Settings->Reserved = NULL;
            ex1Settings->Reserved = ex2Settings.GetRawPointer();
        }

        if (!ex3Settings)
        {
            ex3Settings = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX3>();
            ex3Settings->Reserved = NULL;
            ex2Settings->Reserved = ex3Settings.GetRawPointer();
        }
        
        ex4Settings = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX4>();
        ex4Settings->Reserved = NULL;
        ex3Settings->Reserved = ex4Settings.GetRawPointer();

        if (flags_ & FABRIC_REPLICATOR_LISTEN_ADDRESS)
        {
            ex4Settings->ReplicatorListenAddress = heap.AddString(replicatorListenAddress_);
        }
        if (flags_ & FABRIC_REPLICATOR_PUBLISH_ADDRESS)
        {
            ex4Settings->ReplicatorPublishAddress = heap.AddString(replicatorPublishAddress_);
        }
    }
}

ErrorCode ReplicatorSettings::ReadFromConfigurationPackage(
    __in IFabricCodePackageActivationContextCPtr codePackageActivationContextCPtr,
    __in IFabricConfigurationPackageCPtr configPackageCPtr,
    __in wstring const & sectionName,
    __in wstring const & hostName,
    __in wstring const & publishHostName,
    __out ReplicatorSettingsUPtr & output)
{
    ASSERT_IF(codePackageActivationContextCPtr.GetRawPointer() == NULL, "activationContext is null");
    ASSERT_IF(configPackageCPtr.GetRawPointer() == NULL, "configPackageCPtr is null");

    auto created = unique_ptr<ReplicatorSettings>(new ReplicatorSettings());
    bool hasValue = false;

    REConfigValues configNames; // Dummy object used mainly for retreiving key names of settings
    
    // Try and look for a setting which says -"ReplicatorAddress".
    ErrorCode error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, configNames.ReplicatorAddressEntry.Key, created->replicatorAddress_, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (hasValue)
    {
        created->flags_ |= FABRIC_REPLICATOR_ADDRESS;
    }
    else
    {
        // If not, look for a setting called - "ReplicatorEndpoint", which contains the endpoint name from the
        // service manifest
        wstring endpointName;
        error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, ReplicatorSettingsParameterNames::ReplicatorEndpoint, endpointName, hasValue);
        if (!error.IsSuccess()) { return error; }
        if (hasValue)
        {
            // todo: use certificate name to initialize findValue_
            wstring certificateName;
            error = ReadSettingsFromEndpoint(codePackageActivationContextCPtr, endpointName, hostName, created->replicatorAddress_, certificateName);
            if (!error.IsSuccess()) { return error; }
            created->flags_ |= FABRIC_REPLICATOR_ADDRESS;
        }
    }

    // Try and look for a setting which says -"ReplicatorListenAddress".
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, configNames.ReplicatorListenAddressEntry.Key, created->replicatorListenAddress_, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (hasValue)
    {
        created->flags_ |= FABRIC_REPLICATOR_LISTEN_ADDRESS;
    }
    else
    {
        // If not, look for a setting called - "ReplicatorEndpoint", which contains the endpoint name from the
        // service manifest
        wstring endpointName;
        error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, ReplicatorSettingsParameterNames::ReplicatorEndpoint, endpointName, hasValue);
        if (!error.IsSuccess()) { return error; }
        if (hasValue)
        {
            // todo: use certificate name to initialize findValue_
            wstring certificateName;
            error = ReadSettingsFromEndpoint(codePackageActivationContextCPtr, endpointName, hostName, created->replicatorListenAddress_, certificateName);
            if (!error.IsSuccess()) { return error; }
            created->flags_ |= FABRIC_REPLICATOR_LISTEN_ADDRESS;
        }
    }

    // Try and look for a setting which says -"ReplicatorPublishAddress".
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, configNames.ReplicatorPublishAddressEntry.Key, created->replicatorPublishAddress_, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (hasValue)
    {
        created->flags_ |= FABRIC_REPLICATOR_PUBLISH_ADDRESS;
    }
    else
    {
        // If not, look for a setting called - "ReplicatorEndpoint", which contains the endpoint name from the
        // service manifest
        wstring endpointName;
        error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, ReplicatorSettingsParameterNames::ReplicatorEndpoint, endpointName, hasValue);
        if (!error.IsSuccess()) { return error; }
        if (hasValue)
        {
            // todo: use certificate name to initialize findValue_
            wstring certificateName;
            error = ReadSettingsFromEndpoint(codePackageActivationContextCPtr, endpointName, publishHostName, created->replicatorPublishAddress_, certificateName);
            if (!error.IsSuccess()) { return error; }
            created->flags_ |= FABRIC_REPLICATOR_PUBLISH_ADDRESS;
        }
    }

    // retry interval
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, configNames.RetryIntervalEntry.Key, created->retryInterval_, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_REPLICATOR_RETRY_INTERVAL; }

    // batch acknowledgement interval
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, configNames.BatchAcknowledgementIntervalEntry.Key, created->batchAckInterval_, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_REPLICATOR_BATCH_ACKNOWLEDGEMENT_INTERVAL; }

    // require service ack
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, configNames.RequireServiceAckEntry.Key, created->requireServiceAck_, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_REPLICATOR_REQUIRE_SERVICE_ACK; }

    // initial replication queue size
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, configNames.InitialReplicationQueueSizeEntry.Key, created->initialReplicationQueueSize_, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE; }

    // max replication queue size
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, configNames.MaxReplicationQueueSizeEntry.Key, created->maxReplicationQueueSize_, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE; }

    // initial copy queue size
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, configNames.InitialCopyQueueSizeEntry.Key, created->initialCopyQueueSize_, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE; }

    // max copy queue size
    hasValue = false;
    error =Parser::ReadSettingsValue(configPackageCPtr, sectionName, configNames.MaxCopyQueueSizeEntry.Key, created->maxCopyQueueSize_, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_REPLICATOR_COPY_QUEUE_MAX_SIZE; }

    // max replication queue memory size
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, configNames.MaxReplicationQueueMemorySizeEntry.Key, created->maxReplicationQueueMemorySize_, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE; }

    // secondary clear acknowledged operations
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, configNames.SecondaryClearAcknowledgedOperationsEntry.Key, created->secondaryClearAcknowledgedOperations_, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_REPLICATOR_SECONDARY_CLEAR_ACKNOWLEDGED_OPERATIONS; }

    // max replication message size
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, configNames.MaxReplicationMessageSizeEntry.Key, created->maxReplicationMessageSize_, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE; }

    // Use Stream faults
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, configNames.UseStreamFaultsAndEndOfStreamOperationAckEntry.Key, created->useStreamFaultsAndEndOfStreamOperationAck_, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_REPLICATOR_USE_STREAMFAULTS_AND_ENDOFSTREAM_OPERATIONACK; }

    // initial secondary replication queue size
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, configNames.InitialSecondaryReplicationQueueSizeEntry.Key, created->initialSecondaryReplicationQueueSize_, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE; }

    // max secondary replication queue size
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, configNames.MaxSecondaryReplicationQueueSizeEntry.Key, created->maxSecondaryReplicationQueueSize_, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE; }

    // max secondary replication queue memory size
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, configNames.MaxSecondaryReplicationQueueMemorySizeEntry.Key, created->maxSecondaryReplicationQueueMemorySize_, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE; }

    // initial primary replication queue size
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, configNames.InitialPrimaryReplicationQueueSizeEntry.Key, created->initialPrimaryReplicationQueueSize_, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE; }

    // max primary replication queue size
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, configNames.MaxPrimaryReplicationQueueSizeEntry.Key, created->maxPrimaryReplicationQueueSize_, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE; }

    // max primary replication queue memory size
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, configNames.MaxPrimaryReplicationQueueMemorySizeEntry.Key, created->maxPrimaryReplicationQueueMemorySize_, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE; }

    // primary pending quorum timeout 
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, configNames.PrimaryWaitForPendingQuorumsTimeoutEntry.Key, created->primaryWaitForPendingQuorumsTimeout_, hasValue);
    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_REPLICATOR_PRIMARY_WAIT_FOR_PENDING_QUORUMS_TIMEOUT; }

    return MoveIfValid(
        move(created),
        output);
}

ErrorCode ReplicatorSettings::ReadSettingsFromEndpoint(
    __in IFabricCodePackageActivationContextCPtr codePackageActivationContextCPtr,
    __in std::wstring & endpointName,
    __in std::wstring const & hostName,
    __out wstring & endpointAddress,
    __out wstring & certificateName)
{
    ASSERT_IF(codePackageActivationContextCPtr.GetRawPointer() == NULL, "activationContext is null");

    ULONG port = 0;

    ErrorCode error = GetEndpointResourceFromManifest(codePackageActivationContextCPtr, endpointName, port, certificateName);
    if (!error.IsSuccess()) { return error; }

    wstring hostAndPort;
    StringWriter(hostAndPort).Write("{0}:{1}", hostName, port);

    endpointAddress = hostAndPort;

    return ErrorCode::Success();
}

ErrorCode ReplicatorSettings::GetEndpointResourceFromManifest(
    __in IFabricCodePackageActivationContextCPtr codePackageActivationContextCPtr,
    __in std::wstring & endpointName,
    __out ULONG & port,
    __out std::wstring & certificateName)
{
    ASSERT_IF(NULL == codePackageActivationContextCPtr.GetRawPointer(), "activationContext is null");

    const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST * endpoints = codePackageActivationContextCPtr->get_ServiceEndpointResources();

    if (endpoints == NULL || 
        endpoints->Count == 0 ||
        endpoints->Items == NULL)
    {
        ServiceModelEventSource::Trace->ReplicatorSettingsError(L"No Endpoint Resources found in service manifest");
        return ErrorCode(ErrorCodeValue::InvalidConfiguration);
    }

    wstring text;

    // find endpoint by name
    for (ULONG i = 0; i < endpoints->Count; ++i)
    {
        wstring endpointinfo;
        StringWriter writer(endpointinfo);

        const auto & endpoint = endpoints->Items[i];

        text.append(wformatString(
            "Endpoint{0}:{1};{2};{3};{4} \n\r",
            i,
            endpoint.Name,
            endpoint.Protocol,
            endpoint.Type,
            endpoint.Port));

        ASSERT_IF(endpoint.Name == NULL, "FABRIC_ENDPOINT_RESOURCE_DESCRIPTION with Name == NULL");

        if (StringUtility::AreEqualCaseInsensitive(endpointName.c_str(), endpoint.Name))
        {
            // check protocol is TCP
            if (endpoint.Protocol == NULL ||
                !StringUtility::AreEqualCaseInsensitive(ReplicatorSettingsParameterNames::ProtocolScheme.c_str(), endpoint.Protocol))
            {
                ServiceModelEventSource::Trace->ReplicatorSettingsError(L"Invalid Endpoint Protocol. Replication Protocol can only be TCP");
                return ErrorCode(ErrorCodeValue::InvalidConfiguration);
            }

            // check type is Internal
            if (endpoint.Type == NULL ||
                !StringUtility::AreEqualCaseInsensitive(ReplicatorSettingsParameterNames::EndpointType.c_str(), endpoint.Type))
            {
                ServiceModelEventSource::Trace->ReplicatorSettingsError(L"Invalid Endpoint Type. Replication Endpoints can only be of INTERNAL type");
                return ErrorCode(ErrorCodeValue::InvalidConfiguration);
            }

            // extract the useful values
            port = endpoint.Port;

            if (endpoint.CertificateName != NULL)
            {
                certificateName = std::move(wstring(endpoint.CertificateName));
            }

            return ErrorCode::Success();
        }
    }

    wstring errorMessage;
    StringWriter messageWriter(errorMessage);

    messageWriter.Write("No matching Endpoint Resources found in service manifest. Endpoint found are: {0}", text);
    ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);

    return ErrorCode(ErrorCodeValue::InvalidConfiguration); 
}

HRESULT ReplicatorSettings::FromConfig(
    __in IFabricCodePackageActivationContextCPtr codePackageActivationContextCPtr,
    __in wstring const & configurationPackageName,
    __in wstring const & sectionName,
    __in wstring const & hostName,
    __in wstring const & publishHostName,
    __out IFabricReplicatorSettingsResult ** replicatorSettings)
{
    // Validate code package activation context is not null
    if (codePackageActivationContextCPtr.GetRawPointer() == NULL) 
    {
        ServiceModelEventSource::Trace->ReplicatorSettingsError(L"CodePackageActivationContext is NULL");
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    
    ComPointer<IFabricConfigurationPackage> configPackageCPtr;
    HRESULT hr = codePackageActivationContextCPtr->GetConfigurationPackage(configurationPackageName.c_str(), configPackageCPtr.InitializationAddress());

    // Validate the configuration package exists
    if (hr != S_OK ||
        configPackageCPtr.GetRawPointer() == NULL)
    {
        wstring errorMessage;
        StringWriter messageWriter(errorMessage);

        messageWriter.Write("Failed to load configuration package - {0}, HRESULT = {1}", configurationPackageName, hr);
        ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
        return ComUtility::OnPublicApiReturn(hr);
    }

    ReplicatorSettingsUPtr settings;
    hr = ReplicatorSettings::ReadFromConfigurationPackage(
        codePackageActivationContextCPtr,
        configPackageCPtr,
        sectionName,
        hostName,
        publishHostName,
        settings).ToHResult();

    if (hr == S_OK)
    {
        ASSERT_IFNOT(settings, "Replicator Settings cannot be null if validation succeeded");
        return ComReplicatorSettingsResult::ReturnReplicatorSettingsResult(move(settings), replicatorSettings);
    }
    else
    {
        return ComUtility::OnPublicApiReturn(hr);
    }
}

bool ReplicatorSettings::IsReplicatorEx1FlagsUsed(DWORD const & flags)
{
    return 
        ((flags & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE) != 0 ||
        (flags & FABRIC_REPLICATOR_SECONDARY_CLEAR_ACKNOWLEDGED_OPERATIONS) != 0 ||
        (flags & FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE) != 0);
}

bool ReplicatorSettings::IsReplicatorEx2FlagsUsed(DWORD const & flags)
{
    return
        ((flags & FABRIC_REPLICATOR_USE_STREAMFAULTS_AND_ENDOFSTREAM_OPERATIONACK) != 0);
}

bool ReplicatorSettings::IsReplicatorEx3FlagsUsed(DWORD const & flags)
{
    return
        ((flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE) != 0 ||
        (flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE) != 0 ||
        (flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE) != 0 ||
        (flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE) != 0 ||
        (flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE) != 0 ||
        (flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE) != 0 ||
        (flags & FABRIC_REPLICATOR_PRIMARY_WAIT_FOR_PENDING_QUORUMS_TIMEOUT) != 0);
}

bool ReplicatorSettings::IsReplicatorEx4FlagsUsed(DWORD const & flags)
{
    return
        ((flags & FABRIC_REPLICATOR_LISTEN_ADDRESS) != 0 ||
        (flags & FABRIC_REPLICATOR_PUBLISH_ADDRESS) != 0);
}

ReplicatorSettingsUPtr ReplicatorSettings::Clone() const
{
    auto cloned = unique_ptr<ReplicatorSettings>(new ReplicatorSettings());
    *cloned = *this;
    return move(cloned);
}

ErrorCode ReplicatorSettings::MoveIfValid(
    __in ReplicatorSettingsUPtr && toBeValidated,
    __out ReplicatorSettingsUPtr & output)
{
    wstring errorMessage;
    StringWriter messageWriter(errorMessage);

    if (toBeValidated->flags_ & FABRIC_REPLICATOR_ADDRESS)
    {
        if (toBeValidated->ReplicatorAddress.empty())
        {
            ServiceModelEventSource::Trace->ReplicatorSettingsError(L"ReplicatorAddress cannot be empty if FABRIC_REPLICATOR_ADDRESS flag is enabled");
            return ErrorCodeValue::InvalidArgument;
        }
    }

    if (toBeValidated->flags_ & FABRIC_REPLICATOR_LISTEN_ADDRESS)
    {
        if (toBeValidated->ReplicatorListenAddress.empty())
        {
            ServiceModelEventSource::Trace->ReplicatorSettingsError(L"ReplicatorListenAddress cannot be empty if FABRIC_REPLICATOR_LISTEN_ADDRESS flag is enabled");
            return ErrorCodeValue::InvalidArgument;
        }
    }

    if (toBeValidated->flags_ & FABRIC_REPLICATOR_PUBLISH_ADDRESS)
    {
        if (toBeValidated->ReplicatorPublishAddress.empty())
        {
            ServiceModelEventSource::Trace->ReplicatorSettingsError(L"ReplicatorPublishAddress cannot be empty if FABRIC_REPLICATOR_PUBLISH_ADDRESS flag is enabled");
            return ErrorCodeValue::InvalidArgument;
        }
    }

    if ((toBeValidated->flags_ & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE) &&
        (toBeValidated->flags_ & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE))
    {
        if (toBeValidated->MaxReplicationQueueSize == 0 &&
            toBeValidated->MaxReplicationQueueMemorySize == 0)
        {
            // The operation queue needs some limit
            ServiceModelEventSource::Trace->ReplicatorSettingsError(L"Both MaxReplicationQueueSize and MaxReplicationQueueMemorySize cannot be 0");
            return ErrorCodeValue::InvalidArgument;
        }
    }

    if ((toBeValidated->flags_ & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE) &&
        (toBeValidated->flags_ & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE))
    {
        if (toBeValidated->MaxPrimaryReplicationQueueSize == 0 &&
            toBeValidated->MaxPrimaryReplicationQueueMemorySize == 0)
        {
            // The operation queue needs some limit
            ServiceModelEventSource::Trace->ReplicatorSettingsError(L"Both MaxPrimaryReplicationQueueSize and MaxPrimaryReplicationQueueMemorySize cannot be 0");
            return ErrorCodeValue::InvalidArgument;
        }
    }

    if ((toBeValidated->flags_ & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE) &&
        (toBeValidated->flags_ & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE))
    {
        if (toBeValidated->MaxSecondaryReplicationQueueSize == 0 &&
            toBeValidated->MaxSecondaryReplicationQueueMemorySize == 0)
        {
            // The operation queue needs some limit
            ServiceModelEventSource::Trace->ReplicatorSettingsError(L"Both MaxSecondaryReplicationQueueSize and MaxSecondaryReplicationQueueMemorySize cannot be 0");
            return ErrorCodeValue::InvalidArgument;
        }
    }

    if ((toBeValidated->flags_ & FABRIC_REPLICATOR_BATCH_ACKNOWLEDGEMENT_INTERVAL) &&
        (toBeValidated->flags_ & FABRIC_REPLICATOR_RETRY_INTERVAL))
    {
        if (toBeValidated->BatchAcknowledgementInterval >= toBeValidated->RetryInterval)
        {
            messageWriter.Write(
                "RetryInterval = {0} should be lesser than BatchAckInterval = {1}. This is to ensure we do not perform retries at the primary before giving a chance for the secondary to send an acknowledgement",
                toBeValidated->RetryInterval,
                toBeValidated->BatchAcknowledgementInterval);

            ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
            return ErrorCodeValue::InvalidArgument;
        }
    }

    if (toBeValidated->flags_ & FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE)
    {
        if (toBeValidated->InitialReplicationQueueSize <= 1 ||
            !IsPowerOf2(static_cast<int>(toBeValidated->InitialReplicationQueueSize)))
        {
            messageWriter.Write(
                "InitialReplicationQueueSize = {0} should be greater than 1 and a power of 2",
                toBeValidated->InitialReplicationQueueSize);

            ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
            return ErrorCodeValue::InvalidArgument;
        }
    }

    if (toBeValidated->flags_ & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE)
    {
        if (toBeValidated->InitialPrimaryReplicationQueueSize <= 1 ||
            !IsPowerOf2(static_cast<int>(toBeValidated->InitialPrimaryReplicationQueueSize)))
        {
            messageWriter.Write(
                "InitialPrimaryReplicationQueueSize = {0} should be greater than 1 and a power of 2",
                toBeValidated->InitialPrimaryReplicationQueueSize);

            ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
            return ErrorCodeValue::InvalidArgument;
        }
    }

    if (toBeValidated->flags_ & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE)
    {
        if (toBeValidated->InitialSecondaryReplicationQueueSize <= 1 ||
            !IsPowerOf2(static_cast<int>(toBeValidated->InitialSecondaryReplicationQueueSize)))
        {
            messageWriter.Write(
                "InitialSecondaryReplicationQueueSize = {0} should be greater than 1 and a power of 2",
                toBeValidated->InitialSecondaryReplicationQueueSize);

            ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
            return ErrorCodeValue::InvalidArgument;
        }
    }

    if ((toBeValidated->flags_ & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE) &&
        toBeValidated->MaxReplicationQueueSize > 0)
    {
        if (toBeValidated->MaxReplicationQueueSize <=1 ||
            !IsPowerOf2(static_cast<int>(toBeValidated->MaxReplicationQueueSize)))
        {
            messageWriter.Write(
                "MaxReplicationQueueSize = {0} should be greater than 1 and a power of 2",
                toBeValidated->MaxReplicationQueueSize);

            ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
            return ErrorCodeValue::InvalidArgument;
        }

        if ((toBeValidated->flags_ & FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE) &&
            toBeValidated->InitialReplicationQueueSize > toBeValidated->MaxReplicationQueueSize)
        {
            messageWriter.Write(
                "MaxReplicationQueueSize = {0} should be greater than InitialReplicationQueueSize = {1}",
                toBeValidated->MaxReplicationQueueSize,
                toBeValidated->InitialReplicationQueueSize);

            ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
            return ErrorCodeValue::InvalidArgument;
        }
    }

    if ((toBeValidated->flags_ & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE) &&
        toBeValidated->MaxPrimaryReplicationQueueSize > 0)
    {
        if (toBeValidated->MaxPrimaryReplicationQueueSize <= 1 ||
            !IsPowerOf2(static_cast<int>(toBeValidated->MaxPrimaryReplicationQueueSize)))
        {
            messageWriter.Write(
                "MaxPrimaryReplicationQueueSize = {0} should be greater than 1 and a power of 2",
                toBeValidated->MaxPrimaryReplicationQueueSize);

            ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
            return ErrorCodeValue::InvalidArgument;
        }

        if ((toBeValidated->flags_ & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE) &&
            toBeValidated->InitialPrimaryReplicationQueueSize > toBeValidated->MaxPrimaryReplicationQueueSize)
        {
            messageWriter.Write(
                "MaxPrimaryReplicationQueueSize = {0} should be greater than InitialPrimaryReplicationQueueSize = {1}",
                toBeValidated->MaxPrimaryReplicationQueueSize,
                toBeValidated->InitialPrimaryReplicationQueueSize);

            ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
            return ErrorCodeValue::InvalidArgument;
        }
    }

    if ((toBeValidated->flags_ & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE) &&
        toBeValidated->MaxSecondaryReplicationQueueSize > 0)
    {
        if (toBeValidated->MaxSecondaryReplicationQueueSize <=1 ||
            !IsPowerOf2(static_cast<int>(toBeValidated->MaxSecondaryReplicationQueueSize)))
        {
            messageWriter.Write(
                "MaxSecondaryReplicationQueueSize = {0} should be a power of 2",
                toBeValidated->MaxSecondaryReplicationQueueSize);

            ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
            return ErrorCodeValue::InvalidArgument;
        }

        if ((toBeValidated->flags_ & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE) &&
            toBeValidated->InitialSecondaryReplicationQueueSize > toBeValidated->MaxSecondaryReplicationQueueSize)
        {
            messageWriter.Write(
                "MaxSecondaryReplicationQueueSize = {0} should be greater than InitialSecondaryReplicationQueueSize = {1}",
                toBeValidated->MaxSecondaryReplicationQueueSize,
                toBeValidated->InitialSecondaryReplicationQueueSize);

            ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
            return ErrorCodeValue::InvalidArgument;
        }
    }

    if (toBeValidated->flags_ & FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE)
    {
        if (toBeValidated->InitialCopyQueueSize <= 1 ||
            !IsPowerOf2(static_cast<int>(toBeValidated->InitialCopyQueueSize)))
        {
            messageWriter.Write(
                "InitialCopyQueueSize = {0} should be greater than 1 and a power of 2",
                toBeValidated->InitialCopyQueueSize);

            ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
            return ErrorCodeValue::InvalidArgument;
        }
    }

    if (toBeValidated->flags_ & FABRIC_REPLICATOR_COPY_QUEUE_MAX_SIZE)
    {
        if ((toBeValidated->flags_ & FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE) &&
            toBeValidated->InitialCopyQueueSize > toBeValidated->MaxCopyQueueSize)
        {
            messageWriter.Write(
                "MaxCopyQueueSize = {0} should be greater than InitialCopyQueueSize = {1}",
                toBeValidated->MaxCopyQueueSize,
                toBeValidated->InitialCopyQueueSize);

            ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
            return ErrorCodeValue::InvalidArgument;
        }

        if (toBeValidated->MaxCopyQueueSize <=1 ||
            !IsPowerOf2(static_cast<int>(toBeValidated->MaxCopyQueueSize)))
        {
            messageWriter.Write(
                "MaxCopyQueueSize = {0} should be greater than 1 and a power of 2",
                toBeValidated->MaxCopyQueueSize);

            ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
            return ErrorCodeValue::InvalidArgument;
        }
    }

    if (toBeValidated->flags_ & FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE)
    {
        if ((toBeValidated->flags_ & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE) &&
            toBeValidated->MaxPrimaryReplicationQueueMemorySize > 0 &&
            toBeValidated->MaxReplicationMessageSize > toBeValidated->MaxPrimaryReplicationQueueMemorySize)
        {
            messageWriter.Write(
                "MaxPrimaryReplicationQueueMemorySize = {0} should be greater than MaxReplicationMessageSize = {1}",
                toBeValidated->MaxPrimaryReplicationQueueMemorySize,
                toBeValidated->MaxReplicationMessageSize);

            ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
            return ErrorCodeValue::InvalidArgument;
        }
        if ((toBeValidated->flags_ & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE) &&
            toBeValidated->MaxReplicationQueueMemorySize > 0 &&
            toBeValidated->MaxReplicationMessageSize > toBeValidated->MaxReplicationQueueMemorySize)
        {
            messageWriter.Write(
                "MaxReplicationQueueMemorySize = {0} should be greater than MaxReplicationMessageSize = {1}",
                toBeValidated->MaxReplicationQueueMemorySize,
                toBeValidated->MaxReplicationMessageSize);

            ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
            return ErrorCodeValue::InvalidArgument;
        }
        if ((toBeValidated->flags_ & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE) &&
            toBeValidated->MaxSecondaryReplicationQueueMemorySize > 0 &&
            toBeValidated->MaxReplicationMessageSize > toBeValidated->MaxSecondaryReplicationQueueMemorySize)
        {
            messageWriter.Write(
                "MaxSecondaryReplicationQueueMemorySize = {0} should be greater than MaxReplicationMessageSize = {1}",
                toBeValidated->MaxSecondaryReplicationQueueMemorySize,
                toBeValidated->MaxReplicationMessageSize);

            ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
            return ErrorCodeValue::InvalidArgument;
        }
    }

    output = move(toBeValidated);

    return ErrorCode();
}
