// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'tsTP'

namespace TStoreTests
{
   using namespace ktl;

   class StoreTestIdempotency : public TStoreTestBase<int, int, IntComparer, TestStateSerializer<int>, TestStateSerializer<int>>
   {
   public:
      StoreTestIdempotency()
      {
         Setup(1);
      }

      ~StoreTestIdempotency()
      {
         Cleanup();
      }

      Common::CommonConfig config; // load the config object as its needed for the tracing to work

      ktl::Awaitable<void> VerifyAllDuplicateApplyOperationsAsync(
          __in int key,
          __in int value,
          __in LONG64 commitSequenceNumber,
          bool lockContextExpected = false)
      {
          auto result = co_await ApplyAddOperationAsync(*Store, key, value, commitSequenceNumber, ApplyContext::RecoveryRedo);
          if (lockContextExpected == false)
          {
              CODING_ERROR_ASSERT(result == nullptr);
          }
          else
          {
              Store->Unlock(*result);
          }

          result = co_await ApplyAddOperationAsync(*Store, key, value, commitSequenceNumber, ApplyContext::SecondaryRedo);
          if (lockContextExpected == false)
          {
              CODING_ERROR_ASSERT(result == nullptr);
          }
          else
          {
              Store->Unlock(*result);
          }

          co_return;
      }

      ktl::Awaitable<void> ApplyAndVerifyIdempotentOperationsAsync(
          __in Data::TStore::Store<int, int>& secondaryStore,
          __in LONG64 commitSequenceNumber)
      {
          auto result = co_await ApplyAddOperationAsync(secondaryStore, 0, -1, commitSequenceNumber, ApplyContext::SecondaryRedo);
          CODING_ERROR_ASSERT(result == nullptr);
          result = co_await ApplyAddOperationAsync(secondaryStore, 0, -1, commitSequenceNumber, ApplyContext::SecondaryRedo);
          CODING_ERROR_ASSERT(result == nullptr);
          result = co_await ApplyRemoveOperationAsync(secondaryStore, 0, -1, commitSequenceNumber, ApplyContext::SecondaryRedo);
          CODING_ERROR_ASSERT(result == nullptr);
          result = co_await ApplyRemoveOperationAsync(secondaryStore, 0, -1, commitSequenceNumber, ApplyContext::SecondaryRedo);
          CODING_ERROR_ASSERT(result == nullptr);
          co_return;
      }

#pragma region test functions
    public:
        ktl::Awaitable<void> Idempotency_Normal_SkipDuplicate_Test()
       {
          int key = 5;
          int value = 6;

          CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->CheckpointLSN == 0);
          LONG64 commitLsn = 0;

