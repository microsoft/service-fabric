// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        //
        // Represents the data for a transaction/atomic operation metadata/undo/redo
        // It is a wrapper over an array of user buffers
        //
        class OperationData 
            : public KObject<OperationData>
            , public KShared<OperationData>
        {
            K_FORCE_SHARED_WITH_INHERITANCE(OperationData)

        public:

            static OperationData::SPtr Create(__in KAllocator & allocator);

            static OperationData::CSPtr Create(
                __in KArray<KBuffer::CSPtr> const & buffers,
                __in KAllocator & allocator);

            static OperationData::CSPtr Create(
                __in OperationData const & operationData,
                __in KAllocator & allocator);

            static OperationData::CSPtr Create(
                __in OperationData const & operationData,
                __in ULONG32 startingIndex,
                __in ULONG32 count,
                __in KAllocator & allocator);

            //
            // Helper method to serialize the contents of the 'toSerialize' parameter into
            // a 'serializedOutput' out parameter.
            //
            // The 'binaryWriter' is just a pooled binary writer to be used by the helper during the serialization process
            //
            static void Serialize(
                __inout Utilities::BinaryWriter & binaryWriter,
                __in_opt OperationData const * const toSerialize,
                __out OperationData & serializedOutput);

            //
            // Helper method to de-serialize the contents of the 'binaryReader' parameter into
            // an OperationData (return type)
            //
            static OperationData::CSPtr DeSerialize(
                __in Utilities::BinaryReader & binaryReader,
                __in KAllocator & allocator);
            //
            // Helper method to de-serialize the contents of the 'inputData' parameter into
            // an OperationData (return type)
            //
            // The index indicates the index into the 'inputData' at which de-serialization starts
            //
            static OperationData::CSPtr DeSerialize(
                __in OperationData const & inputData,
                __inout INT & index,
                __in KAllocator & allocator);

            static KBuffer::CSPtr IncrementIndexAndGetBuffer(
                __in OperationData const & operation,
                __inout INT & index);

            static LONG64 GetOperationSize(
                __in OperationData const & operation);

            __declspec(property(get = get_BufferCount)) ULONG32 BufferCount;
            ULONG32 const get_BufferCount() const
            {
                return buffers_.Count();
            }

            KBuffer::CSPtr & operator[](ULONG32 i) const
            {
                return buffers_[i];
            }
            
            // 
            // Adds to the end of the buffers
            //
            void Append(__in KBuffer const & buffer);

            // 
            // Adds all the buffers in the incoming operation data to the end of the buffers
            //
            void Append(__in OperationData const & operationData);

            // 
            // Inserts at the beginning
            //
            void InsertAt0(__in KBuffer const & buffer);

            bool Test_Equals(__in OperationData const & other) const;

        private:

            OperationData(__in KArray<KBuffer::CSPtr> const & buffers);

            static KBuffer::CSPtr DeSerializeBytes(
                __in Utilities::BinaryReader & binaryReader,
                __in KAllocator & allocator);

            static KBuffer::CSPtr DeSerializeBytes(
                __in OperationData const & inputData,
                __inout INT & index,
                __in KAllocator & allocator);

            KArray<KBuffer::CSPtr> buffers_;
        };
    }
}
