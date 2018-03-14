// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <Serialization.h>
#include "ByteCompressor.h"

using namespace Serialization;

template <class T>
struct BlittableValueTester
{
    static bool IsEmpty(T field, FabricSerializationTypes::Enum &)
    {
        return (field == 0);
    }

    static void MakeEmpty(T & field, FabricSerializationTypes::Enum)
    {
        field = 0;
    }
};

template <>
struct BlittableValueTester<bool>
{
    static bool IsEmpty(bool field, FabricSerializationTypes::Enum & metadata)
    {
        metadata = (field ? FabricSerializationTypes::BoolTrue : FabricSerializationTypes::BoolFalse);

        return true; // bools are always compressable
    }

    static void MakeEmpty(bool & field, FabricSerializationTypes::Enum metadata)
    {
        field = (metadata == FabricSerializationTypes::MakeEmpty(FabricSerializationTypes::BoolTrue) ? true : false);
    }
};

template <>
struct BlittableValueTester<GUID>
{
    static bool IsEmpty(GUID field, FabricSerializationTypes::Enum &)
    {
        return ((KGuid() == field) == TRUE);
    }

    static void MakeEmpty(GUID & field, FabricSerializationTypes::Enum)
    {
        RtlZeroMemory(&field, sizeof(GUID));
    }
};

NTSTATUS FabricSerializableStream::Create(__out IFabricSerializableStream ** outPtr, __in_opt IFabricStreamUPtr && stream)
{
    FabricSerializableStream * ptr = nullptr;
    *outPtr = nullptr;

    if (stream)
    {
        ptr = _new (WF_SERIALIZATION_TAG, FabricSerializationCommon::GetPagedKtlAllocator()) FabricSerializableStream(Ktl::Move(stream));
    }
    else
    {
        ptr = _new (WF_SERIALIZATION_TAG, FabricSerializationCommon::GetPagedKtlAllocator()) FabricSerializableStream();
    }

    if (ptr == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = ptr->Status();

    if (NT_SUCCESS(status))
    {
        *outPtr = ptr;
    }
    else
    {
        _delete(ptr);
        ptr = nullptr;
    }

    return status;
}

FabricSerializableStream::FabricSerializableStream()
    : _stream(nullptr)
    , _lastMetadataPosition(0)
    , _objectStack(FabricSerializationCommon::GetPagedKtlAllocator())
    , _noCopyIndex(0)
    , _RefCount(1)
{
    // TODO: make empty array

    IFabricStream * streamPtr = nullptr;
    NTSTATUS status = FabricStream::Create(&streamPtr);

    if (NT_ERROR(status))
    {
        this->SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
    }
    else
    {
        this->_stream = IFabricStreamUPtr(streamPtr);
    }
}

FabricSerializableStream::FabricSerializableStream(IFabricStreamUPtr && stream)
    : _stream(nullptr)
    , _lastMetadataPosition(0)
    , _objectStack(FabricSerializationCommon::GetPagedKtlAllocator())
    , _noCopyIndex(0)
    , _RefCount(1)
{
    // TODO: param of extension buffers

    if (stream)
    {
        this->_stream = Ktl::Move(stream);
    }
    else
    {
        // TODO: new error code
        this->SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
    }
}

FabricSerializableStream::~FabricSerializableStream()
{
    // Unique pointer takes care of _stream
}

NTSTATUS FabricSerializableStream::QueryInterface(
    __in  GUID &,
    __out VOID **Interface
    )
{
    *Interface = nullptr;

    return STATUS_SUCCESS;
}

ULONG FabricSerializableStream::Size() const
{
    return _stream->Size();
}

LONG FabricSerializableStream::AddRef()
{
    return InterlockedIncrement(&_RefCount);
}

LONG FabricSerializableStream::Release()
{
    LONG result = InterlockedDecrement(&_RefCount);

    ASSERT(result >= 0);

    if (result == 0)
    {
        _delete(this);
    }

    return result;
}

NTSTATUS FabricSerializableStream::WriteStartType()
{
    return this->WriteMetadata(FabricSerializationTypes::ScopeBegin);
}

NTSTATUS FabricSerializableStream::WriteEndType(__in_opt FabricCompletionCallback completionCallback, __in_opt VOID * state)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (completionCallback != nullptr)
    {
        status = this->_stream->AddCompletionCallback(completionCallback, state);

        if (NT_ERROR(status))
        {
            return status;
        }
    }

    // Check for unknown data
    ObjectContext * context;
    status = this->CurrentObjectContext(&context);

    if (NT_ERROR(status))
    {
        return status;
    }

    FabricIOBuffer buffer;
    status = context->object_->GetUnknownData(context->currentScope_, buffer);

    if (status == (NTSTATUS)K_STATUS_NO_MORE_UNKNOWN_BUFFERS)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        if (NT_ERROR(status))
        {
            return status;
        }

        if (buffer.length > 0)
        {
            status = this->_stream->WriteBytes(buffer.length, buffer.buffer);

            if (NT_ERROR(status))
            {
                return status;
            }
        }
    }

    // Advance the next scope
    ++context->currentScope_;

    return this->WriteMetadata(FabricSerializationTypes::ScopeEnd);
}

NTSTATUS FabricSerializableStream::ReadStartType()
{
    FabricSerializationTypes::Enum metadata;

    NTSTATUS status = this->ReadMetadata(metadata);

    if (NT_ERROR(status))
    {
        return status;
    }

    if (metadata != FabricSerializationTypes::ScopeBegin)
    {
        if (metadata == FabricSerializationTypes::ObjectEnd)
        {
            status = this->SeekToLastMetadataPosition();

            if (NT_ERROR(status))
            {
                return status;
            }

            return K_STATUS_SCOPE_END;
        }

        return K_STATUS_INVALID_STREAM_FORMAT;
    }

    return status;
}

NTSTATUS FabricSerializableStream::ReadEndType()
{
    FabricSerializationTypes::Enum metadata;

    NTSTATUS status = this->ReadMetadata(metadata);

    if (NT_ERROR(status))
    {
        return status;
    }

    if (metadata != FabricSerializationTypes::ScopeEnd)
    {
        status = this->SeekToLastMetadataPosition();

        if (NT_ERROR(status))
        {
            return status;
        }

        if (metadata == FabricSerializationTypes::ObjectEnd)
        {
            return K_STATUS_SCOPE_END;
        }

        status = this->ReadUnknownData();

        if (NT_ERROR(status))
        {
            return status;
        }

        // Try reading metadata again as it should be ScopeEnd now
        status = this->ReadMetadata(metadata);

        if (NT_ERROR(status))
        {
            return status;
        }

        if (metadata != FabricSerializationTypes::ScopeEnd)
        {
            return K_STATUS_INVALID_STREAM_FORMAT;
        }
    }

    // Advance the next scope
    ObjectContext * context;
    status = this->CurrentObjectContext(&context);

    if (NT_ERROR(status))
    {
        return status;
    }

    ++context->currentScope_;

    return K_STATUS_SCOPE_END;
}

