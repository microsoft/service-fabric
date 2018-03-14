// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <stdlib.h>

using namespace Common;
using namespace Data::Integration;

namespace Data
{
    namespace Interop
    {
        using namespace Common;
        using namespace ktl;
        using namespace Data::Utilities;
        using namespace Data::TStore;

        class CExportTests
        {
        public:
            CExportTests(wstring const & testName)
            {
                testName_ = testName;
                epoch_ = 0;
            }

            void Setup();
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

            CommonConfig config; // load the config object as its needed for the tracing to work
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
        };

        void CExportTests::Setup()
        {
            NTSTATUS status;

            testFolderPath_ = CreateFileName(testName_);
            wstring TestLogFileName = testName_.append(L".log");

            Directory::Delete_WithRetry(testFolderPath_, true, true);

            status = KtlSystem::Initialize(FALSE, &underlyingSystem_); 
            ASSERT_IFNOT(NT_SUCCESS(status), "Status not success: {0}", status); 

            underlyingSystem_->SetStrictAllocationChecks(TRUE); 
            KAllocator & allocator = underlyingSystem_->PagedAllocator(); 
            int seed = GetTickCount(); 
            Common::Random r(seed); 
            rId_ = r.Next(); 
            pId_.CreateNew(); 
            prId_ = PartitionedReplicaId::Create(pId_, rId_, allocator); 

            KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
            InitializeKtlConfig(testFolderPath_, TestLogFileName, underlyingSystem_->PagedAllocator(), sharedLogSettings);

            status = Data::Log::LogManager::Create(underlyingSystem_->PagedAllocator(), logManager_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = SyncAwait(logManager_->OpenAsync(CancellationToken::None, sharedLogSettings));
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
                factory.RawPtr());
            return replica;
        }

        void CExportTests::OpenReplica()
        {
            SyncAwait(OpenReplicaAsync());
        }

        Awaitable<void> CExportTests::OpenReplicaAsync()
        {
            co_await replica_->OpenAsync();

            epoch_++;
            FABRIC_EPOCH epoch1; epoch1.DataLossNumber = epoch_; epoch1.ConfigurationNumber = epoch_; epoch1.Reserved = nullptr;
            co_await replica_->ChangeRoleAsync(epoch1, FABRIC_REPLICA_ROLE_PRIMARY);

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
            wstring testFolderPath = Directory::GetCurrentDirectoryW();
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
            settings->LogContainerId.GetReference().CreateNew();
            settings->LogSize = 1024 * 1024 * 512; // 512 MB.
            settings->MaximumNumberStreams = 0;
            settings->MaximumRecordSize = 0;
            settings->Flags = static_cast<ULONG>(Data::Log::LogCreationFlags::UseSparseFile);
            sharedLogSettings = make_shared<KtlLogger::SharedLogSettings>(std::move(settings));
        };
    }
}

static Data::Interop::CExportTests* t = nullptr;

extern "C" HRESULT Initialize(
    __in LPCWSTR testname, 
    __out void** txnReplicator, 
    __out void** partition,
    __out GUID* partitionId,
    __out int64_t* replicaId)
{
    wstring env;
    HRESULT hr = ReliableCollectionRuntime_Initialize(RELIABLECOLLECTION_API_VERSION);
    if (!SUCCEEDED(hr))
        return hr;

    if (Environment::GetEnvironmentVariable(L"SF_RELIABLECOLLECTION_MOCK", env, NOTHROW()))
    {
        *txnReplicator = (void*)0x100;
        *replicaId = 0;
        return S_OK;
    }

    t = new Data::Interop::CExportTests(testname);
    t->Setup();
    *txnReplicator = t->GetTxnReplicator();
    *partition = nullptr;
    *partitionId = t->PartitionId;
    *replicaId = t->ReplicaId;
    return S_OK;
}

extern "C" void ResetReplica(__out void** txnReplicator)
{
    t->ResetReplica();
    *txnReplicator = t->GetTxnReplicator();
}

extern "C" void OpenReplica()
{
    t->OpenReplica();
}

extern "C" void RequestCheckpointAfterNextTransaction()
{
    t->RequestCheckpointAfterNextTransaction();
}

extern "C" void Cleanup()
{
    if (t != nullptr)
    {
        t->Cleanup();
        delete t;
        t = nullptr;
    }
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


