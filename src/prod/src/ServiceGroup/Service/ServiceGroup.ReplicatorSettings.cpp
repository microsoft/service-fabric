// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ServiceGroup.ReplicatorSettings.h"
#include "Common/Parse.h"
#include "ServiceGroup.Constants.h"

using namespace std;
using namespace Common;
using namespace ServiceGroup;


ReplicatorSettings::ReplicatorSettings(const Common::Guid & partitionId) :
flags_(0),
retryIntervalMilliseconds_(0),
batchAcknowledgementIntervalMilliseconds_(0),
requireServiceAck_(false),
initialReplicationQueueSize_(0),
maxReplicationQueueSize_(0),
initialCopyQueueSize_(0),
maxCopyQueueSize_(0),
maxReplicationQueueMemorySize_(0),
secondaryClearAcknowledgedOperations_(false),
maxReplicationMessageSize_(0),
partitionId_(partitionId),
useStreamFaultsAndEndOfStreamOperationAck_(false),
initialSecondaryReplicationQueueSize_(0),
maxSecondaryReplicationQueueSize_(0),
maxSecondaryReplicationQueueMemorySize_(0),
initialPrimaryReplicationQueueSize_(0),
maxPrimaryReplicationQueueSize_(0),
maxPrimaryReplicationQueueMemorySize_(0),
primaryWaitForPendingQuorumsTimeoutMilliseconds_(0)
{
}

