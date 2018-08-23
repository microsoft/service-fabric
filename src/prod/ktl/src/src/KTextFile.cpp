/*++

Copyright (c) 2010  Microsoft Corporation

Module Name:

    KTextFile.cpp

Abstract:

    This file implements a simple text read that returns data one line at a time.

Author:

    Jeff Havens (jhavens)  30-Mar-2011

Environment:

    Kernel Mode or User Mode

Notes:

Revision History:

--*/

#include "ktl.h"

const KTagType KTextFile::TagName = "TextFile";

KTextFile::KTextFile(
    __in KUniquePtr<WCHAR>& Buffer,
    __in ULONG Length,
    __in ULONG MaxLineLength,
    __in ULONG AllocationTag,
    __in KAllocator* Allocator
    )
    : _MaxLength(MaxLineLength),
    _LineNumber(0),
    _AllocationTag(AllocationTag),
    _Allocator(Allocator)

/*++
 *
 * Routine Description:
 *      This routine initializes the tag and _Buffer pointer fields of the KTextFile
 *      object
 *
 * Arguments:
 *      None
 *
 * Return Value:
 *      None
 *
 * Note:
 *
-*/

{
    SetTypeTag(TagName);

    _Buffer = Ktl::Move(Buffer);
    _EndBuffer = (WCHAR *) _Buffer + (Length / sizeof(WCHAR));

    _LineStart = _Buffer;

    if (_LineStart[0] == UnicodeBom)
    {
        //
        // Skip the unicode byte order mark.
        //

        _LineStart++;

    }
}

NTSTATUS
KTextFile::OpenSynchronous(
    __in const KWString& FileName,
    __in ULONG MaxLineLength,
    __out KTextFile::SPtr& File,
    __in ULONG AllocationTag,
    __in KAllocator* Allocator
    )
