// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'lsTP'

namespace TStoreTests
{
    using namespace ktl;
    using namespace Data::Utilities;

    class StoreBackupTest : public TStoreTestBase<int, int, IntComparer, TestStateSerializer<int>, TestStateSerializer<int>>
    {
    public:

        StoreBackupTest()
        {
            Setup();
            backupDirectorySPtr = CreateBackupFolderNameString(GetAllocator());

            // Clear out the Backup directory.
            if (Common::Directory::Exists(backupDirectorySPtr->operator LPCWSTR()))
            {
                Common::Directory::Delete(backupDirectorySPtr->operator LPCWSTR(), true);
            }

            // Create an empty Backup directory.
            Common::Directory::Create(backupDirectorySPtr->operator LPCWSTR());
        }

        ~StoreBackupTest()
        {
            // Clear out the Backup directory.
            if (Common::Directory::Exists(backupDirectorySPtr->operator LPCWSTR()))
            {
                Common::Directory::Delete(backupDirectorySPtr->operator LPCWSTR(), true);
            }

            backupDirectorySPtr = nullptr;
            Cleanup();
        }

        KString::SPtr CreateBackupFolderNameString(__in KAllocator & allocator)
        {
            KString::SPtr backupDirectory;

            WCHAR currentDirectoryPathCharArray[MAX_PATH];
            GetCurrentDirectory(MAX_PATH, currentDirectoryPathCharArray);

#if !defined(PLATFORM_UNIX)
            NTSTATUS status = KString::Create(backupDirectory, allocator, L"\\??\\");
#else
            NTSTATUS status = KString::Create(backupDirectory, allocator, L"");
#endif
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            BOOLEAN concatSuccess = backupDirectory->Concat(currentDirectoryPathCharArray);
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            concatSuccess = backupDirectory->Concat(Common::Path::GetPathSeparatorWstr().c_str());
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            concatSuccess = backupDirectory->Concat(L"Backup");
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            return backupDirectory.RawPtr();
        }

        ktl::Awaitable<void> PerformBackupAsync(__in KString & backupDirectory)
        {
            // Ensure the Backup directory exists.
            if (!Common::Directory::Exists(backupDirectory.operator LPCWSTR()))
            {
                Common::ErrorCode errorCode = Common::Directory::Create2(backupDirectory.operator LPCWSTR());
                if (errorCode.IsSuccess() == false)
                {
                    throw ktl::Exception(errorCode.ToHResult());
                }
            }

            // Perform a checkpoint first, to ensure we have all store state on disk.
            co_await CheckpointAsync(*Store);

            // Perform the actual backup.
            co_await Store->BackupCheckpointAsync(backupDirectory, CancellationToken::None);
            co_return;
        }

        ktl::Awaitable<void> RestoreFromBackupAsync(__in KString & backupDirectory)
        {
            // Remove all state from this store, in preparation for the Restore.
            co_await Store->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await Store->RemoveStateAsync(CancellationToken::None);

            // Close and re-open the store.
            co_await RemoveStateAndReopenStoreAsync();
            // Restore from Backup.
            co_await Store->RestoreCheckpointAsync(backupDirectory, CancellationToken::None);
            co_await Store->RecoverCheckpointAsync(CancellationToken::None);
            co_return;
        }

        KString::SPtr ToString(__in ULONG seed)
        {
            KString::SPtr value;
            wstring str = wstring(L"") + to_wstring(seed);
            auto status = KString::Create(value, GetAllocator(), str.c_str());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            return value;
        }

        Common::CommonConfig config; // load the config object as it's needed for the tracing to work
        KString::SPtr backupDirectorySPtr;

#pragma region test functions
    public:
        ktl::Awaitable<void> Backup_NoState_Test()
        {
            co_await PerformBackupAsync(*backupDirectorySPtr);
            co_await RestoreFromBackupAsync(*backupDirectorySPtr);
            VerifyCount(0);
            co_return;
        }

        ktl::Awaitable<void> Backup_1000Add_Test()
        {
            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                for (int i = 0; i < 1000; i++)
                {
                    co_await Store->AddAsync(*tx->StoreTransactionSPtr, i, i, Common::TimeSpan::MaxValue, CancellationToken::None);
                }

                co_await tx->CommitAsync();
            }

            co_await PerformBackupAsync(*backupDirectorySPtr);
            co_await RestoreFromBackupAsync(*backupDirectorySPtr);
            VerifyCount(1000);

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                for (int i = 0; i < 1000; i++)
                {
                    co_await VerifyKeyExistsAsync(*Store, i, -1, i);
                }

