/*++
Copyright (c) Microsoft Corporation
Module Name: KtlMemoryMapTests.cpp

Abstract:
This file contains test case implementations for memory map file related APIs in KNt.
--*/

#include "kntmemorymaptests.h"
#include "KtlCommonTests.h"

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#include "KTrace.h"
#endif

NTSTATUS
WriteTest(__in void* baseAddress, __in const int numberOfItems)
{
    // Write in to the memory mapped view
    unsigned char* currentWriteAddress = static_cast<unsigned char *>(baseAddress);
    for (int i = 0; i < numberOfItems; i++)
    {
        void* sourceBytes = static_cast<void *>(&i);
        KMemCpySafe(currentWriteAddress, sizeof(int), sourceBytes, sizeof(int));

        // Move current position.
        currentWriteAddress += sizeof(int);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
ReadTest(__in const void* baseAddress, __in const int numberOfItems)
{
    // Read the memory mapped view
    const unsigned char* currentReadAddress = static_cast<const unsigned char *>(baseAddress);
    for (int i = 0; i < numberOfItems; i++)
    {
        int value = 0;
        void* destBytes = static_cast<void *>(&value);
        KMemCpySafe(destBytes, sizeof(int), currentReadAddress, sizeof(int));

        // Verify the read value.
        KInvariant(value == i);

        // Move current position.
        currentReadAddress += sizeof(int);
    }

    KTestPrintf("All {0} items read successfully.\n", numberOfItems);
    return STATUS_SUCCESS;
}

NTSTATUS
Open(__in KWString& filePath, __out HANDLE& fileHandle, __out HANDLE& sectorHandle, __out void*& mapViewBaseAddress, __in const unsigned long fileSize)
{
    NTSTATUS status;

    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatus;
    LARGE_INTEGER size;
    size.QuadPart = fileSize;
    InitializeObjectAttributes(&objectAttributes, filePath, OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = KNt::CreateFile(
        &fileHandle,
        GENERIC_ALL | SYNCHRONIZE | FILE_ANY_ACCESS,
        &objectAttributes,
        &ioStatus,
        &size,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN_IF,
        FILE_NON_DIRECTORY_FILE | FILE_WRITE_THROUGH | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0);
    if (!NT_SUCCESS(status))
    {
        Close(fileHandle, sectorHandle, mapViewBaseAddress);
        return status;
    }

    InitializeObjectAttributes(&objectAttributes, NULL, OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = KNt::CreateSection(
        sectorHandle,
        SECTION_MAP_READ | SECTION_MAP_WRITE,
        &objectAttributes,
        &size,
        PAGE_READWRITE,
        SEC_COMMIT,
        fileHandle);
    if (!NT_SUCCESS(status))
    {
        Close(fileHandle, sectorHandle, mapViewBaseAddress);
        return status;
    }

    SIZE_T viewSize = fileSize;
    LARGE_INTEGER sectionOffset = { 0, 0 };
    status = KNt::MapViewOfSection(
        sectorHandle,
        mapViewBaseAddress,
        0,
        0,
        &sectionOffset,
        viewSize,
        ViewUnmap,
        MEM_RESERVE,
        PAGE_READWRITE);
    if (!NT_SUCCESS(status))
    {
        Close(fileHandle, sectorHandle, mapViewBaseAddress);
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
Close(__in const HANDLE fileHandle, __in const HANDLE sectionHandle, __in void* mapViewBaseAddress)
{
    if (mapViewBaseAddress != nullptr)
    {
        NTSTATUS status = KNt::UnmapViewOfSection(mapViewBaseAddress);
        KInvariant(NT_SUCCESS(status));
    }

    if (sectionHandle != nullptr)
    {
        NTSTATUS status = KNt::Close(sectionHandle);
        KInvariant(NT_SUCCESS(status));
    }

    if (fileHandle != nullptr)
    {
        NTSTATUS status = KNt::Close(fileHandle);
        KInvariant(NT_SUCCESS(status));
    }

    return STATUS_SUCCESS;
}

/// Owner: MCoskun
/// Summary: Ensures that all memory map related APIs in the KNt are functional.
/// Tested APIs
/// CreateFile, CreateSector, MapViewOfSection, FlushVirtualMemory, FlushBuffersFile, UnmapViewOfSection, Close, DeleteFile
NTSTATUS
MemoryMap_AllMemoryMapApis_EnsureMemoryMapCanBeCreatedUsedRecoveredAndDeleted(__in KWString& testDirectory)
{
    NTSTATUS status;

    KWString filePath = testDirectory;
    
    filePath += L"\\knttestfile.nlog";
    HANDLE fileHandle;
    HANDLE sectorHandle;
    void* mapViewBaseAddress = nullptr;
    const int numberOfItems = 256;
    const unsigned long fileSize = numberOfItems * sizeof(int);

    status = Open(filePath, fileHandle, sectorHandle, mapViewBaseAddress, fileSize);
    KInvariant(NT_SUCCESS(status));

    status = WriteTest(mapViewBaseAddress, numberOfItems);
    KInvariant(NT_SUCCESS(status));

    IO_STATUS_BLOCK ioStatusBlock;
    SIZE_T regionSize = fileSize;
    status = KNt::FlushVirtualMemory(mapViewBaseAddress, regionSize, ioStatusBlock);
    KInvariant(NT_SUCCESS(status));

    status = KNt::FlushBuffersFile(fileHandle, &ioStatusBlock);
    KInvariant(NT_SUCCESS(status));

    status = ReadTest(mapViewBaseAddress, numberOfItems);
    KInvariant(NT_SUCCESS(status));

    status = Close(fileHandle, sectorHandle, mapViewBaseAddress);
    KInvariant(NT_SUCCESS(status));

    status = Open(filePath, fileHandle, sectorHandle, mapViewBaseAddress, fileSize);
    KInvariant(NT_SUCCESS(status));

    status = ReadTest(mapViewBaseAddress, numberOfItems);
    KInvariant(NT_SUCCESS(status));

    status = Close(fileHandle, sectorHandle, mapViewBaseAddress);
    KInvariant(NT_SUCCESS(status));

    status = KNt::DeleteFile(filePath);
    KInvariant(NT_SUCCESS(status));

    return STATUS_SUCCESS;
}

NTSTATUS
TestSequence(__in KWString& testDirectory)
{
    NTSTATUS status = MemoryMap_AllMemoryMapApis_EnsureMemoryMapCanBeCreatedUsedRecoveredAndDeleted(testDirectory);
    KInvariant(NT_SUCCESS(status));

    return STATUS_SUCCESS;
}

NTSTATUS
CreateTestDirectory(__out KWString& testFolderPath)
{
    NTSTATUS status;
    KSynchronizer compSync;

    KWString dirName(*g_Allocator, L"\\??\\D:");
    status = KVolumeNamespace::QueryGlobalVolumeDirectoryName(dirName, *g_Allocator, testFolderPath, compSync);
    KInvariant(NT_SUCCESS(status));
    status = compSync.WaitForCompletion();
    if(status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        KWString dirName1(*g_Allocator, L"\\??\\C:");
        status = KVolumeNamespace::QueryGlobalVolumeDirectoryName(dirName1, *g_Allocator, testFolderPath, compSync);
        KInvariant(NT_SUCCESS(status));
        status = compSync.WaitForCompletion();

        // Out of luck for today. TODO: go through all volumes.        
        KInvariant(NT_SUCCESS(status));
    }
    else
    {
        KInvariant(NT_SUCCESS(status));
    }

    testFolderPath += L"\\KtlCITs";
    status = KVolumeNamespace::CreateDirectory(testFolderPath, *g_Allocator, compSync);
    KInvariant(NT_SUCCESS(status));
    status = compSync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status) || (status == STATUS_OBJECT_NAME_COLLISION));

    return STATUS_SUCCESS;
}

NTSTATUS
DeleteTestDirectory(__in KWString& testFolderPath)
{
    NTSTATUS status;
    KSynchronizer compSync;

    status = KVolumeNamespace::DeleteFileOrDirectory(testFolderPath, *g_Allocator, compSync);
    KInvariant(NT_SUCCESS(status));
    status = compSync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));

    return STATUS_SUCCESS;
}

NTSTATUS
KNtMemoryMapTest(
    __in int argc,
    __in_ecount(argc) WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    KtlSystem* system;
    NTSTATUS Result;
    Result = KtlSystem::Initialize(&system);
    KInvariant(NT_SUCCESS(Result));
    KFinally([&]() { KtlSystem::Shutdown(); });

    system->SetStrictAllocationChecks(TRUE);
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();
    ULONGLONG startingAllocs = KAllocatorSupport::gs_AllocsRemaining;
    
    {
        KWString testFolderPath = KWString(*g_Allocator);
        Result = CreateTestDirectory(testFolderPath);
        KInvariant(NT_SUCCESS(Result));
        KFinally([&]() { DeleteTestDirectory(testFolderPath); });

        NTSTATUS testResult = TestSequence(testFolderPath);
        KInvariant(NT_SUCCESS(testResult));
        KTestPrintf("No errors\n");
    }

    KNt::Sleep(1000);
    ULONGLONG Leaks = KAllocatorSupport::gs_AllocsRemaining - startingAllocs;
    if (Leaks)
    {
        KTestPrintf("Leaks = %u\n", Leaks);
        KInvariant(false);
    }

    KTestPrintf("No leaks.\n");

    return Result;
}