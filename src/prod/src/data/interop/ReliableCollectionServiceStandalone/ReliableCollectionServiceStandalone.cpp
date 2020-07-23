// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <stdlib.h>

#include <ktlevents.um.h>

using namespace Common;
using namespace Data::Integration;

enum ReliableCollectionMode : uint64_t
{
    /// <summary>
    /// Recover from previous checkpoint, if available
    /// </summary>
    OpenOrCreate = 0x4,

    /// <summary>
    /// No Recovery, removes previous checkpoints and provides fresh reliable collections.
    /// </summary>
    CreateAsNew = 0x8,

    /// <summary>
    /// Use Volatile Reliable collections
    /// </summary>
    Volatile = 0x10
};

namespace Data
{
    namespace Interop
    {
        using namespace Common;
        using namespace ktl;
        using namespace Data::Utilities;
        using namespace Data::TStore;
        using namespace Data::Log;

        class InitConfig
        {
        public:
            InitConfig() {
#if defined(PLATFORM_UNIX)
        #define DEFINE_STRING_RESOURCE(id,str) StringResourceRepo::Get_Singleton()->AddResource(id, str);
            #include "rcstringtbl.inc"
#endif
            }
        };

        InitConfig __init;

        class RCStandaloneCommonConfig : ComponentConfig
        {
            DECLARE_SINGLETON_COMPONENT_CONFIG(RCStandaloneCommonConfig, "RCStandaloneCommonConfig")

            PUBLIC_CONFIG_ENTRY(int, L"Trace/Etw", Sampling, 1, ConfigEntryUpgradePolicy::Dynamic);
            PUBLIC_CONFIG_ENTRY(int, L"Trace/Etw", Level, 4, ConfigEntryUpgradePolicy::Dynamic);
        };

        class CExportTests
        {
        public:
            CExportTests(wstring const & testName)
            {
                testName_ = testName;
            }

            void Setup(__in ReliableCollectionMode mode);
            void Cleanup();
            void OpenReplica();
            void RequestCheckpointAfterNextTransaction();
            void ResetReplica();

            void* GetTxnReplicator()
            {
                return replica_->TxnReplicator.RawPtr();
            }

            __declspec(property(get = get_PartitionId)) KGuid PartitionId;
            KGuid get_PartitionId() const
            {
                return pId_;
            }

            __declspec(property(get = get_ReplicaId)) FABRIC_REPLICA_ID ReplicaId;
            FABRIC_REPLICA_ID get_ReplicaId() const
            {
                return rId_;
            }
        protected:
            wstring CreateFileName(
                __in wstring const & folderName);

            void InitializeKtlConfig(
                __in std::wstring workDir,
                __in std::wstring fileName,
                __in KAllocator & allocator,
                __out KtlLogger::SharedLogSettingsSPtr & sharedLogSettings);

            Replica::SPtr CreateReplica();
            void CloseReplica();
            Awaitable<void> OpenReplicaAsync();

            RCStandaloneCommonConfig config; // load the config object as its needed for the tracing to work
            KtlSystem * underlyingSystem_;

            KGuid pId_;
            FABRIC_REPLICA_ID rId_;
            PartitionedReplicaId::SPtr prId_;

            ////
            Data::Log::LogManager::SPtr logManager_;
            Replica::SPtr replica_;
            wstring testFolderPath_;
            wstring testName_;
            LONGLONG epoch_;
            BOOLEAN hasPersistedState_;

            static const wstring DEFAULT_TEST_PARTITION_GUID;
            static const long DEFAULT_TEST_REPLICA_ID;
        };

        const wstring CExportTests::DEFAULT_TEST_PARTITION_GUID = L"45AEFD6F-1CCE-4A2E-8C58-9AD7D6A9CC7C";
        const long CExportTests::DEFAULT_TEST_REPLICA_ID = 1234567890;

