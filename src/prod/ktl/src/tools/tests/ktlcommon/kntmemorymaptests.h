/*++
Copyright (c) Microsoft Corporation
Module Name: KtlMemoryMapTests.h
--*/

#pragma once

#include "ktl.h"

// Declare and define globals. 
KAllocator* g_Allocator = nullptr;

// Function declarations.
NTSTATUS Open(__in KWString& filePath, __out HANDLE& fileHandle, __out HANDLE& sectorHandle, __out void*& mapViewBaseAddress, __in const unsigned long fileSize);
NTSTATUS Close(__in const HANDLE fileHandle, __in const HANDLE sectionHandle, __in void* mapViewBaseAddress);

NTSTATUS CreateTestDirectory(__out KWString& testFolderPath);
NTSTATUS DeleteTestDirectory(__in KWString& testFolderPath);

NTSTATUS TestSequence(__in KWString& testDirectory);
NTSTATUS MemoryMap_AllMemoryMapApis_EnsureMemoryMapCanBeCreatedUsedRecoveredAndDeleted(__in KWString& testDirectory);

NTSTATUS WriteTest(__in void* baseAddress, __in const int numberOfItems);
NTSTATUS ReadTest(__in const void* baseAddress, __in const int numberOfItems);