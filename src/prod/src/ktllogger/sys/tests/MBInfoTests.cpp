// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#if defined(PLATFORM_UNIX)
#include <boost/test/unit_test.hpp>
#include "boost-taef.h"
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>

#if defined(UDRIVER) || defined(UPASSTHROUGH)

#include <ktllogger.h>
#include "KtlLoggerTests.h"
#include <KtlLogShimKernel.h>

#if !defined(PLATFORM_UNIX)
#include "WexTestClass.h"
#endif

#if KTL_USER_MODE
 #define _printf(...)   printf(__VA_ARGS__)
// #define _printf(...)   KDbgPrintf(__VA_ARGS__)

 extern volatile LONGLONG gs_AllocsRemaining;
#else
 #define _printf(...)   KDbgPrintf(__VA_ARGS__)
 #define wprintf(...)    KDbgPrintf(__VA_ARGS__)
#endif

#define ALLOCATION_TAG 'LLKT'

#include "VerifyQuiet.h"
 
extern KAllocator* g_Allocator;

#if KTL_USER_MODE

extern volatile LONGLONG gs_AllocsRemaining;

#endif

static const ULONG GUID_STRING_LENGTH = 40;


NTSTATUS GenerateFileName(
    KWString& FileName,
    KGuid DiskId,
    KGuid Guid
    )
{
    BOOLEAN b;    
    KString::SPtr guidString;
    
    //
    // Caller us using the default file name as only specified the disk
    //
    // Filename expected to be of the form:
    //    "\GLOBAL??\Volume{78572cc3-1981-11e2-be6c-806e6f6e6963}\RvdLog\Log{39a26fb9-eaf6-49d8-8432-cf6d9fb9b5e2}.log"
    // or on Linux
    //    "/RvdLog/Log{39a26fb9-eaf6-49d8-8432-cf6d9fb9b5e2}.log"
    //
    guidString = KString::Create(*g_Allocator,
                                 GUID_STRING_LENGTH);
    if (! guidString)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

#if !defined(PLATFORM_UNIX)
    b = guidString->FromGUID(DiskId);
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    b = guidString->SetNullTerminator();
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }   

    FileName = L"\\GLOBAL??\\Volume";
    FileName += static_cast<WCHAR*>(*guidString);
#else
    FileName = L"";
#endif

    b = guidString->FromGUID(Guid);
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    b = guidString->SetNullTerminator();
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }   

    FileName += KVolumeNamespace::PathSeparator;
    FileName += L"MBInfoTest";
    FileName += KVolumeNamespace::PathSeparator;
    FileName += L"Test";
    FileName += static_cast<WCHAR*>(*guidString);
#if defined(PLATFORM_UNIX)
    FileName += L":MBInfo";
#endif
    return(FileName.Status());
}

NTSTATUS GenerateFileName2(
    KWString& FileName,
    KGuid DiskId,
    KGuid Guid
    )
{    
    KString::SPtr guidString;
    BOOLEAN b;
    
    //
    // Caller us using the default file name as only specified the disk
    //
    // Filename expected to be of the form:
    //    "\GLOBAL??\Volume{78572cc3-1981-11e2-be6c-806e6f6e6963}\RvdLog\Log{39a26fb9-eaf6-49d8-8432-cf6d9fb9b5e2}.log"
    // or on Linux
    //    "/RvdLog/Log{39a26fb9-eaf6-49d8-8432-cf6d9fb9b5e2}.log"
    //

    guidString = KString::Create(*g_Allocator,
                                 GUID_STRING_LENGTH);
    if (! guidString)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

#if !defined(PLATFORM_UNIX)
    b = guidString->FromGUID(DiskId);
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    b = guidString->SetNullTerminator();
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }   

    FileName = L"\\GLOBAL??\\Volume";
    FileName += static_cast<WCHAR*>(*guidString);

#else
    FileName = L"";
#endif

    b = guidString->FromGUID(Guid);
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    b = guidString->SetNullTerminator();
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }   
    
    FileName += KVolumeNamespace::PathSeparator;
    FileName += L"MBInfoTest";
    FileName += KVolumeNamespace::PathSeparator;
    FileName += L"Log";

    FileName += static_cast<WCHAR*>(*guidString);
    FileName += L".Log";
#if defined(PLATFORM_UNIX)
    FileName += L":MBInfo";
#endif
    return(FileName.Status());
}

NTSTATUS GenerateFileName3(
    KWString& FileName,
    KGuid DiskId,
    KGuid Guid
    )
{
    KString::SPtr guidString;
    BOOLEAN b;
    
    //
    // Caller us using the default file name as only specified the disk
    //
    // Filename expected to be of the form:
    //    "\GLOBAL??\Volume{78572cc3-1981-11e2-be6c-806e6f6e6963}\RvdLog\Log{39a26fb9-eaf6-49d8-8432-cf6d9fb9b5e2}.log"
    // or on Linux
    //    "/RvdLog/Log{39a26fb9-eaf6-49d8-8432-cf6d9fb9b5e2}.log"
    //

    guidString = KString::Create(*g_Allocator,
                                 GUID_STRING_LENGTH);
    if (! guidString)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

#if !defined(PLATFORM_UNIX)
    b = guidString->FromGUID(DiskId);
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    b = guidString->SetNullTerminator();
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }   

    FileName = L"\\GLOBAL??\\Volume";
    FileName += static_cast<WCHAR*>(*guidString);

#else
    FileName = L"";
#endif

    b = guidString->FromGUID(Guid);
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    b = guidString->SetNullTerminator();
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }
    
    FileName += KVolumeNamespace::PathSeparator;
    FileName += L"RvdLog";
    FileName += KVolumeNamespace::PathSeparator;
    FileName += L"Log";
    FileName += static_cast<WCHAR*>(*guidString);
    FileName += L".Log";
#if defined(PLATFORM_UNIX)
    FileName += L":MBInfo";
#endif
    return(FileName.Status());
}

NTSTATUS CreateMBInfoFile(    
    MBInfoAccess::SPtr& Mbi,
    KGuid& DiskId,
    KGuid Guid,
    ULONG NumberOfBlocks,
    ULONG BlockSize,
    ULONG NumberOfOverwrites
    )
{
    NTSTATUS status;    
    KWString fileName(*g_Allocator);
    KString::SPtr fileNameString;
    KtlLogContainerId logContainerId = Guid;
    KSynchronizer sync;

    status = GenerateFileName(fileName, DiskId, Guid);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    fileNameString = KString::Create((LPCWSTR)fileName,
                                     *g_Allocator);
    VERIFY_IS_TRUE(fileNameString ? TRUE : FALSE);
    
    status = MBInfoAccess::Create(Mbi,
                                  DiskId,
                                  fileNameString.RawPtr(),
                                  logContainerId,
                                  NumberOfBlocks,
                                  BlockSize,
                                  *g_Allocator,
                                  ALLOCATION_TAG);
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    //
    // Initialize new MB
    //
    Mbi->StartInitializeMetadataBlock(NULL,           // Initialization data array
                                      nullptr,
                                      sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    {
        const ULONG MAX_NUMBER_BLOCKS = 4096;
        KSynchronizer writeSync[MAX_NUMBER_BLOCKS];
        KIoBuffer::SPtr ioBuffer;
        PVOID buffer;

        VERIFY_IS_TRUE(NumberOfBlocks < MAX_NUMBER_BLOCKS);
        
        status = KIoBuffer::CreateSimple(BlockSize,
                                       ioBuffer,
                                       buffer,
                                       *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Open the MB
        //
        Mbi->Reuse();
        Mbi->StartOpenMetadataBlock(nullptr,
                                    sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status)); 
        
        //
        // Acquire an exclusive lock
        //
        MBInfoAccess::AsyncAcquireExclusiveContext::SPtr aaec;
        
        status = Mbi->CreateAsyncAcquireExclusiveContext(aaec);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        aaec->StartAcquireExclusive(nullptr,
                                    sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status)); 

        //
        // Perform writes to the sections
        //
        for (ULONG i = 0; i < NumberOfOverwrites; i++)
        {
            
            memset(buffer, (UCHAR)i, BlockSize);

            for (ULONG j = 0; j < NumberOfBlocks; j++)
            {
                MBInfoAccess::AsyncWriteSectionContext::SPtr write;
                
                status = Mbi->CreateAsyncWriteSectionContext(write);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                write->StartWriteSection(j * BlockSize,
                                         ioBuffer,
                                         nullptr,
                                         sync);
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));                 
            }
        }

        //
        // Release the lock
        //
        status = aaec->ReleaseExclusive();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }
    
    //
    // Close the MB
    //
    Mbi->Reuse();
    Mbi->StartCloseMetadataBlock(nullptr,
                                sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    //
    // This leaves all blocks set to write next at CopyA with a
    // generation number of 3
    //
    
    return(status);
}


