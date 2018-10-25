// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    class TransactionalReplicatorSettings;
    typedef std::unique_ptr<TransactionalReplicatorSettings> TransactionalReplicatorSettingsUPtr;

    class TransactionalReplicatorSettings
    {
    public:

        static const int64 DefaultMinLogSizeInMBDivider;
        static const int64 SmallestMinLogSizeInMB;
        static const int64 SpareMaxStreamSizeInMB;

        // Gets the min log size in MB.
        // 0 MinLogSizeInMB indicates that CheckpointThresholdInMB / DefaultMinLogDivider needs to be used.
        static Common::ErrorCode GetMinLogSizeInMB(
            __in Common::StringWriter & messageWriter,
            __in int64 checkpointThresholdinMB,
            __out int64 & minLogSizeInMB);

        // Load settings from public API
        static Common::ErrorCode FromPublicApi(
            __in TRANSACTIONAL_REPLICATOR_SETTINGS const & transactionalReplicatorSettings2,
            __out TransactionalReplicatorSettingsUPtr & output);

        // Load settings from config object
        static Common::ErrorCode FromConfig(
            __in TRConfigBase const & trConfig,
            __out TransactionalReplicatorSettingsUPtr & output);

        // Factory method that loads the settings from configuration package.
        static HRESULT FromConfig(
            __in Common::ComPointer<IFabricCodePackageActivationContext> codePackageActivationContextCPtr,
            __in std::wstring const & configurationPackageName,
            __in std::wstring const & sectionName,
            __out IFabricTransactionalReplicatorSettingsResult ** replicatorSettings);

        __declspec(property(get = get_Flags)) DWORD64 Flags;
        DWORD64 get_Flags() const
        {
            return flags_;
        }

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        // Define config properties
        DEFINE_TR_OVERRIDEABLE_CONFIG_PROPERTIES();

        void ToPublicApi(__out TRANSACTIONAL_REPLICATOR_SETTINGS & transactionalReplicatorSettings);

    private:

        TransactionalReplicatorSettings();
        TransactionalReplicatorSettings(TransactionalReplicatorSettings const&);

        static Common::ErrorCode ReadFromConfigurationPackage(
            __in Common::ComPointer<IFabricCodePackageActivationContext> codePackageActivationContextCPtr,
            __in Common::ComPointer<IFabricConfigurationPackage> configPackageCPtr,
            __in std::wstring const & sectionName,
            __out TransactionalReplicatorSettingsUPtr & output);

        static Common::ErrorCode MoveIfValid(
            __in TransactionalReplicatorSettingsUPtr && toBeValidated,
            __out TransactionalReplicatorSettingsUPtr & output);

        static int64 GetThrottleThresholdInMB(
            __in int64 throttleThresholdFactor,
            __in int64 checkpointThresholdInMB,
            __in int64 minLogSizeInMb);

        static Common::ErrorCode ValidateMinLogSize(
            __in Common::StringWriter & messageWriter,
            __in TransactionalReplicatorSettingsUPtr & toBeValidated,
            __in std::shared_ptr<TRConfigValues> & defaultValues);

        static Common::ErrorCode ValidateLoggerSettings(
            __in Common::StringWriter & messageWriter,
            __in TransactionalReplicatorSettingsUPtr & toBeValidated);

        static Common::ErrorCode ValidateLogIOHealthReportSettings(
            __in Common::StringWriter & messageWriter,
            __in TransactionalReplicatorSettingsUPtr & toBeValidated,
            __in std::shared_ptr<TRConfigValues> & defaultValues);

        static bool IsMultipleOf4(__in int64 value);

        static bool IsTRSettingsEx1FlagsUsed(__in DWORD64 const & flags);

        DWORD64 flags_;

        DEFINE_TR_PRIVATE_CONFIG_MEMBERS();
    };
}
