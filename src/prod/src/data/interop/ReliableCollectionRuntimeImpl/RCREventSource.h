// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Interop
    {
#define DECLARE_RCR_STRUCTURED_TRACE(trace_name, ...) Common::TraceEventWriter<__VA_ARGS__> trace_name
#define RCR_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
            trace_name( \
                Common::TraceTaskCodes::RCR, \
                trace_id, \
                #trace_name, \
                Common::LogLevel::trace_level, \
                __VA_ARGS__)

        // Structured trace reserved areas.
        //  1. TraceType: [0, 3]
        //  2. CompatRDStateProvider : [10, 20]
        class RCREventSource
        {
        public:
            DECLARE_RCR_STRUCTURED_TRACE(ExceptionError, Common::Guid, LONG64, std::wstring, LONG64);
            DECLARE_RCR_STRUCTURED_TRACE(ConfigPackageLoadFailed, Common::Guid, LONG64, std::wstring, std::wstring, std::wstring, LONG64);
            DECLARE_RCR_STRUCTURED_TRACE(CreateTransactionalReplicator, Common::Guid, LONG64, bool, std::wstring, std::wstring, std::wstring);
            DECLARE_RCR_STRUCTURED_TRACE(Warning, Common::Guid, LONG64, std::wstring);
            DECLARE_RCR_STRUCTURED_TRACE(Info, Common::Guid, LONG64, std::wstring);

            // CompatRDStateProvider
            DECLARE_RCR_STRUCTURED_TRACE(CompatRDStateProviderCtor, Common::Guid, LONG64, LONGLONG, Common::WStringLiteral);
            DECLARE_RCR_STRUCTURED_TRACE(CompatRDStateProviderInitialize, Common::Guid, LONG64, LONGLONG, Common::WStringLiteral);
            DECLARE_RCR_STRUCTURED_TRACE(CompatRDStateProviderError, Common::Guid, LONG64, LONGLONG, std::wstring, LONG64);
            DECLARE_RCR_STRUCTURED_TRACE(CompatRDStateProviderGetChildren, Common::Guid, LONG64, LONGLONG, Common::WStringLiteral, Common::WStringLiteral);

            // ConfigurationPackageChangeHandler
            DECLARE_RCR_STRUCTURED_TRACE(ConfigurationPackageChangeHandlerCtor, Common::Guid, LONG64, uintptr_t, std::wstring, std::wstring, std::wstring);
            DECLARE_RCR_STRUCTURED_TRACE(ConfigurationPackageChangeHandlerEvent, Common::Guid, LONG64, std::wstring, std::wstring);


            RCREventSource() :
                // Note: TraceId starts from 4, this is because 0-3 has already been defined as trace type: Info, Warning, Error, Noise.
                // To move the partition id to the type section, format like RCR.ConfigPackageLoadFailed@{PartitionId}, the first parameter PartitionId should mark as "id".
                RCR_STRUCTURED_TRACE(ExceptionError, 4, Error, "{1}: {2} ErrorCode: {3}", "id", "ReplicaId", "Message", "ErrorCode"),
                RCR_STRUCTURED_TRACE(ConfigPackageLoadFailed, 5, Error, "{1}: Failed to load replicator settings from ConfigPackage: {2} ConfigSection: {3} ConfigSecuritySection: {4} ErrorCode: {5}", "id", "ReplicaId", "ConfigPackage", "ConfigSection", "ConfigSecuritySection", "ErrorCode"),
                RCR_STRUCTURED_TRACE(CreateTransactionalReplicator, 6, Info, "{1}: IsReplicatorSettingsPresent: {2} ReplicatorConfigPackage: {3} ReplicatorConfigSection: {4} ReplicatorSecuritySection: {5}", "id", "ReplicaId", "ReplicatorSettingsProvided", "ReplicatorConfigPackage", "ReplicatorConfigSection", "ReplicatorSecuritySection"),
                RCR_STRUCTURED_TRACE(Warning, 7, Warning, "{1}: {2}", "id", "ReplicaId", "Message"),
                RCR_STRUCTURED_TRACE(Info, 8, Info, "{1}: {2}", "id", "ReplicaId", "Message"),

                // CompatRDStateProvider
                RCR_STRUCTURED_TRACE(CompatRDStateProviderCtor, 10, Info, "{1}: SpId: {2} SpName: {3}", "id", "ReplicaId", "SpId", "SpName"),
                RCR_STRUCTURED_TRACE(CompatRDStateProviderInitialize, 11, Info, "{1}: SpId: {2} SpName: {3}", "id", "ReplicaId", "SpId", "SpName"),
                RCR_STRUCTURED_TRACE(CompatRDStateProviderError, 12, Error, "{1}: SpId: {2} {3} ErrorCode: {4}", "id", "ReplicaId", "SpId", "Message", "ErrorCode"),
                RCR_STRUCTURED_TRACE(CompatRDStateProviderGetChildren, 13, Info, "{1}: SpId: {2} SpName: {3} ChildSpName: {4}", "id", "ReplicaId", "SpId", "SpName", "ChildSPName"),

                // ConfigurationPackageChangeHandler
                RCR_STRUCTURED_TRACE(ConfigurationPackageChangeHandlerCtor, 21, Info, "{1}: Primary Replicator: {2} Config Package: {3} Replicator Settings Section: {4} Replicator Security Section: {5}", "id", "ReplicaId", "PrimaryReplicatorAddress", "ConfigPackage", "ReplicatorSettingsSection", "ReplicatorSecuritySection"),
                RCR_STRUCTURED_TRACE(ConfigurationPackageChangeHandlerEvent, 22, Info, "{1}: Expected Config Package: {2} Actual Config Package: {3}", "id", "ReplicaId", "ExpectedConfigPackage", "ActualConfigPackage")
            {
            }

            static Common::Global<RCREventSource> Events;
        };
    }
}