HRESULT ReplicatorSettings::ReadFromConfigurationPackage(__in IFabricConfigurationPackage * configPackage, __in IFabricCodePackageActivationContext* activationContext)
{
	try
	{
		ASSERT_IF(configPackage == NULL, "configPackage is null");
		ASSERT_IF(activationContext == NULL, "activationContext is null");

		bool hasValue = false;

		// replicator endpoint
		wstring endpointName;
		ErrorCode error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::ReplicatorEndpoint, endpointName, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		if (hasValue)
		{
			// todo: use certificate name to initialize findValue_
			wstring certificateName;
			error = ReadSettingsFromEndpoint(activationContext, endpointName, replicatorAddress_, certificateName);
			if (!error.IsSuccess()) { return error.ToHResult(); }
			flags_ |= FABRIC_REPLICATOR_ADDRESS;
		}
		else
		{
			ServiceGroupStatefulEventSource::GetEvents().Warning_0_StatefulServiceGroupPackage(
				reinterpret_cast<uintptr_t>(this),
				partitionId_,
				ReplicatorSettingsNames::ReplicatorSettings,
				ReplicatorSettingsNames::ReplicatorEndpoint
				);
		}

		// retry interval
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::RetryIntervalMilliseconds, retryIntervalMilliseconds_, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		if (hasValue) { flags_ |= FABRIC_REPLICATOR_RETRY_INTERVAL; }

		// batch acknowledgement interval
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::BatchAcknowledgementIntervalMilliseconds, batchAcknowledgementIntervalMilliseconds_, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		if (hasValue) { flags_ |= FABRIC_REPLICATOR_BATCH_ACKNOWLEDGEMENT_INTERVAL; }

		// require service ack
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::RequireServiceAck, requireServiceAck_, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		if (hasValue) { flags_ |= FABRIC_REPLICATOR_REQUIRE_SERVICE_ACK; }

		// initial replication queue size
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::InitialReplicationQueueSize, initialReplicationQueueSize_, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		if (hasValue) { flags_ |= FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE; }

		// max replication queue size
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::MaxReplicationQueueSize, maxReplicationQueueSize_, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		if (hasValue) { flags_ |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE; }

		// initial copy queue size
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::InitialCopyQueueSize, initialCopyQueueSize_, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		if (hasValue) { flags_ |= FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE; }

		// max copy queue size
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::MaxCopyQueueSize, maxCopyQueueSize_, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		if (hasValue) { flags_ |= FABRIC_REPLICATOR_COPY_QUEUE_MAX_SIZE; }

		// max replication queue memory size
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::MaxReplicationQueueMemorySize, maxReplicationQueueMemorySize_, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		if (hasValue) { flags_ |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE; }

		// secondary clear acknowledged operations
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::SecondaryClearAcknowledgedOperations, secondaryClearAcknowledgedOperations_, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		if (hasValue) { flags_ |= FABRIC_REPLICATOR_SECONDARY_CLEAR_ACKNOWLEDGED_OPERATIONS; }

		// max replication message size
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::MaxReplicationMessageSize, maxReplicationMessageSize_, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		if (hasValue) { flags_ |= FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE; }

		// use end of stream ack 
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::UseStreamFaultsAndEndOfStreamOperationAck, useStreamFaultsAndEndOfStreamOperationAck_, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		if (hasValue)
		{
			flags_ |= FABRIC_REPLICATOR_USE_STREAMFAULTS_AND_ENDOFSTREAM_OPERATIONACK;
			if (useStreamFaultsAndEndOfStreamOperationAck_ == TRUE)
			{
				// NOTE : This config is NOT YET SUPPORTED by service groups. Fail the call as a result with a trace.
				WriteError(
					TraceSubComponentStatefulServiceGroupPackage,
					"this={0} partitionId={1} - ReplicatorSettings::UseStreamFaultsAndEndOfStreamOperationAck is not supported",
					this,
					partitionId_);

				return Common::ErrorCodeValue::InvalidConfiguration;
			}
		}

		// initial secondary queue size
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::InitialSecondaryReplicationQueueSize, initialSecondaryReplicationQueueSize_, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		if (hasValue) { flags_ |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE; }

		// max secondary queue size
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::MaxSecondaryReplicationQueueSize, maxSecondaryReplicationQueueSize_, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		if (hasValue) { flags_ |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE; }

		// max secondary queue memory size
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::MaxSecondaryReplicationQueueMemorySize, maxSecondaryReplicationQueueMemorySize_, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		if (hasValue) { flags_ |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE; }

		// initial primary queue size
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::InitialPrimaryReplicationQueueSize, initialPrimaryReplicationQueueSize_, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		if (hasValue) { flags_ |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE; }

		// max primary queue size
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::MaxPrimaryReplicationQueueSize, maxPrimaryReplicationQueueSize_, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		if (hasValue) { flags_ |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE; }

		// max primary queue memory size
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::MaxPrimaryReplicationQueueMemorySize, maxPrimaryReplicationQueueMemorySize_, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		if (hasValue) { flags_ |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE; }

		// primary pending quorums timeout
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::PrimaryWaitForPedingQuorumsTimeoutMilliseconds, primaryWaitForPendingQuorumsTimeoutMilliseconds_, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		if (hasValue) { flags_ |= FABRIC_REPLICATOR_PRIMARY_WAIT_FOR_PENDING_QUORUMS_TIMEOUT; }

		//
		// security settings
		//
		bool hasSecurity = false;

		wstring credentialType;
		wstring allowedCommonNames;
		wstring issuerThumbprints;
		wstring remoteCertThumbprints;
		wstring findType;
		wstring findValue;
		wstring findValueSecondary;
		wstring storeLocation;
		wstring storeName;
		wstring protectionLevel;

		// credential type
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::CredentialType, credentialType, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		hasSecurity |= hasValue;

		// allowed common names
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::AllowedCommonNames, allowedCommonNames, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		hasSecurity |= hasValue;

		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::IssuerThumbprints, issuerThumbprints, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		hasSecurity |= hasValue;

		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::RemoteCertThumbprints, remoteCertThumbprints, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		hasSecurity |= hasValue;

        // issuer store entries
        ComPointer<IFabricConfigurationPackage2> configPackage2CPtr;
        HRESULT hr = configPackage->QueryInterface(
            IID_IFabricConfigurationPackage2,
            configPackage2CPtr.VoidInitializationAddress());
        if (hr != S_OK)
        {
            WriteError(TraceSubComponentStatefulServiceGroupPackage, "Failed to load configuration package for IFabricConfigurationPackage2 - HRESULT = {0}", hr);
            return ComUtility::OnPublicApiReturn(hr);
        }

        hasValue = false;
        std::map<wstring, wstring> issuerStores;
        error = ReadSettingsValue(configPackage2CPtr.GetRawPointer(), ReplicatorSettingsNames::ApplicationIssuerStorePrefix, issuerStores, hasValue);
        if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }
        hasSecurity |= hasValue;

		// find type
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::FindType, findType, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		hasSecurity |= hasValue;

		// find value
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::FindValue, findValue, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		hasSecurity |= hasValue;

		// find value secondary
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::FindValueSecondary, findValueSecondary, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		hasSecurity |= hasValue;

		// store location
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::StoreLocation, storeLocation, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		hasSecurity |= hasValue;

		// store name
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::StoreName, storeName, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		hasSecurity |= hasValue;

		// protection level
		hasValue = false;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::ProtectionLevel, protectionLevel, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		hasSecurity |= hasValue;

		// service principal name
		wstring servicePrincipalName;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::ServicePrincipalName, servicePrincipalName, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		hasSecurity |= hasValue;

		// Windows identity list of all replicas
		wstring windowsIdentities;
		error = ReadSettingsValue(configPackage, ReplicatorSettingsNames::WindowsIdentities, windowsIdentities, hasValue);
		if (!error.IsSuccess()) { return error.ToHResult(); }
		hasSecurity |= hasValue;

		if (hasSecurity)
		{
			error = Transport::SecuritySettings::FromConfiguration(
				credentialType,
				storeName,
				storeLocation,
				findType,
				findValue,
				findValueSecondary,
				protectionLevel,
				remoteCertThumbprints,
				SecurityConfig::X509NameMap(), // todo, add support X509Name config section in app config
                SecurityConfig::IssuerStoreKeyValueMap::Parse(issuerStores),
				allowedCommonNames,
				issuerThumbprints,
				servicePrincipalName,
				windowsIdentities,
				securitySettings_);

			if (!error.IsSuccess())
			{
				ServiceGroupStatefulEventSource::GetEvents().Error_0_StatefulServiceGroupPackage(error.ToHResult());

				return error.ToHResult();
			}

			flags_ |= FABRIC_REPLICATOR_SECURITY;
		}

		if (flags_ != 0)
		{
			settings_ = heap_.AddItem<FABRIC_REPLICATOR_SETTINGS>();
			ToPublicApi(heap_, *settings_);

			std::wstring asText;
			Common::StringWriter writer(asText);
			WriteTo(writer);
			ServiceGroupStatefulEventSource::GetEvents().Info_0_StatefulServiceGroupPackage(
				reinterpret_cast<uintptr_t>(this),
				partitionId_,
				settings_->Flags,
				asText
				);
		}
		else
		{
			ServiceGroupStatefulEventSource::GetEvents().Info_1_StatefulServiceGroupPackage(
				reinterpret_cast<uintptr_t>(this),
				partitionId_
				);
		}
	}
	catch (...)
	{
		return FABRIC_E_INVALID_CONFIGURATION;
	}

	return S_OK;
}

