/*++

Copyright (c) 2010  Microsoft Corporation

Module Name:

    KTextFile.h

Abstract:

    The file defines the interface for a simple text file reader.

Author:

    Jeff Havens (jhavens)  30-Mar-2011

Environment:

    Kernel Mode or User Mode

Notes:

Revision History:

--*/

#pragma once

class KTextFile : public KObject<KTextFile>, public KShared<KTextFile>
{
    K_FORCE_SHARED(KTextFile);

public:

    static
    NTSTATUS
    OpenSynchronous(
        __in const KWString& FileName,
        __in ULONG MaxLineLength,
        __out KTextFile::SPtr& File,
        __in ULONG AllocationTag,
        __in KAllocator* Allocator
        );

    static
    NTSTATUS
    OpenAsynchronous(
        __in const KWString& FileName,
        __in ULONG MaxLineLength,
        __out KTextFile::SPtr& File,
        __in ULONG AllocationTag,
        __in KAllocator* Allocator,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync
        );

    KTextFile(
        __in KUniquePtr<WCHAR>& Buffer,
        __in ULONG Length,
        __in ULONG MaxLineLength,
        __in ULONG AllocationTag,
        __in KAllocator* Allocator
        );

    BOOLEAN
    EndOfFile()
    {
        return(_LineStart == _EndBuffer);
    }

    ULONG
    GetCurrentLineNumber()
    {
        return(_LineNumber);
    }

    NTSTATUS
    ReadLine(
        __out KWString& Line
        );

    NTSTATUS
    ReadBinary(
        __out KBuffer::SPtr& Result
        );

    NTSTATUS
    ReadRawBinary(
        __out KBuffer::SPtr& Result
        );

    static const WCHAR UnicodeBom = 0xFEFF;

private:

    class OpenContext;

    KUniquePtr<WCHAR> _Buffer;
    PWCHAR _EndBuffer;
    PWCHAR _LineStart;
    ULONG _LineNumber;
    ULONG _MaxLength;
    ULONG _AllocationTag;
    KAllocator* _Allocator;

    static const KTagType TagName;
    static const ULONG MaximumFileSize = 10000000;
};