/*++
 *
 * Routine Description:
 *      This function opens the the text file and reads the entire file into memory.
 *      The text from the file will be converted to unicode if it is not already.
 *
 * Arguments:
 *      FileName - Supplies the path name of the file to open.
 *
 *      MaxLineLength - Inidicates the maximum length of a line that is expected.
 *          Serves as a sanity check that this is really a text file.
 *
 *      File - Returns a pointer to the KTextFile object.
 *
 *      AllocationTag - Tag value to used for memory allocations.
 *
 *      Allocator - Supplies the allocator to used for memory allocations.
 *
 * Return Value:
 *      Returns the status of the operation.
 *
 * Note:
 *      This routine is synchronous and should not be run on one of Rvd's
 *      asynchronous worker threads.
-*/
{
    NTSTATUS LocalStatus;
    OBJECT_ATTRIBUTES ObjAttributes;
    IO_STATUS_BLOCK IoStatus;
    FILE_STANDARD_INFORMATION StandardInformation;
    HANDLE Handle;
    ULONG Attributes;
    KUniquePtr<WCHAR>  Buffer;
    ULONG Size;

    File = nullptr;

    Attributes = OBJ_CASE_INSENSITIVE;

    InitializeObjectAttributes(
        &ObjAttributes,
        const_cast<KWString&>(FileName),
        Attributes,
        NULL,
        NULL
        );

    LocalStatus = KNt::OpenFile(
        &Handle,
        FILE_GENERIC_READ,
        &ObjAttributes,
        &IoStatus,
        FILE_SHARE_READ,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(LocalStatus))
    {
        return(LocalStatus);
    }

    KFinally([&Handle](){
        KNt::Close(Handle);
    });

    LocalStatus = KNt::QueryInformationFile(
        Handle,
        &IoStatus,
        &StandardInformation,
        sizeof(StandardInformation),
        FileStandardInformation
        );

    if (!NT_SUCCESS(LocalStatus))
    {
        return(LocalStatus);
    }

    if ( StandardInformation.EndOfFile.QuadPart > MaximumFileSize ||
        StandardInformation.EndOfFile.QuadPart < sizeof(WCHAR))
    {
        return(STATUS_FILE_TOO_LARGE);
    }

    //
    // Assume the file is already in unicode.  Round the size up to the number of
    // characters.
    //

	HRESULT hr;
	hr = ULongAdd(StandardInformation.EndOfFile.LowPart, sizeof(WCHAR)-1, &Size);
	KInvariant(SUCCEEDED(hr));
	Size = Size / sizeof(WCHAR);

    Buffer = _new(AllocationTag, *Allocator) WCHAR[Size];

    if (Buffer == nullptr)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    LocalStatus = KNt::ReadFile(
        Handle,
        nullptr,
        nullptr,
        nullptr,
        &IoStatus,
        Buffer,
        StandardInformation.EndOfFile.LowPart,
        nullptr,
        nullptr
        );

    if (!NT_SUCCESS(LocalStatus))
    {
        return(LocalStatus);
    }

    KAssert(IoStatus.Information == StandardInformation.EndOfFile.LowPart);

    Size = StandardInformation.EndOfFile.LowPart;

    //
    // Determine if the data is unicode or not.
    //

    if (UnicodeBom != Buffer[0])
    {
       KUniquePtr<CHAR> AnsiBuf = (PCHAR) Buffer.Detach();
       ULONG AnsiSize = StandardInformation.EndOfFile.LowPart;

       LocalStatus = RtlMultiByteToUnicodeSize(
           &Size,
           AnsiBuf,
           AnsiSize
           );

       if (!NT_SUCCESS(LocalStatus))
       {
           return(LocalStatus);
       }

       Buffer = _new(AllocationTag, *Allocator) WCHAR[Size/sizeof(WCHAR)];

       if (Buffer == nullptr)
       {
           return(STATUS_INSUFFICIENT_RESOURCES);
       }

       LocalStatus = RtlMultiByteToUnicodeN(
           Buffer,
           Size,
           &Size,
           AnsiBuf,
           AnsiSize
           );

       if (!NT_SUCCESS(LocalStatus))
       {
           return(LocalStatus);
       }
    }
    else
    {
        //
        // The file was originally in unicode.  Make sure the file size is a multiple
        // of the character size.
        //

        if ((StandardInformation.EndOfFile.LowPart % sizeof(WCHAR)) != 0)
        {
            return(STATUS_FILE_INVALID);
        }

        KAssert(Size == StandardInformation.EndOfFile.LowPart);

    }

    KAssert((Size % sizeof(WCHAR)) == 0);

    KTextFile *LocalFile = _new(AllocationTag, *Allocator) KTextFile(
        Buffer,
        Size,
        MaxLineLength,
        AllocationTag,
        Allocator
        );

    if (LocalFile == nullptr)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    KAssert(Buffer == nullptr);

    File = LocalFile;
    return(STATUS_SUCCESS);
}

class KTextFile::OpenContext : public KAsyncContextBase, public KThreadPool::WorkItem
{
    K_FORCE_SHARED(OpenContext);

public:

    OpenContext(
        __in ULONG AllocationTag
        );

    VOID
    StartOpen(
        __in const KWString& FileName,
        __in ULONG MaxLineLength,
        __out KTextFile::SPtr& File,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync
        );

    VOID Execute();

private:

    VOID OnStart() override;

    KWString _FileName;
    ULONG _MaxLineLength;
    KTextFile::SPtr* _File;
    ULONG _AllocationTag;
};

KTextFile::OpenContext::OpenContext(
    __in ULONG AllocationTag
    ) : _AllocationTag(AllocationTag), _FileName(GetThisAllocator())
{
}

KTextFile::OpenContext::~OpenContext()
{
}

VOID
KTextFile::OpenContext::StartOpen(
    __in const KWString& FileName,
    __in ULONG MaxLineLength,
    __out KTextFile::SPtr& File,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync
    )
{
    _FileName = FileName;
    _MaxLineLength = MaxLineLength;
    _File = &File;

    Start(ParentAsync, Completion);
}

