// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define RedoUndoOperationData_TAG 'tdUR'

namespace Data
{
    namespace TStore
    {
        class RedoUndoOperationData 
           : public Utilities::OperationData
        {
            K_FORCE_SHARED(RedoUndoOperationData)

        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __in_opt OperationData::SPtr valueOperationData,
                __in_opt OperationData::SPtr newValueOperationData,
                __out RedoUndoOperationData::SPtr & result);

        public:
            __declspec(property(get = get_ValueOperationData)) OperationData::SPtr ValueOperationData;
            OperationData::SPtr get_ValueOperationData() const
            {
               return valueOperationDataSPtr_;
            }

            __declspec(property(get = get_NewValueOperationData)) OperationData::SPtr NewValueOperationData;
            OperationData::SPtr get_NewValueOperationData() const
            {
               return newValueOperationDataSPtr_;
            }

            static RedoUndoOperationData::SPtr Deserialize(
               __in OperationData const & operationData,
               __in KAllocator& allocator);

        private:
           void Serialize();

           RedoUndoOperationData(
                __in_opt  OperationData::SPtr valueOperationData,
                __in_opt  OperationData::SPtr newValueOperationData);

            static const ULONG32 SerializedMetadataSize =
                sizeof(ULONG32)+
                sizeof(ULONG32);

            OperationData::SPtr valueOperationDataSPtr_;
            OperationData::SPtr newValueOperationDataSPtr_;
        };
    }
}