          {
             WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
             co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, Common::TimeSpan::MaxValue, CancellationToken::None);
             co_await VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key, -1, value);
             co_await tx->CommitAsync();
             commitLsn = tx->TransactionSPtr->CommitSequenceNumber;
          }

          auto checkpointLSN = co_await CheckpointAsync(*Store);
          co_await CloseAndReOpenStoreAsync();
          CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->CheckpointLSN == checkpointLSN);

          auto result = co_await ApplyAddOperationAsync(*Store, key, value, commitLsn - 1, ApplyContext::RecoveryRedo);
          CODING_ERROR_ASSERT(result == nullptr);
          co_await ChangeRole(*Store, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
          result = co_await ApplyAddOperationAsync(*Store, key, value, commitLsn - 1, ApplyContext::SecondaryRedo);
          CODING_ERROR_ASSERT(result == nullptr);
           co_return;
       }

        ktl::Awaitable<void> Idempotency_DoublePrepare_SkipDuplicate_Test()
       {
           int key1 = 5;
           int key2 = 6;
           int value = 7;

           CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->CheckpointLSN == 0);
           LONG64 commitLsnOne = 0;

           {
               WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
               co_await Store->AddAsync(*tx->StoreTransactionSPtr, key1, value, Common::TimeSpan::MaxValue, CancellationToken::None);
               co_await VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key1, -1, value);
               co_await tx->CommitAsync();
               commitLsnOne = tx->TransactionSPtr->CommitSequenceNumber;
           }

           auto checkpointLSN1 = Replicator->IncrementAndGetCommitSequenceNumber();
           Store->PrepareCheckpoint(checkpointLSN1);

           LONG64 commitLsnTwo = 0;

           {
               WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
               co_await Store->AddAsync(*tx->StoreTransactionSPtr, key2, value, Common::TimeSpan::MaxValue, CancellationToken::None);
               co_await VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key2, -1, value);
               co_await tx->CommitAsync();
               commitLsnTwo = tx->TransactionSPtr->CommitSequenceNumber;
           }

           // Checkpoint includes preparecheckpoint, perform, and complete.
           auto checkpointLSN2 = co_await CheckpointAsync(*Store);
           co_await CloseAndReOpenStoreAsync();
           CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->CheckpointLSN == checkpointLSN2);

           // Duplicate apply before prepare one.
           co_await VerifyAllDuplicateApplyOperationsAsync(key1, value, commitLsnOne - 1);

           // Duplicate apply after prepare one
           co_await VerifyAllDuplicateApplyOperationsAsync(key2, value, commitLsnTwo - 1);

           co_return;
       }

        ktl::Awaitable<void> Idempotency_Upgrade_RecoverOldFormat_Add_ExistingWeakCheckShouldCatch_SkipDuplicate_Test()
       {
           CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->CheckpointLSN == 0);
           LONG64 commitLsn = 0;

           {
               WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
               co_await Store->AddAsync(*tx->StoreTransactionSPtr, 17, 17, Common::TimeSpan::MaxValue, CancellationToken::None);
               co_await tx->CommitAsync();
               commitLsn = tx->TransactionSPtr->CommitSequenceNumber;
           }

           MetadataManagerFileProperties::Test_IgnoreCheckpointLSN = true;
           co_await CheckpointAsync();
           MetadataManagerFileProperties::Test_IgnoreCheckpointLSN = false;
           MetadataTable::Test_IsCheckpointLSNExpected = false;
           co_await CloseAndReOpenStoreAsync();
           MetadataTable::Test_IsCheckpointLSNExpected = true;

           // Verify Recovery checkpoint lsn.
           CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->CheckpointLSN == -1);

           // Test: Recovery Apply (IsReadable does not make a difference) - Known key
           auto lockContext = co_await ApplyAddOperationAsync(*Store, 17, -1, commitLsn - 1, ApplyContext::RecoveryRedo);
           CODING_ERROR_ASSERT(lockContext != nullptr);
           Store->Unlock(*lockContext);
           co_await VerifyKeyExistsAsync(*Store, 17, -1, 17);

           // Test: Recovery Apply (IsReadable does not make a difference) - Unknown key
           lockContext = co_await ApplyAddOperationAsync(*Store, 18, 18, commitLsn - 1, ApplyContext::RecoveryRedo);
           CODING_ERROR_ASSERT(lockContext != nullptr);
           Store->Unlock(*lockContext);
           co_await VerifyKeyExistsAsync(*Store, 18, -1, 18);

           co_await ChangeRole(*Store, FABRIC_REPLICA_ROLE_IDLE_SECONDARY);

           // Test: Secondary Apply (IsReadable = false) - Known key
           Replicator->SetReadable(false);
           lockContext = co_await ApplyAddOperationAsync(*Store, 17, -2, commitLsn - 1, ApplyContext::SecondaryRedo);
           CODING_ERROR_ASSERT(lockContext != nullptr);
           Store->Unlock(*lockContext);
           co_await VerifyKeyExistsAsync(*Store, 17, -1, 17);

           // Test: Secondary Apply (IsReadable = false) - Unknown key.
           Replicator->SetReadable(false);
           lockContext = co_await ApplyAddOperationAsync(*Store, 18, 18, commitLsn - 1, ApplyContext::SecondaryRedo);
           CODING_ERROR_ASSERT(lockContext != nullptr);
           Store->Unlock(*lockContext);
           co_await VerifyKeyExistsAsync(*Store, 18, -1, 18);

           // Note that (IsReadable = true) will assert if SecondaryRedo.
           co_return;
       }

        ktl::Awaitable<void> Idempotency_Upgrade_RecoverOldFormat_Remove_ExistingWeakCheckCannotCatch_NotAsserted_Test()
       {
           CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->CheckpointLSN == 0);

           {
               WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
               co_await Store->AddAsync(*tx->StoreTransactionSPtr, 17, 17, Common::TimeSpan::MaxValue, CancellationToken::None);
               co_await tx->CommitAsync();
           }

           MetadataManagerFileProperties::Test_IgnoreCheckpointLSN = true;
           auto gapLSN = Replicator->IncrementAndGetCommitSequenceNumber();
           co_await CheckpointAsync();

           MetadataManagerFileProperties::Test_IgnoreCheckpointLSN = false;
           MetadataTable::Test_IsCheckpointLSNExpected = false;

           co_await CloseAndReOpenStoreAsync();
           MetadataTable::Test_IsCheckpointLSNExpected = true;

           // Verify Recovery checkpoint lsn.
           CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->CheckpointLSN == -1);

           // Try Recovery Apply
           auto lockContext = co_await ApplyRemoveOperationAsync(*Store, 18, 18, gapLSN, ApplyContext::RecoveryRedo);
           Store->Unlock(*lockContext);

           co_await VerifyKeyExistsAsync(*Store, 17, -1, 17);

           // Verify that blind write is accepted.
           co_await VerifyKeyDoesNotExistAsync(*Store, 18);
           co_return;
       }

        ktl::Awaitable<void> Idempotency_Copy_ReplayOperationsLowerThanCopiedCheckpointLSN_ReplayIgnored_Test()
       {
           const int NumberOfKeys = 1000;

           // Setup state - 1000 keys
           for (long key = 0; key < NumberOfKeys; key++)
           {
               {
                   WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                   co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, key, Common::TimeSpan::MaxValue, CancellationToken::None);
                   co_await tx->CommitAsync();
               }
           }

           // Checkpoint Primary
           auto checkpointLSN = co_await CheckpointAsync(*Store);
           // Copy state to a new secondary
           auto secondaryStore = co_await CreateSecondaryAsync();
           co_await CopyCheckpointToSecondaryAsync(*secondaryStore);

           // Note: Until the first apply / preparecheckpoint, copied checkpoint is not recovered.
           // Since this operation is below checkpointLSN, it should be nooped. Since the operation is nooped, checkpointLSN is still not recovered.
           co_await ApplyAndVerifyIdempotentOperationsAsync(*secondaryStore, checkpointLSN - 2);

           // New operation
           auto lockContext = co_await ApplyAddOperationAsync(*secondaryStore, NumberOfKeys, NumberOfKeys, checkpointLSN + 1, ApplyContext::SecondaryRedo);
           secondaryStore->Unlock(*lockContext);
           lockContext = co_await ApplyRemoveOperationAsync(*secondaryStore, 0, 0, checkpointLSN + 2, ApplyContext::SecondaryRedo);
           secondaryStore->Unlock(*lockContext);

           CODING_ERROR_ASSERT(secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN == checkpointLSN);
           CODING_ERROR_ASSERT(NumberOfKeys == secondaryStore->Count);

           checkpointLSN = Replicator->IncrementAndGetCommitSequenceNumber();
           co_await PerformCheckpointAsync(*secondaryStore, checkpointLSN);
           CODING_ERROR_ASSERT(secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN == checkpointLSN);

           secondaryStore = co_await CloseAndReOpenSecondaryAsync(*secondaryStore);
           CODING_ERROR_ASSERT(secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN == checkpointLSN);
           co_return;
       }

        ktl::Awaitable<void> Idempotency_Copy_CheckpointAtLowerLSNLowerThanCopyCheckpoint_ReplayIgnored_Test()
       {
           const int NumberOfKeys = 1000;

           // Setup state - 1000 keys
           LONG64 firstCheckpointLSN = -1;
           for (long key = 0; key < NumberOfKeys; key++)
           {
               if (key == NumberOfKeys / 2)
               {
                   // Old Checkpoint
                   firstCheckpointLSN = co_await CheckpointAsync(*Store);
               }

               {
                   WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                   co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, key, Common::TimeSpan::MaxValue, CancellationToken::None);
                   co_await tx->CommitAsync();
               }
           }

           // Checkpoint Primary
           auto copyCheckpointLSN = co_await CheckpointAsync(*Store);
           // Copy state to a new secondary
           auto secondaryStore = co_await CreateSecondaryAsync();
           co_await CopyCheckpointToSecondaryAsync(*secondaryStore);

           // Note: Until the first apply / preparecheckpoint, copied checkpoint is not recovered.
           // Since this operation is below checkpointLSN, it should be nooped. Since the operation is nooped, checkpointLSN is still not recovered.
           co_await ApplyAndVerifyIdempotentOperationsAsync(*secondaryStore, firstCheckpointLSN - 2);

           // New operations relative to first checkpoint LSN are also ignored
           co_await ApplyAndVerifyIdempotentOperationsAsync(*secondaryStore, firstCheckpointLSN + 2);

           // Checkpoint the secondary store as part of the end of copy.
           co_await PerformCheckpointAsync(*secondaryStore, firstCheckpointLSN + 3);
           CODING_ERROR_ASSERT(secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN == copyCheckpointLSN);

           // Old operation relative to copy checkpoint LSN are also ignored
           co_await ApplyAndVerifyIdempotentOperationsAsync(*secondaryStore, copyCheckpointLSN - 2);

           // New operations relative to copy checkpoint are applied.
           auto lockContext = co_await ApplyAddOperationAsync(*secondaryStore, NumberOfKeys, NumberOfKeys, copyCheckpointLSN + 1, ApplyContext::SecondaryRedo);
           CODING_ERROR_ASSERT(lockContext != nullptr);
           secondaryStore->Unlock(*lockContext);

           lockContext = co_await ApplyRemoveOperationAsync(*secondaryStore, 0, 0, copyCheckpointLSN + 2, ApplyContext::SecondaryRedo);
           CODING_ERROR_ASSERT(lockContext != nullptr);
           secondaryStore->Unlock(*lockContext);

           lockContext = co_await ApplyRemoveOperationAsync(*secondaryStore, 1, 1, copyCheckpointLSN + 2, ApplyContext::SecondaryRedo);
           CODING_ERROR_ASSERT(lockContext != nullptr);
           secondaryStore->Unlock(*lockContext);

           CODING_ERROR_ASSERT(secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN == copyCheckpointLSN);
           CODING_ERROR_ASSERT(NumberOfKeys -1 == secondaryStore->Count);

           co_await PerformCheckpointAsync(*secondaryStore, copyCheckpointLSN + 3);
           CODING_ERROR_ASSERT(secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN == copyCheckpointLSN + 3);
           co_return;
       }

        ktl::Awaitable<void> Idempotency_Copy_RestartAfterFirstCheckpointInCopyWhenCopyMovedNewerCheckpointLSN_ReplayIgnored_Test()
       {
           const int NumberOfKeys = 1000;

           // Setup state - 1000 keys
           LONG64 firstCheckpointLSN = -1;
           for (long key = 0; key < NumberOfKeys; key++)
           {
               if (key == NumberOfKeys / 2)
               {
                   // Old Checkpoint
                   firstCheckpointLSN = co_await CheckpointAsync(*Store);
               }

               {
                   WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                   co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, key, Common::TimeSpan::MaxValue, CancellationToken::None);
                   co_await tx->CommitAsync();
               }
           }

           // Checkpoint Primary
           auto copyCheckpointLSN = co_await CheckpointAsync(*Store);
           // Copy state to a new secondary
           auto secondaryStore = co_await CreateSecondaryAsync();
           co_await CopyCheckpointToSecondaryAsync(*secondaryStore);

           // Note: Until the first apply / preparecheckpoint, copied checkpoint is not recovered.
           // Since this operation is below checkpointLSN, it should be nooped. Since the operation is nooped, checkpointLSN is still not recovered.
           co_await ApplyAndVerifyIdempotentOperationsAsync(*secondaryStore, firstCheckpointLSN - 2);

           // New operations relative to first checkpoint LSN are also ignored
           co_await ApplyAndVerifyIdempotentOperationsAsync(*secondaryStore, firstCheckpointLSN + 2);

           // Checkpoint the secondary store as part of the end of copy.
           co_await PerformCheckpointAsync(*secondaryStore, firstCheckpointLSN + 3);
           CODING_ERROR_ASSERT(secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN == copyCheckpointLSN);

           // Restart the secondary after the "copy end" checkpoint.
           secondaryStore = co_await CloseAndReOpenSecondaryAsync(*secondaryStore);
           CODING_ERROR_ASSERT(secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN == copyCheckpointLSN);

           // Old operation relative to copy checkpoint LSN are also ignored
           co_await ApplyAndVerifyIdempotentOperationsAsync(*secondaryStore, copyCheckpointLSN - 2);

           // New operations relative to copy checkpoint are applied.
           auto lockContext = co_await ApplyAddOperationAsync(*secondaryStore, NumberOfKeys, NumberOfKeys, copyCheckpointLSN + 1, ApplyContext::SecondaryRedo);
           CODING_ERROR_ASSERT(lockContext != nullptr);
           secondaryStore->Unlock(*lockContext);

           lockContext = co_await ApplyRemoveOperationAsync(*secondaryStore, 0, 0, copyCheckpointLSN + 2, ApplyContext::SecondaryRedo);
           CODING_ERROR_ASSERT(lockContext != nullptr);
           secondaryStore->Unlock(*lockContext);

           lockContext = co_await ApplyRemoveOperationAsync(*secondaryStore, 1, 1, copyCheckpointLSN + 2, ApplyContext::SecondaryRedo);
           CODING_ERROR_ASSERT(lockContext != nullptr);
           secondaryStore->Unlock(*lockContext);

           CODING_ERROR_ASSERT(secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN == copyCheckpointLSN);
           CODING_ERROR_ASSERT(NumberOfKeys - 1 == secondaryStore->Count);

           co_await PerformCheckpointAsync(*secondaryStore, copyCheckpointLSN + 3);
           CODING_ERROR_ASSERT(secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN == copyCheckpointLSN + 3);
           co_return;
       }
    #pragma endregion
   };

   BOOST_FIXTURE_TEST_SUITE(StoreTestIdempotencySuite, StoreTestIdempotency)

   BOOST_AUTO_TEST_CASE(Idempotency_Normal_SkipDuplicate)
   {
       SyncAwait(Idempotency_Normal_SkipDuplicate_Test());
   }

   BOOST_AUTO_TEST_CASE(Idempotency_DoublePrepare_SkipDuplicate)
   {
       SyncAwait(Idempotency_DoublePrepare_SkipDuplicate_Test());
   }

   BOOST_AUTO_TEST_CASE(Idempotency_Upgrade_RecoverOldFormat_Add_ExistingWeakCheckShouldCatch_SkipDuplicate)
   {
       SyncAwait(Idempotency_Upgrade_RecoverOldFormat_Add_ExistingWeakCheckShouldCatch_SkipDuplicate_Test());
   }

   BOOST_AUTO_TEST_CASE(Idempotency_Upgrade_RecoverOldFormat_Remove_ExistingWeakCheckCannotCatch_NotAsserted)
   {
       SyncAwait(Idempotency_Upgrade_RecoverOldFormat_Remove_ExistingWeakCheckCannotCatch_NotAsserted_Test());
   }


   BOOST_AUTO_TEST_CASE(Idempotency_Copy_ReplayOperationsLowerThanCopiedCheckpointLSN_ReplayIgnored)
   {
       SyncAwait(Idempotency_Copy_ReplayOperationsLowerThanCopiedCheckpointLSN_ReplayIgnored_Test());
   }

   BOOST_AUTO_TEST_CASE(Idempotency_Copy_CheckpointAtLowerLSNLowerThanCopyCheckpoint_ReplayIgnored)
   {
       SyncAwait(Idempotency_Copy_CheckpointAtLowerLSNLowerThanCopyCheckpoint_ReplayIgnored_Test());
   }

   BOOST_AUTO_TEST_CASE(Idempotency_Copy_RestartAfterFirstCheckpointInCopyWhenCopyMovedNewerCheckpointLSN_ReplayIgnored)
   {
       SyncAwait(Idempotency_Copy_RestartAfterFirstCheckpointInCopyWhenCopyMovedNewerCheckpointLSN_ReplayIgnored_Test());
   }

   BOOST_AUTO_TEST_SUITE_END()
}