void ReplicatorSettings::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_REPLICATOR_SETTINGS & replicatorSettings)
{
	replicatorSettings.Flags = flags_;

	replicatorSettings.ReplicatorAddress = heap.AddString(replicatorAddress_);

	replicatorSettings.BatchAcknowledgementIntervalMilliseconds = batchAcknowledgementIntervalMilliseconds_;
	replicatorSettings.RetryIntervalMilliseconds = retryIntervalMilliseconds_;

	replicatorSettings.RequireServiceAck = requireServiceAck_;

	replicatorSettings.InitialCopyQueueSize = initialCopyQueueSize_;
	replicatorSettings.MaxCopyQueueSize = maxCopyQueueSize_;
	replicatorSettings.InitialReplicationQueueSize = initialReplicationQueueSize_;
	replicatorSettings.MaxReplicationQueueSize = maxReplicationQueueSize_;
	replicatorSettings.Reserved = NULL;

	if ((flags_ & FABRIC_REPLICATOR_SECURITY) == FABRIC_REPLICATOR_SECURITY)
	{
		Common::ReferencePointer<FABRIC_SECURITY_CREDENTIALS> securityCredentials = heap.AddItem<FABRIC_SECURITY_CREDENTIALS>();
		replicatorSettings.SecurityCredentials = securityCredentials.GetRawPointer();
		securitySettings_.ToPublicApi(heap, *securityCredentials);
	}
	else
	{
		replicatorSettings.SecurityCredentials = NULL;
	}

	if (Reliability::ReplicationComponent::ReplicatorSettings::IsReplicatorEx1FlagsUsed(flags_))
	{
		ex1Settings_ = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX1>();

		ex1Settings_->MaxReplicationQueueMemorySize = maxReplicationQueueMemorySize_;
		ex1Settings_->SecondaryClearAcknowledgedOperations = secondaryClearAcknowledgedOperations_;
		ex1Settings_->MaxReplicationMessageSize = maxReplicationMessageSize_;
		ex1Settings_->Reserved = NULL;

		replicatorSettings.Reserved = ex1Settings_.GetRawPointer();
	}

	if (Reliability::ReplicationComponent::ReplicatorSettings::IsReplicatorEx2FlagsUsed(flags_))
	{
		// If only ex2 settings are used, we still need to allocate ex1 struct
		if (ex1Settings_.GetRawPointer() == nullptr)
		{
			ex1Settings_ = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX1>();
			replicatorSettings.Reserved = ex1Settings_.GetRawPointer();
		}

		ex2Settings_ = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX2>();

		ex2Settings_->UseStreamFaultsAndEndOfStreamOperationAck = useStreamFaultsAndEndOfStreamOperationAck_;
		ex2Settings_->Reserved = NULL;

		ex1Settings_->Reserved = ex2Settings_.GetRawPointer();
	}

	if (Reliability::ReplicationComponent::ReplicatorSettings::IsReplicatorEx3FlagsUsed(flags_))
	{
		// If only ex3 settings are used, we still need to allocate ex1 and ex2 struct if they are null
		if (ex1Settings_.GetRawPointer() == nullptr)
		{
			ex1Settings_ = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX1>();
			replicatorSettings.Reserved = ex1Settings_.GetRawPointer();
		}

		if (ex2Settings_.GetRawPointer() == nullptr)
		{
			ex2Settings_ = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX2>();
			ex1Settings_->Reserved = ex2Settings_.GetRawPointer();
		}

		ex3Settings_ = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX3>();
		ex3Settings_->InitialSecondaryReplicationQueueSize = initialSecondaryReplicationQueueSize_;
		ex3Settings_->MaxSecondaryReplicationQueueSize = maxSecondaryReplicationQueueSize_;
		ex3Settings_->MaxSecondaryReplicationQueueMemorySize = maxSecondaryReplicationQueueMemorySize_;
		ex3Settings_->InitialPrimaryReplicationQueueSize = initialPrimaryReplicationQueueSize_;
		ex3Settings_->MaxPrimaryReplicationQueueSize = maxPrimaryReplicationQueueSize_;
		ex3Settings_->MaxPrimaryReplicationQueueMemorySize = maxPrimaryReplicationQueueMemorySize_;
		ex3Settings_->PrimaryWaitForPendingQuorumsTimeoutMilliseconds = primaryWaitForPendingQuorumsTimeoutMilliseconds_;

		ex3Settings_->Reserved = NULL;

		ex2Settings_->Reserved = ex3Settings_.GetRawPointer();
	}

	if (Reliability::ReplicationComponent::ReplicatorSettings::IsReplicatorEx4FlagsUsed(flags_))
	{
		// If only ex4 settings are used, we still need to allocate ex1 and ex2 and ex3 struct if they are null
		if (ex1Settings_.GetRawPointer() == nullptr)
		{
			ex1Settings_ = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX1>();
			replicatorSettings.Reserved = ex1Settings_.GetRawPointer();
		}

		if (ex2Settings_.GetRawPointer() == nullptr)
		{
			ex2Settings_ = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX2>();
			ex1Settings_->Reserved = ex2Settings_.GetRawPointer();
		}

		if (ex3Settings_.GetRawPointer() == nullptr)
		{
			ex3Settings_ = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX3>();
			ex2Settings_->Reserved = ex3Settings_.GetRawPointer();
		}

		ex4Settings_ = heap.AddItem<FABRIC_REPLICATOR_SETTINGS_EX4>();
		ex4Settings_->ReplicatorListenAddress = heap.AddString(replicatorListenAddress_);
		ex4Settings_->ReplicatorPublishAddress = heap.AddString(replicatorPublishAddress_);

		ex4Settings_->Reserved = NULL;

		ex3Settings_->Reserved = ex4Settings_.GetRawPointer();
	}
}

