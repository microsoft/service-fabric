// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <string>

#include "FabricCommon.h"
#include "FabricRuntime.h"

#include "Common/Common.h"
#include "Transport/Transport.h"

namespace ServiceGroup
{
    //
    // Names for ReplicatorSettings config parameters
    //
    namespace ReplicatorSettingsNames
    {
        // name of the config section containing the replicator settings
        std::wstring const ReplicatorSettings = L"ReplicatorSettings";

        std::wstring const RetryIntervalMilliseconds = L"RetryIntervalMilliseconds";
        std::wstring const BatchAcknowledgementIntervalMilliseconds = L"BatchAcknowledgementIntervalMilliseconds";
        std::wstring const ReplicatorEndpoint = L"ReplicatorEndpoint";
        std::wstring const RequireServiceAck = L"RequireServiceAck";
        std::wstring const InitialReplicationQueueSize = L"InitialReplicationQueueSize";
        std::wstring const MaxReplicationQueueSize = L"MaxReplicationQueueSize";
        std::wstring const InitialCopyQueueSize = L"InitialCopyQueueSize";
        std::wstring const MaxCopyQueueSize = L"MaxCopyQueueSize";
        std::wstring const SecondaryClearAcknowledgedOperations = L"SecondaryClearAcknowledgedOperations";
        std::wstring const MaxReplicationQueueMemorySize = L"MaxReplicationQueueMemorySize";
        std::wstring const MaxReplicationMessageSize = L"MaxReplicationMessageSize"; 
        std::wstring const InitialSecondaryReplicationQueueSize = L"InitialSecondaryReplicationQueueSize";
        std::wstring const MaxSecondaryReplicationQueueSize = L"MaxSecondaryReplicationQueueSize";
        std::wstring const MaxSecondaryReplicationQueueMemorySize = L"MaxSecondaryReplicationQueueMemorySize";
        std::wstring const InitialPrimaryReplicationQueueSize = L"InitialPrimaryReplicationQueueSize";
        std::wstring const MaxPrimaryReplicationQueueSize = L"MaxPrimaryReplicationQueueSize";
        std::wstring const MaxPrimaryReplicationQueueMemorySize = L"MaxPrimaryReplicationQueueMemorySize";
        std::wstring const UseStreamFaultsAndEndOfStreamOperationAck = L"UseStreamFaultsAndEndOfStreamOperationAck";
        std::wstring const PrimaryWaitForPedingQuorumsTimeoutMilliseconds = L"PrimaryWaitForPedingQuorumsTimeoutMilliseconds";

        std::wstring const SecurityCredentials = L"SecurityCredentials";

        std::wstring const CredentialType = L"CredentialType";

        std::wstring const AllowedCommonNames = L"AllowedCommonNames";
        std::wstring const IssuerThumbprints = L"IssuerThumbprints";
        std::wstring const RemoteCertThumbprints = L"RemoteCertThumbprints";
        std::wstring const FindType = L"FindType";
        std::wstring const FindValue = L"FindValue";
        std::wstring const FindValueSecondary = L"FindValueSecondary";
        std::wstring const StoreLocation = L"StoreLocation";
        std::wstring const StoreName = L"StoreName";
        std::wstring const ProtectionLevel = L"ProtectionLevel";
        std::wstring const ApplicationIssuerStorePrefix = L"ApplicationIssuerStore/";

        // Kerberos transport security settings:
        std::wstring const ServicePrincipalName = L"ServicePrincipalName"; // Service principal name of all replication listeners for this service group
        std::wstring const WindowsIdentities = L"WindowsIdentities"; // Used to authorize incoming replication connections for this service group
    }

    namespace ReplicatorSettingsConstants
    {
        std::wstring const EndpointType = L"Internal";
        std::wstring const ProtocolScheme = L"tcp";
    }

    class ReplicatorSettings : public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupStateful>
    {
    public:

        ReplicatorSettings(const Common::Guid & partitionId);

        HRESULT ReadFromConfigurationPackage(__in IFabricConfigurationPackage * configPackage, __in IFabricCodePackageActivationContext * activationContext);

        FABRIC_REPLICATOR_SETTINGS* GetRawPointer();

        void WriteTo(__in Common::TextWriter & w) const;
        
    private:

        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_REPLICATOR_SETTINGS & replicatorSettings);

        Common::ErrorCode ReadSettingsValue(
            __in IFabricConfigurationPackage * configPackage, 
            __in std::wstring const & name, 
            __out std::wstring & value, 
            __out bool & hasValue);

        Common::ErrorCode ReadSettingsValue(
            __in IFabricConfigurationPackage * configPackage, 
            __in std::wstring const & name, 
            __out DWORD & value, 
            __out bool & hasValue);

        Common::ErrorCode ReadSettingsValue(
            __in IFabricConfigurationPackage * configPackage, 
            __in std::wstring const & name, 
            __out BOOLEAN & value, 
            __out bool & hasValue);

        Common::ErrorCode ReadSettingsValue(
            __in IFabricConfigurationPackage2 * configPackage,
            __in std::wstring const & paramPrefix,
            __out std::map<wstring, wstring> & values,
            __out bool & hasValue);

        Common::ErrorCode ReadSettingsFromEndpoint(
            __in IFabricCodePackageActivationContext * activationContext,
            __in std::wstring & endpointName,
            __out std::wstring & endpointAddress,
            __out std::wstring & certificateName);

        Common::ErrorCode GetEndpointResourceFromManifest(
            __in IFabricCodePackageActivationContext * activationContext,
            __in std::wstring & endpointName,
            __out USHORT & port,
            __out std::wstring & certificateName);

        DWORD flags_;
        DWORD retryIntervalMilliseconds_;
        DWORD batchAcknowledgementIntervalMilliseconds_;
        std::wstring replicatorAddress_;
        std::wstring replicatorListenAddress_;
        std::wstring replicatorPublishAddress_;
        BOOLEAN requireServiceAck_;
        DWORD initialReplicationQueueSize_;
        DWORD maxReplicationQueueSize_;
        DWORD initialSecondaryReplicationQueueSize_;
        DWORD initialPrimaryReplicationQueueSize_;
        DWORD maxReplicationQueueMemorySize_;
        DWORD maxSecondaryReplicationQueueSize_;
        DWORD maxPrimaryReplicationQueueSize_;
        DWORD maxSecondaryReplicationQueueMemorySize_;
        DWORD maxPrimaryReplicationQueueMemorySize_;
        DWORD initialCopyQueueSize_;
        DWORD maxCopyQueueSize_;
        BOOLEAN secondaryClearAcknowledgedOperations_;
        DWORD maxReplicationMessageSize_;
        BOOLEAN useStreamFaultsAndEndOfStreamOperationAck_;
        DWORD primaryWaitForPendingQuorumsTimeoutMilliseconds_;
        
        Transport::SecuritySettings securitySettings_;

        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_REPLICATOR_SETTINGS> settings_;
        Common::ReferencePointer<FABRIC_REPLICATOR_SETTINGS_EX1> ex1Settings_;
        Common::ReferencePointer<FABRIC_REPLICATOR_SETTINGS_EX2> ex2Settings_;
        Common::ReferencePointer<FABRIC_REPLICATOR_SETTINGS_EX3> ex3Settings_;
        Common::ReferencePointer<FABRIC_REPLICATOR_SETTINGS_EX4> ex4Settings_;

        // used for tracing
        const Common::Guid partitionId_;
    };
}
