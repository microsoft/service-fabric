// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "ikbase.h"

namespace Serialization
{
    __interface IFabricSerializableStream : public IKBase
    {
        virtual NTSTATUS WriteStartType() = 0;

        virtual NTSTATUS ReadStartType() = 0;

        virtual ULONG Size() const = 0;

        virtual NTSTATUS WriteEndType(__in_opt FabricCompletionCallback completionCallback = nullptr, __in_opt void * state = nullptr) = 0;

        virtual NTSTATUS ReadEndType() = 0;

        virtual NTSTATUS WriteSerializable(__in IFabricSerializable * object) = 0;

        virtual NTSTATUS ReadSerializable(__in IFabricSerializable * object) = 0;

        virtual NTSTATUS ReadSerializableAsPointer(__out IFabricSerializable ** pointer, __in FabricActivatorFunction activator) = 0;

        virtual NTSTATUS WriteSerializableArray(__in IFabricSerializable ** field, __in ULONG count) = 0;

        virtual NTSTATUS ReadSerializableArray(__in IFabricSerializable ** field, __inout ULONG & count) = 0;

        virtual NTSTATUS ReadRawBytes(__in ULONG size, _Out_writes_(size) VOID * destination) = 0;

        virtual NTSTATUS WriteBool(__in bool field) = 0;

        virtual NTSTATUS ReadBool(__out bool & field) = 0;

        virtual NTSTATUS WriteBoolArray(__in bool * field, __in ULONG count) = 0;

        virtual NTSTATUS ReadBoolArray(__out bool * field, __inout ULONG & count) = 0;

        virtual NTSTATUS WriteChar(__in CHAR field) = 0;

        virtual NTSTATUS ReadChar(__out CHAR & field) = 0;

        virtual NTSTATUS WriteCharArray(__in CHAR * field, __in ULONG count) = 0;

        virtual NTSTATUS ReadCharArray(__out CHAR * field, __inout ULONG & count) = 0;

        virtual NTSTATUS WriteUChar(__in UCHAR field) = 0;

        virtual NTSTATUS ReadUChar(__out UCHAR & field) = 0;

        virtual NTSTATUS WriteUCharArray(__in UCHAR * field, __in ULONG count) = 0;

        virtual NTSTATUS ReadUCharArray(__out UCHAR * field, __inout ULONG & count) = 0;

        virtual NTSTATUS WriteShort(__in SHORT field) = 0;

        virtual NTSTATUS ReadShort(__out SHORT & field) = 0;

        virtual NTSTATUS WriteShortArray(__in SHORT * field, __in ULONG count) = 0;

        virtual NTSTATUS ReadShortArray(__out SHORT * field, __inout ULONG & count) = 0;

        virtual NTSTATUS WriteUShort(__in USHORT field) = 0;

        virtual NTSTATUS ReadUShort(__out USHORT & field) = 0;

        virtual NTSTATUS WriteUShortArray(__in USHORT * field, __in ULONG count) = 0;

        virtual NTSTATUS ReadUShortArray(__out USHORT * field, __inout ULONG & count) = 0;

        virtual NTSTATUS WriteInt32(__in LONG field) = 0;

        virtual NTSTATUS ReadInt32(__out LONG & field) = 0;

        virtual NTSTATUS WriteInt32Array(__in LONG * field, __in ULONG count) = 0;

        virtual NTSTATUS ReadInt32Array(__out LONG * field, __inout ULONG & count) = 0;

        virtual NTSTATUS WriteUInt32(__in ULONG field) = 0;

        virtual NTSTATUS ReadUInt32(__out ULONG & field) = 0;

        virtual NTSTATUS WriteUInt32Array(__in ULONG * field, __in ULONG count) = 0;

        virtual NTSTATUS ReadUInt32Array(__out ULONG * field, __inout ULONG & count) = 0;

        virtual NTSTATUS WriteInt64(__in LONG64 field) = 0;

        virtual NTSTATUS ReadInt64(__out LONG64 & field) = 0;

        virtual NTSTATUS WriteInt64Array(__in LONG64 * field, __in ULONG count) = 0;

        virtual NTSTATUS ReadInt64Array(__out LONG64 * field, __inout ULONG & count) = 0;

        virtual NTSTATUS WriteUInt64(__in ULONG64 field) = 0;

        virtual NTSTATUS ReadUInt64(__out ULONG64 & field) = 0;

        virtual NTSTATUS WriteUInt64Array(__in ULONG64 * field, __in ULONG count) = 0;

        virtual NTSTATUS ReadUInt64Array(__out ULONG64 * field, __inout ULONG & count) = 0;

        virtual NTSTATUS WriteGuid(__in GUID field) = 0;

        virtual NTSTATUS ReadGuid(__out GUID & field) = 0;

        virtual NTSTATUS WriteGuidArray(__in GUID * field, __in ULONG count) = 0;

        virtual NTSTATUS ReadGuidArray(__out GUID * field, __inout ULONG & count) = 0;

        virtual NTSTATUS WriteDouble(__in DOUBLE field) = 0;

        virtual NTSTATUS ReadDouble(__out DOUBLE & field) = 0;

        virtual NTSTATUS WriteDoubleArray(__in DOUBLE * field, __in ULONG count) = 0;

        virtual NTSTATUS ReadDoubleArray(__out DOUBLE * field, __inout ULONG & count) = 0;

        virtual NTSTATUS WritePointer(__in IFabricSerializable * field) = 0;

        virtual NTSTATUS ReadPointer(__out IFabricSerializable ** field, __in FabricActivatorFunction activator) = 0;

        virtual NTSTATUS WritePointerArray(__in IFabricSerializable ** field, __in ULONG count) = 0;

        virtual NTSTATUS ReadPointerArray(__out IFabricSerializable ** field, __in FabricActivatorFunction activator, __inout ULONG & count) = 0;

        virtual NTSTATUS WriteString(__in WCHAR * field, __in ULONG count) = 0;

        virtual NTSTATUS ReadString(__out WCHAR * field, __inout ULONG & count) = 0;

        //virtual NTSTATUS WriteStringArray(__in WCHAR * field, __in ULONG count) = 0;

        //virtual NTSTATUS ReadStringArray(__out WCHAR * field, __inout ULONG & count) = 0;



        virtual NTSTATUS WriteByteArrayNoCopy(__in FabricIOBuffer const * buffers, __in ULONG count, __in ULONG flags) = 0;

        virtual NTSTATUS ReadByteArrayNoCopy(__out FabricIOBuffer * bytes, __inout ULONG & count) = 0;

        virtual NTSTATUS WriteRawBytes(ULONG size, void const* source) = 0;

        virtual NTSTATUS SeekToBegin() = 0;

        virtual NTSTATUS SeekToEnd() = 0;

        virtual NTSTATUS Seek(ULONG offset) = 0;

        virtual ULONG Position() const = 0;

        virtual NTSTATUS GetNextBuffer(__out FabricIOBuffer & buffer, __out bool & isExtensionData) = 0;

        virtual NTSTATUS GetAllBuffers(_Out_ FabricIOBuffer* bufferArray, _Inout_ size_t & count) const = 0;

        virtual NTSTATUS InvokeCallbacks() = 0;
    };

    typedef KUniquePtr<IFabricSerializableStream, IKBaseDeleter<IFabricSerializableStream>> IFabricSerializableStreamUPtr;
}