        void CExportTests::Setup(__in ReliableCollectionMode mode)
        {
            NTSTATUS status;

            testFolderPath_ = CreateFileName(testName_);
            wstring TestLogFileName = testName_.append(L".log");

            status = KtlSystem::Initialize(FALSE, &underlyingSystem_); 
            ASSERT_IFNOT(NT_SUCCESS(status), "Status not success: {0}", status); 

            bool allowRecovery = !(mode & ReliableCollectionMode::CreateAsNew);
            hasPersistedState_ = !(mode & ReliableCollectionMode::Volatile);
            if (!allowRecovery)
               Directory::Delete_WithRetry(testFolderPath_, true, true);

            rId_ = DEFAULT_TEST_REPLICA_ID;
            Guid guid(DEFAULT_TEST_PARTITION_GUID);
            KGuid paritionGuid(guid.AsGUID());
            pId_ = paritionGuid;
            underlyingSystem_->SetStrictAllocationChecks(TRUE);
            KAllocator & allocator = underlyingSystem_->PagedAllocator();
            prId_ = PartitionedReplicaId::Create(pId_, rId_, allocator);

            KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
            InitializeKtlConfig(testFolderPath_, TestLogFileName, underlyingSystem_->PagedAllocator(), sharedLogSettings);

            TraceProvider::GetSingleton()->SetDefaultLevel(TraceSinkType::ETW, LogLevel::Noise);
            RegisterKtlTraceCallback(Common::ServiceFabricKtlTraceCallback);

            status = Data::Log::LogManager::Create(underlyingSystem_->PagedAllocator(), logManager_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = SyncAwait(logManager_->OpenAsync(CancellationToken::None, sharedLogSettings, KtlLoggerMode::InProc));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            replica_ = CreateReplica();

            OpenReplica();
        }

        Replica::SPtr CExportTests::CreateReplica()
        {
            wstring testFolder = Path::Combine(testFolderPath_, L"work");
            StateProviderFactory::SPtr factory;
            StateProviderFactory::Create(underlyingSystem_->PagedAllocator(), factory);

            Replica::SPtr replica = Replica::Create(
                pId_,
                rId_,
                testFolder,
                *logManager_,
                underlyingSystem_->PagedAllocator(),
                factory.RawPtr(),
                nullptr,
                nullptr,
                hasPersistedState_);
            return replica;
        }

        void CExportTests::OpenReplica()
        {
            SyncAwait(OpenReplicaAsync());
        }

        Awaitable<void> CExportTests::OpenReplicaAsync()
        {
            co_await replica_->OpenAsync();

            FABRIC_EPOCH epoch;
            NTSTATUS status = replica_->TxnReplicator->GetCurrentEpoch(epoch);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            LONG64 currentConfigurationNumber = epoch.ConfigurationNumber;
            currentConfigurationNumber++;
            epoch.ConfigurationNumber = currentConfigurationNumber;
            co_await replica_->ChangeRoleAsync(epoch, FABRIC_REPLICA_ROLE_PRIMARY);

            replica_->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);
            replica_->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);

            co_return;
        }

        void CExportTests::CloseReplica()
        {
            replica_->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
            replica_->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);

            SyncAwait(replica_->CloseAsync());
        }

        void CExportTests::RequestCheckpointAfterNextTransaction()
        {
            // @TODO - Better error checking by returning error back to managed in CExportTests
            NTSTATUS status = replica_->TxnReplicator->Test_RequestCheckpointAfterNextTransaction();            
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        void CExportTests::ResetReplica()
        {
            CloseReplica();

            replica_ = CreateReplica();
        }

        void CExportTests::Cleanup()
        {
            NTSTATUS status;

            CloseReplica();

            replica_.Reset();

            status = SyncAwait(logManager_->CloseAsync(CancellationToken::None));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            logManager_ = nullptr;

            prId_.Reset();
            
            KtlSystem::Shutdown();

            // Post-clean up
            Directory::Delete_WithRetry(testFolderPath_, true, true);
        }

        wstring CExportTests::CreateFileName(
            __in wstring const & folderName)
        {
            wstring testFolderPath = L"";
            if (!Environment::GetEnvironmentVariable(L"Fabric_RCStandaloneWorkDirOverride", testFolderPath, NOTHROW()))
                testFolderPath = Directory::GetCurrentDirectoryW();

            Path::CombineInPlace(testFolderPath, folderName);

            return testFolderPath;
        }

        void CExportTests::InitializeKtlConfig(
            __in std::wstring workDir,
            __in std::wstring fileName,
            __in KAllocator & allocator,
            __out KtlLogger::SharedLogSettingsSPtr & sharedLogSettings)
        {
            auto settings = std::make_unique<KtlLogManager::SharedLogContainerSettings>();

            KString::SPtr sharedLogFileName = KPath::CreatePath(workDir.c_str(), allocator);
            KPath::CombineInPlace(*sharedLogFileName, fileName.c_str());

            if (!Common::Directory::Exists(workDir))
            {
                Common::Directory::Create(workDir);
            }

            KInvariant(sharedLogFileName->LengthInBytes() + sizeof(WCHAR) < 512 * sizeof(WCHAR)); // check to make sure there is space for the null terminator
            KMemCpySafe(&settings->Path[0], 512 * sizeof(WCHAR), sharedLogFileName->operator PVOID(), sharedLogFileName->LengthInBytes());
            settings->Path[sharedLogFileName->LengthInBytes() / sizeof(WCHAR)] = L'\0'; // set the null terminator
            settings->LogContainerId = KGuid(KtlLogger::Constants::DefaultApplicationSharedLogId.AsGUID()); // always use staging log for standalone
            settings->LogSize = 1024 * 1024 * 512; // 512 MB.
            settings->MaximumNumberStreams = 0;
            settings->MaximumRecordSize = 0;
            settings->Flags = static_cast<ULONG>(Data::Log::LogCreationFlags::UseNonSparseFile);
            sharedLogSettings = make_shared<KtlLogger::SharedLogSettings>(std::move(settings));
        };

    }
}

