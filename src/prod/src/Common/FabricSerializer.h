// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{

const NTSTATUS K_STATUS_NULL_REF_POINTER = KtlStatusCode(STATUS_SEVERITY_ERROR, FacilityKtl, 39);
const NTSTATUS K_STATUS_OTHER_SERIALIZATION_ERRORS = KtlStatusCode(STATUS_SEVERITY_ERROR, FacilityKtl, 40);

#define DECLARE_SERIALIZATION_OVERHEAD_ESTIMATE( ) \
    public: \
    static Common::ErrorCode InitializeSerializationOverheadEstimate(); \
    static size_t SerializationOverheadEstimate; \

#define DEFINE_SERIALIZATION_OVERHEAD_ESTIMATE( CLASS ) \
    size_t CLASS::SerializationOverheadEstimate = 0; \
    Common::ErrorCode CLASS::InitializeSerializationOverheadEstimate() \
    { \
        CLASS temp; \
        return Common::FabricSerializer::EstimateSize(temp, SerializationOverheadEstimate); \
    } \



    struct FabricSerializer
    {
        static ErrorCode Serialize(Serialization::IFabricSerializable const * serializable, std::vector<byte> & bytes);

        static ErrorCode Serialize(Serialization::IFabricSerializable const * serializable, KBuffer::SPtr & bytes);

        static NTSTATUS Serialize(Serialization::IFabricSerializable const * serializable, KBuffer & bytes);

        // This serializer method is used by the replicated store to serialize atomic replication operations to avoid an extra memory copy
        static ErrorCode Serialize(Serialization::IFabricSerializable const * serializable, Serialization::IFabricSerializableStreamUPtr & streamUPtr, ULONG maxGrowBy = 0);

        static ErrorCode Deserialize(Serialization::IFabricSerializable & serializable, std::vector<byte> & bytes);

        static ErrorCode Deserialize(Serialization::IFabricSerializable & serializable, ULONG size, void * buffer);

        // This de-serializer method is used by the replicated store to deserialize atomic replication operations to avoid an extra memory copy
        static ErrorCode Deserialize(Serialization::IFabricSerializable & serializable, ULONG size, std::vector<Serialization::FabricIOBuffer> const & buffers);

        static ErrorCode GetOperationBuffersFromSerializableStream(Serialization::IFabricSerializableStreamUPtr const & streamUPtr, std::vector<FABRIC_OPERATION_DATA_BUFFER> & buffers, ULONG & totalSize);

        template <class T>
        static ErrorCode EstimateSize(T const & object, __inout size_t & size);

        static Serialization::IFabricSerializableStreamUPtr CreateSerializableStream();

        static NTSTATUS CreateKBufferFromStream(Serialization::IFabricSerializableStream const& stream, KBuffer::SPtr& mergeBuffer);

    private:
        static NTSTATUS InnerGetOperationBuffersFromSerializableStream(Serialization::IFabricSerializableStreamUPtr const & streamUPtr, std::vector<FABRIC_OPERATION_DATA_BUFFER> & buffers, ULONG & totalSize);

        // This serializer method is used by the replicated store to serialize atomic replication operations to avoid a memory copy
        static NTSTATUS InnerSerialize(Serialization::IFabricSerializable const * serializable, Serialization::IFabricSerializableStreamUPtr & streamUPtr, ULONG maxGrowBy = 0);

        static NTSTATUS InnerSerialize(Serialization::IFabricSerializable const * serializable, std::vector<byte> & bytes);

        static NTSTATUS InnerDeserialize(Serialization::IFabricSerializable & serializable, ULONG size, void * buffer);

        static NTSTATUS InnerDeserialize(Serialization::IFabricSerializable & serializable, ULONG size, std::vector<Serialization::FabricIOBuffer> const & buffers);
    };

    template <class T>
    ErrorCode FabricSerializer::EstimateSize(
        T const & entry,
        __inout size_t & size)
    {
        Serialization::IFabricSerializableStreamUPtr streamUPtr;
        auto error = Common::FabricSerializer::Serialize(&entry, streamUPtr);
        if (error.IsSuccess())
        {
            size = 0;
            Serialization::FabricIOBuffer buffer;
            bool isExtensionBuffer;
            NTSTATUS status;
            while (NT_SUCCESS(status = streamUPtr->GetNextBuffer(buffer, isExtensionBuffer)))
            {
                if (isExtensionBuffer)
                {
                    return Common::ErrorCode::FromNtStatus(Common::K_STATUS_OTHER_SERIALIZATION_ERRORS);
                }

                size += buffer.length;
            }

            if (status != (NTSTATUS)K_STATUS_NO_MORE_EXTENSION_BUFFERS)
            {
                return Common::ErrorCode::FromNtStatus(status);
            }
        }

        return error;
    }
}
