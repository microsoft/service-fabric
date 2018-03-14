// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TStoreTests
{
   class TestTransactionContext
   {
   public:
      TestTransactionContext();

      TestTransactionContext(
         __in LONG64 seqeunceNumber,
         __in_opt Data::Utilities::OperationData const * const metadata,
         __in_opt Data::Utilities::OperationData const * const undo,
         __in_opt Data::Utilities::OperationData const * const redo,
         __in_opt TxnReplicator::OperationContext const * const operationContext,
         __in_opt LONG64 stateProviderId);

      ~TestTransactionContext();

      __declspec(property(get = get_SequenceNumber)) LONG64 SequenceNumber;
      LONG64 get_SequenceNumber() const
      {
         return sequenceNumber_;
      }

      __declspec(property(get = get_Metadata)) Data::Utilities::OperationData::CSPtr MetaData;
      Data::Utilities::OperationData::CSPtr get_Metadata() const
      {
         return metaData_;
      }

      __declspec(property(get = get_Undo)) Data::Utilities::OperationData::CSPtr Undo;
      Data::Utilities::OperationData::CSPtr get_Undo() const
      {
         return undo_;
      }

      __declspec(property(get = get_Redo)) Data::Utilities::OperationData::CSPtr Redo;
      Data::Utilities::OperationData::CSPtr get_Redo() const
      {
         return redo_;
      }

      __declspec(property(get = get_OperationContext, put = put_OperationContext)) TxnReplicator::OperationContext::CSPtr Context;
      TxnReplicator::OperationContext::CSPtr get_OperationContext() const
      {
         return operationContext_;
      }

      void put_OperationContext(__in TxnReplicator::OperationContext::CSPtr value)
      {
         operationContext_ = value;
      }

      __declspec(property(get = get_StateproviderId)) LONG64 StateproviderId;
      LONG64 get_StateproviderId() const
      {
         return stateproviderId_;
      }

   private:
      LONG64 sequenceNumber_ = LONG64_MIN;
      Data::Utilities::OperationData::CSPtr metaData_;
      Data::Utilities::OperationData::CSPtr undo_;
      Data::Utilities::OperationData::CSPtr redo_;
      TxnReplicator::OperationContext::CSPtr operationContext_;
      LONG64 stateproviderId_;
   };
}