NTSTATUS FabricSerializableStream::WriteSerializable(__in IFabricSerializable * object)
{
    NTSTATUS status;

    if (object == nullptr)
    {
        return STATUS_INVALID_PARAMETER;
    }

    status = this->PushObject(object);

    if (NT_ERROR(status))
    {
        return status;
    }

    status = this->WriteMetadata(FabricSerializationTypes::Object);

    if (NT_ERROR(status))
    {
        return status;
    }

    ObjectHeader header;

    // reserve the header spot and flags
    ULONG headerPosition = this->_stream->get_Position();
    status = this->_stream->WriteBytes(sizeof(ObjectHeader), &header);

    if (NT_ERROR(status))
    {
        return status;
    }

    FabricTypeInformation typeInfo = {0};

    // get the type information
    status = object->GetTypeInformation(typeInfo);

    if (NT_ERROR(status))
    {
        return status;
    }

    if (typeInfo.length > 0 && typeInfo.buffer != nullptr)
    {
        header.flags = static_cast<HeaderFlags>(header.flags | HeaderFlags::ContainsTypeInformation);

        status = this->WriteUInt32WithNoMetaData(typeInfo.length);

        if (NT_ERROR(status))
        {
            return status;
        }

        status = this->_stream->WriteBytes(typeInfo.length, typeInfo.buffer);

        if (NT_ERROR(status))
        {
            return status;
        }
    }

    status = object->Write(this);

    if (NT_ERROR(status))
    {
        return status;
    }

    status = this->WriteUnknownDataInExtraScopes();

    if (NT_ERROR(status))
    {
        return status;
    }

    status = this->WriteMetadata(FabricSerializationTypes::ObjectEnd);

    if (NT_ERROR(status))
    {
        return status;
    }

    // Update header with the bytes written
    header.size = this->_stream->get_Position() - headerPosition;

    // Seek back to the correct spot, blit bytes, and seek to the end
    status = this->_stream->Seek(headerPosition);

    if (NT_ERROR(status))
    {
        return status;
    }

    status = this->_stream->WriteBytes(sizeof(header), &header);

    if (NT_ERROR(status))
    {
        return status;
    }

    IFabricSerializable * poppedPtr;
    status = this->PopObject(&poppedPtr);

    if (NT_ERROR(status))
    {
        return status;
    }

    ASSERT(object == poppedPtr);

    if (this->_objectStack.Count() > 0 && ((header.flags & HeaderFlags::ContainsExtensionData) == HeaderFlags::ContainsExtensionData))
    {
        // We just wrote a child object, update current flags with anything useful
        ObjectContext * contextPtr = nullptr;

        status = this->CurrentObjectContext(&contextPtr);

        if (NT_ERROR(status))
        {
            return status;
        }

        contextPtr->header_.flags = static_cast<HeaderFlags>(contextPtr->header_.flags | header.flags);
    }

    return this->_stream->SeekToEnd();
}

NTSTATUS FabricSerializableStream::ReadSerializable(__in IFabricSerializable * object)
{
    NTSTATUS status;

    if (object == nullptr)
    {
        return STATUS_INVALID_PARAMETER;
    }

    FabricSerializationTypes::Enum readMetadata;

    status = this->ReadMetadata(readMetadata);

    if (NT_ERROR(status))
    {
        return status;
    }

    if (status == K_STATUS_SCOPE_END)
    {
        status = this->SeekToLastMetadataPosition();

        if (NT_SUCCESS(status))
        {
            return K_STATUS_SCOPE_END;
        }

        return status;
    }

    if (readMetadata != FabricSerializationTypes::Object)
    {
        return K_STATUS_UNEXPECTED_METADATA_READ;
    }

    ULONG headerPosition = this->_stream->get_Position();

    ObjectHeader header;
    FabricTypeInformation typeInfo = {0};

    status = this->_stream->ReadBytes(sizeof(ObjectHeader), &header);

    if (NT_ERROR(status))
    {
        return status;
    }

    if ((header.flags & HeaderFlags::ContainsTypeInformation) == HeaderFlags::ContainsTypeInformation)
    {
        // We don't need the information here, but we need to advance the stream anyway

        status = this->ReadUInt32WithNoMetaData(typeInfo.length);

        if (NT_ERROR(status))
        {
            return status;
        }

        if (typeInfo.length == 0)
        {
            return K_STATUS_INVALID_STREAM_FORMAT;
        }

        ULONG position =  this->_stream->get_Position();

        status = this->_stream->Seek(position + typeInfo.length);

        if (NT_ERROR(status))
        {
            return status;
        }

        typeInfo.Clear();
    }

    ObjectContext context(object, header, headerPosition);

    return this->ReadSerializableInternal(object, context);
}

NTSTATUS FabricSerializableStream::ReadSerializableAsPointer(__out IFabricSerializable ** pointer, __in FabricActivatorFunction activator)
{
    if (pointer == nullptr)
    {
        return STATUS_INVALID_PARAMETER;
    }

    *pointer = nullptr;

    if (activator == nullptr)
    {
        return STATUS_INVALID_PARAMETER;
    }

    IFabricSerializable * object = nullptr;
    NTSTATUS status;

    FabricSerializationTypes::Enum readMetadata;

    status = this->ReadMetadata(readMetadata);

    if (NT_ERROR(status))
    {
        return status;
    }

    if (status == K_STATUS_SCOPE_END)
    {
        status = this->SeekToLastMetadataPosition();

        if (NT_SUCCESS(status))
        {
            return K_STATUS_SCOPE_END;
        }

        return status;
    }

    if (readMetadata != FabricSerializationTypes::Object)
    {
        return K_STATUS_UNEXPECTED_METADATA_READ;
    }

    ULONG headerPosition = this->_stream->get_Position();

    ObjectHeader header;
    FabricTypeInformation typeInfo = {0};

    status = this->_stream->ReadBytes(sizeof(ObjectHeader), &header);

    if (NT_ERROR(status))
    {
        return status;
    }

    if ((header.flags & HeaderFlags::ContainsTypeInformation) == HeaderFlags::ContainsTypeInformation)
    {
        status = this->ReadUInt32WithNoMetaData(typeInfo.length);

        if (NT_ERROR(status))
        {
            return status;
        }

        if ((typeInfo.length == 0) || (typeInfo.length > FabricTypeInformation::LengthMax))
        {
            return K_STATUS_INVALID_STREAM_FORMAT;
        }

        if ((0 < _stream->Size()) && (_stream->Size() < typeInfo.length))
        {
            return K_STATUS_INVALID_STREAM_FORMAT;
        }

        UCHAR temporaryBuffer[4];
        KUniquePtr<UCHAR, ArrayDeleter<UCHAR>> temporaryBufferUPtr;
        UCHAR* bufferPtr;
        if (typeInfo.length <= sizeof(temporaryBuffer))
        {
            bufferPtr = temporaryBuffer;
        }
        else
        {
            #pragma prefast(suppress:6001, "saving raw pointer to a smart pointer");
            bufferPtr = _new(WF_SERIALIZATION_TAG, FabricSerializationCommon::GetPagedKtlAllocator()) UCHAR[typeInfo.length];
            if (!bufferPtr)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            temporaryBufferUPtr = bufferPtr;
        }

        status = this->_stream->ReadBytes(typeInfo.length, bufferPtr);
        if (NT_ERROR(status))
        {
            return status;
        }

        typeInfo.buffer = bufferPtr;
        object = activator(typeInfo);

        typeInfo.Clear();
    }
    else
    {
        // Create with no type info
        object = activator(typeInfo);
    }

    if (object == nullptr)
    {
        return K_STATUS_OBJECT_ACTIVATION_FAILED;
    }

    // Give ownership of the object to the caller
    *pointer = object;

    ObjectContext context(object, header, headerPosition);

    return this->ReadSerializableInternal(object, context);
}

NTSTATUS FabricSerializableStream::ReadSerializableInternal(__in IFabricSerializable * object,  __in ObjectContext context)
{
    NTSTATUS status;

    if (object == nullptr)
    {
        return STATUS_INVALID_PARAMETER;
    }
        
    //defect 1178748 when V2 stream is de-serialized as V1 for more than 1 times, UnknowData grows each time. 
    object->ClearUnknownData(); 

    status = this->PushObject(object);

    if (NT_ERROR(status))
    {
        return status;
    }

    status = object->Read(static_cast<IFabricSerializableStream*>(this));

    if (NT_ERROR(status))
    {
        return status;
    }

    FabricSerializationTypes::Enum readEndMetadata;

    status = this->ReadMetadata(readEndMetadata);

    // Reading returned a scope end which means data was missing
    //bool readOldVersion = (status == K_STATUS_SCOPE_END);

    if (NT_ERROR(status))
    {
        return status;
    }

    // There must still be unknown data in other scopes
    while (readEndMetadata != FabricSerializationTypes::ObjectEnd)
    {
        // This will walk the unknown scope
        status = this->ReadEndType();

        if (NT_ERROR(status))
        {
            return status;
        }

        status = this->ReadMetadata(readEndMetadata);

        if (NT_ERROR(status))
        {
            return status;
        }
    }

    IFabricSerializable * poppedPtr;
    status = this->PopObject(&poppedPtr);

    ASSERT(poppedPtr == object);

    if (NT_ERROR(status))
    {
        return status;
    }

    ULONG extraBytes = 0;
    extraBytes = context.header_.size - (this->_stream->get_Position() - context.headerPosition_);
    if (extraBytes != 0)
    {
        return K_STATUS_INVALID_STREAM_FORMAT;
    }

    return status;
}

