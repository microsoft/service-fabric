// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Serialization
{
    class FabricSerializableStream : public IFabricSerializableStream, public KObject<FabricSerializableStream>
    {
    public:
        typedef KUniquePtr<IFabricSerializableStream, IKBaseDeleter<IFabricSerializableStream>> UPtr;

        static NTSTATUS FabricSerializableStream::Create(__out IFabricSerializableStream ** outPtr, __in_opt IFabricStreamUPtr && stream = IFabricStreamUPtr());

        virtual NTSTATUS QueryInterface(
            __in  GUID&  InterfaceId,
            __out VOID **Interface
            );

        virtual LONG AddRef();
        virtual LONG Release();

        ULONG Size() const override;

        virtual NTSTATUS WriteStartType();

        virtual NTSTATUS ReadStartType();

        virtual NTSTATUS WriteEndType(__in_opt FabricCompletionCallback completionCallback = nullptr, __in_opt VOID * state = nullptr);

        virtual NTSTATUS ReadEndType();

        virtual NTSTATUS WriteSerializable(__in IFabricSerializable * object);

        virtual NTSTATUS ReadSerializable(__in IFabricSerializable * object);

        virtual NTSTATUS WriteSerializableArray(_In_reads_(count) IFabricSerializable ** field, __in ULONG count);

        virtual NTSTATUS ReadSerializableArray(_Out_writes_(count) IFabricSerializable ** field, __inout ULONG & count);

        virtual NTSTATUS ReadSerializableAsPointer(__out IFabricSerializable ** pointer, __in FabricActivatorFunction activator);

        virtual NTSTATUS ReadRawBytes(__in ULONG size, _Out_writes_(size) VOID * destination);
        NTSTATUS WriteRawBytes(ULONG size, void const* source) override;

        virtual NTSTATUS WriteBool(__in bool field);

        virtual NTSTATUS ReadBool(__out bool & field);

        virtual NTSTATUS WriteBoolArray(_In_reads_(count) bool * field, __in ULONG count);

        virtual NTSTATUS ReadBoolArray(_Out_writes_(count) bool * field, __inout ULONG & count);

        virtual NTSTATUS WriteChar(__in CHAR field);

        virtual NTSTATUS ReadChar(__out CHAR & field);

        virtual NTSTATUS WriteCharArray(_In_reads_(count) CHAR * field, __in ULONG count);

        virtual NTSTATUS ReadCharArray(_Out_writes_(count) CHAR * field, __inout ULONG & count);

        virtual NTSTATUS WriteUChar(__in UCHAR field);

        virtual NTSTATUS ReadUChar(__out UCHAR & field);

        virtual NTSTATUS WriteUCharArray(_In_reads_(count) UCHAR * field, __in ULONG count);

        virtual NTSTATUS ReadUCharArray(_Out_writes_opt_(count) UCHAR * field, __inout ULONG & count);

        virtual NTSTATUS WriteShort(__in SHORT field);

        virtual NTSTATUS ReadShort(__out SHORT & field);

        virtual NTSTATUS WriteShortArray(_In_reads_(count) SHORT * field, __in ULONG count);

        virtual NTSTATUS ReadShortArray(_Out_writes_(count) SHORT * field, __inout ULONG & count);

        virtual NTSTATUS WriteUShort(__in USHORT field);

        virtual NTSTATUS ReadUShort(__out USHORT & field);

        virtual NTSTATUS WriteUShortArray(_In_reads_(count) USHORT * field, __in ULONG count);

        virtual NTSTATUS ReadUShortArray(_Out_writes_(count) USHORT * field, __inout ULONG & count);

        virtual NTSTATUS WriteInt32(__in LONG field);

        virtual NTSTATUS ReadInt32(__out LONG & field);

        virtual NTSTATUS WriteInt32Array(_In_reads_(count) LONG * field, __in ULONG count);

        virtual NTSTATUS ReadInt32Array(_Out_writes_(count) LONG * field, __inout ULONG & count);

        virtual NTSTATUS WriteUInt32(__in ULONG field);

        virtual NTSTATUS ReadUInt32(__out ULONG & field);

        virtual NTSTATUS WriteUInt32Array(_In_reads_(count) ULONG * field, __in ULONG count);

        virtual NTSTATUS ReadUInt32Array(_Out_writes_(count) ULONG * field, __inout ULONG & count);

        virtual NTSTATUS WriteInt64(__in LONG64 field);

        virtual NTSTATUS ReadInt64(__out LONG64 & field);

        virtual NTSTATUS WriteInt64Array(_In_reads_(count) LONG64 * field, __in ULONG count);

        virtual NTSTATUS ReadInt64Array(_Out_writes_(count) LONG64 * field, __inout ULONG & count);

        virtual NTSTATUS WriteUInt64(__in ULONG64 field);

        virtual NTSTATUS ReadUInt64(__out ULONG64 & field);

        virtual NTSTATUS WriteUInt64Array(_In_reads_(count) ULONG64 * field, __in ULONG count);

        virtual NTSTATUS ReadUInt64Array(_Out_writes_(count) ULONG64 * field, __inout ULONG & count);

        virtual NTSTATUS WriteGuid(__in GUID field);

        virtual NTSTATUS ReadGuid(__out GUID & field);

        virtual NTSTATUS WriteGuidArray(_In_reads_(count) GUID * field, __in ULONG count);

        virtual NTSTATUS ReadGuidArray(_Out_writes_(count) GUID * field, __inout ULONG & count);

        virtual NTSTATUS WriteDouble(__in DOUBLE field);

        virtual NTSTATUS ReadDouble(__out DOUBLE & field);

        virtual NTSTATUS WriteDoubleArray(__in DOUBLE * field, __in ULONG count);

        virtual NTSTATUS ReadDoubleArray(_Out_writes_opt_(count) DOUBLE * field, __inout ULONG & count);

        virtual NTSTATUS WritePointer(__in IFabricSerializable * field);

        virtual NTSTATUS ReadPointer(IFabricSerializable ** field, __in FabricActivatorFunction activator);

        virtual NTSTATUS WritePointerArray(__in_ecount(count) IFabricSerializable ** field, __in ULONG count);

        virtual NTSTATUS ReadPointerArray(_Out_writes_(count) IFabricSerializable ** field, __in FabricActivatorFunction activator, __inout ULONG & count);
    
        virtual NTSTATUS WriteString(__in WCHAR * field, __in ULONG count);

        virtual NTSTATUS ReadString(_Out_writes_opt_(count) WCHAR * field, __inout ULONG & count);

        //virtual NTSTATUS WriteStringArray(__in WCHAR * field, __in ULONG count);

        //virtual NTSTATUS ReadStringArray(__out WCHAR * field, __inout ULONG & count);

        virtual NTSTATUS WriteByteArrayNoCopy(__in FabricIOBuffer const * buffers, __in ULONG count, __in ULONG flags);

        virtual NTSTATUS ReadByteArrayNoCopy(_Inout_updates_(count) FabricIOBuffer * bytes, __inout ULONG & count);


        virtual NTSTATUS SeekToBegin();

        virtual NTSTATUS SeekToEnd();

        NTSTATUS Seek(ULONG offset) override;
        ULONG Position() const override;

        NTSTATUS GetNextBuffer(__out FabricIOBuffer & buffer, __out bool & isExtensionData) override;

        NTSTATUS GetAllBuffers(_Out_ FabricIOBuffer* bufferArray, _Inout_ size_t & count) const override;

        virtual NTSTATUS InvokeCallbacks();

        ~FabricSerializableStream();

    private:

        FabricSerializableStream();

        FabricSerializableStream(IFabricStreamUPtr && stream);

        NTSTATUS WriteUInt32WithNoMetaData(__in ULONG value);

        NTSTATUS ReadUInt32WithNoMetaData(__out ULONG & value);

        NTSTATUS WriteMetadata(__in FabricSerializationTypes::Enum value);

        NTSTATUS ReadMetadata(__out FabricSerializationTypes::Enum & value);

        NTSTATUS SeekToLastMetadataPosition();

        NTSTATUS SkipCompressedValue(__in ULONG maxedSkippedBytes);

        NTSTATUS ReadUnknownData();

        NTSTATUS ReadUnknownDataSingleValue(FabricSerializationTypes::Enum readMetadata);

        NTSTATUS ReadUnknownDataArray(FabricSerializationTypes::Enum readMetadata);

        NTSTATUS WriteUnknownDataInExtraScopes();

        NTSTATUS ReadSerializableArrayInternal(_In_reads_opt_(count) IFabricSerializable ** field, __inout ULONG & count, bool noRead = false);

        NTSTATUS ReadPointerArrayInternal(_Inout_updates_(count) IFabricSerializable ** field, __in FabricActivatorFunction activator, __inout ULONG & count, bool noRead = false);

        NTSTATUS ReadUnknownObject();

        template <class T, FabricSerializationTypes::Enum ST>
        NTSTATUS WriteBlittableValue(__in T field);

        template <class T, FabricSerializationTypes::Enum ST>
        NTSTATUS ReadBlittableValue(T & field);

        template <class T, FabricSerializationTypes::Enum ST>
        NTSTATUS WriteBlittableArray(__in T * field, __in ULONG count);

        template <class T, FabricSerializationTypes::Enum ST>
        NTSTATUS ReadBlittableArray(_Out_writes_opt_(count) T * field, __in ULONG & count, bool noRead = false);

        template <class T, FabricSerializationTypes::Enum ST, class Compressor>
        NTSTATUS WriteCompressableValue(__in T field);

        template <class T, FabricSerializationTypes::Enum ST, class Compressor>
        NTSTATUS ReadCompressableValue(T & field);

        template <class T, FabricSerializationTypes::Enum ST, class Compressor>
        NTSTATUS WriteCompressableArray(_In_reads_(count) T * field, __in ULONG count);

        template <class T, FabricSerializationTypes::Enum ST, class Compressor>
        NTSTATUS ReadCompressableArray(_Out_writes_opt_(count) T * field, __inout ULONG & count, bool noRead = false);

        enum HeaderFlags : unsigned char
        {
            Empty                    = 0x00,
            ContainsTypeInformation  = 0x01,
            ContainsExtensionData    = 0x02,
        };

        struct ObjectHeader
        {
            ObjectHeader()
                : size(0)
                , flags(HeaderFlags::Empty)
            {
            }

            ULONG size;
            HeaderFlags flags;
        };

        struct ObjectContext
        {
            ObjectContext()
                : object_(nullptr)
                , header_()
                , headerPosition_(0)
                , currentScope_(0)
            {
            }

            ObjectContext(IFabricSerializable * object)
                : object_(object)
                , header_()
                , headerPosition_(0)
                , currentScope_(0)
            {
            }

            ObjectContext(IFabricSerializable * object, ObjectHeader header, ULONG headerPosition)
                : object_(object)
                , header_(header)
                , headerPosition_(headerPosition)
                , currentScope_(0)
            {
            }

            IFabricSerializable * object_;
            ULONG headerPosition_;
            ULONG currentScope_;
            ObjectHeader header_;
        };

        NTSTATUS ReadSerializableInternal(__in IFabricSerializable * object, __in ObjectContext context);

        NTSTATUS PushObject(__in IFabricSerializable * object);

        NTSTATUS PopObject(__out IFabricSerializable ** object);

        NTSTATUS CurrentObjectContext(__out ObjectContext ** context);

        IFabricStreamUPtr _stream;
        ULONG _lastMetadataPosition;
        KArray<ObjectContext> _objectStack;
        volatile LONG _noCopyIndex;
        volatile LONG _RefCount;
    };
}
