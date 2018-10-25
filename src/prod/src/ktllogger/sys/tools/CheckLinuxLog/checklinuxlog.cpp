// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <string>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>

#include "Common/Common.h"

#include "../../inc/ktllogger.h"
#include "InternalKtlLogger.h"
#include "KtlLogShimKernel.h"
#include "../../inc/ValidateLinuxLog.h"
#include <windows.h>

#if defined(PLATFORM_UNIX)          
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <errno.h>
#include <linux/fiemap.h>
#include <linux/unistd.h>
#include <sys/ioctl.h>
#endif

typedef enum
{
    Unknown = 0,
    FS = 1,
    BLK = 2
} ACLMODE;

#define VERIFY_IS_TRUE(cond, msg) if (! (cond)) { printf("Failure %s in %s at line %d\n",  msg, __FILE__, __LINE__); ExitProcess(static_cast<UINT>(-1)); }

#define CONVERT_TO_ARGS(argc, cargs) \
    std::vector<WCHAR*> args_vec(argc);\
    WCHAR** args = (WCHAR**)args_vec.data();\
    std::vector<std::wstring> wargs(argc);\
    for (int iter = 0; iter < argc; iter++)\
    {\
        wargs[iter] = Utf8To16(cargs[iter]);\
        args[iter] = (WCHAR*)(wargs[iter].data());\
    }\

KAllocator* g_Allocator;


const LONGLONG DefaultTestLogFileSize = (LONGLONG)1024 * 1024 * 1024;
const LONGLONG DEFAULT_STREAM_SIZE = (LONGLONG)256 * 1024 * 1024;
const ULONG DEFAULT_MAX_RECORD_SIZE = (ULONG)16 * 1024 * 1024;

VOID SetupRawCoreLoggerTests(
    )
{
    NTSTATUS status;
    KtlSystem* System;

    status = KtlSystem::Initialize(FALSE,     // Do not enable VNetwork, we don't need it
                                   &System);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "KtlSystemInitialize");
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    KDbgMirrorToDebugger = TRUE;

    System->SetStrictAllocationChecks(TRUE);

#ifdef UDRIVER
    //
    // For UDRIVER, need to perform work done in PNP Device Add
    //
    status = FileObjectTable::CreateAndRegisterOverlayManager(*g_Allocator, KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateAndRegister");
#endif

}