                co_await tx->CommitAsync();
            }
            co_return;
        }

        ktl::Awaitable<void> Backup_1000Add500Remove_Test()
        {
            for (int i = 0; i < 1000; i++)
            {
                {
                    WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                    co_await Store->AddAsync(*tx->StoreTransactionSPtr, i, i, DefaultTimeout, CancellationToken::None);
                    co_await tx->CommitAsync();
                }

                if (i % 2 == 0)
                {
                    {
                        WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                        co_await Store->ConditionalRemoveAsync(*tx->StoreTransactionSPtr, i, DefaultTimeout, CancellationToken::None);
                        co_await tx->CommitAsync();
                    }
                }
            }

            co_await PerformBackupAsync(*backupDirectorySPtr);
            co_await RestoreFromBackupAsync(*backupDirectorySPtr);
            VerifyCount(500);

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                for (int i = 0; i < 1000; i++)
                {
                    if (i % 2 == 0)
                    {
                        co_await VerifyKeyDoesNotExistAsync(*Store, i);
                    }
                    else
                    {
                        co_await VerifyKeyExistsAsync(*Store, i, -1, i);
                    }
                }

                co_await tx->AbortAsync();
            }
            co_return;
        }

        ktl::Awaitable<void> Backup_MultipleCheckpoints_Test()
        {
            for (int i = 0; i < 1000; i++)
            {
                {
                    WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                    co_await Store->AddAsync(*tx->StoreTransactionSPtr, i, i, Common::TimeSpan::MaxValue, CancellationToken::None);
                    co_await tx->CommitAsync();
                }

                if (i % 100 == 0)
                {
                    co_await CheckpointAsync(*Store);
                }
            }

            co_await PerformBackupAsync(*backupDirectorySPtr);
            co_await RestoreFromBackupAsync(*backupDirectorySPtr);
            VerifyCount(1000);

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                for (int i = 0; i < 1000; i++)
                {
                    co_await VerifyKeyExistsAsync(*Store, i, -1, i);
                }

                co_await tx->AbortAsync();
            }
            co_return;
        }

        ktl::Awaitable<void> Backup_MultipleBackups_Test()
        {
            for (int i = 1; i <= 1000; i++)
            {
                {
                    WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                    co_await Store->AddAsync(*tx->StoreTransactionSPtr, i, i, DefaultTimeout, CancellationToken::None);
                    co_await tx->CommitAsync();
                }

                if (i % 100 == 0)
                {
                    KString::SPtr backupSPtr;
                    NTSTATUS status = KString::Create(backupSPtr, GetAllocator(), *backupDirectorySPtr);
                    CODING_ERROR_ASSERT(NT_SUCCESS(status));
                    auto concatSuccess = backupSPtr->Concat(Common::Path::GetPathSeparatorWstr().c_str());
                    CODING_ERROR_ASSERT(concatSuccess == TRUE);
                    concatSuccess = backupSPtr->Concat(*ToString(i));
                    CODING_ERROR_ASSERT(concatSuccess == TRUE);

                    co_await PerformBackupAsync(*backupSPtr);
                }
            }

            for (int backupId = 100; backupId <= 1000; backupId += 100)
            {
                KString::SPtr backupSPtr;
                NTSTATUS status = KString::Create(backupSPtr, GetAllocator(), *backupDirectorySPtr);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
                auto concatSuccess = backupSPtr->Concat(Common::Path::GetPathSeparatorWstr().c_str());
                CODING_ERROR_ASSERT(concatSuccess == TRUE);
                concatSuccess = backupSPtr->Concat(*ToString(backupId));
                CODING_ERROR_ASSERT(concatSuccess == TRUE);
                co_await RestoreFromBackupAsync(*backupSPtr);
                VerifyCount(backupId);

                {
                    WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                    for (int i = 1; i <= backupId; i++)
                    {
                        co_await VerifyKeyExistsAsync(*Store, i, -1, i);
                    }

                    co_await tx->AbortAsync();
                }
            }
            co_return;
        }
    #pragma endregion
    };

    BOOST_FIXTURE_TEST_SUITE(StoreBackupTestSuite, StoreBackupTest)

    BOOST_AUTO_TEST_CASE(Backup_NoState)
    {
        SyncAwait(Backup_NoState_Test());
    }

    BOOST_AUTO_TEST_CASE(Backup_1000Add)
    {
        SyncAwait(Backup_1000Add_Test());
    }

    BOOST_AUTO_TEST_CASE(Backup_1000Add500Remove)
    {
        SyncAwait(Backup_1000Add500Remove_Test());
    }

    BOOST_AUTO_TEST_CASE(Backup_MultipleCheckpoints)
    {
        SyncAwait(Backup_MultipleCheckpoints_Test());
    }

    BOOST_AUTO_TEST_CASE(Backup_MultipleBackups)
    {
        SyncAwait(Backup_MultipleBackups_Test());
    }

    BOOST_AUTO_TEST_SUITE_END()
}
