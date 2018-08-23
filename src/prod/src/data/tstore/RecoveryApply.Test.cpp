// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace TStoreTests
{
   using namespace ktl;
   using namespace Data::TStore;
   using namespace Data::Utilities;

   class RecoveryApplyTest : public TStoreTestBase<int, int, IntComparer, TestStateSerializer<int>, TestStateSerializer<int>>
   {
   public:
      RecoveryApplyTest()
      {
         Setup();
         NTSTATUS status = TestStateSerializer<int>::Create(GetAllocator(), serializerSPtr_);
         Diagnostics::Validate(status);
      }

      ~RecoveryApplyTest()
      {
         serializerSPtr_ = nullptr;
         Cleanup();
      }

      OperationData::SPtr GetBytes(__in int& value)
      {
         Utilities::BinaryWriter binaryWriter(GetAllocator());
         serializerSPtr_->Write(value, binaryWriter);

         OperationData::SPtr operationDataSPtr = OperationData::Create(this->GetAllocator());
         operationDataSPtr->Append(*binaryWriter.GetBuffer(0));

         return operationDataSPtr;
      }

      ktl::Awaitable<void> AddKeyAsync(int key, int value)
      {
         auto txn = CreateWriteTransaction();
         co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
         co_await txn->CommitAsync();
         co_return;
      }

      ktl::Awaitable<void> UpdateKeyAsync(int key, int value)
      {
         auto txn = CreateWriteTransaction();
         co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
         co_await txn->CommitAsync();
         co_return;
      }

      ktl::Awaitable<void> RemoveKeyAsync(int key)
      {
          auto txn = CreateWriteTransaction();
          co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);
          co_await txn->CommitAsync();
          co_return;
      }

      ktl::Awaitable<void> CheckpointAndRecoverAsync()
      {
          co_await CheckpointAsync();
          co_await CloseAndReOpenStoreAsync();
          co_return;
      }

      ktl::Awaitable<void> RecoverAddAsync(int key, int value, LONG64 commitLSN = -1)
      {
          KAllocator& allocator = GetAllocator();

          if (commitLSN == -1) commitLSN = Replicator->IncrementAndGetCommitSequenceNumber();
          
          Transaction::SPtr tx = CreateReplicatorTransaction();
          Transaction::CSPtr txnCSPtr = tx.RawPtr();

          tx->CommitSequenceNumber = commitLSN;

          KSharedPtr<MetadataOperationDataKV<int, int> const> metadataCSPtr = nullptr;
          NTSTATUS status = MetadataOperationDataKV<int, int>::Create(
             key,
             value,
             Constants::SerializedVersion,
             StoreModificationType::Enum::Add,
             txnCSPtr->TransactionId,
             GetBytes(key),
             allocator,
             metadataCSPtr);
          Diagnostics::Validate(status);

          RedoUndoOperationData::SPtr redoDataSPtr = nullptr;
          RedoUndoOperationData::Create(GetAllocator(), GetBytes(value), nullptr, redoDataSPtr);

          OperationData::SPtr redoSPtr = redoDataSPtr.DownCast<OperationData>();
          OperationData::SPtr undoSPtr = nullptr;

          auto operationContext = co_await Store->ApplyAsync(
             commitLSN,
             *txnCSPtr,
             ApplyContext::RecoveryRedo,
             metadataCSPtr.RawPtr(),
             redoDataSPtr.RawPtr());

          if (operationContext != nullptr)
          {
              Store->Unlock(*operationContext);
          }

          co_return;
      }

      ktl::Awaitable<void> RecoverUpdateAsync(int key, int value, LONG64 commitLSN = -1)
      {
          KAllocator& allocator = GetAllocator();
          
          if (commitLSN == -1) commitLSN = Replicator->IncrementAndGetCommitSequenceNumber();

          Transaction::SPtr tx = CreateReplicatorTransaction();
          Transaction::CSPtr txnCSPtr = tx.RawPtr();
          tx->CommitSequenceNumber = commitLSN;

          KSharedPtr<MetadataOperationDataKV<int, int> const> metadataCSPtr = nullptr;
          NTSTATUS status = MetadataOperationDataKV<int, int>::Create(
             key,
             value,
             Constants::SerializedVersion,
             StoreModificationType::Enum::Update,
             txnCSPtr->TransactionId,
             GetBytes(key),
             allocator,
             metadataCSPtr);
          Diagnostics::Validate(status);

          RedoUndoOperationData::SPtr redoDataSPtr = nullptr;
          RedoUndoOperationData::Create(GetAllocator(), GetBytes(value), nullptr, redoDataSPtr);

          OperationData::SPtr redoSPtr = redoDataSPtr.DownCast<OperationData>();
          OperationData::SPtr undoSPtr = nullptr;

          auto operationContext = co_await Store->ApplyAsync(
             commitLSN,
             *txnCSPtr,
             ApplyContext::RecoveryRedo,
             metadataCSPtr.RawPtr(),
             redoDataSPtr.RawPtr());

          if (operationContext != nullptr)
          {
              Store->Unlock(*operationContext);
          }

          co_return;
      }

      ktl::Awaitable<void> RecoverRemoveAsync(int key, LONG64 commitLSN = -1)
      {
          KAllocator& allocator = GetAllocator();
          
          if (commitLSN == -1) commitLSN = Replicator->IncrementAndGetCommitSequenceNumber();

          Transaction::SPtr tx = CreateReplicatorTransaction();
          Transaction::CSPtr txnCSPtr = tx.RawPtr();
          tx->CommitSequenceNumber = commitLSN;

          KSharedPtr<MetadataOperationDataK<int> const> metadataCSPtr = nullptr;
          NTSTATUS status = MetadataOperationDataK<int>::Create(
             key,
             Constants::SerializedVersion,
             StoreModificationType::Enum::Remove,
             txnCSPtr->TransactionId,
             GetBytes(key),
             allocator,
             metadataCSPtr);
          Diagnostics::Validate(status);

          RedoUndoOperationData::SPtr redoDataSPtr = nullptr;
          RedoUndoOperationData::Create(GetAllocator(), nullptr, nullptr, redoDataSPtr);

          OperationData::SPtr redoSPtr = redoDataSPtr.DownCast<OperationData>();
          OperationData::SPtr undoSPtr = nullptr;

          auto operationContext = co_await Store->ApplyAsync(
             commitLSN,
             *txnCSPtr,
             ApplyContext::RecoveryRedo,
             metadataCSPtr.RawPtr(),
             redoDataSPtr.RawPtr());

          if (operationContext != nullptr)
          {
              Store->Unlock(*operationContext);
          }

          co_return;
      }

   private:
      KtlSystem* ktlSystem_;
      KSharedPtr<TestStateSerializer<int>> serializerSPtr_;

#pragma region test functions
    public:
        ktl::Awaitable<void> Recover_AddOperation_ShouldSucceed_Test()
       {
           auto key = 7;
           auto value = 6;
           co_await RecoverAddAsync(key, value);
           co_await VerifyKeyExistsAsync(*Store, key, -1, value);
           co_return;
       }

        ktl::Awaitable<void> Recover_UpdateOperation_ShouldSucceed_Test()
        {
            int key = 7;
            int value = 6;
            int updatedValue = 9;

            co_await AddKeyAsync(key, value);
            co_await VerifyKeyExistsAsync(*Store, key, -1, value);

            co_await RecoverUpdateAsync(key, updatedValue);
            co_await VerifyKeyExistsAsync(*Store, key, -1, updatedValue);
            co_return;
        }

        ktl::Awaitable<void> Recover_RemoveOperation_ShouldSucceed_Test()
        {
            int key = 7;
            int value = 6;

            co_await AddKeyAsync(key, value);
            co_await VerifyKeyExistsAsync(*Store, key, -1, value);

            co_await RecoverRemoveAsync(key);
            co_await VerifyKeyDoesNotExistAsync(*Store, key);
            co_return;
        }

        ktl::Awaitable<void> RecoverAdd_Checkpoint_Recover_ShouldSucceed_Test()
        {
            int key = 7;
            int value = 6;

            co_await RecoverAddAsync(key, value);
            co_await VerifyKeyExistsAsync(*Store, key, -1, value);

            co_await CheckpointAndRecoverAsync();

            co_await VerifyKeyExistsAsync(*Store, key, -1, value);
            co_return;
        }

        ktl::Awaitable<void> Add_RecoverUpdate_Checkpoint_Recover_ShouldSucceed_Test()
        {
            int key = 7;
            int value = 6;
            int updatedValue = 8;

            co_await AddKeyAsync(key, value);
            co_await VerifyKeyExistsAsync(*Store, key, -1, value);

            co_await RecoverUpdateAsync(key, updatedValue);
            co_await VerifyKeyExistsAsync(*Store, key, -1, updatedValue);

            co_await CheckpointAndRecoverAsync();
        
            co_await VerifyKeyExistsAsync(*Store, key, -1, updatedValue);
            co_return;
        }

        ktl::Awaitable<void> Add_RecoverRemove_Checkpoint_Recover_ShouldSucceed_Test()
        {
            int key = 7;
            int value = 6;

            co_await AddKeyAsync(key, value);
            co_await VerifyKeyExistsAsync(*Store, key, -1, value);

            co_await RecoverRemoveAsync(key);
            co_await VerifyKeyDoesNotExistAsync(*Store, key);

            co_await CheckpointAndRecoverAsync();
        
            co_await VerifyKeyDoesNotExistAsync(*Store, key);
            co_return;
        }

        ktl::Awaitable<void> Add_RecoverUpdate_Remove_Checkpoint_Recover_ShouldSucceed_Test()
        {
            int key = 7;
            int value = 6;
            int updateValue = 8;

            co_await AddKeyAsync(key, value);
            co_await VerifyKeyExistsAsync(*Store, key, -1, value);

            co_await RecoverUpdateAsync(key, updateValue);
            co_await VerifyKeyExistsAsync(*Store, key, -1, updateValue);

            co_await RemoveKeyAsync(key);
            co_await VerifyKeyDoesNotExistAsync(*Store, key);

            co_await CheckpointAndRecoverAsync();
            co_await VerifyKeyDoesNotExistAsync(*Store, key);

            co_return;
        }

        ktl::Awaitable<void> Add_RecoverAddDifferent_Checkpoint_Recover_ShouldSucceed_Test()
        {
            int key1 = 7;
            int key2 = 8;
            int value = 6;

            co_await AddKeyAsync(key1, value);
            co_await RecoverAddAsync(key2, value);

            co_await VerifyKeyExistsAsync(*Store, key1, -1, value);
            co_await VerifyKeyExistsAsync(*Store, key2, -1, value);

            co_await CheckpointAndRecoverAsync();

            co_await VerifyKeyExistsAsync(*Store, key1, -1, value);
            co_await VerifyKeyExistsAsync(*Store, key2, -1, value);
            co_return;
        }

        ktl::Awaitable<void> RecoveryApply_MultipleTimes_ShouldSucceed_Test()
        {
            int key = 7;
            int value = 6;

            co_await AddKeyAsync(key, value);

            co_await CheckpointAndRecoverAsync();

            for (ULONG32 i = 0; i < 81; i++)
            {
                switch (i % 3)
                {
                case 0: co_await RecoverUpdateAsync(key, value); break;
                case 1: co_await RecoverRemoveAsync(key); break;
                case 2: co_await RecoverAddAsync(key, value); break;
                }
            }

            co_await VerifyKeyExistsAsync(*Store, key, -1, value);
            co_await CheckpointAndRecoverAsync();
            co_await VerifyKeyExistsAsync(*Store, key, -1, value);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_AddMany_RecoverRemoveMany_ShouldSucceed_Test()
        {
            LONG32 numItems = 100;
        
            for (int i = numItems - 1; i >= 0; i--)
            {
                co_await AddKeyAsync(i, i);
            }

            co_await CheckpointAsync();

            for (int i = 0; i < numItems; i++)
            {
                co_await RecoverRemoveAsync(i);
            }

            co_await CheckpointAsync();

            for (int i = 0; i < numItems; i++)
            {
                co_await VerifyKeyDoesNotExistAsync(*Store, i);
            }
            co_return;
        }

        ktl::Awaitable<void> Recover_AddNotInCheckpoint_ShouldSucceed_Test()
        {
            co_await RecoverAddAsync(1, 1);
            co_await RecoverAddAsync(2, 2);
            co_await RecoverAddAsync(3, 3);

            co_await UpdateKeyAsync(1, 11);
            co_await UpdateKeyAsync(2, 22);
            co_await UpdateKeyAsync(3, 33);

            co_await RemoveKeyAsync(3);

            co_await CheckpointAsync();

            co_await AddKeyAsync(4, 44);  // This add won't be in the checkpoint

            co_await CloseAndReOpenStoreAsync();  // Recover without checkpointing again

            co_await VerifyKeyDoesNotExistAsync(*Store, 4);

            co_await RecoverAddAsync(4, 44); // Replay the add

            co_await VerifyKeyExistsAsync(*Store, 4, -1, 44);
            co_return;
        }

        ktl::Awaitable<void> Recover_UpdateNotInCheckpoint_ShouldSucceed_Test()
        {
            co_await RecoverAddAsync(1, 1);
            co_await RecoverAddAsync(2, 2);
            co_await RecoverAddAsync(3, 3);

            co_await UpdateKeyAsync(1, 11);
            co_await UpdateKeyAsync(2, 22);
            co_await UpdateKeyAsync(3, 33);

            co_await RemoveKeyAsync(3);

            co_await AddKeyAsync(4, 4);

            co_await CheckpointAsync();

            co_await UpdateKeyAsync(4, 44);  // This update won't be in the checkpoint

            co_await CloseAndReOpenStoreAsync();  // Recover without checkpointing again

            co_await VerifyKeyExistsAsync(*Store, 4, -1, 4);

            co_await RecoverUpdateAsync(4, 44); // Replay the update

            co_await VerifyKeyExistsAsync(*Store, 4, -1, 44);
            co_return;
        }

        ktl::Awaitable<void> Recover_RemoveNotInCheckpoint_ShouldSucceed_Test()
        {
            co_await RecoverAddAsync(1, 1);
            co_await RecoverAddAsync(2, 2);
            co_await RecoverAddAsync(3, 3);

            co_await UpdateKeyAsync(1, 11);
            co_await UpdateKeyAsync(2, 22);
            co_await UpdateKeyAsync(3, 33);

            co_await RemoveKeyAsync(3);

            co_await AddKeyAsync(4, 4);

            co_await CheckpointAsync();

            co_await RemoveKeyAsync(4);  // This remove won't be in the checkpoint

            co_await CloseAndReOpenStoreAsync();  // Recover without checkpointing again

            co_await VerifyKeyExistsAsync(*Store, 4, -1, 4);

            co_await RecoverRemoveAsync(4); // Replay the remove

            co_await VerifyKeyDoesNotExistAsync(*Store, 4);
            co_return;
        }

        ktl::Awaitable<void> Recover_DuplicateAdd_ShouldSucceed_Test()
        {
            co_await AddKeyAsync(1, 1);
            co_await CheckpointAndRecoverAsync();

            LONG64 checkpointLSN;
            Diagnostics::Validate(Replicator->GetLastCommittedSequenceNumber(checkpointLSN));

            auto lastCommitLSN = checkpointLSN - 1; // Checkpoint LSN is +1 of last commit
            co_await RecoverAddAsync(1, 1, lastCommitLSN);
            co_return;
        }

        ktl::Awaitable<void> Recover_DuplicateUpdate_ShouldSucceed_Test()
        {
            co_await AddKeyAsync(1, 1);
            co_await UpdateKeyAsync(1, 2);
            co_await CheckpointAndRecoverAsync();

            LONG64 checkpointLSN;
            Diagnostics::Validate(Replicator->GetLastCommittedSequenceNumber(checkpointLSN));

            auto lastCommitLSN = checkpointLSN - 1; // Checkpoint LSN is +1 of last commit
            co_await RecoverAddAsync(1, 2, lastCommitLSN);
            co_return;
        }

        ktl::Awaitable<void> Recover_DuplicateRemove_ShouldSucceed_Test()
        {
            co_await AddKeyAsync(1, 1);
            co_await RemoveKeyAsync(1);
            co_await CheckpointAndRecoverAsync();

            LONG64 checkpointLSN;
            Diagnostics::Validate(Replicator->GetLastCommittedSequenceNumber(checkpointLSN));

            auto lastCommitLSN = checkpointLSN - 1; // Checkpoint LSN is +1 of last commit
            co_await RecoverRemoveAsync(1, lastCommitLSN);
            co_return;
        }
    #pragma endregion
   };

   BOOST_FIXTURE_TEST_SUITE(RecoveryApplyTestSuite, RecoveryApplyTest)

   BOOST_AUTO_TEST_CASE(Recover_AddOperation_ShouldSucceed)
   {
       SyncAwait(Recover_AddOperation_ShouldSucceed_Test());
   }

    BOOST_AUTO_TEST_CASE(Recover_UpdateOperation_ShouldSucceed)
    {
        SyncAwait(Recover_UpdateOperation_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Recover_RemoveOperation_ShouldSucceed)
    {
        SyncAwait(Recover_RemoveOperation_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(RecoverAdd_Checkpoint_Recover_ShouldSucceed)
    {
        SyncAwait(RecoverAdd_Checkpoint_Recover_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Add_RecoverUpdate_Checkpoint_Recover_ShouldSucceed)
    {
        SyncAwait(Add_RecoverUpdate_Checkpoint_Recover_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Add_RecoverRemove_Checkpoint_Recover_ShouldSucceed)
    {
        SyncAwait(Add_RecoverRemove_Checkpoint_Recover_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Add_RecoverUpdate_Remove_Checkpoint_Recover_ShouldSucceed)
    {
        SyncAwait(Add_RecoverUpdate_Remove_Checkpoint_Recover_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Add_RecoverAddDifferent_Checkpoint_Recover_ShouldSucceed)
    {
        SyncAwait(Add_RecoverAddDifferent_Checkpoint_Recover_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(RecoveryApply_MultipleTimes_ShouldSucceed)
    {
        SyncAwait(RecoveryApply_MultipleTimes_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_AddMany_RecoverRemoveMany_ShouldSucceed)
    {
        SyncAwait(Checkpoint_AddMany_RecoverRemoveMany_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Recover_AddNotInCheckpoint_ShouldSucceed)
    {
        SyncAwait(Recover_AddNotInCheckpoint_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Recover_UpdateNotInCheckpoint_ShouldSucceed)
    {
        SyncAwait(Recover_UpdateNotInCheckpoint_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Recover_RemoveNotInCheckpoint_ShouldSucceed)
    {
        SyncAwait(Recover_RemoveNotInCheckpoint_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Recover_DuplicateAdd_ShouldSucceed)
    {
        SyncAwait(Recover_DuplicateAdd_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Recover_DuplicateUpdate_ShouldSucceed)
    {
        SyncAwait(Recover_DuplicateUpdate_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Recover_DuplicateRemove_ShouldSucceed)
    {
        SyncAwait(Recover_DuplicateRemove_ShouldSucceed_Test());
    }

   BOOST_AUTO_TEST_SUITE_END()
}
