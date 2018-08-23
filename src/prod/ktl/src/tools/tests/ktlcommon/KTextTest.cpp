/*++

Copyright (c) 2010  Microsoft Corporation

Module Name:

    KTextTest.cpp

Abstract:

    This file implements tests for KTextFile

Author:

    Jeff Havens (jhavens)  30-Mar-2011

Environment:

    Kernel Mode or User Mode

Notes:

Revision History:

--*/

#include "KtlCommonTests.h"
#include "ktl.h"
#include "ktrace.h"
#include "CommandLineParser.h"
#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif

NTSTATUS
WriteSynchronous(
    __in const KWString& FileName,
    __in KBuffer::SPtr Buffer,
    __in KBlockFile::CreateDisposition Disposition
    )
/*++
 *
 * Routine Description:
 *      This function opens the the text file and writes the supplied buffer into the
 *      file.
 *
 * Arguments:
 *      FileName - Supplies the path name of the file to open.
 *
 *      Buffer - Supplies the data to be written.
 *
 *      Disposition - Indicates how the file should be opened.
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
    HANDLE Handle;
    ULONG Attributes;
    ULONG CreateDisposition;

    //
    // Translate the create disposition.
    //

    switch (Disposition) {

        case KBlockFile::eCreateNew:
            CreateDisposition = FILE_CREATE;
            break;

        case KBlockFile::eCreateAlways:
            CreateDisposition = FILE_OVERWRITE_IF;
            break;

        case KBlockFile::eOpenExisting:
            CreateDisposition = FILE_OPEN;
            break;

        case KBlockFile::eOpenAlways:
            CreateDisposition = FILE_OPEN_IF;
            break;

        case KBlockFile::eTruncateExisting:
            CreateDisposition = FILE_OVERWRITE;
            break;

        default:
            return(STATUS_INVALID_PARAMETER);
    }


    Attributes = OBJ_CASE_INSENSITIVE;

    InitializeObjectAttributes(
        &ObjAttributes,
        const_cast<KWString&>(FileName),
        Attributes,
        NULL,
        NULL
        );

    LocalStatus = KNt::CreateFile(
        &Handle,
        FILE_GENERIC_WRITE,
        &ObjAttributes,
        &IoStatus,
        nullptr,
        0,
        0,
        CreateDisposition,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        nullptr,
        0
        );

    if (!NT_SUCCESS(LocalStatus))
    {
        return(LocalStatus);
    }

    KFinally([&Handle](){
        KNt::Close(Handle);
    });


    LocalStatus = KNt::WriteFile(
        Handle,
        nullptr,
        nullptr,
        nullptr,
        &IoStatus,
        Buffer->GetBuffer(),
        Buffer->QuerySize(),
        nullptr,
        nullptr
        );

    if (!NT_SUCCESS(LocalStatus))
    {
        return(LocalStatus);
    }

    return(STATUS_SUCCESS);
}

CHAR TestData[] = {
    "The quick fox jumped over the lazy brown dog.\r\n"
    "Line 2\r\n\r\n"
    "Line4"
};

NTSTATUS
WriteAndCount(
    KBuffer::SPtr &Buffer,
    ULONG &LineCount,
    ULONG &CharCount
    )
{
    NTSTATUS LocalStatus = STATUS_SUCCESS;
#if !defined(PLATFORM_UNIX)
    PCWCHAR Path = L"\\SystemRoot\\temp\\KTextTest.txt";
#else
    PCWCHAR Path = L"/tmp/KTextTest.txt";
#endif

    CharCount = 0;
    LineCount = 0;

    LocalStatus = WriteSynchronous(
        KWString(KtlSystem::GlobalNonPagedAllocator(), Path),
        Buffer,
        KBlockFile::eCreateAlways
        );

    if (!NT_SUCCESS(LocalStatus))
    {
        KDbgCheckpointWStatus(static_cast<ULONG>(-1), "WriteSynchronous failed.", LocalStatus);
        return(LocalStatus);
    }

    KTextFile::SPtr File;

    for (ULONG TestRun = 0; TestRun < 2; TestRun++)
    {
        if (TestRun % 2 == 0)
        {
            File.Reset();

            LocalStatus = KTextFile::OpenSynchronous(
                KWString(KtlSystem::GlobalNonPagedAllocator(), Path),
                256,
                File,
                KTL_TAG_TEST,
                &KtlSystem::GlobalNonPagedAllocator()
                );

            if (!NT_SUCCESS(LocalStatus))
            {
                KDbgCheckpointWStatus(static_cast<ULONG>(-1), "KTextFile:OpenSynchronous failed", LocalStatus);
                return(LocalStatus);
            }
        }
        else
        {
            KSynchronizer sync;

            File.Reset();

            LocalStatus = KTextFile::OpenAsynchronous(
                KWString(KtlSystem::GlobalNonPagedAllocator(), Path),
                256,
                File,
                KTL_TAG_TEST,
                &KtlSystem::GlobalNonPagedAllocator(),
                sync,
                nullptr
                );

            if (!K_ASYNC_SUCCESS(LocalStatus))
            {
                KDbgCheckpointWStatus(static_cast<ULONG>(-1), "KTextFile:OpenAsynchronous failed", LocalStatus);
                return(LocalStatus);
            }

            LocalStatus = sync.WaitForCompletion();

            if (!NT_SUCCESS(LocalStatus))
            {
                KDbgCheckpointWStatus(static_cast<ULONG>(-1), "KTextFile:OpenAsynchronous failed", LocalStatus);
                return(LocalStatus);
            }
        }

        ULONG TmpLineCount = LineCount;
        ULONG TmpCharCount = CharCount;

        CharCount = 0;
        while(!File->EndOfFile())
        {
            KWString Line(KtlSystem::GlobalNonPagedAllocator());

            LocalStatus = File->ReadLine(Line);
            if (!NT_SUCCESS(LocalStatus))
            {
                LineCount = File->GetCurrentLineNumber();
                return(LocalStatus);
            }

            CharCount += Line.Length()/sizeof(WCHAR);
        }

        if (TmpCharCount && TmpCharCount != CharCount)
        {
            KTestPrintf("File read returns %d and %d characters. Expected identical count. \n", TmpCharCount, CharCount);
            return(STATUS_UNSUCCESSFUL);
        }

        LineCount = File->GetCurrentLineNumber();
        if (TmpLineCount && TmpLineCount != LineCount)
        {
            KTestPrintf("File read returns %d and %d lines. Expected identical count. \n", TmpLineCount, LineCount);
            return(STATUS_UNSUCCESSFUL);
        }
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
KtlTextTest(
    __in int argc,
    __in_ecount(argc) WCHAR* args[]
    )

{
    NTSTATUS LocalStatus = STATUS_SUCCESS;
    KtlSystem* System;

    EventRegisterMicrosoft_Windows_KTL();
    KFinally([](){EventUnregisterMicrosoft_Windows_KTL();});

    LocalStatus = KtlSystem::Initialize(FALSE,     // Do not enable VNetwork, we don't need it
                                   &System);
    KInvariant(NT_SUCCESS(LocalStatus));
    System->SetStrictAllocationChecks(TRUE);

    KFinally([](){KtlSystem::Shutdown();});

    CmdLineParser Parser(KtlSystem::GlobalNonPagedAllocator());
    Parameter *Param;
    PCWCHAR Path = nullptr;
    BOOLEAN DefaultTest = TRUE;
    ULONG i;
    ULONG CharCount;

    if (Parser.Parse( argc, args))
    {
        for (i = 0; i < Parser.ParameterCount(); i++)
        {
            Param = Parser.GetParam( i );

            if ( _wcsicmp(Param->_Name, L"Path") == 0 )
            {


                if (Param->_ValueCount)
                {
                    DefaultTest = FALSE;
                    Path = Param->_Values[0];
                }
            }

        }
    }

    if (DefaultTest)
    {
        KTestPrintf("Starting text test\n");

        KBuffer::SPtr Buffer;

        LocalStatus = KBuffer::Create(
            sizeof(TestData),
            Buffer,
            KtlSystem::GlobalNonPagedAllocator(),
            KTL_TAG_TEST
            );

        if (!NT_SUCCESS(LocalStatus))
        {
            KDbgCheckpointWStatus(static_cast<ULONG>(-1), "KBuffer::Create failed.", LocalStatus);
            return(LocalStatus);
        }

        KMemCpySafe(Buffer->GetBuffer(), Buffer->QuerySize(), TestData, sizeof(TestData));

        //
        // The buffer ends with a null so a error is expected on line 4.
        //

        LocalStatus = WriteAndCount( Buffer, i, CharCount);

        if (NT_SUCCESS(LocalStatus) || i != 4)
        {

            KTestPrintf("File has %d lines and %d characters. Status = %8x\nExpected 4 lines and falied status. \n", i, CharCount, LocalStatus);
            return(STATUS_UNSUCCESSFUL);
        }

        //
        // Remove the null from the buffer. Line 4 now ends with <eof> and should
        // succeed.
        //

        Buffer->SetSize(sizeof(TestData) - 1);

        LocalStatus = WriteAndCount( Buffer, i, CharCount);

        if (!NT_SUCCESS(LocalStatus) || i != 4 || CharCount != 56)
        {

            KTestPrintf("File has %d lines and %d characters. Status = %8x\n"
                "Expected 4 lines and 56 characters.\n", i, CharCount, LocalStatus);
            return(LocalStatus);
        }

        //
        // Remove the "Line4\0" so line 3 whens with \r\n and the file only has 3
        // lines and should succeed.
        //

        Buffer->SetSize(sizeof(TestData) - 6);

        LocalStatus = WriteAndCount( Buffer, i, CharCount);

        if (!NT_SUCCESS(LocalStatus) || i != 3 || CharCount != 51)
        {

            KTestPrintf("File has %d lines and %d characters. Status = %8x\n"
                "Expected 3 lines and 51 characters.\n", i, CharCount, LocalStatus);
            return(LocalStatus);
        }

        //
        // Create a unicode version of the file.
        //

        LocalStatus = RtlMultiByteToUnicodeSize(
            &i,
            TestData,
            sizeof(TestData)
            );

        if (!NT_SUCCESS(LocalStatus))
        {
            KDbgCheckpointWStatus(static_cast<ULONG>(-1), "RtlMultiByteToUnicodeSize failed", LocalStatus);
            return(LocalStatus);
        }

        LocalStatus = KBuffer::Create(
            i,
            Buffer,
            KtlSystem::GlobalNonPagedAllocator(),
            KTL_TAG_TEST
            );

        if (!NT_SUCCESS(LocalStatus))
        {
            KDbgCheckpointWStatus(static_cast<ULONG>(-1), "KBuffer::Create failed.", LocalStatus);
            return(LocalStatus);
        }

        PWCHAR CharPtr = (PWCHAR) Buffer->GetBuffer();

        //
        // Set the first character to the byte order mark.
        //

        *CharPtr = KTextFile::UnicodeBom;
        CharPtr++;

        //
        // Convert the test data to unicode but do not include the <nul> at the end.
        // This should succeed.
        //

        LocalStatus = RtlMultiByteToUnicodeN(
            CharPtr,
            i - sizeof(WCHAR),
            nullptr,
            TestData,
            sizeof(TestData) -1
            );

        if (!NT_SUCCESS(LocalStatus))
        {
            KDbgCheckpointWStatus(static_cast<ULONG>(-1), "RtlMultiByteToUnicodeN failed", LocalStatus);
            return(LocalStatus);
        }

        LocalStatus = WriteAndCount( Buffer, i, CharCount);

        if (!NT_SUCCESS(LocalStatus) || i != 4 || CharCount != 56)
        {

            KTestPrintf("File has %d lines and %d characters. Status = %8x\n"
                "Expected 4 lines and 56 characters.\n", i, CharCount, LocalStatus);
            return(LocalStatus);
        }

        //
        // Set the buffer to a unicode file with no characters.  This should succeed.
        //

        Buffer->SetSize(sizeof(WCHAR));

        LocalStatus = WriteAndCount( Buffer, i, CharCount);

        if (!NT_SUCCESS(LocalStatus) || i != 0 || CharCount != 0)
        {

            KTestPrintf("File has %d lines and %d characters. Status = %8x\n"
                "Expected 0 lines and 0 characters.\n", i, CharCount, LocalStatus);
            return(LocalStatus);
        }


#if !defined(PLATFORM_UNIX)
        PCWCHAR Path1 = L"\\SystemRoot\\temp\\KTextTest.txt";
#else
        PCWCHAR Path1 = L"/tmp/KTextTest.txt";
#endif

        LocalStatus = KNt::DeleteFile(Path1);

        if (!NT_SUCCESS(LocalStatus))
        {
            KDbgCheckpointWStatus(static_cast<ULONG>(-1), "RtlMultiByteToUnicodeN failed", LocalStatus);
            return(LocalStatus);
        }
    }
    else
    {

        KTextFile::SPtr File;
        i = CharCount = 0;

        for (ULONG TestRun = 0; TestRun < 2; TestRun++)
        {
            if (TestRun % 2 == 0)
            {
                File.Reset();

                LocalStatus = KTextFile::OpenSynchronous(
                    KWString(KtlSystem::GlobalNonPagedAllocator(), Path),
                    256,
                    File,
                    KTL_TAG_TEST,
                    &KtlSystem::GlobalNonPagedAllocator()
                    );

                if (!NT_SUCCESS(LocalStatus))
                {
                    KDbgCheckpointWStatus(static_cast<ULONG>(-1), "KTextFile:OpenSynchronous failed", LocalStatus);
                    return(LocalStatus);
                }
            }
            else
            {
                KSynchronizer sync;

                File.Reset();

                LocalStatus = KTextFile::OpenAsynchronous(
                    KWString(KtlSystem::GlobalNonPagedAllocator(), Path),
                    256,
                    File,
                    KTL_TAG_TEST,
                    &KtlSystem::GlobalNonPagedAllocator(),
                    sync,
                    nullptr
                    );

                if (!K_ASYNC_SUCCESS(LocalStatus))
                {
                    KDbgCheckpointWStatus(static_cast<ULONG>(-1), "KTextFile:OpenAsynchronous failed", LocalStatus);
                    return(LocalStatus);
                }

                LocalStatus = sync.WaitForCompletion();

                if (!NT_SUCCESS(LocalStatus))
                {
                    KDbgCheckpointWStatus(static_cast<ULONG>(-1), "KTextFile:OpenAsynchronous failed", LocalStatus);
                    return(LocalStatus);
                }
            }

            ULONG TmpLineCount = i;
            ULONG TmpCharCount = CharCount;

            i = CharCount = 0;
            while(!File->EndOfFile())
            {
                KWString Line(KtlSystem::GlobalNonPagedAllocator());

                LocalStatus = File->ReadLine(Line);
                if (!NT_SUCCESS(LocalStatus))
                {
                    KDbgCheckpointWStatus(static_cast<ULONG>(-1), "KTextFile:ReadLine failed", LocalStatus);
                    return(LocalStatus);
                }

                i++;
                CharCount += Line.Length()/sizeof(WCHAR);
            }

            KTestPrintf("File has %d lines and %d characters\n", i, CharCount);

            if (TmpLineCount && TmpLineCount != i)
            {
                KTestPrintf("File read returns %d and %d lines. Expected identical count. \n", TmpLineCount, i);
                return(STATUS_UNSUCCESSFUL);
            }

            if (TmpCharCount && TmpCharCount != CharCount)
            {
                KTestPrintf("File read returns %d and %d characters. Expected identical count. \n", TmpCharCount, CharCount);
                return(STATUS_UNSUCCESSFUL);
            }

        }

    }

    Parser.Reset();

    //
    // Sleep to give time for asyncs to be cleaned up before allocation
    // check. It requires a context switch and a little time on UP machines
    //

    if (NT_SUCCESS(LocalStatus))
    {
        KTestPrintf("Text returning tests Passed\n");
    }

    return(LocalStatus);

}