VOID CleanupRawCoreLoggerTests(
    )
{
    //
    // For UDRIVER need to perform work done in PNP RemoveDevice
    //
#ifdef UDRIVER
    NTSTATUS status;
    status = FileObjectTable::StopAndUnregisterOverlayManager(*g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Stopand Unregister");
#endif
    
    EventUnregisterMicrosoft_Windows_KTL();

    KtlSystem::Shutdown();
}

NTSTATUS CheckFile(
    __in PWCHAR LogFileName
    )
{
    NTSTATUS status;
    KSynchronizer sync;
        AsyncValidateLinuxLogForDDContext::SPtr validateLog;

    status = AsyncValidateLinuxLogForDDContext::Create(validateLog, *g_Allocator, KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateAsyncValidateLinuxLogForDDContext");

    
    validateLog->StartValidate(LogFileName,
                               nullptr,
                               sync);
	
    status = sync.WaitForCompletion();
    
    return(status);
}

NTSTATUS FixFile(
    __in LPCWSTR LogFile,
    __in ACLMODE AclMode
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    KBlockFile::SPtr inputFile;
    KBlockFile::SPtr outputFile;
    KWString inFileName(*g_Allocator);
    KWString outFileName(*g_Allocator);
    KWString backupFileName(*g_Allocator);
    ULONGLONG fileOffset;
    ULONGLONG fileSize;
    static const ULONG loopSize = 1024 * 1024;
    static const ULONGLONG loopSizeU = 1024 * 1024;
    ULONGLONG loopsU;
    ULONG loops;
    ULONGLONG lastLoopSizeU;
    ULONG lastLoopSize;
    KIoBuffer::SPtr fileBuffer;
    PVOID p;
    

    inFileName = LogFile;
    outFileName = LogFile;
    outFileName += L".Copied";
    backupFileName = LogFile;
    backupFileName += L".Backup";

    //
    // In file is opened in the mode specified by AclMode and writes
    // using the file system.
    //

    ULONG createOptions =  static_cast<ULONG>(KBlockFile::CreateOptions::eShareRead);

    if (AclMode == ACLMODE::FS)
    {
        createOptions |= static_cast<ULONG>(KBlockFile::CreateOptions::eAlwaysUseFileSystem);
    } else {
        createOptions |= static_cast<ULONG>(KBlockFile::CreateOptions::eAvoidUseFileSystem);
    }
    
    status = KBlockFile::Create(inFileName,
                                FALSE,
                                KBlockFile::eOpenExisting,
                                static_cast<KBlockFile::CreateOptions>(createOptions),
                                inputFile,
                                sync,
                                NULL,
                                *g_Allocator,
                                KTL_TAG_TEST);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }
    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    fileSize = inputFile->QueryFileSize();
    loopsU = fileSize / loopSizeU;
    lastLoopSizeU = fileSize - (loopsU * loopSizeU);
    loops = (ULONG)loopsU;
    lastLoopSize = (ULONG)lastLoopSizeU;

    createOptions = static_cast<ULONG>(KBlockFile::CreateOptions::eAlwaysUseFileSystem);
    status = KBlockFile::Create(outFileName,
                                FALSE,
                                KBlockFile::eCreateAlways,
                                static_cast<KBlockFile::CreateOptions>(createOptions),
                                outputFile,
                                sync,
                                NULL,
                                *g_Allocator,
                                KTL_TAG_TEST);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }
    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    status = outputFile->SetFileSize(fileSize, sync, NULL);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }
    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }
            
    status = KIoBuffer::CreateSimple(loopSize, fileBuffer, p, *g_Allocator, KTL_TAG_TEST);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    fileOffset = 0;
    for (ULONG i = 0; i < loops; i++)
    {
        status = inputFile->Transfer(KBlockFile::eForeground,
                         KBlockFile::eRead,
                         fileOffset,
                         *fileBuffer,
                         sync,
                         NULL);
        
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, 0, 0);
            return(status);
        }
        status = sync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, 0, 0);
            return(status);
        }

        status = outputFile->Transfer(KBlockFile::eForeground,
                         KBlockFile::eWrite,
                         fileOffset,
                         *fileBuffer,
                         sync,
                         NULL);
        
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, 0, 0);
            return(status);
        }
        status = sync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, 0, 0);
            return(status);
        }

        fileOffset += loopSize;
    }

    if (lastLoopSize != 0)
    {
        status = inputFile->Transfer(KBlockFile::eForeground,
                         KBlockFile::eRead,
                         fileOffset,
                         (PVOID)fileBuffer->First()->GetBuffer(),
                         lastLoopSize,
                         sync,
                         NULL);
        
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, 0, 0);
            return(status);
        }
        status = sync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, 0, 0);
            return(status);
        }

        status = outputFile->Transfer(KBlockFile::eForeground,
                         KBlockFile::eWrite,
                         fileOffset,
                         (PVOID)fileBuffer->First()->GetBuffer(),
                         lastLoopSize,
                         sync,
                         NULL);
        
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, 0, 0);
            return(status);
        }
        status = sync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, 0, 0);
            return(status);
        }               
    }

    status = outputFile->Flush(sync, NULL);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }
    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    inputFile->Close();
    outputFile->Close();
        
    //
    // Rename the files
    //
    status = KVolumeNamespace::RenameFile(inFileName,
                                          backupFileName,
                                          TRUE,         // Overwrite
                                          *g_Allocator,
                                          sync,
                                          NULL);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }
    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    status = KVolumeNamespace::RenameFile(outFileName,
                                          inFileName,
                                          FALSE,         // Overwrite
                                          *g_Allocator,
                                          sync,
                                          NULL);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }
    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    return(STATUS_SUCCESS);
}

void Usage()
{
    printf("Checks the contents of a KTL core log container\n");
    printf("\n");
    printf("    Usage: CheckLinuxLog -l:<filename of log container> \n");
    printf("        -l specifies the filename that holds the log container\n");
    printf("        -b specifies the current mode for the acl is set or BLK\n");
    printf("        -f specifies the current mode for the acl is not set or FS\n");
    printf("        -r specifies that the tool should recondition the file if not correct\n");

#if !defined(PLATFORM_UNIX)
    printf("    Note: filename must be a fully qualifed path, ie, c:\\wf_n.b.chk\\WinFabricTest\\MCFQueue-CIT.test.Data\\ReplicatorShared.Log\n");
#endif
}

typedef struct
{
    PWCHAR LogFileName;
    ACLMODE AclMode;
    BOOLEAN Recondition;
} COMMAND_OPTIONS, *PCOMMAND_OPTIONS;

ULONG ParseCommandLine(__in int argc, __in wchar_t** args, __out PCOMMAND_OPTIONS Options)
{
    Options->LogFileName = NULL;
    Options->AclMode = ACLMODE::Unknown;
    Options->Recondition = FALSE;

    WCHAR flag;
    size_t len;
    PWCHAR arg;

    for (int i = 1; i < argc; i++)
    {
        arg = args[i];

        len = wcslen(arg);
        if (len < 3)
        {
            Usage();
            return(ERROR_INVALID_PARAMETER);
        }

        if ((arg[0] != L'-') || (arg[2] != L':'))
        {
            Usage();
            return(ERROR_INVALID_PARAMETER);
        }

        flag = (WCHAR)tolower(arg[1]);

        switch (flag)
        {
            case L'l':
            {
                Options->LogFileName = arg + 3;
                break;
            }

            case L'f':
            {
                Options->AclMode = ACLMODE::FS;
                break;
            }

            case L'b':
            {
                Options->AclMode = ACLMODE::BLK;
                break;
            }

            case L'r':
            {
                Options->Recondition = TRUE;
                break;
            }

            default:
            {
                Usage();
                return(ERROR_INVALID_PARAMETER);
            }
        }
    }

    return(ERROR_SUCCESS);
}

