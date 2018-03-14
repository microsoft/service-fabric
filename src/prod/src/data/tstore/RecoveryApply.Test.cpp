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

      void AddKey(int key, int value)
      {
         auto txn = CreateWriteTransaction();
         SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
         SyncAwait(txn->CommitAsync());
      }

      void UpdateKey(int key, int value)
      {
         auto txn = CreateWriteTransaction();
         SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
         SyncAwait(txn->CommitAsync());
      }

      void RemoveKey(int key)
      {
          auto txn = CreateWriteTransaction();
          SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
          SyncAwait(txn->CommitAsync());
      }

      void CheckpointAndRecover()
      {
          Checkpoint();
          CloseAndReOpenStore();
      }

      void RecoverAdd(int key, int value, LONG64 commitLSN = -1)
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

          auto operationContext = SyncAwait(Store->ApplyAsync(
             commitLSN,
             *txnCSPtr,
             ApplyContext::RecoveryRedo,
             metadataCSPtr.RawPtr(),
             redoDataSPtr.RawPtr()));

          if (operationContext != nullptr)
          {
              Store->Unlock(*operationContext);
          }
      }

      void RecoverUpdate(int key, int value, LONG64 commitLSN = -1)
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

          auto operationContext = SyncAwait(Store->ApplyAsync(
             commitLSN,
             *txnCSPtr,
             ApplyContext::RecoveryRedo,
             metadataCSPtr.RawPtr(),
             redoDataSPtr.RawPtr()));

          if (operationContext != nullptr)
          {
              Store->Unlock(*operationContext);
          }
      }

      void RecoverRemove(int key, LONG64 commitLSN = -1)
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

          auto operationContext = SyncAwait(Store->ApplyAsync(
             commitLSN,
             *txnCSPtr,
             ApplyContext::RecoveryRedo,
             metadataCSPtr.RawPtr(),
             redoDataSPtr.RawPtr()));

          if (operationContext != nullptr)
          {
              Store->Unlock(*operationContext);
          }
      }

   private:
      KtlSystem* ktlSystem_;
      KSharedPtr<TestStateSerializer<int>> serializerSPtr_;
   };

   BOOST_FIXTURE_TEST_SUITE(RecoveryApplyTestSuite, RecoveryApplyTest)

   BOOST_AUTO_TEST_CASE(Recover_AddOperation_ShouldSucceed)
   {
       auto key = 7;
       auto value = 6;
       RecoverAdd(key, value);
       SyncAwait(VerifyKeyExistsAsync(*Store, key, -1, value));
   }

    BOOST_AUTO_TEST_CASE(Recover_UpdateOperation_ShouldSucceed)
    {
        int key = 7;
        int value = 6;
        int updatedValue = 9;

        AddKey(key, value);
        SyncAwait(VerifyKeyExistsAsync(*Store, key, -1, value));

        RecoverUpdate(key, updatedValue);
        SyncAwait(VerifyKeyExistsAsync(*Store, key, -1, updatedValue));
    }

    BOOST_AUTO_TEST_CASE(Recover_RemoveOperation_ShouldSucceed)
    {
        int key = 7;
        int value = 6;

        AddKey(key, value);
        SyncAwait(VerifyKeyExistsAsync(*Store, key, -1, value));

        RecoverRemove(key);
        SyncAwait(VerifyKeyDoesNotExistAsync(*Store, key));
    }

    BOOST_AUTO_TEST_CASE(RecoverAdd_Checkpoint_Recover_ShouldSucceed)
    {
        int key = 7;
        int value = 6;

        RecoverAdd(key, value);
        SyncAwait(VerifyKeyExistsAsync(*Store, key, -1, value));

        CheckpointAndRecover();

        SyncAwait(VerifyKeyExistsAsync(*Store, key, -1, value));
    }

    BOOST_AUTO_TEST_CASE(Add_RecoverUpdate_Checkpoint_Recover_ShouldSucceed)
    {
        int key = 7;
        int value = 6;
        int updatedValue = 8;

        AddKey(key, value);
        SyncAwait(VerifyKeyExistsAsync(*Store, key, -1, value));

        RecoverUpdate(key, updatedValue);
        SyncAwait(VerifyKeyExistsAsync(*Store, key, -1, updatedValue));

        CheckpointAndRecover();
        
        SyncAwait(VerifyKeyExistsAsync(*Store, key, -1, updatedValue));
    }

    BOOST_AUTO_TEST_CASE(Add_RecoverRemove_Checkpoint_Recover_ShouldSucceed)
    {
        int key = 7;
        int value = 6;

        AddKey(key, value);
        SyncAwait(VerifyKeyExistsAsync(*Store, key, -1, value));

        RecoverRemove(key);
        SyncAwait(VerifyKeyDoesNotExistAsync(*Store, key));

        CheckpointAndRecover();
        
        SyncAwait(VerifyKeyDoesNotExistAsync(*Store, key));
    }

    BOOST_AUTO_TEST_CASE(Add_RecoverUpdate_Remove_Checkpoint_Recover_ShouldSucceed)
    {
        int key = 7;
        int value = 6;
        int updateValue = 8;

        AddKey(key, value);
        SyncAwait(VerifyKeyExistsAsync(*Store, key, -1, value));

        RecoverUpdate(key, updateValue);
        SyncAwait(VerifyKeyExistsAsync(*Store, key, -1, updateValue));

        RemoveKey(key);
        SyncAwait(VerifyKeyDoesNotExistAsync(*Store, key));

        CheckpointAndRecover();
        SyncAwait(VerifyKeyDoesNotExistAsync(*Store, key));

    }

    BOOST_AUTO_TEST_CASE(Add_RecoverAddDifferent_Checkpoint_Recover_ShouldSucceed)
    {
        int key1 = 7;
        int key2 = 8;
        int value = 6;

        AddKey(key1, value);
        RecoverAdd(key2, value);

        SyncAwait(VerifyKeyExistsAsync(*Store, key1, -1, value));
        SyncAwait(VerifyKeyExistsAsync(*Store, key2, -1, value));

        CheckpointAndRecover();

        SyncAwait(VerifyKeyExistsAsync(*Store, key1, -1, value));
        SyncAwait(VerifyKeyExistsAsync(*Store, key2, -1, value));
    }

    BOOST_AUTO_TEST_CASE(RecoveryApply_MultipleTimes_ShouldSucceed)
    {
        int key = 7;
        int value = 6;

        AddKey(key, value);

        CheckpointAndRecover();

        for (ULONG32 i = 0; i < 81; i++)
        {
            switch (i % 3)
            {
            case 0: RecoverUpdate(key, value); break;
            case 1: RecoverRemove(key); break;
            case 2: RecoverAdd(key, value); break;
            }
        }

        SyncAwait(VerifyKeyExistsAsync(*Store, key, -1, value));
        CheckpointAndRecover();
        SyncAwait(VerifyKeyExistsAsync(*Store, key, -1, value));
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_AddMany_RecoverRemoveMany_ShouldSucceed)
    {
        LONG32 numItems = 100;
        
        for (int i = numItems - 1; i >= 0; i--)
        {
            AddKey(i, i);
        }

        Checkpoint();

        for (int i = 0; i < numItems; i++)
        {
            RecoverRemove(i);
        }

        Checkpoint();

        for (int i = 0; i < numItems; i++)
        {
            SyncAwait(VerifyKeyDoesNotExistAsync(*Store, i));
        }
    }

    BOOST_AUTO_TEST_CASE(Recover_AddNotInCheckpoint_ShouldSucceed)
    {
        RecoverAdd(1, 1);
        RecoverAdd(2, 2);
        RecoverAdd(3, 3);

        UpdateKey(1, 11);
        UpdateKey(2, 22);
        UpdateKey(3, 33);

        RemoveKey(3);

        Checkpoint();

        AddKey(4, 44);  // This add won't be in the checkpoint

        CloseAndReOpenStore();  // Recover without checkpointing again

        SyncAwait(VerifyKeyDoesNotExistAsync(*Store, 4));

        RecoverAdd(4, 44); // Replay the add

        SyncAwait(VerifyKeyExistsAsync(*Store, 4, -1, 44));
    }

    BOOST_AUTO_TEST_CASE(Recover_UpdateNotInCheckpoint_ShouldSucceed)
    {
        RecoverAdd(1, 1);
        RecoverAdd(2, 2);
        RecoverAdd(3, 3);

        UpdateKey(1, 11);
        UpdateKey(2, 22);
        UpdateKey(3, 33);

        RemoveKey(3);

        AddKey(4, 4);

        Checkpoint();

        UpdateKey(4, 44);  // This update won't be in the checkpoint

        CloseAndReOpenStore();  // Recover without checkpointing again

        SyncAwait(VerifyKeyExistsAsync(*Store, 4, -1, 4));

        RecoverUpdate(4, 44); // Replay the update

        SyncAwait(VerifyKeyExistsAsync(*Store, 4, -1, 44));
    }

    BOOST_AUTO_TEST_CASE(Recover_RemoveNotInCheckpoint_ShouldSucceed)
    {
        RecoverAdd(1, 1);
        RecoverAdd(2, 2);
        RecoverAdd(3, 3);

        UpdateKey(1, 11);
        UpdateKey(2, 22);
        UpdateKey(3, 33);

        RemoveKey(3);

        AddKey(4, 4);

        Checkpoint();

        RemoveKey(4);  // This remove won't be in the checkpoint

        CloseAndReOpenStore();  // Recover without checkpointing again

        SyncAwait(VerifyKeyExistsAsync(*Store, 4, -1, 4));

        RecoverRemove(4); // Replay the remove

        SyncAwait(VerifyKeyDoesNotExistAsync(*Store, 4));
    }

    BOOST_AUTO_TEST_CASE(Recover_DuplicateAdd_ShouldSucceed)
    {
        AddKey(1, 1);
        CheckpointAndRecover();

        LONG64 checkpointLSN;
        Diagnostics::Validate(Replicator->GetLastCommittedSequenceNumber(checkpointLSN));

        auto lastCommitLSN = checkpointLSN - 1; // Checkpoint LSN is +1 of last commit
        RecoverAdd(1, 1, lastCommitLSN);
    }

    BOOST_AUTO_TEST_CASE(Recover_DuplicateUpdate_ShouldSucceed)
    {
        AddKey(1, 1);
        UpdateKey(1, 2);
        CheckpointAndRecover();

        LONG64 checkpointLSN;
        Diagnostics::Validate(Replicator->GetLastCommittedSequenceNumber(checkpointLSN));

        auto lastCommitLSN = checkpointLSN - 1; // Checkpoint LSN is +1 of last commit
        RecoverAdd(1, 2, lastCommitLSN);
    }

    BOOST_AUTO_TEST_CASE(Recover_DuplicateRemove_ShouldSucceed)
    {
        AddKey(1, 1);
        RemoveKey(1);
        CheckpointAndRecover();

        LONG64 checkpointLSN;
        Diagnostics::Validate(Replicator->GetLastCommittedSequenceNumber(checkpointLSN));

        auto lastCommitLSN = checkpointLSN - 1; // Checkpoint LSN is +1 of last commit
        RecoverRemove(1, lastCommitLSN);
    }

   BOOST_AUTO_TEST_SUITE_END()
}