FABRIC_REPLICATOR_SETTINGS * ReplicatorSettings::GetRawPointer()
{
	return settings_.GetRawPointer();
}

Common::ErrorCode ReplicatorSettings::ReadSettingsValue(
	__in IFabricConfigurationPackage * configPackage,
	__in std::wstring const & name,
	__out wstring & value,
	__out bool & hasValue)
{
	LPCWSTR configValue = nullptr;
	BOOLEAN isEncrypted = FALSE;

	HRESULT hr = configPackage->GetValue(ReplicatorSettingsNames::ReplicatorSettings.c_str(), name.c_str(), &isEncrypted, &configValue);

	if (hr == FABRIC_E_CONFIGURATION_SECTION_NOT_FOUND || hr == FABRIC_E_CONFIGURATION_PARAMETER_NOT_FOUND)
	{
		hasValue = false;
		return ErrorCode::Success();
	}

	ASSERT_IF(isEncrypted == TRUE, "ReplicatorSettings value cannot be encrypted");

	if (FAILED(hr))
	{
		ServiceGroupStatefulEventSource::GetEvents().Error_1_StatefulServiceGroupPackage(
			reinterpret_cast<uintptr_t>(this),
			partitionId_,
			name,
			hr
			);

		return ErrorCode::FromHResult(hr);
	}

	value = wstring(configValue);
	hasValue = true;

	return ErrorCode::Success();
}
ErrorCode ServiceGroup::ReplicatorSettings::ReadSettingsValue(
    __in IFabricConfigurationPackage2 * configPackage,
    __in std::wstring const & paramPrefix,
    __out std::map<wstring, wstring> & values,
    __out bool & hasValue)
{
    FABRIC_CONFIGURATION_PARAMETER_LIST *configValues = nullptr;

    HRESULT hr = configPackage->GetValues(ReplicatorSettingsNames::ReplicatorSettings.c_str(), paramPrefix.c_str(), &configValues);

    if (hr == FABRIC_E_CONFIGURATION_SECTION_NOT_FOUND || hr == FABRIC_E_CONFIGURATION_PARAMETER_NOT_FOUND)
    {
        hasValue = false;
        return ErrorCode::Success();
    }

    if (FAILED(hr))
    {
        ServiceGroupStatefulEventSource::GetEvents().Error_1_StatefulServiceGroupPackage(
            reinterpret_cast<uintptr_t>(this),
            partitionId_,
            paramPrefix,
            hr
        );

        return ErrorCode::FromHResult(hr);
    }

    for (size_t ix = 0; ix < configValues->Count; ++ix)
    {
        ASSERT_IF(configValues->Items[ix].IsEncrypted == TRUE, "ReplicatorSettings value cannot be encrypted");

        wstring parameterNameWithoutPrefix = wstring(configValues->Items[ix].Name).substr(paramPrefix.size());
        values.insert(std::pair<wstring, wstring>(parameterNameWithoutPrefix, wstring(configValues->Items[ix].Value)));
    }

    hasValue = true;
    return ErrorCode::Success();
}

