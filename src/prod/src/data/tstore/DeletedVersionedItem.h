// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data 
{
    namespace TStore
    {
        template<typename TValue>
        class DeletedVersionedItem : public VersionedItem<TValue>
        {
            K_FORCE_SHARED(DeletedVersionedItem)

        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                result = _new(VERSIONEDITEM_TAG, allocator) DeletedVersionedItem();

                if (!result)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                if (!NT_SUCCESS(result->Status()))
                {
                    return Ktl::Move(result)->Status();
                }

                return STATUS_SUCCESS;
            }

            void Initialize()
            {
                this->versionSequenceNumber_ = 0;
            }

            void InitializeOnApply(__in LONG64 versionSequenceNumber)
            {
                this->versionSequenceNumber_ = versionSequenceNumber;
            }

            void InitializeOnRecovery(
                __in LONG64 versionSequenceNumber,
                __in ULONG fileId)
            {
                versionSequenceNumber_ = versionSequenceNumber;
                fileId_ = fileId;
            }

            virtual RecordKind GetRecordKind() const
            {
                return RecordKind::DeletedVersion;
            }

            virtual TValue GetValue() const
            {
                throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
            }

            virtual void SetValue(__in const TValue&)
            {
                throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
            }

            virtual void UnSetValue()
            {
                throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
            }

            virtual LONG64 GetOffset() const
            {
                throw ktl::Exception(SF_STATUS_INVALID_OPERATION); 
            }

            virtual void SetOffset(__in LONG64, __in StoreTraceComponent&)
            {
                throw ktl::Exception(SF_STATUS_INVALID_OPERATION); 
            }

            virtual int GetValueSize() const
            {
                return 0;
            }

            virtual void SetValueSize(__in int)
            {
                throw ktl::Exception(SF_STATUS_INVALID_OPERATION); 
            }

            virtual ULONG64 GetValueChecksum() const 
            {
                throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
            }

            virtual void SetValueChecksum(__in ULONG64)
            {
                throw ktl::Exception(SF_STATUS_INVALID_OPERATION); 
            }

            virtual bool GetInUse() const
            {
                throw ktl::Exception(SF_STATUS_INVALID_OPERATION); 
            }

            virtual void SetInUse(__in bool) override
            {
                throw ktl::Exception(SF_STATUS_INVALID_OPERATION); 
            }

            virtual bool GetLockBit()
            {
               throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
            }

            virtual void SetLockBit(__in bool) override
            {
               throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
            }

            virtual bool IsInMemory() const override
            {
               // Always true.
               return true;
            }

            virtual ktl::Awaitable<TValue> GetValueAsync(
               __in MetadataTable&,
               __in Data::StateManager::IStateSerializer<TValue>&,
               __in ReadMode,
               __in StoreTraceComponent &,
               __in ktl::CancellationToken const &) override
            {
               throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
            }

            virtual ktl::Awaitable<TValue> GetValueAsync(
               __in FileMetadata&,
               __in Data::StateManager::IStateSerializer<TValue>&,
               __in ReadMode,
               __in StoreTraceComponent &,
               __in ktl::CancellationToken const &) override
            {
               throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
            }
        };

        template <typename TValue>
        DeletedVersionedItem<TValue>::DeletedVersionedItem()
        {
        }

        template <typename TValue>
        DeletedVersionedItem<TValue>::~DeletedVersionedItem()
        {
        }
    }
}
