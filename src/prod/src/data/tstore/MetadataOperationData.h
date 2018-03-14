// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define MetadataOperationData_TAG 'doTM'

namespace Data
{
    namespace TStore
    {
        class MetadataOperationData :
            public Utilities::OperationData
        {
           K_FORCE_SHARED_WITH_INHERITANCE(MetadataOperationData);

        public:

           //
           // Operation data can be null - revisit convention.
           //
           static NTSTATUS Create(
              __in ULONG32 version,
              __in StoreModificationType::Enum modificationType,
              __in LONG64 transactionId,
              __in_opt OperationData::SPtr operationData,
              __in KAllocator& allocator,
              __out MetadataOperationData::CSPtr & result);

            __declspec(property(get = get_ModificationType)) StoreModificationType::Enum ModificationType;
            StoreModificationType::Enum get_ModificationType() const
            {
                return modificationType_;
            }

            __declspec(property(get = get_KeyBytes)) OperationData::SPtr KeyBytes;
            OperationData::SPtr get_KeyBytes() const
            {
               return keyBytesSPtr_;
            }

            __declspec(property(get = get_TransactionId)) LONG64 TransactionId;
            LONG64 get_TransactionId() const
            {
               return transactionId_;
            }

            static MetadataOperationData::CSPtr Deserialize(
               __in ULONG32 version,
               __in KAllocator & allocator,
               __in_opt OperationData::CSPtr operationData);

            virtual MetadataOperationType::Enum Type()
            {
               return MetadataOperationType::Unknown;
            }

            //
            // Operation data can be null - revisit convention.
            //
            MetadataOperationData(
               __in ULONG32 version,
               __in StoreModificationType::Enum modificationType,
               __in LONG64 transactionId,
               __in_opt OperationData::SPtr operationData);

        private:

           void Serialize();

           ULONG32 version_;
           StoreModificationType::Enum modificationType_;
           LONG64 transactionId_;
           OperationData::SPtr keyBytesSPtr_;

        };

        template <typename TKey>
        class MetadataOperationDataK : public MetadataOperationData
        {
           K_FORCE_SHARED_WITH_INHERITANCE(MetadataOperationDataK);

        public:
           //
           // Operation data can be null - revisit convention.
           //
           static NTSTATUS Create(
              __in TKey key,
              __in ULONG32 version,
              __in StoreModificationType::Enum modificationType,
              __in LONG64 transactionId,
              __in_opt OperationData::SPtr operationData,
              __in KAllocator& allocator,
              __out CSPtr & result)
           {
                 NTSTATUS status;
                 CSPtr output = _new(MetadataOperationData_TAG, allocator) MetadataOperationDataK(key, version, modificationType, transactionId, operationData);

                 if (!output)
                 {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    return status;
                 }

                 status = output->Status();
                 if (!NT_SUCCESS(status))
                 {
                    return status;
                 }

                 result = Ktl::Move(output);
                 return STATUS_SUCCESS;
           }

           __declspec(property(get = get_Key)) TKey Key;
           TKey get_Key() const
           {
              return key_;
           }

           MetadataOperationType::Enum Type() override
           {
              return MetadataOperationType::Key;
           }

           MetadataOperationDataK(
              __in TKey key,
              __in ULONG32 version,
              __in StoreModificationType::Enum modificationType,
              __in LONG64 transactionId,
              __in_opt OperationData::SPtr operationData);

        private:
           TKey key_;
        };

        template <typename TKey>
        MetadataOperationDataK<TKey>::MetadataOperationDataK(
           __in TKey key,
           __in ULONG32 version,
           __in StoreModificationType::Enum modificationType,
           __in LONG64 transactionId,
           __in_opt OperationData::SPtr operationData)
           :key_(key),
           MetadataOperationData(version, modificationType, transactionId, operationData)
        {
           KInvariant(modificationType == StoreModificationType::Enum::Add ||
              modificationType == StoreModificationType::Enum::Update ||
              modificationType == StoreModificationType::Enum::Remove);
        }

        template <typename TKey>
        MetadataOperationDataK<TKey>::~MetadataOperationDataK()
        {
        }

        template <typename TKey, typename TValue>
        class MetadataOperationDataKV :public MetadataOperationDataK<TKey>
        {
           K_FORCE_SHARED(MetadataOperationDataKV);

        public:
           //
           // Operation data can be null - revisit convention.
           //
           static NTSTATUS Create(
              __in TKey key,
              __in TValue value,
              __in ULONG32 version,
              __in StoreModificationType::Enum modificationType,
              __in LONG64 transactionId,
              __in_opt OperationData::SPtr operationData,
              __in KAllocator& allocator,
              __out CSPtr & result)
           {
              NTSTATUS status;
              CSPtr output = _new(MetadataOperationData_TAG, allocator) MetadataOperationDataKV(key, value, version, modificationType, transactionId, operationData);

              if (!output)
              {
                 status = STATUS_INSUFFICIENT_RESOURCES;
                 return status;
              }

              status = output->Status();
              if (!NT_SUCCESS(status))
              {
                 return status;
              }

              result = Ktl::Move(output);
              return STATUS_SUCCESS;
           }

           __declspec(property(get = get_Value)) TValue Value;
           TValue get_Value() const
           {
              return value_;
           }

           MetadataOperationType::Enum Type() override
           {
              return MetadataOperationType::KeyAndValue;
           }

        private :
           MetadataOperationDataKV(
              __in TKey key,
              __in TValue value,
              __in ULONG32 version,
              __in StoreModificationType::Enum modificationType,
              __in LONG64 transactionId,
              __in_opt OperationData::SPtr operationData);

           TValue value_;
        };

        template <typename TKey, typename TValue>
        MetadataOperationDataKV<TKey, TValue>::MetadataOperationDataKV(
           __in TKey key,
           __in TValue value,
           __in ULONG32 version,
           __in StoreModificationType::Enum modificationType,
           __in LONG64 transactionId,
           __in_opt OperationData::SPtr operationData)
           :value_(value),
           MetadataOperationDataK<TKey>(key, version, modificationType, transactionId, operationData)
        {
           KInvariant(modificationType == StoreModificationType::Enum::Add ||
              modificationType == StoreModificationType::Enum::Update);
        }

        template <typename TKey, typename TValue>
        MetadataOperationDataKV<TKey, TValue>::~MetadataOperationDataKV()
        {
        }
    }
}