int
TheMain(__in int argc, __in wchar_t** args)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    ULONG error;
    COMMAND_OPTIONS options;

    SetupRawCoreLoggerTests();
    KFinally([&] { CleanupRawCoreLoggerTests(); });
    
    
    
    KString::SPtr s = KString::Create(*g_Allocator);

    if (! s)
    {
        KTraceFailedAsyncRequest(STATUS_INSUFFICIENT_RESOURCES, nullptr, 0, 0);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    error = ParseCommandLine(argc, args, &options);
    if (error != ERROR_SUCCESS)
    {
        KTraceFailedAsyncRequest(STATUS_INVALID_PARAMETER, nullptr, 0, 0);
        return(error);
    }

    if ((options.LogFileName == NULL) || (options.AclMode == ACLMODE::Unknown))
    {
        Usage();
        KTraceFailedAsyncRequest(STATUS_INVALID_PARAMETER, nullptr, 0, 0);
        return(ERROR_INVALID_PARAMETER);
    }

    std::wstring traceFileName = options.LogFileName;
    traceFileName += L"-CheckLinuxLog.trace";
    Common::TraceTextFileSink::SetPath(traceFileName);

    //
    // Enable KTL tracing into the SF file log
    //
    RegisterKtlTraceCallback(Common::ServiceFabricKtlTraceCallback);
    
    
#if !defined(PLATFORM_UNIX)
    KDbgPrintfInformational("CheckLinuxLog processing file %ws from %s mode\n",
                            options.LogFileName,
                            options.AclMode == ACLMODE::FS ? "File System" : "Block Device");
#else
    KDbgPrintfInformational("CheckLinuxLog processing file %s from %s mode\n",
                            Utf16To8(options.LogFileName).c_str(),
                            options.AclMode == ACLMODE::FS ? "File System" : "Block Device");
#endif
    
    status = CheckFile(options.LogFileName);

    if (NT_SUCCESS(status))
    {
#if !defined(PLATFORM_UNIX)
        printf("    %ws is in good form\n", options.LogFileName);
        KDbgPrintfInformational("CheckLinuxLog %ws is in good form\n", options.LogFileName);
#else
        printf("    %s is in good form\n", Utf16To8(options.LogFileName).c_str());
        KDbgPrintfInformational("CheckLinuxLog %s is in good form", Utf16To8(options.LogFileName).c_str());
#endif
    } else {
        if (status == K_STATUS_LOG_DATA_FILE_FAULT)
        {
            if (options.Recondition)
            {
#if !defined(PLATFORM_UNIX)
                printf("    Fixing %ws for %s\n", options.LogFileName, options.AclMode == ACLMODE::FS ? "File System" : "Block Device");
#else
                printf("    Fixing %s for %s\n", Utf16To8(options.LogFileName).c_str(), options.AclMode == ACLMODE::FS ? "File System" : "Block Device");
#endif
                status = FixFile(options.LogFileName, options.AclMode);
                if (NT_SUCCESS(status))
                {
#if !defined(PLATFORM_UNIX)
                    KDbgPrintfInformational("CheckLinuxLog successfully fixed file %ws", options.LogFileName);
#else
                    KDbgPrintfInformational("CheckLinuxLog successfully fixed file %s", Utf16To8(options.LogFileName).c_str());
                    printf("CheckLinuxLog successfully fixed file %s\n", Utf16To8(options.LogFileName).c_str());
#endif
                } else {
#if !defined(PLATFORM_UNIX)
                    KDbgPrintfInformational("CheckLinuxLog failed fixing file %ws %x", options.LogFileName, status);
#else
                    KDbgPrintfInformational("CheckLinuxLog failed fixing file %s %x", Utf16To8(options.LogFileName).c_str(), status);
                    printf("CheckLinuxLog failed fixing file %s  %x\n", Utf16To8(options.LogFileName).c_str(), status);
#endif
				}
            } else {
#if !defined(PLATFORM_UNIX)
                printf("    %ws needs reconditioning, use -r\n", options.LogFileName);
                KDbgPrintfInformational("%ws needs reconditioning, use -r", options.LogFileName);
#else
                printf("    %s needs reconditioning, use -r\n", Utf16To8(options.LogFileName).c_str());
                KDbgPrintfInformational("%s needs reconditioning, use -r", Utf16To8(options.LogFileName).c_str());
#endif              
            }
        } else {
#if !defined(PLATFORM_UNIX)
            printf("    %ws had failed %x", options.LogFileName, status);
#else
            printf("    %s had failed %x", Utf16To8(options.LogFileName).c_str(), status);
#endif          
        }
    }

	int ret = 0;
	if (! NT_SUCCESS(status))
	{
		ret = -1;
	}
	printf("ret %d\n", ret);
    return(ret);
}

#if !defined(PLATFORM_UNIX)
int
wmain(int argc, WCHAR* args[])
{
    return TheMain(argc, args);
}
#else
#include <vector>
int main(int argc, char* cargs[])
{
    CONVERT_TO_ARGS(argc, cargs);
    return TheMain(argc, args); 
}
#endif
