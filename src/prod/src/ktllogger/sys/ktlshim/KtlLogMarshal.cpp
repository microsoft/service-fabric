// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifdef UNIFY
#define KDRIVER 1
#endif

#include <ktl.h>
#include <ktrace.h>

#include "../inc/ktllogger.h"

#if KTL_USER_MODE
extern "C"
{
#if !defined(PLATFORM_UNIX)
#include <winioctl.h>
#endif
};
#else
extern "C"
{
#include <ntddk.h>
};
#endif

#include "KtlLogMarshal.h"

const PWCHAR RequestMarshaller::KTLLOGGER_DEVICE_NAME = DEVICEFILE_NAME_STRING;

//
// Use device type outside the range of Windows
//
const ULONG RequestMarshaller::KTLLOGGER_DEVICE_IOCTL = static_cast<ULONG>(KTLLOGGER_DEVICE_IOCTL_X);

NTSTATUS
RequestMarshaller::SetWriteIndex(
    __in ULONG WriteIndex
)
{
    NTSTATUS status = STATUS_SUCCESS;

    KInvariant((_KBuffer == nullptr) || ((PVOID)_Buffer == _KBuffer->GetBuffer()));
    
    if (WriteIndex > _MaxIndex)
    {
        if (_ExtendBuffer)
        {
            ULONG newSize = 2 * _KBuffer->QuerySize();
            
            status = _KBuffer->SetSize(newSize,
                                          TRUE,                        // PreserveOldContents
                                          _AllocationTag);
            if (! NT_SUCCESS(status))
            {
                return(status);
            }
            _Buffer = (PUCHAR)_KBuffer->GetBuffer();
            _MaxIndex = newSize;
        }
    }
    
    _WriteIndex = WriteIndex;
    return(status);
}


NTSTATUS
RequestMarshaller::WriteBytes(
    __in_bcount(ByteCount) PUCHAR Bytes,
    __in ULONG ByteCount,
    __in ULONG Alignment
)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG bytesLeft;
    ULONG alignmentNeeded;
    ULONG nextWriteIndex;
    ULONG oldWriteIndex;

    if (_WriteIndex > _MaxIndex)
    {
        bytesLeft = 0;
    } else {
        bytesLeft = _MaxIndex - _WriteIndex;
    }
    
    alignmentNeeded = AlignmentNeeded(_WriteIndex, Alignment);
    if (alignmentNeeded >= bytesLeft)
    {
        bytesLeft = 0;
    } else {
        bytesLeft -= alignmentNeeded;
    }
    
    if (ByteCount > bytesLeft)
    {
        bytesLeft = 0;
    }

    nextWriteIndex = _WriteIndex + (alignmentNeeded + ByteCount);
    oldWriteIndex = _WriteIndex;
    status = SetWriteIndex(nextWriteIndex);
    
    if (NT_SUCCESS(status) && (nextWriteIndex <= _MaxIndex))
    {
        PUCHAR destination = static_cast<PUCHAR>(_Buffer) + (oldWriteIndex + alignmentNeeded);
        KMemCpySafe(destination, _MaxIndex - (oldWriteIndex + alignmentNeeded), Bytes, ByteCount);
    }
    
    return(status);
}


