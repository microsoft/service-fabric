// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        namespace ReplicatorSettingsParameterNames
        {
            std::wstring const ReplicatorEndpoint = L"ReplicatorEndpoint";

            std::wstring const ProtocolScheme = L"TCP";
            std::wstring const EndpointType = L"Internal";
        }

        class ReplicatorSettings;
        typedef std::unique_ptr<ReplicatorSettings> ReplicatorSettingsUPtr;

        class ReplicatorSettings
        {
        public:
            // Factory method that loads the settings from configuration package.
            // All replicator settings, except for security settings are loaded.
            // For loading security settings, use similar factory method defined in transport
            static HRESULT FromConfig(
                __in Common::ComPointer<IFabricCodePackageActivationContext> codePackageActivationContextCPtr,
                __in std::wstring const & configurationPackageName,
                __in std::wstring const & sectionName,
                __in std::wstring const & hostName,
                __in std::wstring const & publishHostName,
                __out IFabricReplicatorSettingsResult ** replicatorSettings);

             // Used by internal services that load replicator settings from the config
            static Common::ErrorCode FromConfig(
                __in REConfigBase const& reConfig,
                __in std::wstring const& replicatorAddress,
                __in std::wstring const& replicatorListenAddress,
                __in std::wstring const& replicatorPublishAddress,
                __in Transport::SecuritySettings const& securitySettings,
                __out ReplicatorSettingsUPtr & output);

            static Common::ErrorCode FromPublicApi(
                __in FABRIC_REPLICATOR_SETTINGS const& replicatorSettings,
                __out ReplicatorSettingsUPtr & output);

            // Primarily Used during UpdateReplicatorSettings call of internal components which allows for only changes in the security settings
            static Common::ErrorCode FromSecuritySettings(
                __in Transport::SecuritySettings const& securitySettings,
                __out ReplicatorSettingsUPtr & output);

            static bool IsReplicatorEx1FlagsUsed(DWORD const & flags);

            static bool IsReplicatorEx2FlagsUsed(DWORD const & flags);

            static bool IsReplicatorEx3FlagsUsed(DWORD const & flags);

            static bool IsReplicatorEx4FlagsUsed(DWORD const & flags);

            DECLARE_RE_OVERRIDABLE_SETTINGS_PROPERTIES()

            __declspec(property(get=get_Flags)) DWORD Flags ;
            DWORD get_Flags() const
            {
                return flags_;
            }

            __declspec(property(get=get_SecuritySettings)) Transport::SecuritySettings const & SecuritySettings ;
            Transport::SecuritySettings const & get_SecuritySettings() const
            {
                return securitySettings_;
            }

            void ToPublicApi(
                __in Common::ScopedHeap & heap,
                __out FABRIC_REPLICATOR_SETTINGS & replicatorSettings) const;

            ReplicatorSettingsUPtr Clone() const;

        private:

            ReplicatorSettings();
            ReplicatorSettings(ReplicatorSettings const&);

            static Common::ErrorCode Validate(ReplicatorSettingsUPtr const & object);

            static Common::ErrorCode ReadSettingsFromEndpoint(
                __in Common::ComPointer<IFabricCodePackageActivationContext> codePackageActivationContextCPtr,
                __in std::wstring & endpointName,
                __in std::wstring const & hostName,
                __out std::wstring & endpointAddress,
                __out std::wstring & certificateName);

            static Common::ErrorCode GetEndpointResourceFromManifest(
                __in Common::ComPointer<IFabricCodePackageActivationContext> codePackageActivationContextCPtr,
                __in std::wstring & endpointName,
                __out ULONG & port,
                __out std::wstring & certificateName);

            static Common::ErrorCode ReadFromConfigurationPackage(
                __in Common::ComPointer<IFabricCodePackageActivationContext> codePackageActivationContextCPtr,
                __in Common::ComPointer<IFabricConfigurationPackage> configPackageCPtr,
                __in std::wstring const & sectionName,
                __in std::wstring const & hostName,
                __in std::wstring const & publishHostName,
                __out ReplicatorSettingsUPtr & output);

            static Common::ErrorCode MoveIfValid(
                __in ReplicatorSettingsUPtr && toBeValidated,
                __out ReplicatorSettingsUPtr & output);

            static bool IsPowerOf2(int value) { return 0 == (value & (value-1)); }

            DWORD flags_;
            
            Common::TimeSpan retryInterval_;
            Common::TimeSpan batchAckInterval_;
            Common::TimeSpan primaryWaitForPendingQuorumsTimeout_;

            std::wstring replicatorAddress_;
            std::wstring replicatorListenAddress_;
            std::wstring replicatorPublishAddress_;

            bool requireServiceAck_;
            bool secondaryClearAcknowledgedOperations_;
            bool useStreamFaultsAndEndOfStreamOperationAck_;

            int64 initialReplicationQueueSize_;
            int64 maxReplicationQueueSize_;
            int64 initialCopyQueueSize_;
            int64 maxCopyQueueSize_;
            int64 maxReplicationQueueMemorySize_;
            int64 maxReplicationMessageSize_;
            int64 initialPrimaryReplicationQueueSize_;
            int64 maxPrimaryReplicationQueueSize_;
            int64 maxPrimaryReplicationQueueMemorySize_;
            int64 initialSecondaryReplicationQueueSize_;
            int64 maxSecondaryReplicationQueueSize_;
            int64 maxSecondaryReplicationQueueMemorySize_;
            
            Transport::SecuritySettings securitySettings_;
        };
    }
}