Common::ErrorCode ReplicatorSettings::ReadSettingsValue(
	__in IFabricConfigurationPackage * configPackage,
	__in std::wstring const & name,
	__out DWORD & value,
	__out bool & hasValue)
{
	wstring configValue;
	hasValue = false;
	value = 0;

	ErrorCode error = ReadSettingsValue(configPackage, name, configValue, hasValue);
	if (!error.IsSuccess()) { return error; }
	if (!hasValue) { return error; }

	__int64 rawValue = 0;
	if (!TryParseInt64(configValue, rawValue))
	{
		ServiceGroupStatefulEventSource::GetEvents().Error_2_StatefulServiceGroupPackage(
			reinterpret_cast<uintptr_t>(this),
			partitionId_,
			name
			);

		return ErrorCode(ErrorCodeValue::InvalidConfiguration);
	}

	if (rawValue < 0 || rawValue > 0xFFFFFFFF)
	{
		ServiceGroupStatefulEventSource::GetEvents().Error_3_StatefulServiceGroupPackage(
			reinterpret_cast<uintptr_t>(this),
			partitionId_,
			name
			);

		return ErrorCode(ErrorCodeValue::InvalidConfiguration);
	}

	value = static_cast<DWORD>(rawValue);
	hasValue = true;

	return ErrorCode::Success();
}