VOID
KTextFile::OpenContext::OnStart()
{
    NTSTATUS LocalStatus = _FileName.Status();
    if (!NT_SUCCESS(LocalStatus))
    {
        Complete(LocalStatus);
        return;
    }

    //
    // Continues in KTextFile::OpenContext::Execute()
    //
    GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);
}

VOID KTextFile::OpenContext::Execute()
{
    NTSTATUS LocalStatus = KTextFile::OpenSynchronous(
        _FileName,
        _MaxLineLength,
        *_File,
        _AllocationTag,
        &GetThisAllocator()
        );
    Complete(LocalStatus);
}

NTSTATUS
KTextFile::OpenAsynchronous(
    __in const KWString& FileName,
    __in ULONG MaxLineLength,
    __out KTextFile::SPtr& File,
    __in ULONG AllocationTag,
    __in KAllocator* Allocator,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync
    )

/*++
 *
 * Routine Description:
 *      This function opens the the text file and reads the entire file into memory.
 *      The text from the file will be converted to unicode if it is not already.
 *
 * Arguments:
 *      FileName - Supplies the path name of the file to open.
 *
 *      MaxLineLength - Inidicates the maximum length of a line that is expected.
 *          Serves as a sanity check that this is really a text file.
 *
 *      File - Returns a pointer to the KTextFile object.
 *
 *      AllocationTag - Tag value to used for memory allocations.
 *
 *      Allocator - Supplies the allocator to used for memory allocations.
 *
 *      Completion - Supplies the async completion callback.
 *
 *      ParentAsync - Supplies the parent async context.
 *
 * Return Value:
 *      Returns STATUS_PENDING if the operation starts successfully.
 *
 * Note:
 *      This routine is asynchronous and can be run on one of Rvd's
 *      asynchronous worker threads. It uses KTextFile::OpenSynchronous() to do the real work.
-*/

{
    NTSTATUS LocalStatus;
    OpenContext::SPtr Async = _new(AllocationTag, *Allocator) OpenContext(AllocationTag);
    if (!Async)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    LocalStatus = Async->Status();
    if (!NT_SUCCESS(LocalStatus))
    {
        return LocalStatus;
    }

    Async->StartOpen(
        FileName,
        MaxLineLength,
        File,
        Completion,
        ParentAsync
        );
    return STATUS_PENDING;
}

NTSTATUS
KTextFile::ReadLine(
    __out KWString& Line
    )