NTSTATUS FabricSerializableStream::WriteSerializableArray(__in_ecount(count) IFabricSerializable ** field, __in ULONG count)
{
    FabricSerializationTypes::Enum metadata = FabricSerializationTypes::MakeArray(FabricSerializationTypes::Object);

    if (count == 0)
    {
        return this->WriteMetadata(FabricSerializationTypes::MakeEmpty(metadata));
    }

    NTSTATUS status = this->WriteMetadata(metadata);

    if (NT_ERROR(status))
    {
        return status;
    }

    status = this->WriteUInt32WithNoMetaData(count);

    if (NT_ERROR(status))
    {
        return status;
    }

    for (ULONG i = 0; i < count; ++i)
    {
        status = this->WriteSerializable(field[i]);

        if (NT_ERROR(status))
        {
            return status;
        }
    }

    return status;
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::ReadSerializableArray(IFabricSerializable ** field, ULONG & count)
{
    return this->ReadSerializableArrayInternal(field, count);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::ReadSerializableArrayInternal(IFabricSerializable ** field, ULONG & count, bool noRead)
{
    size_t typeSize = sizeof(ULONG) + 1; // size of length field + 1

    if ((field != nullptr) && ((*field) != nullptr))
    {
        FabricTypeInformation typeInfo = {};
        NTSTATUS status = (**field).GetTypeInformation(typeInfo);
        if (NT_SUCCESS(status) && (typeInfo.length > 0))
        {
            typeSize = sizeof(ULONG) + typeInfo.length; // size of length field + object size
        }
    }

    if ((0 < _stream->Size()) && (_stream->Size() < (typeSize * count)))
    {
        return K_STATUS_INVALID_STREAM_FORMAT;
    }

    FabricSerializationTypes::Enum baseMetadata = FabricSerializationTypes::Object;
    FabricSerializationTypes::Enum readMetadata = FabricSerializationTypes::Object;

    NTSTATUS status = this->ReadMetadata(readMetadata);

    if (NT_ERROR(status))
    {
        return status;
    }

    if (status == K_STATUS_SCOPE_END)
    {
        status = this->SeekToLastMetadataPosition();

        if (NT_SUCCESS(status))
        {
            return K_STATUS_SCOPE_END;
        }

        return status;
    }

    if (!FabricSerializationTypes::IsOfBaseType(readMetadata, baseMetadata))
    {
        // we have the wrong primative
        return K_STATUS_UNEXPECTED_METADATA_READ;
    }
    else if (!FabricSerializationTypes::IsArray(readMetadata))
    {
        // array/single mismatch
        return K_STATUS_UNEXPECTED_METADATA_READ;
    }

    if (FabricSerializationTypes::IsEmpty(readMetadata))
    {
        count = 0;
        return STATUS_SUCCESS;
    }

    ULONG readItemCount;
    status = this->ReadUInt32WithNoMetaData(readItemCount);

    if (NT_ERROR(status))
    {
        return status;
    }

    if (count < readItemCount && !noRead)
    {
        count = readItemCount;
        if ((0 < _stream->Size()) && (_stream->Size() < (typeSize * count)))
        {
            return K_STATUS_INVALID_STREAM_FORMAT;
        }

        status = this->SeekToLastMetadataPosition();

        if (NT_ERROR(status))
        {
            return status;
        }

        return K_STATUS_BUFFER_TOO_SMALL;
    }

    count = readItemCount;

    for (ULONG i = 0; i < count; ++i)
    {
        if (!noRead)
        {
            status = this->ReadSerializable(field[i]);
        }
        else
        {
            status = this->ReadUnknownObject();
        }

        if (NT_ERROR(status))
        {
            return status;
        }
    }

    return status;
}

NTSTATUS FabricSerializableStream::ReadRawBytes(__in ULONG size, _Out_writes_(size) VOID * destination)
{
    return this->_stream->ReadBytes(size, destination);
}

NTSTATUS FabricSerializableStream::WriteRawBytes(ULONG size, void const* source)
{
    return _stream->WriteBytes(size, source);
}

NTSTATUS FabricSerializableStream::WriteBool(__in bool field)
{
    return this->WriteBlittableValue<bool, FabricSerializationTypes::Bool>(field);
}

NTSTATUS FabricSerializableStream::ReadBool(__out bool & field)
{
    return this->ReadBlittableValue<bool, FabricSerializationTypes::Bool>(field);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::WriteBoolArray(bool * field, ULONG count)
{
    return this->WriteBlittableArray<bool, FabricSerializationTypes::Bool>(field, count);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::ReadBoolArray(bool * field, ULONG & count)
{
    return this->ReadBlittableArray<bool, FabricSerializationTypes::Bool>(field, count);
}

NTSTATUS FabricSerializableStream::WriteChar(__in CHAR field)
{
    return this->WriteBlittableValue<CHAR, FabricSerializationTypes::Char>(field);
}

NTSTATUS FabricSerializableStream::ReadChar(__out CHAR & field)
{
    return this->ReadBlittableValue<CHAR, FabricSerializationTypes::Char>(field);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::WriteCharArray(CHAR * field, ULONG count)
{
    return this->WriteBlittableArray<CHAR, FabricSerializationTypes::Char>(field, count);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::ReadCharArray(CHAR * field, ULONG & count)
{
    return this->ReadBlittableArray<CHAR, FabricSerializationTypes::Char>(field, count);
}

NTSTATUS FabricSerializableStream::WriteUChar(__in UCHAR field)
{
    return this->WriteBlittableValue<UCHAR, FabricSerializationTypes::UChar>(field);
}

NTSTATUS FabricSerializableStream::ReadUChar(__out UCHAR & field)
{
    return this->ReadBlittableValue<UCHAR, FabricSerializationTypes::UChar>(field);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::WriteUCharArray(UCHAR * field, ULONG count)
{
    return this->WriteBlittableArray<UCHAR, FabricSerializationTypes::UChar>(field, count);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::ReadUCharArray(UCHAR * field, ULONG & count)
{
    return this->ReadBlittableArray<UCHAR, FabricSerializationTypes::UChar>(field, count);
}

NTSTATUS FabricSerializableStream::WriteShort(__in SHORT field)
{
    return this->WriteCompressableValue<SHORT, FabricSerializationTypes::Short, SignedByteCompressor>(field);
}

NTSTATUS FabricSerializableStream::ReadShort(__out SHORT & field)
{
    return this->ReadCompressableValue<SHORT, FabricSerializationTypes::Short, SignedByteCompressor>(field);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::WriteShortArray(SHORT * field, ULONG count)
{
    return this->WriteCompressableArray<SHORT, FabricSerializationTypes::Short, SignedByteCompressor>(field, count);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::ReadShortArray(SHORT * field, ULONG & count)
{
    return this->ReadCompressableArray<SHORT, FabricSerializationTypes::Short, SignedByteCompressor>(field, count);
}

NTSTATUS FabricSerializableStream::WriteUShort(__in USHORT field)
{
    return this->WriteCompressableValue<USHORT, FabricSerializationTypes::UShort, UnsignedByteCompressor>(field);
}

NTSTATUS FabricSerializableStream::ReadUShort(__out USHORT & field)
{
    return this->ReadCompressableValue<USHORT, FabricSerializationTypes::UShort, UnsignedByteCompressor>(field);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::WriteUShortArray(USHORT * field, ULONG count)
{
    return this->WriteCompressableArray<USHORT, FabricSerializationTypes::UShort, UnsignedByteCompressor>(field, count);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::ReadUShortArray(USHORT * field, ULONG & count)
{
    return this->ReadCompressableArray<USHORT, FabricSerializationTypes::UShort, UnsignedByteCompressor>(field, count);
}

NTSTATUS FabricSerializableStream::WriteInt32(__in LONG field)
{
    return this->WriteCompressableValue<LONG, FabricSerializationTypes::Int32, SignedByteCompressor>(field);
}

NTSTATUS FabricSerializableStream::ReadInt32(__out LONG & field)
{
    return this->ReadCompressableValue<LONG, FabricSerializationTypes::Int32, SignedByteCompressor>(field);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::WriteInt32Array(LONG * field, ULONG count)
{
    return this->WriteCompressableArray<LONG, FabricSerializationTypes::Int32, SignedByteCompressor>(field, count);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::ReadInt32Array(LONG * field, ULONG & count)
{
    return this->ReadCompressableArray<LONG, FabricSerializationTypes::Int32, SignedByteCompressor>(field, count);
}

NTSTATUS FabricSerializableStream::WriteUInt32(__in ULONG field)
{
    return this->WriteCompressableValue<ULONG, FabricSerializationTypes::UInt32, UnsignedByteCompressor>(field);
}

NTSTATUS FabricSerializableStream::ReadUInt32(__out ULONG & field)
{
    return this->ReadCompressableValue<ULONG, FabricSerializationTypes::UInt32, UnsignedByteCompressor>(field);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::WriteUInt32Array(ULONG * field, ULONG count)
{
    return this->WriteCompressableArray<ULONG, FabricSerializationTypes::UInt32, UnsignedByteCompressor>(field, count);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::ReadUInt32Array(ULONG * field, ULONG & count)
{
    return this->ReadCompressableArray<ULONG, FabricSerializationTypes::UInt32, UnsignedByteCompressor>(field, count);
}

NTSTATUS FabricSerializableStream::WriteInt64(__in LONG64 field)
{
    return this->WriteCompressableValue<LONG64, FabricSerializationTypes::Int64, SignedByteCompressor>(field);
}

NTSTATUS FabricSerializableStream::ReadInt64(__out LONG64 & field)
{
    return this->ReadCompressableValue<LONG64, FabricSerializationTypes::Int64, SignedByteCompressor>(field);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::WriteInt64Array(LONG64 * field, ULONG count)
{
    return this->WriteCompressableArray<LONG64, FabricSerializationTypes::Int64, SignedByteCompressor>(field, count);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::ReadInt64Array(LONG64 * field, ULONG & count)
{
    return this->ReadCompressableArray<LONG64, FabricSerializationTypes::Int64, SignedByteCompressor>(field, count);
}

NTSTATUS FabricSerializableStream::WriteUInt64(__in ULONG64 field)
{
    return this->WriteCompressableValue<ULONG64, FabricSerializationTypes::UInt64, UnsignedByteCompressor>(field);
}

NTSTATUS FabricSerializableStream::ReadUInt64(__out ULONG64 & field)
{
    return this->ReadCompressableValue<ULONG64, FabricSerializationTypes::UInt64, UnsignedByteCompressor>(field);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::WriteUInt64Array(ULONG64 * field, ULONG count)
{
    return this->WriteCompressableArray<ULONG64, FabricSerializationTypes::UInt64, UnsignedByteCompressor>(field, count);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::ReadUInt64Array(ULONG64 * field, ULONG & count)
{
    return this->ReadCompressableArray<ULONG64, FabricSerializationTypes::UInt64, UnsignedByteCompressor>(field, count);
}

NTSTATUS FabricSerializableStream::WriteGuid(__in GUID field)
{
    return this->WriteBlittableValue<GUID, FabricSerializationTypes::Guid>(field);
}

NTSTATUS FabricSerializableStream::ReadGuid(__out GUID & field)
{
    return this->ReadBlittableValue<GUID, FabricSerializationTypes::Guid>(field);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::WriteGuidArray(GUID * field, ULONG count)
{
    return this->WriteBlittableArray<GUID, FabricSerializationTypes::Guid>(field, count);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::ReadGuidArray(GUID * field, ULONG & count)
{
    return this->ReadBlittableArray<GUID, FabricSerializationTypes::Guid>(field, count);
}

NTSTATUS FabricSerializableStream::WriteDouble(__in DOUBLE field)
{
    return this->WriteBlittableValue<DOUBLE, FabricSerializationTypes::Double>(field);
}

NTSTATUS FabricSerializableStream::ReadDouble(__out DOUBLE & field)
{
    return this->ReadBlittableValue<DOUBLE, FabricSerializationTypes::Double>(field);
}

NTSTATUS FabricSerializableStream::WriteDoubleArray(__in DOUBLE * field, __in ULONG count)
{
    return this->WriteBlittableArray<DOUBLE, FabricSerializationTypes::Double>(field, count);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::ReadDoubleArray(DOUBLE * field, ULONG & count)
{
    return this->ReadBlittableArray<DOUBLE, FabricSerializationTypes::Double>(field, count);
}

NTSTATUS FabricSerializableStream::WriteString(__in WCHAR * field, __in ULONG count)
{
    return this->WriteBlittableArray<WCHAR, FabricSerializationTypes::WString>(field, count);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::ReadString(WCHAR * field, ULONG & count)
{
    return this->ReadBlittableArray<WCHAR, FabricSerializationTypes::WString>(field, count);
}

NTSTATUS FabricSerializableStream::WritePointer(__in IFabricSerializable * field)
{
    FabricSerializationTypes::Enum metadata = FabricSerializationTypes::Pointer;

    if (field == nullptr)
    {
        return this->WriteMetadata(FabricSerializationTypes::MakeEmpty(metadata));
    }

    NTSTATUS status = this->WriteMetadata(metadata);

    if (NT_ERROR(status))
    {
        return status;
    }

    status = this->WriteSerializable(field);

    return status;
}

NTSTATUS FabricSerializableStream::ReadPointer(IFabricSerializable ** field, __in FabricActivatorFunction activator)
{
    FabricSerializationTypes::Enum baseMetadata = FabricSerializationTypes::Pointer;
    FabricSerializationTypes::Enum readMetadata = FabricSerializationTypes::Pointer;

    NTSTATUS status = this->ReadMetadata(readMetadata);

    if (NT_ERROR(status))
    {
        return status;
    }

    if (status == K_STATUS_SCOPE_END)
    {
        status = this->SeekToLastMetadataPosition();

        if (NT_SUCCESS(status))
        {
            return K_STATUS_SCOPE_END;
        }

        return status;
    }

    if (!FabricSerializationTypes::IsOfBaseType(readMetadata, baseMetadata))
    {
        // we have the wrong primative
        return K_STATUS_UNEXPECTED_METADATA_READ;
    }
    else if (FabricSerializationTypes::IsArray(readMetadata))
    {
        // array/single mismatch
        return K_STATUS_UNEXPECTED_METADATA_READ;
    }

    if (FabricSerializationTypes::IsEmpty(readMetadata))
    {
        *field = nullptr;
        return STATUS_SUCCESS;
    }

    status = this->ReadSerializableAsPointer(field, activator);

    return status;
}

NTSTATUS FabricSerializableStream::WritePointerArray(__in_ecount(count) IFabricSerializable ** field, __in ULONG count)
{
    FabricSerializationTypes::Enum metadata = FabricSerializationTypes::MakeArray(FabricSerializationTypes::Pointer);

    if (count == 0)
    {
        return this->WriteMetadata(FabricSerializationTypes::MakeEmpty(metadata));
    }

    NTSTATUS status = this->WriteMetadata(metadata);

    if (NT_ERROR(status))
    {
        return status;
    }

    status = this->WriteUInt32WithNoMetaData(count);

    if (NT_ERROR(status))
    {
        return status;
    }

    for (ULONG i = 0; i < count; ++i)
    {
        if (field[i] == nullptr)
        {
            status = this->WriteMetadata(FabricSerializationTypes::MakeEmpty(FabricSerializationTypes::Pointer));
        }
        else
        {
            status = this->WriteMetadata(FabricSerializationTypes::Pointer);

            if (NT_ERROR(status))
            {
                return status;
            }

            status = this->WriteSerializable(field[i]);
        }

        if (NT_ERROR(status))
        {
            return status;
        }
    }

    return status;
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::ReadPointerArray(IFabricSerializable ** field, FabricActivatorFunction activator, ULONG & count)
{
    return this->ReadPointerArrayInternal(field, activator, count);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::ReadPointerArrayInternal(IFabricSerializable ** field, FabricActivatorFunction activator, ULONG & count, bool noRead)
{
    if ((0 < _stream->Size()) && (_stream->Size() < (sizeof(void*) * count)))
    {
        return K_STATUS_INVALID_STREAM_FORMAT;
    }

    FabricSerializationTypes::Enum baseMetadata = FabricSerializationTypes::Pointer;
    FabricSerializationTypes::Enum readMetadata = FabricSerializationTypes::Pointer;

    NTSTATUS status = this->ReadMetadata(readMetadata);
    if (NT_ERROR(status))
    {
        return status;
    }

    if (status == K_STATUS_SCOPE_END)
    {
        status = this->SeekToLastMetadataPosition();

        if (NT_SUCCESS(status))
        {
            return K_STATUS_SCOPE_END;
        }

        return status;
    }

    if (!FabricSerializationTypes::IsOfBaseType(readMetadata, baseMetadata))
    {
        // we have the wrong primative
        return K_STATUS_UNEXPECTED_METADATA_READ;
    }
    else if (!FabricSerializationTypes::IsArray(readMetadata))
    {
        // array/single mismatch
        return K_STATUS_UNEXPECTED_METADATA_READ;
    }

    if (FabricSerializationTypes::IsEmpty(readMetadata))
    {
        count = 0;
        return STATUS_SUCCESS;
    }

    ULONG readItemCount;
    status = this->ReadUInt32WithNoMetaData(readItemCount);

    if (NT_ERROR(status))
    {
        return status;
    }

    if (count < readItemCount && !noRead)
    {
        count = readItemCount;
        if ((0 < _stream->Size()) && (_stream->Size() < (sizeof(void*) * count)))
        {
            return K_STATUS_INVALID_STREAM_FORMAT;
        }

        status = this->SeekToLastMetadataPosition();

        if (NT_ERROR(status))
        {
            return status;
        }

        return K_STATUS_BUFFER_TOO_SMALL;
    }

    count = readItemCount;

    for (ULONG i = 0; i < count; ++i)
    {
        FabricSerializationTypes::Enum pointerMetadata;
        status = this->ReadMetadata(pointerMetadata);

        if (!FabricSerializationTypes::IsOfBaseType(pointerMetadata, FabricSerializationTypes::Pointer))
        {
            return K_STATUS_UNEXPECTED_METADATA_READ;
        }

        if (FabricSerializationTypes::IsEmpty(pointerMetadata))
        {
            if (!noRead)
            {
                field[i] = nullptr;
            }
        }
        else
        {
            if (!noRead)
            {
                status = this->ReadSerializableAsPointer( &(field[i]), activator);
            }
            else
            {
                status = this->ReadUnknownObject();
            }

            if (NT_ERROR(status))
            {
                return status;
            }
        }
    }

    return status;
}

NTSTATUS FabricSerializableStream::WriteByteArrayNoCopy(__in FabricIOBuffer const * buffers, __in ULONG count, __in ULONG)
{
    FabricSerializationTypes::Enum metadata = FabricSerializationTypes::ByteArrayNoCopy;

    ObjectContext * contextPtr = nullptr;

    NTSTATUS status = this->CurrentObjectContext(&contextPtr);

    if (NT_ERROR(status))
    {
        return status;
    }

    contextPtr->header_.flags = static_cast<HeaderFlags>(contextPtr->header_.flags | HeaderFlags::ContainsExtensionData);

    if (count == 0)
    {
        return this->WriteMetadata(FabricSerializationTypes::MakeEmpty(metadata));
    }

    status = this->WriteMetadata(metadata);

    // TODO: flags?

    if (NT_ERROR(status))
    {
        return status;
    }

    status = this->WriteUInt32WithNoMetaData(count);

    if (NT_ERROR(status))
    {
        return status;
    }

    for (ULONG i = 0; i < count; ++i)
    {
        // TODO: increment index, do we need interlocked or is volatile good enough since this should be single threaded?
        //LONG index = InterLockedIncrement(&this->_noCopyIndex);

        LONG index = this->_noCopyIndex;
        ++this->_noCopyIndex;

        status = this->WriteUInt32WithNoMetaData(index);

        if (NT_ERROR(status))
        {
            return status;
        }

        status = this->_stream->WriteBytesNoCopy(buffers[i].length, buffers[i].buffer);

        if (NT_ERROR(status))
        {
            return status;
        }
    }

    return status;
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::ReadByteArrayNoCopy(FabricIOBuffer * bytes, ULONG & count)
{
    FabricSerializationTypes::Enum readMetadata;

    NTSTATUS status = this->ReadMetadata(readMetadata);

    if (NT_ERROR(status))
    {
        return status;
    }

    if (status == K_STATUS_SCOPE_END)
    {
        status = this->SeekToLastMetadataPosition();

        if (NT_SUCCESS(status))
        {
            return K_STATUS_SCOPE_END;
        }

        return status;
    }

    if (readMetadata == FabricSerializationTypes::MakeEmpty(FabricSerializationTypes::ByteArrayNoCopy))
    {
        count = 0;
        return STATUS_SUCCESS;
    }

    if (readMetadata != FabricSerializationTypes::ByteArrayNoCopy)
    {
        return K_STATUS_UNEXPECTED_METADATA_READ;
    }

    ULONG readCount;
    status = this->ReadUInt32WithNoMetaData(readCount);

    if (NT_ERROR(status))
    {
        return status;
    }

    if (count < readCount)
    {
        count = readCount;

        status = this->SeekToLastMetadataPosition();

        if (NT_ERROR(status))
        {
            return status;
        }

        return K_STATUS_BUFFER_TOO_SMALL;
    }

    count = readCount;

    for (ULONG i = 0; i < count; ++i)
    {
        ULONG index;
        status = this->ReadUInt32WithNoMetaData(index);

        if (NT_ERROR(status))
        {
            return status;
        }

        status = this->_stream->ReadBytesNoCopy(index, bytes[i]);

        if (NT_ERROR(status))
        {
            return status;
        }
    }

    return status;
}

NTSTATUS FabricSerializableStream::SeekToBegin()
{
    return this->_stream->Seek(0);
}

NTSTATUS FabricSerializableStream::SeekToEnd()
{
    return this->_stream->SeekToEnd();
}

NTSTATUS FabricSerializableStream::Seek(ULONG offset)
{
    return this->_stream->Seek(offset);
}

ULONG FabricSerializableStream::Position() const
{
    return _stream->get_Position();
}

NTSTATUS FabricSerializableStream::GetNextBuffer(__out FabricIOBuffer & buffer, __out bool & isExtensionData)
{
    return this->_stream->GetNextBuffer(buffer, isExtensionData);
}

_Use_decl_annotations_
NTSTATUS FabricSerializableStream::GetAllBuffers(_Out_ FabricIOBuffer* bufferArray, _Inout_ size_t & count) const
{
    return _stream->GetAllBuffers(bufferArray, count);
}

NTSTATUS FabricSerializableStream::InvokeCallbacks()
{
    return this->_stream->InvokeCompletionCallbacks();
}

NTSTATUS FabricSerializableStream::WriteUInt32WithNoMetaData(__in ULONG value)
{
    const ULONG Size = MAX_COMPRESSION_SIZE(sizeof(ULONG));
    UCHAR buffer[Size];

    ULONG compressedBytesCount = UnsignedByteCompressor::Compress<ULONG>(value, buffer);

    if (compressedBytesCount == 0)
    {
        // Since there is no metadata to indicate empty we have to write at least one byte
        buffer[Size-1] = 0;
        compressedBytesCount = 1;
    }

    return this->_stream->WriteBytes(compressedBytesCount, buffer + Size - compressedBytesCount);
}

NTSTATUS FabricSerializableStream::ReadUInt32WithNoMetaData(__out ULONG & value)
{ 
    return UnsignedByteCompressor::ReadAndUncompress<ULONG>(this, value);     
}

NTSTATUS FabricSerializableStream::WriteMetadata(__in FabricSerializationTypes::Enum value)
{
    return this->_stream->WriteBytes(sizeof(value), &value);
}

NTSTATUS FabricSerializableStream::ReadMetadata(__out FabricSerializationTypes::Enum & value)
{
    ULONG position = this->_stream->get_Position();

    NTSTATUS status = this->_stream->ReadBytes(sizeof(value), &value);

    if (NT_SUCCESS(status))
    {
        this->_lastMetadataPosition = position;
    }

    if (value == FabricSerializationTypes::ScopeEnd || value == FabricSerializationTypes::ObjectEnd)
    {
        return K_STATUS_SCOPE_END;
    }

    return status;
}

NTSTATUS FabricSerializableStream::SeekToLastMetadataPosition()
{
    return this->_stream->Seek(this->_lastMetadataPosition);
}

NTSTATUS FabricSerializableStream::SkipCompressedValue(__in ULONG maxedSkippedBytes)
{       
    UCHAR byteTemp = 0;

    NTSTATUS status = STATUS_SUCCESS;

    for (ULONG i = 0; i < maxedSkippedBytes; ++i)
    {
        status = this->_stream->ReadBytes(1, &byteTemp);

        if (NT_ERROR(status))
        {
            return status;
        }

        if ((byteTemp & MASK_MOREDATA) == 0)
        {
            // last byte
            return status;
        }
    }

    // we have read the max number of bytes but not the end byte yet...
    return K_STATUS_INVALID_STREAM_FORMAT;
}

NTSTATUS FabricSerializableStream::ReadUnknownData()
{
    // This method will read unknown data like real data and simply blit the contents when ScopeEnd is found.

    FabricSerializationTypes::Enum expectedMetaData = FabricSerializationTypes::ScopeEnd;
    FabricSerializationTypes::Enum readMetadata = FabricSerializationTypes::ScopeEnd;

    NTSTATUS status;

    ULONG positionOfStart = this->_stream->get_Position();

    status = this->ReadMetadata(readMetadata);

    if (NT_ERROR(status))
    {
        return status;
    }

    while (readMetadata != expectedMetaData)
    {
        if (NT_ERROR(status))
        {
            return status;
        }

        if (FabricSerializationTypes::IsEmpty(readMetadata))
        {
            if (readMetadata == FabricSerializationTypes::MakeEmpty(FabricSerializationTypes::ByteArrayNoCopy))
            {
                return K_STATUS_INVALID_UNKNOWN_EXTENSION_BUFFERS;
            }

            // shouldn't be anything left to read for this metadata, read next metadata
        }
        else
        {
            // Reset so we can call the regular read methods
            status = this->SeekToLastMetadataPosition();

            if (NT_ERROR(status))
            {
                return status;
            }

            if (FabricSerializationTypes::IsArray(readMetadata))
            {
                status = this->ReadUnknownDataArray(readMetadata);
            }
            else
            {
                status = this->ReadUnknownDataSingleValue(readMetadata);
            }
        }

        if (NT_ERROR(status))
        {
            return status;
        }

        // issue another read
        status = this->ReadMetadata(readMetadata);
    }

    ULONG positionOfEnd = this->_stream->get_Position();
    ULONG sizeOfUnknownData = positionOfEnd - positionOfStart - (sizeof(FabricSerializationTypes::ScopeEnd)); // Do not include the scope ending

    status = this->_stream->Seek(positionOfStart);

    if (NT_ERROR(status))
    {
        return status;
    }

    if ((0 < _stream->Size()) && ( _stream->Size() < sizeOfUnknownData))
    {
        return K_STATUS_INVALID_STREAM_FORMAT;
    }

#pragma warning (suppress : 6001) // Assigning to UPtr
    KUniquePtr<UCHAR, ArrayDeleter<UCHAR>> unknownDataBufferUPtr(_new (WF_SERIALIZATION_TAG, FabricSerializationCommon::GetPagedKtlAllocator()) UCHAR[sizeOfUnknownData]);
    if (!unknownDataBufferUPtr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = this->_stream->ReadBytes(sizeOfUnknownData, unknownDataBufferUPtr);

    if (NT_ERROR(status))
    {
        return status;
    }

    FabricIOBuffer buffer(unknownDataBufferUPtr, sizeOfUnknownData);

    ObjectContext * context;
    status = this->CurrentObjectContext(&context);

    if (NT_ERROR(status))
    {
        return status;
    }

    // Unknown data has been read and we should be at the start of a new scope or end of object

    return context->object_->SetUnknownData(context->currentScope_, buffer);
}

NTSTATUS FabricSerializableStream::ReadUnknownDataSingleValue(FabricSerializationTypes::Enum readMetadata)
{
    NTSTATUS status = K_STATUS_UNEXPECTED_METADATA_READ;

    if (FabricSerializationTypes::Bool == readMetadata)
    {
        bool temp;
        status = this->ReadBool(temp);
    }
    else if (FabricSerializationTypes::Char == readMetadata)
    {
        CHAR temp;
        status = this->ReadChar(temp);
    }
    else if (FabricSerializationTypes::UChar == readMetadata)
    {
        UCHAR temp;
        status = this->ReadUChar(temp);
    }
    else if (FabricSerializationTypes::Short == readMetadata)
    {
        SHORT temp;
        status = this->ReadShort(temp);
    }
    else if (FabricSerializationTypes::UShort == readMetadata)
    {
        USHORT temp;
        status = this->ReadUShort(temp);
    }
    else if (FabricSerializationTypes::Int32 == readMetadata)
    {
        LONG temp;
        status = this->ReadInt32(temp);
    }
    else if (FabricSerializationTypes::UInt32 == readMetadata)
    {
        ULONG temp;
        status = this->ReadUInt32(temp);
    }
    else if (FabricSerializationTypes::Int64 == readMetadata)
    {
        LONG64 temp;
        status = this->ReadInt64(temp);
    }
    else if (FabricSerializationTypes::UInt64 == readMetadata)
    {
        ULONG64 temp;
        status = this->ReadUInt64(temp);
    }
    else if (FabricSerializationTypes::Guid == readMetadata)
    {
        GUID temp;
        status = this->ReadGuid(temp);
    }
    else if (FabricSerializationTypes::Double == readMetadata)
    {
        DOUBLE temp;
        status = this->ReadDouble(temp);
    }
    else if (FabricSerializationTypes::Pointer == readMetadata)
    {
        // We have a non empty pointer
        status = this->ReadMetadata(readMetadata);

        if (NT_SUCCESS(status))
        {
            status = this->ReadUnknownObject();
        }
    }
    else if (FabricSerializationTypes::Object == readMetadata)
    {
        status = this->ReadUnknownObject();
    }

    return status;
}

NTSTATUS FabricSerializableStream::ReadUnknownDataArray(FabricSerializationTypes::Enum readMetadata)
{
    NTSTATUS status = K_STATUS_UNEXPECTED_METADATA_READ;
    ULONG count = 0;

    if (FabricSerializationTypes::IsOfBaseType(readMetadata, FabricSerializationTypes::Bool))
    {
        status = this->ReadBlittableArray<bool, FabricSerializationTypes::Bool>(nullptr, count, true);
    }
    else if (FabricSerializationTypes::IsOfBaseType(readMetadata, FabricSerializationTypes::Char))
    {
        status = this->ReadBlittableArray<CHAR, FabricSerializationTypes::Char>(nullptr, count, true);
    }
    else if (FabricSerializationTypes::IsOfBaseType(readMetadata, FabricSerializationTypes::UChar))
    {
        status = this->ReadBlittableArray<UCHAR, FabricSerializationTypes::UChar>(nullptr, count, true);
    }
    else if (FabricSerializationTypes::IsOfBaseType(readMetadata, FabricSerializationTypes::Short))
    {
        status = this->ReadCompressableArray<SHORT, FabricSerializationTypes::Short, SignedByteCompressor>(nullptr, count, true);
    }
    else if (FabricSerializationTypes::IsOfBaseType(readMetadata, FabricSerializationTypes::UShort))
    {
        status = this->ReadCompressableArray<USHORT, FabricSerializationTypes::UShort, UnsignedByteCompressor>(nullptr, count, true);
    }
    else if (FabricSerializationTypes::IsOfBaseType(readMetadata, FabricSerializationTypes::Int32))
    {
        status = this->ReadCompressableArray<LONG, FabricSerializationTypes::Int32, SignedByteCompressor>(nullptr, count, true);
    }
    else if (FabricSerializationTypes::IsOfBaseType(readMetadata, FabricSerializationTypes::UInt32))
    {
        status = this->ReadCompressableArray<ULONG, FabricSerializationTypes::UInt32, UnsignedByteCompressor>(nullptr, count, true);
    }
    else if (FabricSerializationTypes::IsOfBaseType(readMetadata, FabricSerializationTypes::Int64))
    {
        status = this->ReadCompressableArray<LONG64, FabricSerializationTypes::Int64, SignedByteCompressor>(nullptr, count, true);
    }
    else if (FabricSerializationTypes::IsOfBaseType(readMetadata, FabricSerializationTypes::UInt64))
    {
        status = this->ReadCompressableArray<ULONG64, FabricSerializationTypes::UInt64, UnsignedByteCompressor>(nullptr, count, true);
    }
    else if (FabricSerializationTypes::IsOfBaseType(readMetadata, FabricSerializationTypes::Double))
    {
        status = this->ReadBlittableArray<DOUBLE, FabricSerializationTypes::Double>(nullptr, count, true);
    }
    else if (FabricSerializationTypes::IsOfBaseType(readMetadata, FabricSerializationTypes::Guid))
    {
        status = this->ReadBlittableArray<GUID, FabricSerializationTypes::Guid>(nullptr, count, true);
    }
    else if (FabricSerializationTypes::IsOfBaseType(readMetadata, FabricSerializationTypes::WString))
    {
        status = this->ReadBlittableArray<WCHAR, FabricSerializationTypes::WString>(nullptr, count, true);
    }
    else if (FabricSerializationTypes::IsOfBaseType(readMetadata, FabricSerializationTypes::Pointer))
    {
        status = this->ReadPointerArrayInternal(nullptr, nullptr, count, true);
    }
    else if (FabricSerializationTypes::IsOfBaseType(readMetadata, FabricSerializationTypes::Object))
    {
        status = this->ReadSerializableArrayInternal(nullptr, count, true);
    }
    else if (FabricSerializationTypes::ByteArrayNoCopy == readMetadata)
    {
        status = K_STATUS_INVALID_UNKNOWN_EXTENSION_BUFFERS;
    }

    return status;
}

NTSTATUS FabricSerializableStream::ReadUnknownObject()
{
    FabricSerializationTypes::Enum readMetadata;

    NTSTATUS status = this->ReadMetadata(readMetadata);

    if (NT_ERROR(status))
    {
        return status;
    }

    if (readMetadata != FabricSerializationTypes::Object)
    {
        return K_STATUS_UNEXPECTED_METADATA_READ;
    }

    ULONG headerPosition = this->_stream->get_Position();

    ObjectHeader header;

    status = this->_stream->ReadBytes(sizeof(header), &header);

    if (NT_ERROR(status))
    {
        return status;
    }

    if ((header.flags & HeaderFlags::ContainsExtensionData) == HeaderFlags::ContainsExtensionData)
    {
        return K_STATUS_INVALID_UNKNOWN_EXTENSION_BUFFERS;
    }

    // We have the header, skip the whole thing
    return this->_stream->Seek(headerPosition + header.size);
}

template <class T, FabricSerializationTypes::Enum ST>
NTSTATUS FabricSerializableStream::WriteBlittableValue(__in T field)
{
    FabricSerializationTypes::Enum metadata = ST;

    if (BlittableValueTester<T>::IsEmpty(field, metadata))
    {
        return this->WriteMetadata(FabricSerializationTypes::MakeEmpty(metadata));
    }

    NTSTATUS status = this->WriteMetadata(metadata);

    if (NT_ERROR(status))
    {
        return status;
    }

    return this->_stream->WriteBytes(sizeof(T), &field);
}

template <class T, FabricSerializationTypes::Enum ST>
NTSTATUS FabricSerializableStream::ReadBlittableValue(T & field)
{
    FabricSerializationTypes::Enum baseMetadata = ST;
    FabricSerializationTypes::Enum readMetadata = ST;

    NTSTATUS status = this->ReadMetadata(readMetadata);

    if (NT_ERROR(status))
    {
        return status;
    }

    if (status == K_STATUS_SCOPE_END)
    {
        status = this->SeekToLastMetadataPosition();

        if (NT_SUCCESS(status))
        {
            return K_STATUS_SCOPE_END;
        }

        return status;
    }

    if (!FabricSerializationTypes::IsOfBaseType(readMetadata, baseMetadata))
    {
        // we have the wrong primative
        return K_STATUS_UNEXPECTED_METADATA_READ;
    }
    else if (FabricSerializationTypes::IsArray(readMetadata))
    {
        // array/single mismatch
        return K_STATUS_UNEXPECTED_METADATA_READ;
    }

    if (FabricSerializationTypes::IsEmpty(readMetadata))
    {
        BlittableValueTester<T>::MakeEmpty(field, readMetadata);

        return status;
    }

    return this->_stream->ReadBytes(sizeof(T), &field);
}

template <class T, FabricSerializationTypes::Enum ST>
NTSTATUS FabricSerializableStream::WriteBlittableArray(__in T * field, __in ULONG count)
{
    FabricSerializationTypes::Enum metadata = FabricSerializationTypes::MakeArray(ST);

    if (count == 0)
    {
        metadata = FabricSerializationTypes::MakeEmpty(metadata);
        return this->WriteMetadata(metadata);
    }

    NTSTATUS status = this->WriteMetadata(metadata);

    if (NT_ERROR(status))
    {
        return status;
    }

    status = this->WriteUInt32WithNoMetaData(count);

    if (NT_ERROR(status))
    {
        return status;
    }

    return this->_stream->WriteBytes(sizeof(T) * count, field);
}

template <class T, FabricSerializationTypes::Enum ST>
_Use_decl_annotations_
NTSTATUS FabricSerializableStream::ReadBlittableArray(T * field, ULONG & count, bool noRead)
{
    if ((0 < _stream->Size()) && (_stream->Size() < (sizeof(T) * count)))
    {
        return K_STATUS_INVALID_STREAM_FORMAT;
    }

    FabricSerializationTypes::Enum baseMetadata = ST;
    FabricSerializationTypes::Enum readMetadata = ST;

    NTSTATUS status = this->ReadMetadata(readMetadata);

    if (NT_ERROR(status))
    {
        return status;
    }

    if (status == K_STATUS_SCOPE_END)
    {
        status = this->SeekToLastMetadataPosition();

        if (NT_SUCCESS(status))
        {
            return K_STATUS_SCOPE_END;
        }

        return status;
    }

    if (!FabricSerializationTypes::IsOfBaseType(readMetadata, baseMetadata))
    {
        // we have the wrong primative
        return K_STATUS_UNEXPECTED_METADATA_READ;
    }
    else if (!FabricSerializationTypes::IsArray(readMetadata))
    {
        // array/single mismatch
        return K_STATUS_UNEXPECTED_METADATA_READ;
    }

    if (FabricSerializationTypes::IsEmpty(readMetadata))
    {
        count = 0;
        return STATUS_SUCCESS;
    }

    ULONG readItemCount;
    status = this->ReadUInt32WithNoMetaData(readItemCount);

    if (NT_ERROR(status))
    {
        return status;
    }

    if (count < readItemCount && !noRead)
    {
        count = readItemCount;
        if ((0 < _stream->Size()) && (_stream->Size() < (sizeof(T) * count)))
        {
            return K_STATUS_INVALID_STREAM_FORMAT;
        }

        // reset stream
        status = this->SeekToLastMetadataPosition();

        if (NT_ERROR(status))
        {
            return status;
        }

        return K_STATUS_BUFFER_TOO_SMALL;
    }

    count = readItemCount;

    if (noRead)
    {
        ULONG position = this->_stream->get_Position();

        return this->_stream->Seek(position + (sizeof(T) * readItemCount));
    }

    return this->_stream->ReadBytes(sizeof(T) * readItemCount, field);
}

template <class T, FabricSerializationTypes::Enum ST, class Compressor>
NTSTATUS FabricSerializableStream::WriteCompressableValue(__in T field)
{
    const ULONG Size = MAX_COMPRESSION_SIZE(sizeof(T));

    FabricSerializationTypes::Enum metadata = ST;
    UCHAR buffer[Size];
    NTSTATUS status;

    ULONG compressedByteCount = Compressor::Compress(field, buffer);

    if (compressedByteCount == 0)
    {
        metadata = FabricSerializationTypes::MakeEmpty(metadata);
        status = this->WriteMetadata(metadata);
    }
    else
    {
        status = this->WriteMetadata(metadata);

        if (NT_ERROR(status))
        {
            return status;
        }

        status = this->_stream->WriteBytes(compressedByteCount, buffer + Size - compressedByteCount);
    }

    return status;
}

template <class T, FabricSerializationTypes::Enum ST, class Compressor>
NTSTATUS FabricSerializableStream::ReadCompressableValue(T & field)
{
    FabricSerializationTypes::Enum baseMetadata = ST;
    FabricSerializationTypes::Enum readMetadata = ST;

    NTSTATUS status;

    status = this->ReadMetadata(readMetadata);

    if (NT_ERROR(status))
    {
        return status;
    }

    if (status == K_STATUS_SCOPE_END)
    {
        status = this->SeekToLastMetadataPosition();

        if (NT_SUCCESS(status))
        {
            return K_STATUS_SCOPE_END;
        }

        return status;
    }

    if (!FabricSerializationTypes::IsOfBaseType(readMetadata, baseMetadata))
    {
        // we have the wrong primative
        return K_STATUS_UNEXPECTED_METADATA_READ;
    }
    else if (FabricSerializationTypes::IsArray(readMetadata))
    {
        // array/single mismatch
        return K_STATUS_UNEXPECTED_METADATA_READ;
    }

    if (FabricSerializationTypes::IsEmpty(readMetadata))
    {
        field = 0;
        return STATUS_SUCCESS;
    }

    return Compressor::ReadAndUncompress<T>(this, field);   
}

template <class T, FabricSerializationTypes::Enum ST, class Compressor>
_Use_decl_annotations_
NTSTATUS FabricSerializableStream::WriteCompressableArray(T * field, ULONG count)
{
    const ULONG Size = MAX_COMPRESSION_SIZE(sizeof(T));

    FabricSerializationTypes::Enum metadata = FabricSerializationTypes::MakeArray(ST);
    UCHAR buffer[Size];
    NTSTATUS status;

    if (count == 0)
    {
        metadata = FabricSerializationTypes::MakeEmpty(metadata);
    }

    status = this->WriteMetadata(metadata);

    if (NT_ERROR(status))
    {
        return status;
    }

    if (count == 0)
    {
        return status;
    }

    status = this->WriteUInt32WithNoMetaData(count);

    if (NT_ERROR(status))
    {
        return status;
    }

    for (ULONG i = 0; i < count; ++i)
    {
        ULONG compressedByteCount = Compressor::Compress(*(field + i), buffer);

        if (compressedByteCount == 0)
        {
            // In an array, the value must be present even if empty
            compressedByteCount = 1;
            buffer[Size - 1] = 0;
        }

        status = this->_stream->WriteBytes(compressedByteCount, buffer + Size - compressedByteCount);

        if (NT_ERROR(status))
        {
            return status;
        }
    }

    return status;
}


#pragma push
#pragma warning (disable : 6001) // "field" may not be set with some successful return values

template <class T, FabricSerializationTypes::Enum ST, class Compressor>
_Use_decl_annotations_
NTSTATUS FabricSerializableStream::ReadCompressableArray(T * field, ULONG & count, bool noRead)
{
    // The following checking is NOT as strict as "(0 < _stream->Size()) && (_stream->Size() < (sizeof(T)*count))",
    // because the compression rate can be as large as sizeof(T) / sizeof(UCHAR)
    if ((0 < _stream->Size()) && (_stream->Size() < count))
    {
        return K_STATUS_INVALID_STREAM_FORMAT;
    }

    FabricSerializationTypes::Enum baseMetadata = ST;
    FabricSerializationTypes::Enum readMetadata = ST;

    const ULONG Size = MAX_COMPRESSION_SIZE(sizeof(T));
    
    NTSTATUS status = this->ReadMetadata(readMetadata);
    if (NT_ERROR(status))
    {
        return status;
    }

    if (status == K_STATUS_SCOPE_END)
    {
        status = this->SeekToLastMetadataPosition();

        if (NT_SUCCESS(status))
        {
            return K_STATUS_SCOPE_END;
        }

        return status;
    }

    if (!FabricSerializationTypes::IsOfBaseType(readMetadata, baseMetadata))
    {
        // we have the wrong primative
        return K_STATUS_UNEXPECTED_METADATA_READ;
    }
    else if (!FabricSerializationTypes::IsArray(readMetadata))
    {
        // array/single mismatch
        return K_STATUS_UNEXPECTED_METADATA_READ;
    }

    if (FabricSerializationTypes::IsEmpty(readMetadata))
    {
        count = 0;
        return STATUS_SUCCESS;
    }

    ULONG readItemCount;
    status = this->ReadUInt32WithNoMetaData(readItemCount);

    if (NT_ERROR(status))
    {
        return status;
    }

    if (count < readItemCount && !noRead)
    {
        count = readItemCount;

        // The following checking is NOT as strict as "(0 < _stream->Size()) && (_stream->Size() < (sizeof(T)*count))",
        // because the compression rate can be as large as sizeof(T) / sizeof(UCHAR)
        if ((0 < _stream->Size()) && (_stream->Size() < count))
        {
            return K_STATUS_INVALID_STREAM_FORMAT;
        }

        // reset stream
        status = this->SeekToLastMetadataPosition();

        if (NT_ERROR(status))
        {
            return status;
        }

        return K_STATUS_BUFFER_TOO_SMALL;
    }

    count = readItemCount;

    for (ULONG i = 0; i < readItemCount; ++i)
    {
        ULONG readBytes = Size;   
        if (noRead) 
        {
            status = this->SkipCompressedValue(readBytes);
        }
        else
        {
            status = Compressor::ReadAndUncompress<T>(this, field[i]);
        }
        
        if (NT_ERROR(status))
        {
            return status;
        }   
        
    }

    return status;
}

#pragma pop

NTSTATUS FabricSerializableStream::PushObject(__in IFabricSerializable * object)
{
    ObjectContext context(object);

    return this->_objectStack.Append(context);
}

NTSTATUS FabricSerializableStream::PopObject(__out IFabricSerializable ** object)
{
    if (this->_objectStack.IsEmpty())
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *object = (this->_objectStack[this->_objectStack.Count() - 1]).object_;

    // assert?
    this->_objectStack.Remove(this->_objectStack.Count() - 1);

    return STATUS_SUCCESS;
}

NTSTATUS FabricSerializableStream::CurrentObjectContext(__out ObjectContext ** context)
{
    if (this->_objectStack.IsEmpty())
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *context = &(this->_objectStack[this->_objectStack.Count() - 1]);

    return STATUS_SUCCESS;
}

NTSTATUS FabricSerializableStream::WriteUnknownDataInExtraScopes()
{
    // Write more unknown data
    ObjectContext * context;
    NTSTATUS status = this->CurrentObjectContext(&context);

    if (NT_ERROR(status))
    {
        return status;
    }

    while (status != (NTSTATUS)K_STATUS_NO_MORE_UNKNOWN_BUFFERS)
    {
        FabricIOBuffer buffer;
        status = context->object_->GetUnknownData(context->currentScope_, buffer);

        if (status != (NTSTATUS)K_STATUS_NO_MORE_UNKNOWN_BUFFERS)
        {
            if (NT_ERROR(status))
            {
                return status;
            }

            status = this->WriteStartType();

            if (NT_ERROR(status))
            {
                return status;
            }

            status = this->WriteEndType();

            if (NT_ERROR(status))
            {
                return status;
            }
        }
    }

    return STATUS_SUCCESS;
}