Common::ErrorCode ReplicatorSettings::ReadSettingsValue(
	__in IFabricConfigurationPackage * configPackage,
	__in std::wstring const & name,
	__out BOOLEAN & value,
	__out bool & hasValue)
{
	wstring configValue;
	hasValue = false;
	value = FALSE;

	ErrorCode error = ReadSettingsValue(configPackage, name, configValue, hasValue);
	if (!error.IsSuccess()) { return error; }
	if (!hasValue) { return error; }

	StringUtility::TrimWhitespaces(configValue);
	StringUtility::ToLower(configValue);

	if (configValue == L"true")
	{
		value = TRUE;
		hasValue = true;
	}
	else if (configValue == L"false")
	{
		value = FALSE;
		hasValue = true;
	}
	else
	{
		ServiceGroupStatefulEventSource::GetEvents().Error_4_StatefulServiceGroupPackage(
			reinterpret_cast<uintptr_t>(this),
			partitionId_,
			name
			);

		return ErrorCode(ErrorCodeValue::InvalidConfiguration);
	}

	return ErrorCode::Success();
}

Common::ErrorCode ReplicatorSettings::ReadSettingsFromEndpoint(
	__in IFabricCodePackageActivationContext * activationContext,
	__in std::wstring & endpointName,
	__out wstring & endpointAddress,
	__out wstring & certificateName)
{
	ASSERT_IF(activationContext == NULL, "activationContext is null");

	USHORT port = 0;

	ErrorCode error = GetEndpointResourceFromManifest(activationContext, endpointName, port, certificateName);
	if (!error.IsSuccess()) { return error; }

	Common::ComPointer<IFabricNodeContextResult2> nodeContext;
	HRESULT hr = Hosting2::ApplicationHostContainer::FabricGetNodeContext(nodeContext.VoidInitializationAddress());
	if (hr != S_OK)
	{
		WriteError(
			TraceSubComponentStatefulServiceGroupPackage,
			"this={0} - failed to get node context",
			this);

		return ErrorCodeValue::InvalidConfiguration;
	}

	const FABRIC_NODE_CONTEXT * fabricNodeContext = nodeContext->get_NodeContext();

	WriteNoise(
		TraceSubComponentStatefulServiceGroupPackage,
		"this={0} partitionId={1} - TcpTransportUtility::GetLocalFqdn succeeded, hostName = {2}",
		this,
		partitionId_,
		fabricNodeContext->IPAddressOrFQDN
		);

	endpointAddress = Transport::TcpTransportUtility::ConstructAddressString(fabricNodeContext->IPAddressOrFQDN, port);

	WriteNoise(
		TraceSubComponentStatefulServiceGroupPackage,
		"this={0} partitionId={1} - Using endpoint {2} as replicator address {3}",
		this,
		partitionId_,
		endpointName,
		endpointAddress);

	return ErrorCode::Success();
}

Common::ErrorCode ReplicatorSettings::GetEndpointResourceFromManifest(
	__in IFabricCodePackageActivationContext * activationContext,
	__in std::wstring & endpointName,
	__out USHORT & port,
	__out std::wstring & certificateName)
{
	ASSERT_IF(NULL == activationContext, "activationContext is null");

	const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST * endpoints = activationContext->get_ServiceEndpointResources();

	if (endpoints == NULL ||
		endpoints->Count == 0 ||
		endpoints->Items == NULL)
	{
		ServiceGroupStatefulEventSource::GetEvents().Error_6_StatefulServiceGroupPackage(
			reinterpret_cast<uintptr_t>(this),
			partitionId_
			);

		return ErrorCode(ErrorCodeValue::InvalidConfiguration);
	}

	// find endpoint by name
	for (ULONG i = 0; i < endpoints->Count; ++i)
	{
		const auto & endpoint = endpoints->Items[i];

		ASSERT_IF(endpoint.Name == NULL, "FABRIC_ENDPOINT_RESOURCE_DESCRIPTION with Name == NULL");

		if (StringUtility::AreEqualCaseInsensitive(endpointName.c_str(), endpoint.Name))
		{
			// check protocol is TCP
			if (endpoint.Protocol == NULL ||
				!StringUtility::AreEqualCaseInsensitive(ReplicatorSettingsConstants::ProtocolScheme.c_str(), endpoint.Protocol))
			{
				ServiceGroupStatefulEventSource::GetEvents().Error_7_StatefulServiceGroupPackage(
					reinterpret_cast<uintptr_t>(this),
					partitionId_,
					ReplicatorSettingsConstants::ProtocolScheme
					);

				return ErrorCode(ErrorCodeValue::InvalidConfiguration);
			}

			// check type is Internal
			if (endpoint.Type == NULL ||
				!StringUtility::AreEqualCaseInsensitive(ReplicatorSettingsConstants::EndpointType.c_str(), endpoint.Type))
			{
				ServiceGroupStatefulEventSource::GetEvents().Error_8_StatefulServiceGroupPackage(
					reinterpret_cast<uintptr_t>(this),
					partitionId_,
					ReplicatorSettingsConstants::EndpointType
					);

				return ErrorCode(ErrorCodeValue::InvalidConfiguration);
			}

            // todo, this range checking should be moved to activationContext->get_ServiceEndpointResources or
            // somewhere closer to public API, which takes port number as uint, incorrect type that cannot be fixed.
            if (endpoint.Port > std::numeric_limits<USHORT>::max())
            {
                WriteError(
                    TraceSubComponentStatefulServiceGroupPackage,
                    "this={0} port number {1} is beyond ushort range",
                    this,
                    endpoint.Port);

                return ErrorCode(ErrorCodeValue::InvalidConfiguration);
            }

			port = (USHORT)endpoint.Port;

			if (endpoint.CertificateName != NULL)
			{
				certificateName = std::move(wstring(endpoint.CertificateName));
			}

			return ErrorCode::Success();
		}
	}

	// if we get here, no endpoint with the specified name is in the manifest
	ServiceGroupStatefulEventSource::GetEvents().Error_9_StatefulServiceGroupPackage(
		reinterpret_cast<uintptr_t>(this),
		partitionId_,
		endpointName
		);

	return ErrorCode(ErrorCodeValue::InvalidConfiguration);
}

