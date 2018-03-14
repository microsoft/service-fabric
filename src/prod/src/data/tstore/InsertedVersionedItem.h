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
        class InsertedVersionedItem : public VersionedItem<TValue>
        {
            K_FORCE_SHARED(InsertedVersionedItem)

        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                NTSTATUS status;
                SPtr output = _new(VERSIONEDITEM_TAG, allocator) InsertedVersionedItem();

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

            virtual TValue GetValue() const
            {
                return value_;
            }

            virtual void SetValue(__in const TValue& value)
            {
                value_ = value;
                this->SetIsInMemory(true);
            }

            virtual void UnSetValue()
            {
                this->SetIsInMemory(false);
                value_ = TValue();
            }

            virtual RecordKind GetRecordKind() const
            {
                return RecordKind::InsertedVersion;
            }

            // todo: Make overloaded create methods instead
            void Initialize(__in const TValue& value)
            {
                this->versionSequenceNumber_ = 0;
                value_ = value;
                this->SetIsInMemory(true);
                this->SetInUse(true);
            }

            void InitializeOnApply(
                __in LONG64 versionSequenceNumber,
                __in const TValue& value)
            {
                this->versionSequenceNumber_ = versionSequenceNumber;
                this->value_ = value;
                this->SetIsInMemory(true);
                this->valueSize_ = -1;
                this->SetInUse(true);
            }

            void InitializeOnRecovery(
                __in LONG64 versionSequenceNumber,
                __in ULONG32 fileId,
                __in LONG64 fileOffset,
                __in int valueSize,
                __in ULONG64 valueChecksum)
            {
                this->versionSequenceNumber_ = versionSequenceNumber;
                this->fileId_ = fileId;
                this->valueOffset_ = fileOffset;
                this->valueSize_ = valueSize;
                this->valueChecksum_ = valueChecksum;
                this->SetInUse(false);
                this->SetIsInMemory(false);
            }

        private:
            TValue value_;
        };

        template <typename TValue>
        InsertedVersionedItem<TValue>::InsertedVersionedItem()
        {
        }

        template <typename TValue>
        InsertedVersionedItem<TValue>::~InsertedVersionedItem()
        {
        }
    }
}