NTSTATUS
RequestMarshaller::WriteKBuffer(
    __in KBuffer::SPtr& Buffer
)
{
    NTSTATUS status;
    ULONG oldWriteIndex;

    oldWriteIndex = _WriteIndex;

    if (Buffer)
    {
        status = WriteData(Buffer->QuerySize());
        if (! NT_SUCCESS(status))
        {
            return(status);
        }

        status = WriteBytes((PUCHAR)Buffer->GetBuffer(),
                            Buffer->QuerySize(),
                            sizeof(ULONGLONG));
    } else {
        status = WriteData(0);
        if (! NT_SUCCESS(status))
        {
            return(status);
        }       
    }
    
    if (! NT_SUCCESS(status))
    {
        _WriteIndex = oldWriteIndex;
        return(status);
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshaller::WriteRecordMetadata(
#ifdef USE_RVDLOGGER_OBJECTS
    __in RvdLogStream::RecordMetadata& Record
#else
    __in RecordMetadata& Record     
#endif
)
{
    NTSTATUS status;
    ULONG oldWriteIndex;

    oldWriteIndex = _WriteIndex;

    status = WriteData(Record.Asn);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = WriteData(Record.Version);
    if (! NT_SUCCESS(status))
    {
        _WriteIndex = oldWriteIndex;
        return(status);
    }

    status = WriteData(Record.Disposition);
    if (! NT_SUCCESS(status))
    {
        _WriteIndex = oldWriteIndex;
        return(status);
    }

    status = WriteData(Record.Size);
    if (! NT_SUCCESS(status))
    {
        _WriteIndex = oldWriteIndex;
        return(status);
    }

    status = WriteData(Record.Debug_LsnValue);
    if (! NT_SUCCESS(status))
    {
        _WriteIndex = oldWriteIndex;
        return(status);
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshaller::WriteRecordMetadataArray(
#ifdef USE_RVDLOGGER_OBJECTS
    __in KArray<RvdLogStream::RecordMetadata>& Array
#else
    __in KArray<RecordMetadata>& Array
#endif
)
{
    NTSTATUS status;
    ULONG oldWriteIndex;

    oldWriteIndex = _WriteIndex;
    status = WriteData<ULONG>(Array.Count());
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    for (ULONG i = 0; (i < Array.Count()); i++)
    {
        status = WriteRecordMetadata(Array[i]);
        if (! NT_SUCCESS(status))
        {
            _WriteIndex = oldWriteIndex;
            return(status);
        }
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshaller::WriteKtlLogContainerIdArray(
#ifdef USE_RVDLOGGER_OBJECTS
    __in KArray<RvdLogId>& Array
#else
    __in KArray<KtlLogContainerId>& Array
#endif
)
{
    NTSTATUS status;
    ULONG oldWriteIndex;

    oldWriteIndex = _WriteIndex;
    status = WriteData<ULONG>(Array.Count());
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    for (ULONG i = 0; (i < Array.Count()); i++)
    {
        status = WriteKtlLogContainerId(Array[i]);
        if (! NT_SUCCESS(status))
        {
            _WriteIndex = oldWriteIndex;
            return(status);
        }
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshaller::WriteKtlLogStreamIdArray(
#ifdef USE_RVDLOGGER_OBJECTS
    __in KArray<RvdLogStreamId>& Array
#else
    __in KArray<KtlLogStreamId>& Array
#endif
)
{
    NTSTATUS status;
    ULONG oldWriteIndex;

    oldWriteIndex = _WriteIndex;
    status = WriteData<ULONG>(Array.Count());
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    for (ULONG i = 0; (i < Array.Count()); i++)
    {
        status = WriteKtlLogStreamId(Array[i]);
        if (! NT_SUCCESS(status))
        {
            _WriteIndex = oldWriteIndex;
            return(status);
        }
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshaller::WriteString(
    __in KString::CSPtr String
    )
{
    NTSTATUS status;
    ULONG oldWriteIndex;

    if (! String)
    {
        status = WriteData<ULONG>(0);
        if (! NT_SUCCESS(status))
        {
            return(status);
        }
    } else {
        oldWriteIndex = _WriteIndex;
        status = WriteData(String->Length()+1);
        if (! NT_SUCCESS(status))
        {
            return(status);
        }

        status = WriteBytes((PUCHAR)((PVOID)(*String)),
                            String->LengthInBytes(),
                            sizeof(WCHAR));

        if (! NT_SUCCESS(status))
        {
            _WriteIndex = oldWriteIndex;
            return(status);
        }

        status = WriteData<USHORT>(0);
        if (! NT_SUCCESS(status))
        {
            _WriteIndex = oldWriteIndex;
            return(status);
        }
    }

    return(STATUS_SUCCESS);
}

//
// Read apis
//
NTSTATUS
RequestMarshaller::SetReadIndex(
    __in ULONG ReadIndex
)
{
    if (ReadIndex > _MaxIndex)
    {
        return(STATUS_BUFFER_OVERFLOW);
    }
    
    _ReadIndex = ReadIndex;
    return(STATUS_SUCCESS);
}

NTSTATUS RequestMarshaller::ReadBytes(
    __in_bcount(ByteCount) PUCHAR Bytes,
    __in ULONG ByteCount,
    __in ULONG Alignment
)
{
    ULONG bytesLeft;
    ULONG alignmentNeeded;
    
    bytesLeft = _MaxIndex - _ReadIndex;

    alignmentNeeded = AlignmentNeeded(_ReadIndex, Alignment);

    if (alignmentNeeded > bytesLeft)
    {
        return(STATUS_BUFFER_OVERFLOW);
    }

    bytesLeft -= alignmentNeeded;   
    
    if (ByteCount > bytesLeft)
    {
        return(STATUS_BUFFER_OVERFLOW);
    }

    _ReadIndex += alignmentNeeded;

    PUCHAR source = static_cast<PUCHAR>(_Buffer) + _ReadIndex;
    KMemCpySafe(Bytes, bytesLeft, source, ByteCount);

    _ReadIndex += ByteCount;
    
    return(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshaller::ReadObjectTypeAndId(
    __out OBJECT_TYPEANDID& ObjectTypeAndId 
)
{
    NTSTATUS status;

    status = ReadBytes((PUCHAR)(&ObjectTypeAndId),
                       sizeof(ObjectTypeAndId),
                       sizeof(LONGLONG));

    return(status);
}

NTSTATUS
RequestMarshaller::ReadKBuffer(
    __out KBuffer::SPtr& Buffer
)
{
    NTSTATUS status;
    ULONG oldReadIndex;
    ULONG bufferSize = 0;

    oldReadIndex = _ReadIndex;
    
    status = ReadData<ULONG>(bufferSize);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    if (bufferSize != 0)
    {
        status = KBuffer::Create(bufferSize, Buffer, GetThisAllocator(), GetThisAllocationTag());
        if (! NT_SUCCESS(status))
        {
            _ReadIndex = oldReadIndex;
            return(status);
        }

        status = ReadBytes((PUCHAR)Buffer->GetBuffer(),
                           bufferSize,
                           sizeof(ULONGLONG));
        if (! NT_SUCCESS(status))
        {
            _ReadIndex = oldReadIndex;
            return(status);
        }
    } else {
        Buffer = nullptr;
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshaller::ReadRecordMetadata(
#ifdef USE_RVDLOGGER_OBJECTS
    __in RvdLogStream::RecordMetadata& Record
#else
    __in RecordMetadata& Record     
#endif
)
{
    NTSTATUS status;
    ULONG oldReadIndex;

    oldReadIndex = _ReadIndex;

    status = ReadData<KtlLogAsn>(Record.Asn);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = ReadData<ULONGLONG>(Record.Version);
    if (! NT_SUCCESS(status))
    {
        _ReadIndex = oldReadIndex;
        return(status);
    }

    status = ReadRecordDisposition(Record.Disposition);
    if (! NT_SUCCESS(status))
    {
        _ReadIndex = oldReadIndex;
        return(status);
    }

    status = ReadData<ULONG>(Record.Size);
    if (! NT_SUCCESS(status))
    {
        _ReadIndex = oldReadIndex;
        return(status);
    }

    status = ReadData<LONGLONG>(Record.Debug_LsnValue);
    if (! NT_SUCCESS(status))
    {
        _ReadIndex = oldReadIndex;
        return(status);
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshaller::ReadRecordMetadataArray(
#ifdef USE_RVDLOGGER_OBJECTS
    __in KArray<RvdLogStream::RecordMetadata>& Array
#else
    __in KArray<RecordMetadata>& Array
#endif
)
{
    NTSTATUS status;
    ULONG oldReadIndex;
    ULONG recordCount = 0;

    oldReadIndex = _ReadIndex;
    status = ReadData<ULONG>(recordCount);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    BOOLEAN b;

    status = Array.Reserve(recordCount);
    if (! NT_SUCCESS(status))
    {
        _ReadIndex = oldReadIndex;
        return(status);
    }
    
    b = Array.SetCount(recordCount);
    if (! b)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
        _ReadIndex = oldReadIndex;
        return(status);             
    }

    for (ULONG i = 0; (i < Array.Count()); i++)
    {
        status = ReadRecordMetadata(Array[i]);
        if (! NT_SUCCESS(status))
        {
            _ReadIndex = oldReadIndex;
            return(status);
        }
    }

    return(STATUS_SUCCESS);
}


NTSTATUS
RequestMarshaller::ReadKtlLogContainerIdArray(
#ifdef USE_RVDLOGGER_OBJECTS
    __in KArray<RvdLogId>& Array
#else
    __in KArray<KtlLogContainerId>& Array
#endif
)
{
    NTSTATUS status;
    ULONG oldReadIndex;
    ULONG recordCount = 0;

    oldReadIndex = _ReadIndex;
    status = ReadData<ULONG>(recordCount);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    BOOLEAN b;

    status = Array.Reserve(recordCount);
    if (! NT_SUCCESS(status))
    {
        _ReadIndex = oldReadIndex;
        return(status);
    }
    
    b = Array.SetCount(recordCount);
    if (! b)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
        _ReadIndex = oldReadIndex;
        return(status);             
    }

    for (ULONG i = 0; (i < Array.Count()); i++)
    {
        status = ReadKtlLogContainerId(Array[i]);
        if (! NT_SUCCESS(status))
        {
            _ReadIndex = oldReadIndex;
            return(status);
        }
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshaller::ReadKtlLogStreamIdArray(
#ifdef USE_RVDLOGGER_OBJECTS
    __in KArray<RvdLogStreamId>& Array
#else
    __in KArray<KtlLogStreamId>& Array
#endif
)
{
    NTSTATUS status;
    ULONG oldReadIndex;
    ULONG recordCount = 0;

    oldReadIndex = _ReadIndex;
    status = ReadData<ULONG>(recordCount);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    BOOLEAN b;

    status = Array.Reserve(recordCount);
    if (! NT_SUCCESS(status))
    {
        _ReadIndex = oldReadIndex;
        return(status);
    }
    
    b = Array.SetCount(recordCount);
    if (! b)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
        _ReadIndex = oldReadIndex;
        return(status);             
    }

    for (ULONG i = 0; (i < Array.Count()); i++)
    {
        status = ReadKtlLogStreamId(Array[i]);
        if (! NT_SUCCESS(status))
        {
            _ReadIndex = oldReadIndex;
            return(status);
        }
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
RequestMarshaller::ReadString(
    __out KString::CSPtr& String
)
{
    NTSTATUS status;
    KString::SPtr str;
    
    status = ReadString(str);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    
    String = KString::CSPtr(str.RawPtr());
    return STATUS_SUCCESS;
}

NTSTATUS
RequestMarshaller::ReadString(
    __out KString::SPtr& String
)
{
    NTSTATUS status;
    ULONG oldReadIndex;
    ULONG length = 0;

    oldReadIndex = _ReadIndex;

    status = ReadData<ULONG>(length);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    if (length == 0)
    {
        String = nullptr;
    } else {
        String = KString::Create(GetThisAllocator(),
                                 length);
        if (! String)
        {
            _ReadIndex = oldReadIndex;
            KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
            return(STATUS_INSUFFICIENT_RESOURCES);
        }


        status = ReadBytes((PUCHAR)((PVOID)*String),
                           length * sizeof(WCHAR),
                           sizeof(WCHAR));
        if (! NT_SUCCESS(status))
        {
            String = nullptr;
            _ReadIndex = oldReadIndex;
            return(status);
        }
        String->MeasureThis();
    }

    return(STATUS_SUCCESS);
}


RequestMarshaller::RequestMarshaller()
{
}

RequestMarshaller::RequestMarshaller(
    __in ULONG AllocationTag
    )
{
    _AllocationTag = AllocationTag;
}

RequestMarshaller::~RequestMarshaller()
{
}