NTSTATUS RawReadDataBlock(
    KGuid& DiskId,
    KGuid Guid,
    ULONG NumberOfBlocks,
    ULONG BlockSize,
    BOOLEAN ReadCopyA,
    ULONG BlockIndex,
    KIoBuffer::SPtr& Data
    )
{
    NTSTATUS status;
    KWString fileName(*g_Allocator);
    KSynchronizer sync;
    KBlockFile::SPtr file;

    status = GenerateFileName(fileName, DiskId, Guid);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    fileName += L":MBInfo";
    VERIFY_IS_TRUE(NT_SUCCESS(fileName.Status()));

    status = KBlockFile::Create(fileName,
                                TRUE,        // IsWriteThrough
                                KBlockFile::eOpenExisting,
                                file,
                                sync,
                                NULL,        // Parent
                                *g_Allocator);  
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    if (! ReadCopyA)
    {
        BlockIndex += NumberOfBlocks;
    }

    PVOID buffer;
    status = KIoBuffer::CreateSimple(BlockSize,
                                   Data,
                                   buffer,
                                   *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    ULONGLONG fileOffset = BlockIndex * BlockSize;
    status = file->Transfer(KBlockFile::eForeground,
                                      KBlockFile::eRead,
                                      fileOffset,
                                      *Data,
                                      sync,
                                      NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    file->Close();
    
    return(status);
}

NTSTATUS RawWriteDataBlock(
    KGuid& DiskId,
    KGuid Guid,
    ULONG NumberOfBlocks,
    ULONG BlockSize,
    BOOLEAN ReadCopyA,
    ULONG BlockIndex,
    KIoBuffer::SPtr Data
    )
{
    NTSTATUS status;
    KWString fileName(*g_Allocator);
    KSynchronizer sync;
    KBlockFile::SPtr file;

    status = GenerateFileName(fileName, DiskId, Guid);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    fileName += L":MBInfo";
    VERIFY_IS_TRUE(NT_SUCCESS(fileName.Status()));

    status = KBlockFile::Create(fileName,
                                TRUE,        // IsWriteThrough
                                KBlockFile::eOpenExisting,
                                file,
                                sync,
                                NULL,        // Parent
                                *g_Allocator);  
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    if (! ReadCopyA)
    {
        BlockIndex += NumberOfBlocks;
    }

    ULONGLONG fileOffset = BlockIndex * BlockSize;

    status = file->Transfer(KBlockFile::eForeground,
                                      KBlockFile::eWrite,
                                      fileOffset,
                                      *Data,
                                      sync,
                                      NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    file->Close();
    
    return(status);
}

NTSTATUS ReadDataBlock(
    MBInfoAccess::SPtr Mbi,
    ULONG BlockIndex,
    ULONG BlockSize,
    KIoBuffer::SPtr& ReadBuffer
    )
{
    NTSTATUS status;
    MBInfoAccess::AsyncReadSectionContext::SPtr read;
    KSynchronizer sync;
    
    //
    // Open the MB
    //
    Mbi->Reuse();
    Mbi->StartOpenMetadataBlock(nullptr,
                                sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status)); 

    Mbi->DumpCachedReads();
    
    //
    // Acquire an exclusive lock
    //
    MBInfoAccess::AsyncAcquireExclusiveContext::SPtr aaec;

    status = Mbi->CreateAsyncAcquireExclusiveContext(aaec);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    aaec->StartAcquireExclusive(nullptr,
                                sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    status = Mbi->CreateAsyncReadSectionContext(read);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    read->StartReadSection( BlockIndex * BlockSize,
                            BlockSize,
                            FALSE,
                            &ReadBuffer,
                            nullptr,
                            sync);

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Release the lock
    //
    status = aaec->ReleaseExclusive();
    VERIFY_IS_TRUE(NT_SUCCESS(status)); 
    
    //
    // Close the MB
    //
    Mbi->Reuse();
    Mbi->StartCloseMetadataBlock(nullptr,
                                sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status)); 

    return(status);
}

VOID BasicMBInfoTest(
    KGuid& DiskId,
    ULONG NumberOfLoops,
    ULONG NumberOfBlocks,
    ULONG BlockSize
)
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    KSynchronizer sync;
    KSynchronizer syncArr[4096];
    KIoBuffer::SPtr writeIoBuffer[4096];
    KIoBuffer::SPtr readIoBuffer[4096];
    
    MBInfoAccess::SPtr mbi;
    MBInfoAccess::AsyncAcquireExclusiveContext::SPtr aaec;

    VERIFY_IS_TRUE((NumberOfBlocks * NumberOfLoops) <= 4096);

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);
    
    status = MBInfoAccess::Create(mbi,
                                  DiskId,
                                  nullptr,
                                  logContainerId,
                                  NumberOfBlocks,
                                  BlockSize,
                                  *g_Allocator,
                                  ALLOCATION_TAG);
    VERIFY_IS_TRUE(NT_SUCCESS(status), L"Create MBInfoAccess");


    //
    // Initialize new MB
    //
    mbi->StartInitializeMetadataBlock(NULL,
                                      nullptr,
                                      sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status)); 

    for (ULONG loops = 0; loops < NumberOfLoops; loops++)
    {
        //
        // Open the MB
        //
        mbi->Reuse();
        mbi->StartOpenMetadataBlock(nullptr,
                                    sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status)); 

        //
        // Acquire an exclusive lock
        //
        status = mbi->CreateAsyncAcquireExclusiveContext(aaec);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        aaec->StartAcquireExclusive(nullptr,
                                    sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status)); 

        //
        // Write random stuff to each block
        //
        for (ULONG i = 0; i < NumberOfBlocks; i++)
        {
            ULONG index = i;
            MBInfoAccess::AsyncWriteSectionContext::SPtr write;

            status = CreateAndFillIoBuffer((UCHAR)(index + (i / NumberOfBlocks)), BlockSize, writeIoBuffer[index]);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            
            status = mbi->CreateAsyncWriteSectionContext(write);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            write->StartWriteSection(index * BlockSize,
                                     writeIoBuffer[index],
                                     nullptr,
                                     syncArr[i]);
        }

        for (ULONG i = 0; i < NumberOfBlocks; i++)
        {
            status = syncArr[i].WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status)); 
        }

        //
        // Release the lock
        //
        status = aaec->ReleaseExclusive();
        VERIFY_IS_TRUE(NT_SUCCESS(status)); 
        

        //
        // Close the MB
        //
        mbi->Reuse();
        mbi->StartCloseMetadataBlock(nullptr,
                                    sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status)); 

        //
        // Reopen the MB
        //
        mbi->Reuse();
        mbi->StartOpenMetadataBlock(nullptr,
                                    sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status)); 

        //
        // Acquire an exclusive lock
        //
        aaec->Reuse();
        aaec->StartAcquireExclusive(nullptr,
                                    sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status)); 

        //
        // Read the stuff from each block and verify
        //
        for (ULONG i = 0; i < NumberOfBlocks; i++)
        {
            MBInfoAccess::AsyncReadSectionContext::SPtr read;

            status = mbi->CreateAsyncReadSectionContext(read);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            read->StartReadSection( i * BlockSize,
                                    BlockSize,
                                    TRUE,
                                    &readIoBuffer[i],
                                    nullptr,
                                    syncArr[i]);
            
        }

        for (ULONG i = 0; i < NumberOfBlocks; i++)
        {
            status = syncArr[i].WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status)); 

            status = CompareKIoBuffers(*writeIoBuffer[i],
                                       *readIoBuffer[i],
                                       sizeof(MBInfoAccess::MBInfoHeader));
            VERIFY_IS_TRUE(NT_SUCCESS(status)); 
        }

        //
        // Dump any cache
        //
        mbi->DumpCachedReads();
        
        //
        // Release the lock
        //
        status = aaec->ReleaseExclusive();
        VERIFY_IS_TRUE(NT_SUCCESS(status)); 

        //
        // Close the MB
        //
        mbi->Reuse();
        mbi->StartCloseMetadataBlock(nullptr,
                                    sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    //
    // Cleanup the MB
    //
    mbi->Reuse();
    mbi->StartCleanupMetadataBlock(nullptr,
                                   sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

#if !PLATFORM_UNIX
    //
    // Delete the metadata file
    //
    KWString s(*g_Allocator);
    status = GenerateFileName3(s, DiskId, logContainerId.Get());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = KVolumeNamespace::DeleteFileOrDirectory(s, *g_Allocator, sync, NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif
}

VOID BasicMBInfoDataIntegrityTest(
    KGuid& DiskId
)
{
    // Data corruption test 1: Corrupt the latest copy of the MBInfoAccess
    // so that the previous copy is returned.
    //
    // Header fields to corrupt:
    //         
    //            ULONGLONG Signature;
    //            ULONGLONG FileOffset;
    //            ULONG SectionSize;
    //            ULONG Version;
    //            ULONGLONG BlockChecksum; (over checksumSize)
    //
    // Corrupt data within the section


    //
    // Test 1:  Create file with sections setup to read 6
    //          but corrupt Signature first to force read 5 from older
    //          block
    //
    {
        NTSTATUS status;
        const ULONG numberBlocks = 512;
        const ULONG blockSize = 0x2000;
        ULONG blockIndex = numberBlocks / 2;
        MBInfoAccess::SPtr mbi;
        MBInfoAccess::MBInfoHeader* mbInfoHeader;
        KGuid guid;
        KIoBuffer::SPtr ioBuffer;
        KIoBuffer::SPtr readBuffer;
        KSynchronizer sync;

        //
        // Create a brand new MB
        //
        guid.CreateNew();
        status = CreateMBInfoFile(mbi,
                                  DiskId,
                                  guid,
                                  numberBlocks,
                                  blockSize,
                                  7);          
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Since we overwrote an odd number of times the last write
        // went to CopyA. Therefore we want to corrupt CopyA
        //
        
        status = RawReadDataBlock(DiskId,
                                  guid,
                                  numberBlocks,
                                  blockSize,
                                  TRUE,            // read copy A
                                  blockIndex,
                                  ioBuffer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        mbInfoHeader = (MBInfoAccess::MBInfoHeader*)ioBuffer->First()->GetBuffer();

        //
        // Corrupt signature
        //
        mbInfoHeader->Signature = 0x1234;

        status = RawWriteDataBlock(DiskId,
                                   guid,
                                   numberBlocks,
                                   blockSize,
                                   TRUE,            // write copy A
                                   blockIndex,
                                   ioBuffer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Read back the section and verify the results
        //
        status = ReadDataBlock(mbi,
                               blockIndex,
                               blockSize,
                               readBuffer);

        mbInfoHeader = (MBInfoAccess::MBInfoHeader*)readBuffer->First()->GetBuffer();

        PUCHAR buffer = (PUCHAR)mbInfoHeader;
        for (ULONG i = sizeof(MBInfoAccess::MBInfoHeader); i < blockSize; i++)
        {
            VERIFY_IS_TRUE(buffer[i] == 5);
        }

        status = ReadDataBlock(mbi,
                               blockIndex+1,
                               blockSize,
                               readBuffer);

        mbInfoHeader = (MBInfoAccess::MBInfoHeader*)readBuffer->First()->GetBuffer();

        buffer = (PUCHAR)mbInfoHeader;
        for (ULONG i = sizeof(MBInfoAccess::MBInfoHeader); i < blockSize; i++)
        {
            VERIFY_IS_TRUE(buffer[i] == 6);
        }
        
        //
        // Cleanup the MB
        //
        mbi->Reuse();
        mbi->StartCleanupMetadataBlock(nullptr,
                                       sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

#if !PLATFORM_UNIX
        //
        // Delete the metadata file
        //
        KWString s(*g_Allocator);
        status = GenerateFileName(s, DiskId, guid);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = KVolumeNamespace::DeleteFileOrDirectory(s, *g_Allocator, sync, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif
    }

    //
    // Test 2:  Create file with sections setup to read 6
    //          but corrupt FileOffset first to force read 5 from older
    //          block
    //
    {
        NTSTATUS status;
        const ULONG numberBlocks = 512;
        const ULONG blockSize = 0x2000;
        ULONG blockIndex = numberBlocks / 2;
        MBInfoAccess::SPtr mbi;
        MBInfoAccess::MBInfoHeader* mbInfoHeader;
        KGuid guid;
        KIoBuffer::SPtr ioBuffer;
        KIoBuffer::SPtr readBuffer;
        KSynchronizer sync;

        //
        // Create a brand new MB
        //
        guid.CreateNew();
        status = CreateMBInfoFile(mbi,
                                  DiskId,
                                  guid,
                                  numberBlocks,
                                  blockSize,
                                  7);          
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Since we overwrote an odd number of times the last write
        // went to CopyA. Therefore we want to corrupt CopyA
        //
        
        status = RawReadDataBlock(DiskId,
                                  guid,
                                  numberBlocks,
                                  blockSize,
                                  TRUE,            // read copy A
                                  blockIndex,
                                  ioBuffer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        mbInfoHeader = (MBInfoAccess::MBInfoHeader*)ioBuffer->First()->GetBuffer();

        //
        // Corrupt FileOffset
        //
        mbInfoHeader->FileOffset = 0x1234;

        status = RawWriteDataBlock(DiskId,
                                   guid,
                                   numberBlocks,
                                   blockSize,
                                   TRUE,            // write copy A
                                   blockIndex,
                                   ioBuffer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Read back the section and verify the results
        //
        status = ReadDataBlock(mbi,
                               blockIndex,
                               blockSize,
                               readBuffer);

        mbInfoHeader = (MBInfoAccess::MBInfoHeader*)readBuffer->First()->GetBuffer();

        PUCHAR buffer = (PUCHAR)mbInfoHeader;
        for (ULONG i = sizeof(MBInfoAccess::MBInfoHeader); i < blockSize; i++)
        {
            VERIFY_IS_TRUE(buffer[i] == 5);
        }

        status = ReadDataBlock(mbi,
                               blockIndex+1,
                               blockSize,
                               readBuffer);

        mbInfoHeader = (MBInfoAccess::MBInfoHeader*)readBuffer->First()->GetBuffer();

        buffer = (PUCHAR)mbInfoHeader;
        for (ULONG i = sizeof(MBInfoAccess::MBInfoHeader); i < blockSize; i++)
        {
            VERIFY_IS_TRUE(buffer[i] == 6);
        }
        
        //
        // Cleanup the MB
        //
        mbi->Reuse();
        mbi->StartCleanupMetadataBlock(nullptr,
                                       sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

#if !PLATFORM_UNIX
        //
        // Delete the metadata file
        //
        KWString s(*g_Allocator);
        status = GenerateFileName(s, DiskId, guid);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = KVolumeNamespace::DeleteFileOrDirectory(s, *g_Allocator, sync, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif
    }


    //
    // Test 3:  Create file with sections setup to read 6
    //          but corrupt SectionSize first to force read 5 from older
    //          block
    //
    {
        NTSTATUS status;
        const ULONG numberBlocks = 512;
        const ULONG blockSize = 0x2000;
        ULONG blockIndex = numberBlocks / 2;
        MBInfoAccess::SPtr mbi;
        MBInfoAccess::MBInfoHeader* mbInfoHeader;
        KGuid guid;
        KIoBuffer::SPtr ioBuffer;
        KIoBuffer::SPtr readBuffer;
        KSynchronizer sync;

        //
        // Create a brand new MB
        //
        guid.CreateNew();
        status = CreateMBInfoFile(mbi,
                                  DiskId,
                                  guid,
                                  numberBlocks,
                                  blockSize,
                                  7);          
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Since we overwrote an odd number of times the last write
        // went to CopyA. Therefore we want to corrupt CopyA
        //
        
        status = RawReadDataBlock(DiskId,
                                  guid,
                                  numberBlocks,
                                  blockSize,
                                  TRUE,            // read copy A
                                  blockIndex,
                                  ioBuffer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        mbInfoHeader = (MBInfoAccess::MBInfoHeader*)ioBuffer->First()->GetBuffer();

        //
        // Corrupt FileOffset
        //
        mbInfoHeader->SectionSize = 0x1234;

        status = RawWriteDataBlock(DiskId,
                                   guid,
                                   numberBlocks,
                                   blockSize,
                                   TRUE,            // write copy A
                                   blockIndex,
                                   ioBuffer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Read back the section and verify the results
        //
        status = ReadDataBlock(mbi,
                               blockIndex,
                               blockSize,
                               readBuffer);

        mbInfoHeader = (MBInfoAccess::MBInfoHeader*)readBuffer->First()->GetBuffer();

        PUCHAR buffer = (PUCHAR)mbInfoHeader;
        for (ULONG i = sizeof(MBInfoAccess::MBInfoHeader); i < blockSize; i++)
        {
            VERIFY_IS_TRUE(buffer[i] == 5);
        }

        status = ReadDataBlock(mbi,
                               blockIndex+1,
                               blockSize,
                               readBuffer);

        mbInfoHeader = (MBInfoAccess::MBInfoHeader*)readBuffer->First()->GetBuffer();

        buffer = (PUCHAR)mbInfoHeader;
        for (ULONG i = sizeof(MBInfoAccess::MBInfoHeader); i < blockSize; i++)
        {
            VERIFY_IS_TRUE(buffer[i] == 6);
        }
        
        //
        // Cleanup the MB
        //
        mbi->Reuse();
        mbi->StartCleanupMetadataBlock(nullptr,
                                       sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

#if !PLATFORM_UNIX
        //
        // Delete the metadata file
        //
        KWString s(*g_Allocator);
        status = GenerateFileName(s, DiskId, guid);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = KVolumeNamespace::DeleteFileOrDirectory(s, *g_Allocator, sync, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif
    }
    
    //
    // Test 4:  Create file with sections setup to read 5
    //          but corrupt Version first to force read 4 from older
    //          block
    //
    {
        NTSTATUS status;
        const ULONG numberBlocks = 512;
        const ULONG blockSize = 0x2000;
        ULONG blockIndex = numberBlocks / 2;
        MBInfoAccess::SPtr mbi;
        MBInfoAccess::MBInfoHeader* mbInfoHeader;
        KGuid guid;
        KIoBuffer::SPtr ioBuffer;
        KIoBuffer::SPtr readBuffer;
        KSynchronizer sync;

        //
        // Create a brand new MB
        //
        guid.CreateNew();
        status = CreateMBInfoFile(mbi,
                                  DiskId,
                                  guid,
                                  numberBlocks,
                                  blockSize,
                                  6);          
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Since we overwrote an even number of times the last write
        // went to CopyB. Therefore we want to corrupt CopyB
        //
        
        status = RawReadDataBlock(DiskId,
                                  guid,
                                  numberBlocks,
                                  blockSize,
                                  FALSE,            // read copy B
                                  blockIndex,
                                  ioBuffer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        mbInfoHeader = (MBInfoAccess::MBInfoHeader*)ioBuffer->First()->GetBuffer();

        //
        // Corrupt Version
        //
        mbInfoHeader->Version = 0x1234;

        status = RawWriteDataBlock(DiskId,
                                   guid,
                                   numberBlocks,
                                   blockSize,
                                   FALSE,            // write copy B
                                   blockIndex,
                                   ioBuffer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Read back the section and verify the results
        //
        status = ReadDataBlock(mbi,
                               blockIndex,
                               blockSize,
                               readBuffer);

        mbInfoHeader = (MBInfoAccess::MBInfoHeader*)readBuffer->First()->GetBuffer();

        PUCHAR buffer = (PUCHAR)mbInfoHeader;
        for (ULONG i = sizeof(MBInfoAccess::MBInfoHeader); i < blockSize; i++)
        {
            VERIFY_IS_TRUE(buffer[i] == 4);
        }

        status = ReadDataBlock(mbi,
                               blockIndex+1,
                               blockSize,
                               readBuffer);

        mbInfoHeader = (MBInfoAccess::MBInfoHeader*)readBuffer->First()->GetBuffer();

        buffer = (PUCHAR)mbInfoHeader;
        for (ULONG i = sizeof(MBInfoAccess::MBInfoHeader); i < blockSize; i++)
        {
            VERIFY_IS_TRUE(buffer[i] == 5);
        }
        
        //
        // Cleanup the MB
        //
        mbi->Reuse();
        mbi->StartCleanupMetadataBlock(nullptr,
                                       sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

#if !PLATFORM_UNIX
        //
        // Delete the metadata file
        //
        KWString s(*g_Allocator);
        status = GenerateFileName(s, DiskId, guid);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = KVolumeNamespace::DeleteFileOrDirectory(s, *g_Allocator, sync, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif
    }
    
    //
    // Test 5:  Create file with sections setup to read 5
    //          but corrupt checksum first to force read 4 from older
    //          block
    //
    {
        NTSTATUS status;
        const ULONG numberBlocks = 512;
        const ULONG blockSize = 0x2000;
        ULONG blockIndex = numberBlocks / 2;
        MBInfoAccess::SPtr mbi;
        MBInfoAccess::MBInfoHeader* mbInfoHeader;
        KGuid guid;
        KIoBuffer::SPtr ioBuffer;
        KIoBuffer::SPtr readBuffer;
        KSynchronizer sync;

        //
        // Create a brand new MB
        //
        guid.CreateNew();
        status = CreateMBInfoFile(mbi,
                                  DiskId,
                                  guid,
                                  numberBlocks,
                                  blockSize,
                                  6);          
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Since we overwrote an even number of times the last write
        // went to CopyB. Therefore we want to corrupt CopyB
        //
        
        status = RawReadDataBlock(DiskId,
                                  guid,
                                  numberBlocks,
                                  blockSize,
                                  FALSE,            // read copy B
                                  blockIndex,
                                  ioBuffer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        mbInfoHeader = (MBInfoAccess::MBInfoHeader*)ioBuffer->First()->GetBuffer();

        //
        // Corrupt checksum
        //
        mbInfoHeader->BlockChecksum += 0x1234;

        status = RawWriteDataBlock(DiskId,
                                   guid,
                                   numberBlocks,
                                   blockSize,
                                   FALSE,            // write copy B
                                   blockIndex,
                                   ioBuffer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Read back the section and verify the results
        //
        status = ReadDataBlock(mbi,
                               blockIndex,
                               blockSize,
                               readBuffer);

        mbInfoHeader = (MBInfoAccess::MBInfoHeader*)readBuffer->First()->GetBuffer();

        PUCHAR buffer = (PUCHAR)mbInfoHeader;
        for (ULONG i = sizeof(MBInfoAccess::MBInfoHeader); i < blockSize; i++)
        {
            VERIFY_IS_TRUE(buffer[i] == 4);
        }

        status = ReadDataBlock(mbi,
                               blockIndex+1,
                               blockSize,
                               readBuffer);

        mbInfoHeader = (MBInfoAccess::MBInfoHeader*)readBuffer->First()->GetBuffer();

        buffer = (PUCHAR)mbInfoHeader;
        for (ULONG i = sizeof(MBInfoAccess::MBInfoHeader); i < blockSize; i++)
        {
            VERIFY_IS_TRUE(buffer[i] == 5);
        }
        
        //
        // Cleanup the MB
        //
        mbi->Reuse();
        mbi->StartCleanupMetadataBlock(nullptr,
                                       sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

#if !PLATFORM_UNIX
        //
        // Delete the metadata file
        //
        KWString s(*g_Allocator);
        status = GenerateFileName(s, DiskId, guid);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = KVolumeNamespace::DeleteFileOrDirectory(s, *g_Allocator, sync, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif
    }
    

    //
    // Test 6:  Create file with sections setup to read 5
    //          but corrupt data first to force read 4 from older
    //          block
    //
    {
        NTSTATUS status;
        const ULONG numberBlocks = 512;
        const ULONG blockSize = 0x2000;
        ULONG blockIndex = numberBlocks / 2;
        MBInfoAccess::SPtr mbi;
        MBInfoAccess::MBInfoHeader* mbInfoHeader;
        KGuid guid;
        KIoBuffer::SPtr ioBuffer;
        KIoBuffer::SPtr readBuffer;
        KSynchronizer sync;

        //
        // Create a brand new MB
        //
        guid.CreateNew();
        status = CreateMBInfoFile(mbi,
                                  DiskId,
                                  guid,
                                  numberBlocks,
                                  blockSize,
                                  6);          
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Since we overwrote an even number of times the last write
        // went to CopyB. Therefore we want to corrupt CopyB
        //
        
        status = RawReadDataBlock(DiskId,
                                  guid,
                                  numberBlocks,
                                  blockSize,
                                  FALSE,            // read copy B
                                  blockIndex,
                                  ioBuffer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        PUCHAR buf = (PUCHAR)ioBuffer->First()->GetBuffer();

        //
        // Corrupt data
        //
        buf[0x100] += 1;     // single bit error

        status = RawWriteDataBlock(DiskId,
                                   guid,
                                   numberBlocks,
                                   blockSize,
                                   FALSE,            // write copy B
                                   blockIndex,
                                   ioBuffer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Read back the section and verify the results
        //
        status = ReadDataBlock(mbi,
                               blockIndex,
                               blockSize,
                               readBuffer);

        mbInfoHeader = (MBInfoAccess::MBInfoHeader*)readBuffer->First()->GetBuffer();

        PUCHAR buffer = (PUCHAR)mbInfoHeader;
        for (ULONG i = sizeof(MBInfoAccess::MBInfoHeader); i < blockSize; i++)
        {
            VERIFY_IS_TRUE(buffer[i] == 4);
        }

        status = ReadDataBlock(mbi,
                               blockIndex+1,
                               blockSize,
                               readBuffer);

        mbInfoHeader = (MBInfoAccess::MBInfoHeader*)readBuffer->First()->GetBuffer();

        buffer = (PUCHAR)mbInfoHeader;
        for (ULONG i = sizeof(MBInfoAccess::MBInfoHeader); i < blockSize; i++)
        {
            VERIFY_IS_TRUE(buffer[i] == 5);
        }
        
        //
        // Cleanup the MB
        //
        mbi->Reuse();
        mbi->StartCleanupMetadataBlock(nullptr,
                                       sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

#if !PLATFORM_UNIX
        //
        // Delete the metadata file
        //
        KWString s(*g_Allocator);
        status = GenerateFileName(s, DiskId, guid);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = KVolumeNamespace::DeleteFileOrDirectory(s, *g_Allocator, sync, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif
    }    
}

VOID FailOpenCorruptedMBInfoTest(
    KGuid& DiskId
    )
{
    NTSTATUS status;    
    KWString fileName(*g_Allocator);
    KString::SPtr fileNameString;
    KGuid Guid;
    KtlLogContainerId logContainerId;
    KSynchronizer sync;
    MBInfoAccess::SPtr Mbi;
    ULONG NumberOfBlocks = 1024;
    ULONG BlockSize = 0x2000;
    ULONG NumberOfOverwrites = 2;

    //
    // This test creates a completely corrupted MBInfo block and then
    // attempts to open it. The open should fail.
    //
    // The corrupted MBInfo is created by performing writes to many
    // blocks at the same time using the same buffer. Since the buffer
    // contains the file offset, this will corrupt the block
    //

    Guid.CreateNew();
    logContainerId = Guid;
    
    status = GenerateFileName(fileName, DiskId, Guid);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    fileNameString = KString::Create((LPCWSTR)fileName,
                                     *g_Allocator);
    VERIFY_IS_TRUE(fileNameString ? TRUE : FALSE);
    
    status = MBInfoAccess::Create(Mbi,
                                  DiskId,
                                  fileNameString.RawPtr(),
                                  logContainerId,
                                  NumberOfBlocks,
                                  BlockSize,
                                  *g_Allocator,
                                  ALLOCATION_TAG);
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    //
    // Initialize new MB
    //
    Mbi->StartInitializeMetadataBlock(NULL,           // Initialization data array
                                      nullptr,
                                      sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    {
        const ULONG MAX_NUMBER_BLOCKS = 4096;
        KSynchronizer writeSync[MAX_NUMBER_BLOCKS];
        KIoBuffer::SPtr ioBuffer;
        PVOID buffer;

        VERIFY_IS_TRUE(NumberOfBlocks < MAX_NUMBER_BLOCKS);
        
        status = KIoBuffer::CreateSimple(BlockSize,
                                       ioBuffer,
                                       buffer,
                                       *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Open the MB
        //
        Mbi->Reuse();
        Mbi->StartOpenMetadataBlock(nullptr,
                                    sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status)); 
        
        //
        // Acquire an exclusive lock
        //
        MBInfoAccess::AsyncAcquireExclusiveContext::SPtr aaec;
        
        status = Mbi->CreateAsyncAcquireExclusiveContext(aaec);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        aaec->StartAcquireExclusive(nullptr,
                                    sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status)); 

        //
        // Perform writes to the sections
        //
        for (ULONG i = 0; i < NumberOfOverwrites; i++)
        {
            memset(buffer, (UCHAR)i, BlockSize);

            for (ULONG j = 0; j < NumberOfBlocks; j++)
            {
                MBInfoAccess::AsyncWriteSectionContext::SPtr write;
                
                status = Mbi->CreateAsyncWriteSectionContext(write);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                write->StartWriteSection(j * BlockSize,
                                         ioBuffer,
                                         nullptr,
                                         writeSync[j]);
            }

            for (ULONG j = 0; j < NumberOfBlocks; j++)
            {
                status = writeSync[j].WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));                 
            }
        }

        //
        // Release the lock
        //
        status = aaec->ReleaseExclusive();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }
    
    //
    // Close the MB
    //
    Mbi->Reuse();
    Mbi->StartCloseMetadataBlock(nullptr,
                                sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    //
    // Now the block is corrupted, re-open it
    //
    Mbi->Reuse();
    Mbi->StartOpenMetadataBlock(nullptr,
                                sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(status == K_STATUS_LOG_STRUCTURE_FAULT);

    //
    // Cleanup the MB
    //
    Mbi->Reuse();
    Mbi->StartCleanupMetadataBlock(nullptr,
                                   sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status)); 

#if !PLATFORM_UNIX
    //
    // Delete the metadata file
    //
    KWString s(*g_Allocator);
    status = GenerateFileName(s, DiskId, Guid);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = KVolumeNamespace::DeleteFileOrDirectory(s, *g_Allocator, sync, NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif
}


#define MAXLLMETADATASIZE  (16 * 0x1000)
#define MAXRECORDSIZE  (16 * 0x1000)
#define MAXSTREAMSIZE  (16 * 0x10000)
#define ENTRYFLAGS (SharedLCMBInfoAccess::DispositionFlags)(0xCC)

class SharedLCMBInfoTestClass : public KObject<SharedLCMBInfoTestClass>, public KShared<SharedLCMBInfoTestClass>
{
    K_FORCE_SHARED(SharedLCMBInfoTestClass);
    
    public:
        SharedLCMBInfoTestClass::SharedLCMBInfoTestClass(
            SharedLCMBInfoAccess::SPtr Slcmb,
            ULONG NumberOfEntries
            );
                
        NTSTATUS ExpectEmptyEnumCallback(__in SharedLCMBInfoAccess::LCEntryEnumAsync& Async);
        NTSTATUS ReturnErrorEnumCallback(__in SharedLCMBInfoAccess::LCEntryEnumAsync& Async);
        NTSTATUS VerifyEachEntryEnumCallback(__in SharedLCMBInfoAccess::LCEntryEnumAsync& Async);

        class TestEntry
        {
            public:
                TestEntry();
                ~TestEntry();

                KtlLogStreamId StreamId;
                KString::SPtr Path;
                KString::SPtr Alias;
                BOOLEAN Found;
                BOOLEAN IsFree;
                ULONG EntryIndex;
        };
        
        //
        // general members
        //
        SharedLCMBInfoAccess::SPtr _Slcmb;
        ULONG _CallbackCounter;
        KArray<TestEntry> _TestArray;
        ULONG _NumberOfEntries;
};

SharedLCMBInfoTestClass::TestEntry::TestEntry()
{
}

SharedLCMBInfoTestClass::TestEntry::~TestEntry()
{
}

SharedLCMBInfoTestClass::SharedLCMBInfoTestClass(
    SharedLCMBInfoAccess::SPtr Slcmb,
    ULONG NumberOfEntries
    ) : _Slcmb(Slcmb),
        _NumberOfEntries(NumberOfEntries),
        _TestArray(*g_Allocator)
{
}

SharedLCMBInfoTestClass::~SharedLCMBInfoTestClass()
{
}

NTSTATUS SharedLCMBInfoTestClass::ExpectEmptyEnumCallback(__in SharedLCMBInfoAccess::LCEntryEnumAsync& Async)
{
    SharedLCMBInfoAccess::SharedLCEntry& Entry = Async.GetEntry();
    GUID nullGUID = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };
    VERIFY_IS_TRUE(! IsEqualGUID(Entry.LogStreamId.Get(), nullGUID));
    
    #pragma warning(disable:4127)   // C4127: conditional expression is constant
    VERIFY_IS_TRUE(FALSE, L"Expected no entries but called back for one");
    return(STATUS_UNSUCCESSFUL);
}

NTSTATUS SharedLCMBInfoTestClass::ReturnErrorEnumCallback(__in SharedLCMBInfoAccess::LCEntryEnumAsync& Async)
{
    SharedLCMBInfoAccess::SharedLCEntry& Entry = Async.GetEntry();
    GUID nullGUID = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };
    VERIFY_IS_TRUE(! IsEqualGUID(Entry.LogStreamId.Get(), nullGUID));
    
    _CallbackCounter++;
    return(STATUS_INVALID_PARAMETER_MIX);
}

NTSTATUS SharedLCMBInfoTestClass::VerifyEachEntryEnumCallback(
    __in SharedLCMBInfoAccess::LCEntryEnumAsync& Async  
    )
{
    GUID nullGUID = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };
    SharedLCMBInfoAccess::SharedLCEntry& Entry = Async.GetEntry();
    
    VERIFY_IS_TRUE(! IsEqualGUID(Entry.LogStreamId.Get(), nullGUID));

    for (ULONG i = 0; i < _NumberOfEntries; i++)
    {
        LONG l;
        
        if (IsEqualGUID(Entry.LogStreamId.Get(), _TestArray[i].StreamId.Get()))
        {
            VERIFY_IS_TRUE(! _TestArray[i].IsFree);
            
            VERIFY_IS_TRUE(! _TestArray[i].Found);
            _TestArray[i].Found = TRUE;
            
            VERIFY_IS_TRUE(Entry.Flags == ENTRYFLAGS);
            
            VERIFY_IS_TRUE(Entry.MaxLLMetadataSize == MAXLLMETADATASIZE);
            
            VERIFY_IS_TRUE(_TestArray[i].EntryIndex == Entry.Index);
            
            if (_TestArray[i].Alias)
            {
                l = _TestArray[i].Alias->Compare(KStringView(Entry.AliasName));
                VERIFY_IS_TRUE(l == 0);
            } else {
                VERIFY_IS_TRUE(*Entry.AliasName == 0);
            }

            if (_TestArray[i].Path)
            {
                l = _TestArray[i].Path->Compare(KStringView(Entry.PathToDedicatedContainer));
                VERIFY_IS_TRUE(l == 0);
            } else {
                VERIFY_IS_TRUE(*Entry.PathToDedicatedContainer == 0);
            }           
        }
    }
    return(STATUS_SUCCESS);
}

NTSTATUS VerifyEachEntry(
    SharedLCMBInfoAccess::SPtr slcmb,
    SharedLCMBInfoTestClass::SPtr test,
    ULONG NumberOfEntries
)
{
    NTSTATUS status;
    KSynchronizer sync;

    for (ULONG i = 0; i < NumberOfEntries; i++)
    {
        test->_TestArray[i].Found = FALSE;
    }
    
    slcmb->Reuse();
    slcmb->StartCloseMetadataBlock(nullptr,
                                sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status)); 

    //
    // Reopen the MB and verify each entry
    //
    SharedLCMBInfoAccess::LCEntryEnumCallback verifyEachEntryEnumCallback(test.RawPtr(), &SharedLCMBInfoTestClass::VerifyEachEntryEnumCallback);
    slcmb->Reuse();
    slcmb->StartOpenMetadataBlock(verifyEachEntryEnumCallback,
                                  nullptr,
                                  sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    for (ULONG i = 0; i < NumberOfEntries; i++)
    {
        VERIFY_IS_TRUE( (test->_TestArray[i].Found) || (test->_TestArray[i].IsFree) );      
    }

    return(STATUS_SUCCESS);
}

VOID SharedLCMBInfoTest(
    KGuid& DiskId,
    ULONG NumberOfLoops,
    ULONG NumberOfEntries
)
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    KSynchronizer sync;
    SharedLCMBInfoAccess::SPtr slcmb;
    ULONG r;
    KString::SPtr alias;
    KString::SPtr path;
    KSynchronizer syncArr[16536];

    VERIFY_IS_TRUE((NumberOfEntries < 16536), L"Number of entries must be less than 16536");
    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);
    
    status = SharedLCMBInfoAccess::Create(slcmb,
                                          DiskId,
                                          nullptr,
                                          logContainerId,
                                          NumberOfEntries,
                                          *g_Allocator,
                                          ALLOCATION_TAG);
    VERIFY_IS_TRUE(NT_SUCCESS(status), L"Create SharedLCMBInfoAccess");

    ULONG_PTR foo = (ULONG_PTR)slcmb.RawPtr();
    ULONG foo1 = (ULONG)foo;
    srand(foo1);

    //
    // Initialize new MB and verify empty
    //
    SharedLCMBInfoTestClass::SPtr test = _new(ALLOCATION_TAG, *g_Allocator) SharedLCMBInfoTestClass(slcmb, NumberOfEntries);
    VERIFY_IS_TRUE((test != nullptr) ? TRUE : FALSE);
        
    for (ULONG loops = 0; loops < NumberOfLoops; loops++)
    {
        slcmb->Reuse();
        slcmb->StartInitializeMetadataBlock(nullptr,
                                            sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status)); 

        //
        // Test 1: Verify an new SLCMB has no entries
        //
        SharedLCMBInfoAccess::LCEntryEnumCallback expectEmptyEnumCallback(test.RawPtr(), &SharedLCMBInfoTestClass::ExpectEmptyEnumCallback);
        slcmb->Reuse();
        slcmb->StartOpenMetadataBlock(expectEmptyEnumCallback,
                                      nullptr,
                                      sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        slcmb->Reuse();
        slcmb->StartCloseMetadataBlock(nullptr,
                                       sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status)); 
        
        //
        // Open the MB - no callback
        //
        slcmb->Reuse();
        slcmb->StartOpenMetadataBlock(nullptr,
                                      nullptr,
                                      sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status)); 

        //
        // Write up to the maximum number of entries
        //
        BOOLEAN b;

        test->_TestArray.Clear();
        
        status = test->_TestArray.Reserve(NumberOfEntries);
        VERIFY_IS_TRUE(NT_SUCCESS(status)); 
        
        b = test->_TestArray.SetCount(NumberOfEntries);
        VERIFY_IS_TRUE(b ? TRUE : FALSE);
                
        for (ULONG i = 0; i < NumberOfEntries; i++)
        {
            KGuid id;
            KtlLogStreamId streamId;
            SharedLCMBInfoAccess::AsyncAddEntryContext::SPtr context;
            
            test->_TestArray[i].IsFree = FALSE;
            
            id.CreateNew();
            streamId = id;
            test->_TestArray[i].StreamId = streamId;

            r = rand() % 3;
            if (r != 1)
            {
                id.CreateNew();
                alias = KString::Create(*g_Allocator,
                                        KtlLogContainer::MaxAliasLength+1);
                VERIFY_IS_TRUE((alias != nullptr) ? TRUE : FALSE);
                b = alias->FromGUID(id);
                VERIFY_IS_TRUE(b ? TRUE : FALSE);
                b = alias->Concat(KStringView(L"_Alias"), TRUE);   // AppendNul
                VERIFY_IS_TRUE(b ? TRUE : FALSE);
            } else {
                alias = nullptr;
            }
            test->_TestArray[i].Alias = alias;

            r = rand() % 3;
            if (r != 1)
            {
                id.CreateNew();
                path = KString::Create(*g_Allocator,
                                       KtlLogManager::MaxPathnameLengthInChar);
                VERIFY_IS_TRUE((path != nullptr) ? TRUE : FALSE);
                b = path->FromGUID(id);
                VERIFY_IS_TRUE(b ? TRUE : FALSE);
                b = path->Concat(KStringView(L"_Path"), TRUE);   // AppendNul
                VERIFY_IS_TRUE(b ? TRUE : FALSE);
            } else {
                path = nullptr;
            }
            test->_TestArray[i].Path = path;
            
            status = slcmb->CreateAsyncAddEntryContext(context);
            VERIFY_IS_TRUE(NT_SUCCESS(status)); 

            context->StartAddEntry(streamId,
                                   MAXLLMETADATASIZE,            // MaxLLMetadataSize
                                   MAXRECORDSIZE,
                                   MAXSTREAMSIZE,
                                   ENTRYFLAGS,
                                   path.RawPtr(),
                                   alias.RawPtr(),
                                   &test->_TestArray[i].EntryIndex,
                                   nullptr,
                                   syncArr[i]);
            
        }

        for (ULONG i = 0; i < NumberOfEntries; i++)
        {
            status = syncArr[i].WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        //
        // Read back the entries one at a time and verify them
        //
        status = VerifyEachEntry(slcmb, test, NumberOfEntries);
        VERIFY_IS_TRUE(NT_SUCCESS(status)); 

                 
        //
        // After verifying the entries, update them somehow and then
        // reverify
        //
        for (ULONG i = 0; i < NumberOfEntries; i++)
        {
            KGuid id;
            SharedLCMBInfoAccess::AsyncUpdateEntryContext::SPtr context;
            
            r = rand() % 3;
            switch (r)
            {
                case 0:
                {
                    id.CreateNew();
                    alias = KString::Create(*g_Allocator,
                                            KtlLogContainer::MaxAliasLength+1);
                    VERIFY_IS_TRUE((alias != nullptr) ? TRUE : FALSE);
                    b = alias->FromGUID(id);
                    VERIFY_IS_TRUE(b ? TRUE : FALSE);
                    b = alias->Concat(KStringView(L"_2Alias"), TRUE);   // AppendNul
                    VERIFY_IS_TRUE(b ? TRUE : FALSE);
                    
                    test->_TestArray[i].Alias = nullptr;
                    test->_TestArray[i].Alias = alias;
                    break;
                }

                case 1:
                {
                    alias = KString::Create(KStringView(L""),
                                            *g_Allocator);
                    VERIFY_IS_TRUE((alias != nullptr) ? TRUE : FALSE);
                    test->_TestArray[i].Alias = nullptr;
                    break;
                }

                case 2:
                {
                    alias = nullptr;
                    break;
                }
            }            
            
            r = rand() % 3;
            switch (r)
            {
                case 0:
                {
                    id.CreateNew();
                    path = KString::Create(*g_Allocator,
                                           KtlLogManager::MaxPathnameLengthInChar);
                    VERIFY_IS_TRUE((path != nullptr) ? TRUE : FALSE);
                    b = path->FromGUID(id);
                    VERIFY_IS_TRUE(b ? TRUE : FALSE);
                    b = path->Concat(KStringView(L"_2Path"), TRUE);   // AppendNul
                    VERIFY_IS_TRUE(b ? TRUE : FALSE);
                    
                    test->_TestArray[i].Path = nullptr;
                    test->_TestArray[i].Path = path;
                    break;
                }

                case 1:
                {
                    path = KString::Create(KStringView(L""),
                                            *g_Allocator);
                    VERIFY_IS_TRUE((path != nullptr) ? TRUE : FALSE);
                    test->_TestArray[i].Path = nullptr;
                    break;
                }

                case 2:
                {
                    path = nullptr;
                    break;
                }
            }            
                        
            status = slcmb->CreateAsyncUpdateEntryContext(context);
            VERIFY_IS_TRUE(NT_SUCCESS(status)); 
                        
            context->StartUpdateEntry(test->_TestArray[i].StreamId,
                                      path.RawPtr(),
                                      alias.RawPtr(),
                                      ENTRYFLAGS,
                                      FALSE,    // RemoveEntry
                                      test->_TestArray[i].EntryIndex,
                                      nullptr,
                                      syncArr[i]);
            
        }

        for (ULONG i = 0; i < NumberOfEntries; i++)
        {
            status = syncArr[i].WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status)); 
        }
        
        //
        // Read back the entries one at a time and verify them
        //
        status = VerifyEachEntry(slcmb, test, NumberOfEntries);
        VERIFY_IS_TRUE(NT_SUCCESS(status)); 


        //
        // Remove about a third of the entries
        //
        for (ULONG i = 0; i < (NumberOfEntries/3); i++)
        {
            SharedLCMBInfoAccess::AsyncUpdateEntryContext::SPtr context;
            ULONG index;

            do
            {
                index = rand() % NumberOfEntries;
            } while (test->_TestArray[index].IsFree);

            test->_TestArray[index].IsFree = TRUE;
            
            status = slcmb->CreateAsyncUpdateEntryContext(context);
            VERIFY_IS_TRUE(NT_SUCCESS(status)); 
                        
            context->StartUpdateEntry(test->_TestArray[index].StreamId,
                                      NULL,
                                      NULL,
                                      ENTRYFLAGS,
                                      TRUE,    // RemoveEntry
                                      test->_TestArray[index].EntryIndex,
                                      nullptr,
                                      syncArr[i]);
        }
        
        for (ULONG i = 0; (i < NumberOfEntries/3); i++)
        {
            status = syncArr[i].WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status)); 
        }

        //
        // Read back the entries one at a time and verify them
        //
        status = VerifyEachEntry(slcmb, test, NumberOfEntries);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        slcmb->Reuse();
        slcmb->StartCloseMetadataBlock(nullptr,
                                    sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status)); 

        //
        // Test error return
        //
        SharedLCMBInfoAccess::LCEntryEnumCallback returnErrorEnumCallback(test.RawPtr(), &SharedLCMBInfoTestClass::ReturnErrorEnumCallback);
        test->_CallbackCounter = 0;
        slcmb->Reuse();
        slcmb->StartOpenMetadataBlock(returnErrorEnumCallback,
                                      nullptr,
                                      sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER_MIX); 

        //
        // Ensure we can reopen after an error
        //
        test->_CallbackCounter = 0;
        slcmb->Reuse();
        slcmb->StartOpenMetadataBlock(nullptr,
                                      nullptr,
                                      sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status)); 

        slcmb->Reuse();
        slcmb->StartCloseMetadataBlock(nullptr,
                                    sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status)); 
        
        //
        // Cleanup the MB
        //
        slcmb->Reuse();
        slcmb->StartCleanupMetadataBlock(nullptr,
                                       sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

#if !PLATFORM_UNIX
        //
        // Delete the metadata file
        //
        KWString s(*g_Allocator);
        status = GenerateFileName3(s, DiskId, logContainerId.Get());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = KVolumeNamespace::DeleteFileOrDirectory(s, *g_Allocator, sync, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif
    }
}

VOID DedicatedLCMBInfoTest(
    KGuid& DiskId,
    ULONG NumberOfLoops
)
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    KSynchronizer sync;
    ULONG r;

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);
    
    srand((ULONG)GetTickCount());

    //
    // Create containers of different sizes while reading and writing
    // metadata
    //  
    for (ULONG loops = 0; loops < NumberOfLoops; loops++)
    {
        DedicatedLCMBInfoAccess::SPtr dlcmb;
        ULONG maxLLMetadataBlocks;
        ULONG maxLLMetadataSize;
        KBuffer::SPtr securityDescriptor;
        DedicatedLCMBInfoAccess::AsyncWriteMetadataContext::SPtr writeContext;
        DedicatedLCMBInfoAccess::AsyncReadMetadataContext::SPtr readContext;
        
        //
        // Pick a random number of 4K blocks for the metadataSize up to
        // 4K blocks or 16MB total
        //
        maxLLMetadataBlocks = (rand() % (0x1000-1)) + 1;
        maxLLMetadataSize = maxLLMetadataBlocks * 0x1000;

        status = DedicatedLCMBInfoAccess::Create(dlcmb,
                                              DiskId,
                                              nullptr,
                                              logContainerId,
                                              maxLLMetadataSize,
                                              *g_Allocator,
                                              ALLOCATION_TAG);
        VERIFY_IS_TRUE(NT_SUCCESS(status), L"Create DedicatedLCMBInfoAccess");


        status = dlcmb->CreateAsyncWriteMetadataContext(writeContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        status = dlcmb->CreateAsyncReadMetadataContext(readContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        
        if (loops == 0)
        {
            r = DedicatedLCMBInfoAccess::_MaxSecurityDescriptorSize;
        } else {
            r = (rand() % (DedicatedLCMBInfoAccess::_MaxSecurityDescriptorSize-1)) + 1;
        }
        status = KBuffer::Create(r,
                                 securityDescriptor,
                                 *g_Allocator,
                                 ALLOCATION_TAG);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        PUCHAR p = (PUCHAR)securityDescriptor->GetBuffer();
        for (ULONG i = 0; i < r; i++)
        {
            p[i] = (UCHAR)((i * maxLLMetadataSize) % r);
        }
                                                    
        dlcmb->StartInitializeMetadataBlock(securityDescriptor,                                         
                                            nullptr,
                                            sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status)); 

        for (ULONG loops2 = 0; loops2 < NumberOfLoops; loops2++)
        {
            KIoBuffer::SPtr writeIoBuffer;
            KIoBuffer::SPtr readIoBuffer;
            
            //
            // Open up access to the metadata block
            //
            dlcmb->Reuse();
            dlcmb->StartOpenMetadataBlock(nullptr,
                                          sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));     

            if (loops2 == 0)
            {
                //
                // Read initial metadata - should be size 0
                //
                readContext->StartReadMetadata(readIoBuffer,
                                               nullptr,
                                               sync);
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status == STATUS_OBJECT_NAME_NOT_FOUND));

                readIoBuffer = nullptr;
                readContext->Reuse();

                //
                // Always write largest size of first loop2
                //
                r = maxLLMetadataBlocks;                
            } else {
                r = (rand() % (maxLLMetadataBlocks-1)) + 1;
            }

            if (loops2 == 0)
            {
            }
            
            if ((loops != 0) || (loops2 != 0))
            {
                //
                // Don't reuse on first time use
                //
                writeContext->Reuse();
                readContext->Reuse();
            }
            
            status = CreateAndFillIoBuffer((UCHAR)(loops * loops2),
                                           r * 0x1000,
                                           writeIoBuffer);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // write metadata
            //
            writeContext->StartWriteMetadata(writeIoBuffer,
                                             nullptr,
                                             sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));


            //
            // read it back
            //
            readContext->StartReadMetadata(readIoBuffer,
                                           nullptr,
                                           sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // Compare the blocks
            //
            status = CompareKIoBuffers(*readIoBuffer,
                                       *writeIoBuffer,
                                       0);             // header to skip
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            
            dlcmb->Reuse();
            dlcmb->StartCloseMetadataBlock(nullptr,
                                           sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        //
        // Cleanup the MB
        //
        dlcmb->Reuse();
        dlcmb->StartCleanupMetadataBlock(nullptr,
                                       sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
#if !PLATFORM_UNIX
        //
        // Delete the metadata file
        //
        KWString s(*g_Allocator);
        status = GenerateFileName3(s, DiskId, logContainerId.Get());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = KVolumeNamespace::DeleteFileOrDirectory(s, *g_Allocator, sync, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif
    }
}

VOID SetupMBInfoTests(
    KGuid& DiskId,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    )
{       
    NTSTATUS status;
    KServiceSynchronizer        sync;
    KSynchronizer sync1;

    status = KtlSystem::Initialize(FALSE,     // Do not enable VNetwork, we don't need it
                                   &System);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    System->SetStrictAllocationChecks(TRUE);

    StartingAllocs = KAllocatorSupport::gs_AllocsRemaining;

    EventRegisterMicrosoft_Windows_KTL();

#if !defined(PLATFORM_UNIX)
    UCHAR driveLetter = 0;
    WCHAR driveLetterW[2];
    status = FindDiskIdForDriveLetter(driveLetter, DiskId);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif
    
#if defined(UDRIVER)
    //
    // For UDRIVER, need to perform work done in PNP Device Add
    //
    status = FileObjectTable::CreateAndRegisterOverlayManager(*g_Allocator, ALLOCATION_TAG);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif
    
    DeleteAllContainersOnDisk(DiskId);

    KWString s(*g_Allocator);

    //
    // Create \MBInfoTest and \RvdLog directories as they are needed for the tests
    //
#if !defined(PLATFORM_UNIX)
    driveLetterW[0] = driveLetter;
    driveLetterW[1] = 0;
    s = L"\\??\\";
    s += KStringView(driveLetterW);
    s += L":";
#else
    s = L"";
#endif
    s += KVolumeNamespace::PathSeparator;
    s += L"MBInfoTest";
    
    status = KVolumeNamespace::CreateDirectory(s, *g_Allocator, sync1, NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync1.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status) || (status == STATUS_OBJECT_NAME_COLLISION));

#if !defined(PLATFORM_UNIX)
    driveLetterW[0] = driveLetter;
    driveLetterW[1] = 0;
    s = L"\\??\\";
    s += KStringView(driveLetterW);
    s += L":";
#else
    s = L"";
#endif
    s += KVolumeNamespace::PathSeparator;
    s += L"RvdLog";
    
    status = KVolumeNamespace::CreateDirectory(s, *g_Allocator, sync1, NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync1.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status) || (status == STATUS_OBJECT_NAME_COLLISION));

}

VOID CleanupMBInfoTests(
    KGuid& DiskId,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    )
{
    UNREFERENCED_PARAMETER(DiskId);
    UNREFERENCED_PARAMETER(System);
    UNREFERENCED_PARAMETER(StartingAllocs);

#if defined(UDRIVER)
    //
    // For UDRIVER need to perform work done in PNP RemoveDevice
    //
    NTSTATUS status;
    
    status = FileObjectTable::StopAndUnregisterOverlayManager(*g_Allocator);
    KInvariant(NT_SUCCESS(status));
#endif
    
    KtlSystem::Shutdown();
    
    EventUnregisterMicrosoft_Windows_KTL();

    KtlSystem::Shutdown();
}

#endif
