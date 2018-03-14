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

        void PerformBackup(__in KString & backupDirectory)
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
            Checkpoint(*Store);

            // Perform the actual backup.
            SyncAwait(Store->BackupCheckpointAsync(backupDirectory, CancellationToken::None));
        }

        void RestoreFromBackup(__in KString & backupDirectory)
        {
            // Remove all state from this store, in preparation for the Restore.
            SyncAwait(Store->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None));
            SyncAwait(Store->RemoveStateAsync(CancellationToken::None));

            // Close and re-open the store.
            RemoveStateAndReopenStore();
            // Restore from Backup.
            SyncAwait(Store->RestoreCheckpointAsync(backupDirectory, CancellationToken::None));
            SyncAwait(Store->RecoverCheckpointAsync(CancellationToken::None));
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
    };

    BOOST_FIXTURE_TEST_SUITE(StoreBackupTestSuite, StoreBackupTest)

    BOOST_AUTO_TEST_CASE(Backup_NoState)
    {
        PerformBackup(*backupDirectorySPtr);
        RestoreFromBackup(*backupDirectorySPtr);
        VerifyCount(0);
    }

    BOOST_AUTO_TEST_CASE(Backup_1000Add)
    {
        {
            WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
            for (int i = 0; i < 1000; i++)
            {
                SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, i, i, Common::TimeSpan::MaxValue, CancellationToken::None));
            }

            SyncAwait(tx->CommitAsync());
        }

        PerformBackup(*backupDirectorySPtr);
        RestoreFromBackup(*backupDirectorySPtr);
        VerifyCount(1000);

        {
            WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
            for (int i = 0; i < 1000; i++)
            {
                SyncAwait(VerifyKeyExistsAsync(*Store, i, -1, i));
            }

            SyncAwait(tx->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Backup_1000Add500Remove)
    {
        for (int i = 0; i < 1000; i++)
        {
            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, i, i, DefaultTimeout, CancellationToken::None));
                SyncAwait(tx->CommitAsync());
            }

            if (i % 2 == 0)
            {
                {
                    WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                    SyncAwait(Store->ConditionalRemoveAsync(*tx->StoreTransactionSPtr, i, DefaultTimeout, CancellationToken::None));
                    SyncAwait(tx->CommitAsync());
                }
            }
        }

        PerformBackup(*backupDirectorySPtr);
        RestoreFromBackup(*backupDirectorySPtr);
        VerifyCount(500);

        {
            WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
            for (int i = 0; i < 1000; i++)
            {
                if (i % 2 == 0)
                {
                    SyncAwait(VerifyKeyDoesNotExistAsync(*Store, i));
                }
                else
                {
                    SyncAwait(VerifyKeyExistsAsync(*Store, i, -1, i));
                }
            }

            SyncAwait(tx->AbortAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Backup_MultipleCheckpoints)
    {
        for (int i = 0; i < 1000; i++)
        {
            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, i, i, Common::TimeSpan::MaxValue, CancellationToken::None));
                SyncAwait(tx->CommitAsync());
            }

            if (i % 100 == 0)
            {
                Checkpoint(*Store);
            }
        }

        PerformBackup(*backupDirectorySPtr);
        RestoreFromBackup(*backupDirectorySPtr);
        VerifyCount(1000);

        {
            WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
            for (int i = 0; i < 1000; i++)
            {
                SyncAwait(VerifyKeyExistsAsync(*Store, i, -1, i));
            }

            SyncAwait(tx->AbortAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Backup_MultipleBackups)
    {
        for (int i = 1; i <= 1000; i++)
        {
            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, i, i, DefaultTimeout, CancellationToken::None));
                SyncAwait(tx->CommitAsync());
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

                PerformBackup(*backupSPtr);
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
            RestoreFromBackup(*backupSPtr);
            VerifyCount(backupId);

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                for (int i = 1; i <= backupId; i++)
                {
                    SyncAwait(VerifyKeyExistsAsync(*Store, i, -1, i));
                }

                SyncAwait(tx->AbortAsync());
            }
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