void ReplicatorSettings::WriteTo(__in Common::TextWriter & w) const
{
	w.Write("ReplicatorSettings { ");

	if (flags_ & FABRIC_REPLICATOR_RETRY_INTERVAL)
	{
		w.Write("{0} = {1}, ", ReplicatorSettingsNames::RetryIntervalMilliseconds, retryIntervalMilliseconds_);
	}
	else
	{
		w.Write("{0} = default, ", ReplicatorSettingsNames::RetryIntervalMilliseconds);
	}

	if (flags_ & FABRIC_REPLICATOR_BATCH_ACKNOWLEDGEMENT_INTERVAL)
	{
		w.Write("{0} = {1}, ", ReplicatorSettingsNames::BatchAcknowledgementIntervalMilliseconds, batchAcknowledgementIntervalMilliseconds_);
	}
	else
	{
		w.Write("{0} = default, ", ReplicatorSettingsNames::BatchAcknowledgementIntervalMilliseconds);
	}

	if (flags_ & FABRIC_REPLICATOR_REQUIRE_SERVICE_ACK)
	{
		w.Write("{0} = {1}, ", ReplicatorSettingsNames::RequireServiceAck, requireServiceAck_);
	}
	else
	{
		w.Write("{0} = default, ", ReplicatorSettingsNames::RequireServiceAck);
	}

	if (flags_ & FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE)
	{
		w.Write("{0} = {1}, ", ReplicatorSettingsNames::InitialReplicationQueueSize, initialReplicationQueueSize_);
	}
	else
	{
		w.Write("{0} = default, ", ReplicatorSettingsNames::InitialReplicationQueueSize);
	}

	if (flags_ & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE)
	{
		w.Write("{0} = {1}, ", ReplicatorSettingsNames::MaxReplicationQueueSize, maxReplicationQueueSize_);
	}
	else
	{
		w.Write("{0} = default, ", ReplicatorSettingsNames::MaxReplicationQueueSize);
	}

	if (flags_ & FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE)
	{
		w.Write("{0} = {1}, ", ReplicatorSettingsNames::InitialCopyQueueSize, initialCopyQueueSize_);
	}
	else
	{
		w.Write("{0} = default, ", ReplicatorSettingsNames::InitialCopyQueueSize);
	}

	if (flags_ & FABRIC_REPLICATOR_COPY_QUEUE_MAX_SIZE)
	{
		w.Write("{0} = {1}, ", ReplicatorSettingsNames::MaxCopyQueueSize, maxCopyQueueSize_);
	}
	else
	{
		w.Write("{0} = default, ", ReplicatorSettingsNames::MaxCopyQueueSize);
	}

	if (flags_ & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE)
	{
		w.Write("{0} = {1}, ", ReplicatorSettingsNames::MaxReplicationQueueMemorySize, maxReplicationQueueMemorySize_);
	}
	else
	{
		w.Write("{0} = default, ", ReplicatorSettingsNames::MaxReplicationQueueMemorySize);
	}

	if (flags_ & FABRIC_REPLICATOR_SECONDARY_CLEAR_ACKNOWLEDGED_OPERATIONS)
	{
		w.Write("{0} = {1}, ", ReplicatorSettingsNames::SecondaryClearAcknowledgedOperations, secondaryClearAcknowledgedOperations_);
	}
	else
	{
		w.Write("{0} = default, ", ReplicatorSettingsNames::SecondaryClearAcknowledgedOperations);
	}

	if (flags_ & FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE)
	{
		w.Write("{0} = {1}, ", ReplicatorSettingsNames::MaxReplicationMessageSize, maxReplicationMessageSize_);
	}
	else
	{
		w.Write("{0} = default, ", ReplicatorSettingsNames::MaxReplicationMessageSize);
	}

	if (flags_ & FABRIC_REPLICATOR_USE_STREAMFAULTS_AND_ENDOFSTREAM_OPERATIONACK)
	{
		w.Write("{0} = {1}, ", ReplicatorSettingsNames::UseStreamFaultsAndEndOfStreamOperationAck, useStreamFaultsAndEndOfStreamOperationAck_);
	}
	else
	{
		w.Write("{0} = default, ", ReplicatorSettingsNames::UseStreamFaultsAndEndOfStreamOperationAck);
	}

	if (flags_ & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE)
	{
		w.Write("{0} = {1}, ", ReplicatorSettingsNames::InitialSecondaryReplicationQueueSize, initialSecondaryReplicationQueueSize_);
	}
	else
	{
		w.Write("{0} = default, ", ReplicatorSettingsNames::InitialSecondaryReplicationQueueSize);
	}

	if (flags_ & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE)
	{
		w.Write("{0} = {1}, ", ReplicatorSettingsNames::MaxSecondaryReplicationQueueSize, maxSecondaryReplicationQueueSize_);
	}
	else
	{
		w.Write("{0} = default, ", ReplicatorSettingsNames::MaxSecondaryReplicationQueueSize);
	}

	if (flags_ & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE)
	{
		w.Write("{0} = {1}, ", ReplicatorSettingsNames::MaxSecondaryReplicationQueueMemorySize, maxSecondaryReplicationQueueMemorySize_);
	}
	else
	{
		w.Write("{0} = default, ", ReplicatorSettingsNames::MaxSecondaryReplicationQueueMemorySize);
	}

	if (flags_ & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE)
	{
		w.Write("{0} = {1}, ", ReplicatorSettingsNames::InitialPrimaryReplicationQueueSize, initialPrimaryReplicationQueueSize_);
	}
	else
	{
		w.Write("{0} = default, ", ReplicatorSettingsNames::InitialPrimaryReplicationQueueSize);
	}

	if (flags_ & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE)
	{
		w.Write("{0} = {1}, ", ReplicatorSettingsNames::MaxPrimaryReplicationQueueSize, maxPrimaryReplicationQueueSize_);
	}
	else
	{
		w.Write("{0} = default, ", ReplicatorSettingsNames::MaxPrimaryReplicationQueueSize);
	}

	if (flags_ & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE)
	{
		w.Write("{0} = {1}, ", ReplicatorSettingsNames::MaxPrimaryReplicationQueueMemorySize, maxPrimaryReplicationQueueMemorySize_);
	}
	else
	{
		w.Write("{0} = default, ", ReplicatorSettingsNames::MaxPrimaryReplicationQueueMemorySize);
	}

	if (flags_ & FABRIC_REPLICATOR_PRIMARY_WAIT_FOR_PENDING_QUORUMS_TIMEOUT)
	{
		w.Write("{0} = {1}, ", ReplicatorSettingsNames::PrimaryWaitForPedingQuorumsTimeoutMilliseconds, primaryWaitForPendingQuorumsTimeoutMilliseconds_);
	}
	else
	{
		w.Write("{0} = default, ", ReplicatorSettingsNames::PrimaryWaitForPedingQuorumsTimeoutMilliseconds);
	}

	w.Write("SecuritySettings = {0} ", securitySettings_);

	w.Write("}");
}