static Data::Interop::CExportTests* tests = nullptr;

extern "C" HRESULT ReliableCollectionStandalone_InitializeEx(
    __in LPCWSTR testname,
    __in ReliableCollectionMode mode,
    __out void** txnReplicator,
    __out void** partition,
    __out GUID* partitionId,
    __out int64_t* replicaId)
{
    wstring env;

    //
    // Enable KTL tracing
    //
    EventRegisterMicrosoft_Windows_KTL();

    if (Environment::GetEnvironmentVariable(L"SF_RELIABLECOLLECTION_MOCK", env, NOTHROW()))
    {
        *txnReplicator = (void*)0x100;
        *replicaId = 0;
        return S_OK;
    }

    tests = new Data::Interop::CExportTests(testname);
    tests->Setup(mode);
    *txnReplicator = tests->GetTxnReplicator();
    *partition = nullptr;
    *partitionId = tests->PartitionId;
    *replicaId = tests->ReplicaId;
    return S_OK;
}

extern "C" HRESULT ReliableCollectionStandalone_Initialize(
    __in LPCWSTR testname,
    __in bool allowRecovery,
    __out void** txnReplicator,
    __out void** partition,
    __out GUID* partitionId,
    __out int64_t* replicaId)
{
    ReliableCollectionMode reliableCollectionMode = allowRecovery ? ReliableCollectionMode::OpenOrCreate : ReliableCollectionMode::CreateAsNew;
    return ReliableCollectionStandalone_InitializeEx(testname, reliableCollectionMode, txnReplicator, partition, partitionId, replicaId);
}

extern "C" void ReliableCollectionStandalone_ResetReplica(__out void** txnReplicator)
{
    tests->ResetReplica();
    *txnReplicator = tests->GetTxnReplicator();
}

extern "C" void ReliableCollectionStandalone_OpenReplica()
{
    tests->OpenReplica();
}

extern "C" void ReliableCollectionStandalone_RequestCheckpointAfterNextTransaction()
{
    tests->RequestCheckpointAfterNextTransaction();
}

extern "C" void ReliableCollectionStandalone_Cleanup()
{
    if (tests != nullptr)
    {
        tests->Cleanup();
        delete tests;
        tests = nullptr;
    }

    //
    // Disable KTL Tracing
    //
    EventUnregisterMicrosoft_Windows_KTL();
}

extern "C" HRESULT FabricGetReliableCollectionApiTable(uint16_t apiVersion, ReliableCollectionApis* reliableCollectionApis)
{
    UNREFERENCED_PARAMETER(apiVersion);
    wstring env;

    if (apiVersion > RELIABLECOLLECTION_API_VERSION)
        return E_NOTIMPL;

    if (reliableCollectionApis == nullptr)
        return E_INVALIDARG;

    if (Environment::GetEnvironmentVariable(L"SF_RELIABLECOLLECTION_MOCK", env, NOTHROW()))
        GetReliableCollectionMockApiTable(reliableCollectionApis);
    else
        GetReliableCollectionImplApiTable(reliableCollectionApis);
    return S_OK;
}

// Dummy methods required to satisfy link of ReliableCollectionServiceStandalone.dll
// These methods are not used at runtime for ReliableCollectionServiceStandalone
HRESULT FabricGetActivationContext( 
    REFIID riid,
    void **activationContext)
{
    UNREFERENCED_PARAMETER(riid);
    UNREFERENCED_PARAMETER(activationContext);
    return S_OK;
}

HRESULT FabricLoadReplicatorSettings(
    IFabricCodePackageActivationContext const * codePackageActivationContext,
    LPCWSTR configurationPackageName,
    LPCWSTR sectionName,
    IFabricReplicatorSettingsResult ** replicatorSettings)
{
    UNREFERENCED_PARAMETER(codePackageActivationContext);
    UNREFERENCED_PARAMETER(configurationPackageName);
    UNREFERENCED_PARAMETER(sectionName);
    UNREFERENCED_PARAMETER(replicatorSettings);
    return S_OK;
}

HRESULT FabricLoadSecurityCredentials(
    IFabricCodePackageActivationContext const * codePackageActivationContext,
    LPCWSTR configurationPackageName,
    LPCWSTR sectionName,
    IFabricSecurityCredentialsResult ** securitySettings)
{
    UNREFERENCED_PARAMETER(codePackageActivationContext);
    UNREFERENCED_PARAMETER(configurationPackageName);
    UNREFERENCED_PARAMETER(sectionName);
    UNREFERENCED_PARAMETER(securitySettings);
    return S_OK;
}