/*++
 *
 * Routine Description:
 *      This routine returns back each line for from the file.  The scan will stop if
 *      it sees a line greater than maximum line length, a NUL or end of file.
 *
 * Arguments:
 *      Line - Used to return a pointer to the line.
 *
 * Return Value:
 *      Indicates the status of the operation.  An status indicates the line was not
 *      updated.  Once an error is returned the same error will be returned forever
 *      except for STATUS_INSUFFICIENT_RESOURCES.
 *
 * Note:
 *
-*/
{
    PWCHAR Limit;
    BOOLEAN Eol = FALSE;
    PWCHAR Cursor;

    if (_LineStart == _EndBuffer)
    {
        return(STATUS_END_OF_FILE);
    }

    //
    // Increment the line number count.
    //

    _LineNumber++;

    Limit = __min(_LineStart + _MaxLength, _EndBuffer);

    for (Cursor = _LineStart; Cursor < Limit; Cursor++)
    {
        if (*Cursor == L'\n' || *Cursor == L'\0')
        {
            //
            // Files with malformed lines or NULL are rejected.
            //

            return(STATUS_FILE_INVALID);
        }

        if (*Cursor == L'\r')
        {
            if ((Cursor + 1) < Limit && *(Cursor+1) == L'\n')
            {
                Eol = TRUE;
                break;
            }
            else
            {
                //
                // Files will <cr> no <nl> are rejected.
                //

                return(STATUS_FILE_INVALID);
            }
        }
    }

    //
    // If no EOL was found and we are not at EOF then the line was too long.
    //

    if (!Eol && Cursor != _EndBuffer)
    {
        return(RPC_NT_STRING_TOO_LONG);
    }

    //
    // Create a unicode string that can be used to generate a KWString
    //

    UNICODE_STRING LineString;

    LineString.Buffer = _LineStart;
    LineString.Length = LineString.MaximumLength = (USHORT)((Cursor - _LineStart) * sizeof(WCHAR));

    Line = LineString;

    if (!NT_SUCCESS(Line.Status()))
    {
        return(Line.Status());
    }

    //
    // Everything worked so update to the next line.
    //

    if (Cursor == _EndBuffer)
    {
        //
        // Indicate we are at the end of the buffer/file.
        //

        _LineStart = _EndBuffer;
    }
    else
    {
        KAssert(Eol);

        //
        // Set the _LineStart to the next character after the <cr><nl>
        //

        _LineStart = Cursor + 2;

        KAssert(_LineStart <= _EndBuffer);
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
KTextFile::ReadBinary(
    __out KBuffer::SPtr& Result
    )
/*++
 *
 * Routine Description:
 *      This routine returns back the unread binary contents from the file into a generated
 *      KBuffer. The leaves the state at end of file.
 *
 * Arguments:
 *      Result - Used to return a pointer to the remaining contents.
 *
 * Return Value:
 *      Indicates the status of the operation.
 *      Once an error is returned the same error will be returned forever
 *      except for STATUS_INSUFFICIENT_RESOURCES.
 *
 * Note:
 *
-*/
{
    if (_LineStart == _EndBuffer)
    {
        return(STATUS_END_OF_FILE);
    }

    ULONG sizeOfResult;
    ULONG_PTR sizeOfResultUlongPtr;
    HRESULT hr;
    hr = ULongPtrSub((ULONG_PTR)_EndBuffer, (ULONG_PTR)_LineStart, &sizeOfResultUlongPtr);
    KInvariant(SUCCEEDED(hr));

    hr = ULongPtrToULong(sizeOfResultUlongPtr, &sizeOfResult);
    KInvariant(SUCCEEDED(hr));

    hr = ULongMult(sizeOfResult, sizeof(WCHAR), &sizeOfResult);
    KInvariant(SUCCEEDED(hr));

    Result = nullptr;

    NTSTATUS    status = KBuffer::CreateOrCopy(
        Result,
        _LineStart,
        sizeOfResult,
        GetThisAllocator(),
        _AllocationTag);

    if (NT_SUCCESS(status))
    {
        _LineStart = _EndBuffer;        // Set at EOF
    }

    return status;
}

NTSTATUS
KTextFile::ReadRawBinary(
    __out KBuffer::SPtr& Result
    )
/*++
 *
 * Routine Description:
 *      This routine returns back the raw binary contents from the file into a generated
 *      KBuffer. It does not change the state of the file object.
 *
 * Arguments:
 *      Result - Used to return a pointer to the raw buffer.
 *
 * Return Value:
 *      Indicates the status of the operation.
 *      Once an error is returned the same error will be returned forever
 *      except for STATUS_INSUFFICIENT_RESOURCES.
 *
 * Note:
 *
-*/
{
    ULONG sizeOfResult;
    ULONG_PTR sizeOfResultUlongPtr;
    HRESULT hr;
    hr = ULongPtrSub((ULONG_PTR)_EndBuffer, (ULONG_PTR)_Buffer, &sizeOfResultUlongPtr);
    KInvariant(SUCCEEDED(hr));

    hr = ULongPtrToULong(sizeOfResultUlongPtr, &sizeOfResult);
    KInvariant(SUCCEEDED(hr));

    hr = ULongMult(sizeOfResult, sizeof(WCHAR), &sizeOfResult);
    KInvariant(SUCCEEDED(hr));

    Result = nullptr;

    NTSTATUS    status = KBuffer::CreateOrCopy(
        Result,
        (WCHAR*)_Buffer,
        sizeOfResult,
        GetThisAllocator(),
        _AllocationTag);

    return status;
}


KTextFile::~KTextFile(){}

