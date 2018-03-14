// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class DataSerializer
        : public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
        DENY_COPY( DataSerializer );

    public:
        DataSerializer(
            Store::PartitionedReplicaId const & partitionedReplicaId,
            __in ILocalStore & store) 
            : PartitionedReplicaTraceComponent(partitionedReplicaId)
            , store_(store)
        { 
        }

        template <class T>
        Common::ErrorCode WriteData(
            ILocalStore::TransactionSPtr const & txSPtr,
            T const & data,
            std::wstring const & type,
            std::wstring const & key);

        template <class T>
        Common::ErrorCode TryReadData(
            ILocalStore::TransactionSPtr const & txSPtr,
            std::wstring const & type,
            std::wstring const & key,
            __out T & result);

        template <class T>
        Common::ErrorCode ReadCurrentData(
            ILocalStore::EnumerationSPtr const & enumSPtr, 
            __out T & result);

    private:
        ILocalStore & store_;
    };

    typedef std::unique_ptr<DataSerializer> DataSerializerUPtr;

    template <class T>
    Common::ErrorCode DataSerializer::WriteData(
        ILocalStore::TransactionSPtr const & txSPtr,
        T const & data,
        std::wstring const & type,
        std::wstring const & key)
    {
        KBuffer::SPtr bufferSPtr;
        auto error = FabricSerializer::Serialize(&data, bufferSPtr);

        if (error.IsSuccess())
        {
            FABRIC_SEQUENCE_NUMBER currentLsn = ILocalStore::SequenceNumberIgnore;
            error = store_.GetOperationLSN(
                txSPtr,
                type,
                key,
                currentLsn);

            if (error.IsError(ErrorCodeValue::StoreRecordNotFound))
            {
                error = store_.Insert(
                    txSPtr, 
                    type,
                    key,
                    bufferSPtr->GetBuffer(),
                    bufferSPtr->QuerySize(),
                    Constants::StoreMetadataSequenceNumber);
            }
            else
            {
                error = store_.Update(
                    txSPtr, 
                    type,
                    key,
                    currentLsn,
                    key,
                    bufferSPtr->GetBuffer(),
                    bufferSPtr->QuerySize(),
                    Constants::StoreMetadataSequenceNumber);
            }
        }

        WriteNoise(
            Constants::DataSerializerTraceComponent,
            "{0} wrote data item {1}:{2} with {3}", 
            this->TraceId,
            type,
            key,
            error);

        return error;
    }

    template <class T>
    Common::ErrorCode DataSerializer::TryReadData(
        ILocalStore::TransactionSPtr const & txSPtr,
        std::wstring const & type,
        std::wstring const & key,
        __out T & result)
    {
        ILocalStore::EnumerationSPtr enumSPtr;
        Common::ErrorCode error = store_.CreateEnumerationByTypeAndKey(txSPtr, type, key, enumSPtr);

        if (error.IsSuccess())
        {
            error = enumSPtr->MoveNext();
        }

        if (error.IsSuccess())
        {
            std::wstring currentKey;
            error = enumSPtr->CurrentKey(currentKey);
            
            if (error.IsSuccess())
            {
                if (currentKey == key)
                {
                    error = ReadCurrentData(enumSPtr, result);
                }
                else
                {
                    error = ErrorCodeValue::NotFound;
                }
            }
        }
        else if (error.IsError(ErrorCodeValue::EnumerationCompleted))
        {
            error = ErrorCodeValue::NotFound;
        }

        return error;
    }

    template <class T>
    Common::ErrorCode DataSerializer::ReadCurrentData(
        ILocalStore::EnumerationSPtr const & enumSPtr, 
        __out T & result)
    {
        std::vector<byte> buffer;
        Common::ErrorCode error = enumSPtr->CurrentValue(buffer);
        if (error.IsSuccess())
        {
            error = FabricSerializer::Deserialize(result, buffer);
        }
        return error;
    }
}
