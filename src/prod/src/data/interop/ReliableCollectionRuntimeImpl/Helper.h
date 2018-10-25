// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Data
{
    namespace Interop
    {
        class IsolationHelper
        {
        public: // TODO const

            enum OperationType
            {
                Invalid = 0,

                SingleEntity = 1,

                MultiEntity = 2,
            };

            static Data::TStore::StoreTransactionReadIsolationLevel::Enum GetIsolationLevel(TxnReplicator::Transaction& txn, OperationType operationType);
        };

        template <class T>
        bool IsComplete(ktl::Awaitable<T>& awaitable)
        {
            bool forceAsync = testSettings.forceAsync();
            if (forceAsync)
                return false;
            return awaitable.IsComplete();
        }

        struct ReplicatorConfigSettingsResult
        {
            Common::ComPointer<IFabricReplicatorSettingsResult> v1ReplicatorSettingsResult;
            Common::ComPointer<IFabricTransactionalReplicatorSettingsResult> txnReplicatorSettingsResult;
            Common::ComPointer<IFabricSharedLogSettingsResult> sharedLogSettingsResult;
            Common::ComPointer<IFabricSecurityCredentialsResult> replicatorSecurityCredentialsResult;
        };

        HRESULT LoadReplicatorSettingsFromConfigPackage(
            __in Data::Utilities::PartitionedReplicaId const & prId,
            __in Common::ScopedHeap& heap,
            __in ReplicatorConfigSettingsResult& configSettingsResult,
            __in wstring& configPackageName,
            __in wstring& replicatorSettingsSectionName,
            __in wstring& replicatorSecuritySectionName,
            __out FABRIC_REPLICATOR_SETTINGS const ** fabricReplicatorSettings,
            __out TRANSACTIONAL_REPLICATOR_SETTINGS const ** txnReplicatorSettings,
            __out KTLLOGGER_SHARED_LOG_SETTINGS const ** ktlLoggerSharedLogSettings);
    }
}

#define CO_RETURN_VOID_ON_FAILURE(status) \
    if (!NT_SUCCESS(status)) \
    { \
        co_return; \
    } \

#define EXCEPTION_TO_STATUS(expr, status) \
    try \
    { \
        expr; \
    } \
    catch (ktl::Exception const & e) \
    { \
        status = e.GetStatus(); \
    } \

#define ON_FAIL_RETURN_HR(status) \
    if (!NT_SUCCESS(status))   \
    { \
        return StatusConverter::ToHResult(status); \
    } \
