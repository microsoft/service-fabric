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

      void VerifyAllDuplicateApplyOperations(
          __in int key,
          __in int value,
          __in LONG64 commitSequenceNumber,
          bool lockContextExpected = false)
      {
          auto result = SyncAwait(ApplyAddOperationAsync(*Store, key, value, commitSequenceNumber, ApplyContext::RecoveryRedo));
          if (lockContextExpected == false)
          {
              CODING_ERROR_ASSERT(result == nullptr);
          }
          else
          {
              Store->Unlock(*result);
          }

          result = SyncAwait(ApplyAddOperationAsync(*Store, key, value, commitSequenceNumber, ApplyContext::SecondaryRedo));
          if (lockContextExpected == false)
          {
              CODING_ERROR_ASSERT(result == nullptr);
          }
          else
          {
              Store->Unlock(*result);
          }
      }

      void ApplyAndVerifyIdempotentOperations(
          __in Data::TStore::Store<int, int>& secondaryStore,
          __in LONG64 commitSequenceNumber)
      {
          auto result = SyncAwait(ApplyAddOperationAsync(secondaryStore, 0, -1, commitSequenceNumber, ApplyContext::SecondaryRedo));
          CODING_ERROR_ASSERT(result == nullptr);
          result = SyncAwait(ApplyAddOperationAsync(secondaryStore, 0, -1, commitSequenceNumber, ApplyContext::SecondaryRedo));
          CODING_ERROR_ASSERT(result == nullptr);
          result = SyncAwait(ApplyRemoveOperationAsync(secondaryStore, 0, -1, commitSequenceNumber, ApplyContext::SecondaryRedo));
          CODING_ERROR_ASSERT(result == nullptr);
          result = SyncAwait(ApplyRemoveOperationAsync(secondaryStore, 0, -1, commitSequenceNumber, ApplyContext::SecondaryRedo));
          CODING_ERROR_ASSERT(result == nullptr);
      }

   };

   BOOST_FIXTURE_TEST_SUITE(StoreTestIdempotencySuite, StoreTestIdempotency)

   BOOST_AUTO_TEST_CASE(Idempotency_Normal_SkipDuplicate)
   {
      int key = 5;
      int value = 6;

      CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->CheckpointLSN == 0);
      LONG64 commitLsn = 0;

      {
         WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
         SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, Common::TimeSpan::MaxValue, CancellationToken::None));
         SyncAwait(VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key, -1, value));
         SyncAwait(tx->CommitAsync());
         commitLsn = tx->TransactionSPtr->CommitSequenceNumber;
      }

      auto checkpointLSN = Checkpoint(*Store);
      CloseAndReOpenStore();
      CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->CheckpointLSN == checkpointLSN);

      auto result = SyncAwait(ApplyAddOperationAsync(*Store, key, value, commitLsn - 1, ApplyContext::RecoveryRedo));
      CODING_ERROR_ASSERT(result == nullptr);
      SyncAwait(ChangeRole(*Store, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY));
      result = SyncAwait(ApplyAddOperationAsync(*Store, key, value, commitLsn - 1, ApplyContext::SecondaryRedo));
      CODING_ERROR_ASSERT(result == nullptr);
   }

   BOOST_AUTO_TEST_CASE(Idempotency_DoublePrepare_SkipDuplicate)
   {
       int key1 = 5;
       int key2 = 6;
       int value = 7;

       CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->CheckpointLSN == 0);
       LONG64 commitLsnOne = 0;

       {
           WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
           SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key1, value, Common::TimeSpan::MaxValue, CancellationToken::None));
           SyncAwait(VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key1, -1, value));
           SyncAwait(tx->CommitAsync());
           commitLsnOne = tx->TransactionSPtr->CommitSequenceNumber;
       }

       auto checkpointLSN1 = Replicator->IncrementAndGetCommitSequenceNumber();
       Store->PrepareCheckpoint(checkpointLSN1);

       LONG64 commitLsnTwo = 0;

       {
           WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
           SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key2, value, Common::TimeSpan::MaxValue, CancellationToken::None));
           SyncAwait(VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key2, -1, value));
           SyncAwait(tx->CommitAsync());
           commitLsnTwo = tx->TransactionSPtr->CommitSequenceNumber;
       }

       // Checkpoint includes preparecheckpoint, perform, and complete.
       auto checkpointLSN2 = Checkpoint(*Store);
       CloseAndReOpenStore();
       CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->CheckpointLSN == checkpointLSN2);

       // Duplicate apply before prepare one.
       VerifyAllDuplicateApplyOperations(key1, value, commitLsnOne - 1);

       // Duplicate apply after prepare one
       VerifyAllDuplicateApplyOperations(key2, value, commitLsnTwo - 1);

   }

   BOOST_AUTO_TEST_CASE(Idempotency_Upgrade_RecoverOldFormat_Add_ExistingWeakCheckShouldCatch_SkipDuplicate)
   {
       CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->CheckpointLSN == 0);
       LONG64 commitLsn = 0;

       {
           WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
           SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, 17, 17, Common::TimeSpan::MaxValue, CancellationToken::None));
           SyncAwait(tx->CommitAsync());
           commitLsn = tx->TransactionSPtr->CommitSequenceNumber;
       }

       MetadataManagerFileProperties::Test_IgnoreCheckpointLSN = true;
       Checkpoint();
       MetadataManagerFileProperties::Test_IgnoreCheckpointLSN = false;
       MetadataTable::Test_IsCheckpointLSNExpected = false;
       CloseAndReOpenStore();
       MetadataTable::Test_IsCheckpointLSNExpected = true;

       // Verify Recovery checkpoint lsn.
       CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->CheckpointLSN == -1);

       // Test: Recovery Apply (IsReadable does not make a difference) - Known key
       auto lockContext = SyncAwait(ApplyAddOperationAsync(*Store, 17, -1, commitLsn - 1, ApplyContext::RecoveryRedo));
       CODING_ERROR_ASSERT(lockContext != nullptr);
       Store->Unlock(*lockContext);
       SyncAwait(VerifyKeyExistsAsync(*Store, 17, -1, 17));

       // Test: Recovery Apply (IsReadable does not make a difference) - Unknown key
       lockContext = SyncAwait(ApplyAddOperationAsync(*Store, 18, 18, commitLsn - 1, ApplyContext::RecoveryRedo));
       CODING_ERROR_ASSERT(lockContext != nullptr);
       Store->Unlock(*lockContext);
       SyncAwait(VerifyKeyExistsAsync(*Store, 18, -1, 18));

       SyncAwait(ChangeRole(*Store, FABRIC_REPLICA_ROLE_IDLE_SECONDARY));

       // Test: Secondary Apply (IsReadable = false) - Known key
       Replicator->SetReadable(false);
       lockContext = SyncAwait(ApplyAddOperationAsync(*Store, 17, -2, commitLsn - 1, ApplyContext::SecondaryRedo));
       CODING_ERROR_ASSERT(lockContext != nullptr);
       Store->Unlock(*lockContext);
       SyncAwait(VerifyKeyExistsAsync(*Store, 17, -1, 17));

       // Test: Secondary Apply (IsReadable = false) - Unknown key.
       Replicator->SetReadable(false);
       lockContext = SyncAwait(ApplyAddOperationAsync(*Store, 18, 18, commitLsn - 1, ApplyContext::SecondaryRedo));
       CODING_ERROR_ASSERT(lockContext != nullptr);
       Store->Unlock(*lockContext);
       SyncAwait(VerifyKeyExistsAsync(*Store, 18, -1, 18));

       // Note that (IsReadable = true) will assert if SecondaryRedo.
   }

   BOOST_AUTO_TEST_CASE(Idempotency_Upgrade_RecoverOldFormat_Remove_ExistingWeakCheckCannotCatch_NotAsserted)
   {
       CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->CheckpointLSN == 0);

       {
           WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
           SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, 17, 17, Common::TimeSpan::MaxValue, CancellationToken::None));
           SyncAwait(tx->CommitAsync());
       }

       MetadataManagerFileProperties::Test_IgnoreCheckpointLSN = true;
       auto gapLSN = Replicator->IncrementAndGetCommitSequenceNumber();
       Checkpoint();

       MetadataManagerFileProperties::Test_IgnoreCheckpointLSN = false;
       MetadataTable::Test_IsCheckpointLSNExpected = false;

       CloseAndReOpenStore();
       MetadataTable::Test_IsCheckpointLSNExpected = true;

       // Verify Recovery checkpoint lsn.
       CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->CheckpointLSN == -1);

       // Try Recovery Apply
       auto lockContext = SyncAwait(ApplyRemoveOperationAsync(*Store, 18, 18, gapLSN, ApplyContext::RecoveryRedo));
       Store->Unlock(*lockContext);

       SyncAwait(VerifyKeyExistsAsync(*Store, 17, -1, 17));

       // Verify that blind write is accepted.
       SyncAwait(VerifyKeyDoesNotExistAsync(*Store, 18));
   }


   BOOST_AUTO_TEST_CASE(Idempotency_Copy_ReplayOperationsLowerThanCopiedCheckpointLSN_ReplayIgnored)
   {
       const int NumberOfKeys = 1000;

       // Setup state - 1000 keys
       for (long key = 0; key < NumberOfKeys; key++)
       {
           {
               WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
               SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, key, Common::TimeSpan::MaxValue, CancellationToken::None));
               SyncAwait(tx->CommitAsync());
           }
       }

       // Checkpoint Primary
       auto checkpointLSN = Checkpoint(*Store);
       // Copy state to a new secondary
       auto secondaryStore = CreateSecondary();
       CopyCheckpointToSecondary(*secondaryStore);

       // Note: Until the first apply / preparecheckpoint, copied checkpoint is not recovered.
       // Since this operation is below checkpointLSN, it should be nooped. Since the operation is nooped, checkpointLSN is still not recovered.
       ApplyAndVerifyIdempotentOperations(*secondaryStore, checkpointLSN - 2);

       // New operation
       auto lockContext = SyncAwait(ApplyAddOperationAsync(*secondaryStore, NumberOfKeys, NumberOfKeys, checkpointLSN + 1, ApplyContext::SecondaryRedo));
       secondaryStore->Unlock(*lockContext);
       lockContext = SyncAwait(ApplyRemoveOperationAsync(*secondaryStore, 0, 0, checkpointLSN + 2, ApplyContext::SecondaryRedo));
       secondaryStore->Unlock(*lockContext);

       CODING_ERROR_ASSERT(secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN == checkpointLSN);
       CODING_ERROR_ASSERT(NumberOfKeys == secondaryStore->Count);

       checkpointLSN = Replicator->IncrementAndGetCommitSequenceNumber();
       PerformCheckpoint(*secondaryStore, checkpointLSN);
       CODING_ERROR_ASSERT(secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN == checkpointLSN);

       secondaryStore = CloseAndReOpenSecondary(*secondaryStore);
       CODING_ERROR_ASSERT(secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN == checkpointLSN);
   }

   BOOST_AUTO_TEST_CASE(Idempotency_Copy_CheckpointAtLowerLSNLowerThanCopyCheckpoint_ReplayIgnored)
   {
       const int NumberOfKeys = 1000;

       // Setup state - 1000 keys
       LONG64 firstCheckpointLSN = -1;
       for (long key = 0; key < NumberOfKeys; key++)
       {
           if (key == NumberOfKeys / 2)
           {
               // Old Checkpoint
               firstCheckpointLSN = Checkpoint(*Store);
           }

           {
               WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
               SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, key, Common::TimeSpan::MaxValue, CancellationToken::None));
               SyncAwait(tx->CommitAsync());
           }
       }

       // Checkpoint Primary
       auto copyCheckpointLSN = Checkpoint(*Store);
       // Copy state to a new secondary
       auto secondaryStore = CreateSecondary();
       CopyCheckpointToSecondary(*secondaryStore);

       // Note: Until the first apply / preparecheckpoint, copied checkpoint is not recovered.
       // Since this operation is below checkpointLSN, it should be nooped. Since the operation is nooped, checkpointLSN is still not recovered.
       ApplyAndVerifyIdempotentOperations(*secondaryStore, firstCheckpointLSN - 2);

       // New operations relative to first checkpoint LSN are also ignored
       ApplyAndVerifyIdempotentOperations(*secondaryStore, firstCheckpointLSN + 2);

       // Checkpoint the secondary store as part of the end of copy.
       PerformCheckpoint(*secondaryStore, firstCheckpointLSN + 3);
       CODING_ERROR_ASSERT(secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN == copyCheckpointLSN);

       // Old operation relative to copy checkpoint LSN are also ignored
       ApplyAndVerifyIdempotentOperations(*secondaryStore, copyCheckpointLSN - 2);

       // New operations relative to copy checkpoint are applied.
       auto lockContext = SyncAwait(ApplyAddOperationAsync(*secondaryStore, NumberOfKeys, NumberOfKeys, copyCheckpointLSN + 1, ApplyContext::SecondaryRedo));
       CODING_ERROR_ASSERT(lockContext != nullptr);
       secondaryStore->Unlock(*lockContext);

       lockContext = SyncAwait(ApplyRemoveOperationAsync(*secondaryStore, 0, 0, copyCheckpointLSN + 2, ApplyContext::SecondaryRedo));
       CODING_ERROR_ASSERT(lockContext != nullptr);
       secondaryStore->Unlock(*lockContext);

       lockContext = SyncAwait(ApplyRemoveOperationAsync(*secondaryStore, 1, 1, copyCheckpointLSN + 2, ApplyContext::SecondaryRedo));
       CODING_ERROR_ASSERT(lockContext != nullptr);
       secondaryStore->Unlock(*lockContext);

       CODING_ERROR_ASSERT(secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN == copyCheckpointLSN);
       CODING_ERROR_ASSERT(NumberOfKeys -1 == secondaryStore->Count);

       PerformCheckpoint(*secondaryStore, copyCheckpointLSN + 3);
       CODING_ERROR_ASSERT(secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN == copyCheckpointLSN + 3);
   }

   BOOST_AUTO_TEST_CASE(Idempotency_Copy_RestartAfterFirstCheckpointInCopyWhenCopyMovedNewerCheckpointLSN_ReplayIgnored)
   {
       const int NumberOfKeys = 1000;

       // Setup state - 1000 keys
       LONG64 firstCheckpointLSN = -1;
       for (long key = 0; key < NumberOfKeys; key++)
       {
           if (key == NumberOfKeys / 2)
           {
               // Old Checkpoint
               firstCheckpointLSN = Checkpoint(*Store);
           }

           {
               WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
               SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, key, Common::TimeSpan::MaxValue, CancellationToken::None));
               SyncAwait(tx->CommitAsync());
           }
       }

       // Checkpoint Primary
       auto copyCheckpointLSN = Checkpoint(*Store);
       // Copy state to a new secondary
       auto secondaryStore = CreateSecondary();
       CopyCheckpointToSecondary(*secondaryStore);

       // Note: Until the first apply / preparecheckpoint, copied checkpoint is not recovered.
       // Since this operation is below checkpointLSN, it should be nooped. Since the operation is nooped, checkpointLSN is still not recovered.
       ApplyAndVerifyIdempotentOperations(*secondaryStore, firstCheckpointLSN - 2);

       // New operations relative to first checkpoint LSN are also ignored
       ApplyAndVerifyIdempotentOperations(*secondaryStore, firstCheckpointLSN + 2);

       // Checkpoint the secondary store as part of the end of copy.
       PerformCheckpoint(*secondaryStore, firstCheckpointLSN + 3);
       CODING_ERROR_ASSERT(secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN == copyCheckpointLSN);

       // Restart the secondary after the "copy end" checkpoint.
       secondaryStore = CloseAndReOpenSecondary(*secondaryStore);
       CODING_ERROR_ASSERT(secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN == copyCheckpointLSN);

       // Old operation relative to copy checkpoint LSN are also ignored
       ApplyAndVerifyIdempotentOperations(*secondaryStore, copyCheckpointLSN - 2);

       // New operations relative to copy checkpoint are applied.
       auto lockContext = SyncAwait(ApplyAddOperationAsync(*secondaryStore, NumberOfKeys, NumberOfKeys, copyCheckpointLSN + 1, ApplyContext::SecondaryRedo));
       CODING_ERROR_ASSERT(lockContext != nullptr);
       secondaryStore->Unlock(*lockContext);

       lockContext = SyncAwait(ApplyRemoveOperationAsync(*secondaryStore, 0, 0, copyCheckpointLSN + 2, ApplyContext::SecondaryRedo));
       CODING_ERROR_ASSERT(lockContext != nullptr);
       secondaryStore->Unlock(*lockContext);

       lockContext = SyncAwait(ApplyRemoveOperationAsync(*secondaryStore, 1, 1, copyCheckpointLSN + 2, ApplyContext::SecondaryRedo));
       CODING_ERROR_ASSERT(lockContext != nullptr);
       secondaryStore->Unlock(*lockContext);

       CODING_ERROR_ASSERT(secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN == copyCheckpointLSN);
       CODING_ERROR_ASSERT(NumberOfKeys - 1 == secondaryStore->Count);

       PerformCheckpoint(*secondaryStore, copyCheckpointLSN + 3);
       CODING_ERROR_ASSERT(secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN == copyCheckpointLSN + 3);
   }

   BOOST_AUTO_TEST_SUITE_END()
}
