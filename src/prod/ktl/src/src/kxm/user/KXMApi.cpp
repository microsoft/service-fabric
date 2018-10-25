/*++

Copyright (c) Microsoft Corporation

Module Name:

    KXMApi.cpp

Abstract:

    Implementation of user space APIs to acces KXM driver.

--*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <palio.h>
#include <palerr.h>
#include "KXMApi.h"
#include <ktl.h>
#include <ktrace.h>

#include <palhandle.h>

#include "KXMUKApi.h"

#define Message(a, b...) //printf("[%s@%d] "a"\n", __FUNCTION__, __LINE__, ##b)

static const char *KXMDeviceName = "/dev/sfkxm";

//////////////////////////////////////
//////////////////////////////////////
////////////KXM Dev APIs/////////////
//////////////////////////////////////
//////////////////////////////////////
HANDLE KXMOpenDev(void)
{
    int devFD;
    devFD = open(KXMDeviceName, O_RDWR|O_CLOEXEC);
    if(devFD < 0)
        return KXM_USER_EMULATION_FD;
    return (HANDLE)(long)devFD; 
}

NTSTATUS KXMCloseDev(HANDLE DevFD)
{
    int ret;
    
    if(DevFD == KXM_USER_EMULATION_FD)
        return STATUS_SUCCESS;

    ret = close((int)(long)DevFD);
    if(ret != 0)
        return LinuxError::LinuxErrorToNTStatus(errno);
    return STATUS_SUCCESS;
}


//////////////////////////////////////
//////////////////////////////////////
////////CompletionPort APIs///////////
//////////////////////////////////////
//////////////////////////////////////

HANDLE KXMCreateIoCompletionPort(HANDLE DevFD, HANDLE FileHandle, 
                        HANDLE CompHandle, ULONG_PTR CompletionKey, 
                        DWORD MaxConcurrentThreads)
{
   KXMFileHandle::SPtr handle;
   
   if (FileHandle == INVALID_HANDLE_VALUE)
    {
        handle = nullptr;
    } else {    
        handle = ResolveHandle<KXMFileHandle>(FileHandle);
    }
    
    if ((handle) && ((handle->UseFileSystemOnly()) || DevFD == KXM_USER_EMULATION_FD))
    {
        if (CompHandle != INVALID_HANDLE_VALUE)
        {
            //
            // If the file handle being registered with the completion port
            // is for user mode access then keep track of the completion
            // port handle and completion port key so that the IO
            // completions can use them.
            //
            // This is not the first handle being registered on the
            // completion port and so we can capture it now.
            //
            BOOLEAN isAlreadySet;

            isAlreadySet = handle->SetCompletionPort(CompHandle);
            if (isAlreadySet)
            {
                SetLastError(ERROR_ALREADY_ASSIGNED);
                return(NULL);
            }

            handle->SetCompletionKey(CompletionKey);
            handle->SetMaxConcurrentThreads(MaxConcurrentThreads);

            return(CompHandle);
        } else {
            //
            // Do not expect to register a file handle and create
            // completion port at the same time.
            //
            KInvariant(FALSE);
        }
        
    }
        
    if (DevFD == KXM_USER_EMULATION_FD)
    {
        CompHandle = CreateIoCompletionPortU(FileHandle, CompHandle,
                                               CompletionKey, MaxConcurrentThreads);
    } else {    
        CompHandle = CreateIoCompletionPortK(DevFD,
                                             handle ? handle->GetKxmHandle() : INVALID_HANDLE_VALUE,
                                             CompHandle,
                                             CompletionKey,
                                             MaxConcurrentThreads);
    }

    if (CompHandle != NULL)
    {
        if (handle)
        {
            //
            // If the file handle being registered with the completion port
            // is for user mode access then keep track of the completion
            // port handle and completion port key so that the IO
            // completions can use them.
            //
            BOOLEAN isAlreadySet;

            isAlreadySet = handle->SetCompletionPort(CompHandle);
            if (isAlreadySet)
            {
                SetLastError(ERROR_ALREADY_ASSIGNED);
                return(NULL);
            }

            handle->SetCompletionKey(CompletionKey);
        }
    }
    return(CompHandle);
}


BOOL KXMGetQueuedCompletionStatus(HANDLE DevFD, HANDLE CompHandle, 
                        LPDWORD DataXfred,
                        PULONG_PTR CompletionKey, 
                        LPOVERLAPPED *Poverlapped, DWORD MilliSecondsTimeout)
{
    if(DevFD == KXM_USER_EMULATION_FD)
        return GetQueuedCompletionStatusU(CompHandle, DataXfred, 
                    CompletionKey, Poverlapped, MilliSecondsTimeout);

    return GetQueuedCompletionStatusK(DevFD, CompHandle, DataXfred, 
                    CompletionKey, Poverlapped, MilliSecondsTimeout);
}

BOOL KXMPostQueuedCompletionStatus(HANDLE DevFD, HANDLE CompHandle, 
                        DWORD DataXfred, ULONG_PTR CompletionKey, 
                        LPOVERLAPPED Overlapped)
{
    if(DevFD == KXM_USER_EMULATION_FD)
        return PostQueuedCompletionStatusU(CompHandle, 
                        DataXfred, CompletionKey, Overlapped);

    return PostQueuedCompletionStatusK(DevFD, CompHandle, 
                DataXfred, CompletionKey, Overlapped);
}

BOOL KXMRegisterCompletionThread(HANDLE DevFD, HANDLE CompHandle)
{
    if(DevFD == KXM_USER_EMULATION_FD)
        return RegisterCompletionThreadU(CompHandle);

    return RegisterCompletionThreadK(DevFD, CompHandle);
}

BOOL KXMUnregisterCompletionThread(HANDLE DevFD, HANDLE CompHandle)
{
    if(DevFD == KXM_USER_EMULATION_FD)
        return UnregisterCompletionThreadU(CompHandle);

    return UnregisterCompletionThreadK(DevFD, CompHandle);
}

BOOL KXMCloseIoCompletionPort(HANDLE DevFD, HANDLE CompHandle)
{
    if(DevFD == KXM_USER_EMULATION_FD)
        return CloseIoCompletionPortU(CompHandle);

    return CloseIoCompletionPortK(DevFD, CompHandle);
}


//////////////////////////////////////
//////////////////////////////////////
////////////KXM File APIs/////////////
//////////////////////////////////////
//////////////////////////////////////

KXMFileHandle::KXMFileHandle()
{
    _IsOpen = 0;
    _UseFileSystemOnly = FALSE;
    _KxmHandle = NULL;
    _CompletionPort = NULL;
    _CompletionKey = 0;
}

KXMFileHandle::~KXMFileHandle()
{
}


KXMFileHandle::SPtr KXMFileHandle::Create()
{
    return _new(KXM_FILEHANDLE_TAG, KtlSystemCoreImp::TryGetDefaultKtlSystemCore()->PagedAllocator()) KXMFileHandle();
}

BOOL KXMFileHandle::Close()
{
    LONG wasOpen;

    wasOpen = InterlockedCompareExchange(&_IsOpen, 0, 1);
    if (wasOpen == 0)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    //
    // Release refcount taken when Handle was created. Note that this
    // will not be valid after the call to Release().
    //
    Release();
    
    return TRUE;
}

VOID KXMGetCompletionPortInfo(
    HANDLE FileHandle,
    HANDLE& CompletionPort,
    ULONG_PTR& CompletionKey,
    ULONG& MaxConcurrentThreads
    )
{
    KXMFileHandle::SPtr handle;

    handle = ResolveHandle<KXMFileHandle>(FileHandle);  
    CompletionPort = handle->GetCompletionPort();
    CompletionKey = handle->GetCompletionKey();
    MaxConcurrentThreads = handle->GetMaxConcurrentThreads();
}

HANDLE KXMCreateFile(
    HANDLE DevFD,
    int FileFD,
    const char *FileName,
    unsigned long *NumCacheEntries,
    BOOLEAN IsEventFile,
    FileMode Mode,
    pid_t RemotePID,
    ULONG& Flags
    )
{
    BOOLEAN useFileSystemOnly;
    KXMFileHandle::SPtr handle;
    HANDLE kxmHandle;

    useFileSystemOnly = ((Flags & KXMFLAG_USE_FILESYSTEM_ONLY) != 0);
    handle = KXMFileHandle::Create();
    if (! handle)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(INVALID_HANDLE_VALUE);
    }
    
    if (useFileSystemOnly) 
    {
		//
		// Configure to use file system only
		//
        handle->SetUseFileSystemOnly();
        kxmHandle = KXMCreateFileUFS(FileFD, IsEventFile, Mode, RemotePID, Flags);        
    } else if (DevFD == KXM_USER_EMULATION_FD) {
		//
		// First try to access via direct read/write
		//
        kxmHandle = KXMCreateFileUDW(FileName, IsEventFile, Mode, RemotePID, Flags);
		
		if (kxmHandle == INVALID_HANDLE_VALUE)
		{
			//
			// If direct read/write failed, it could be because the
			// caller has no access to the disk block device. Fall back
			// and try to use file system only
			//
			useFileSystemOnly = TRUE;
			handle->SetUseFileSystemOnly();
			kxmHandle = KXMCreateFileUFS(FileFD, IsEventFile, Mode, RemotePID, Flags);
			Flags |= KXMFLAG_USE_FILESYSTEM_ONLY;
		}
    } else {
        kxmHandle = KXMCreateFileK(DevFD, FileName, NumCacheEntries, IsEventFile, Mode, RemotePID, Flags);
    }

    if (kxmHandle != INVALID_HANDLE_VALUE)
    {
        handle->SetKxmHandle(kxmHandle);
        handle->SetOpen();

        //
        // Ref is released when the handle is closed in KXMCloseFile
        //
        handle->AddRef();
    } else {
        return(INVALID_HANDLE_VALUE);
    }

    return(handle.RawPtr());
}

NTSTATUS KXMReadFileAsync(HANDLE DevFD, HANDLE FileHandle, void *UserAddress,
                        int NumPages, LPOVERLAPPED Overlapped)
{
    NTSTATUS status;
    KXMFileHandle::SPtr handle;

    handle = ResolveHandle<KXMFileHandle>(FileHandle);
    
    if ((handle->UseFileSystemOnly()) || (DevFD == KXM_USER_EMULATION_FD))
    {
        status = KXMReadFileAsyncU(DevFD, *handle, UserAddress, NumPages, Overlapped);
    } else {
        status = KXMReadFileAsyncK(DevFD, handle->GetKxmHandle(), UserAddress, NumPages, Overlapped);
    }

    return(status);
}

NTSTATUS KXMWriteFileAsync(HANDLE DevFD, HANDLE FileHandle, void *UserAddress,
                        int NumPages, LPOVERLAPPED Overlapped)
{
    NTSTATUS status;
    KXMFileHandle::SPtr handle;

    handle = ResolveHandle<KXMFileHandle>(FileHandle);  
    
    if ((handle->UseFileSystemOnly()) || (DevFD == KXM_USER_EMULATION_FD))
    {
        status = KXMWriteFileAsyncU(DevFD, *handle, UserAddress, NumPages, Overlapped);
    } else {
        status = KXMWriteFileAsyncK(DevFD, handle->GetKxmHandle(), UserAddress, NumPages, Overlapped);
    }

    return(status);
}

NTSTATUS KXMReadFileScatterAsync(HANDLE DevFD, HANDLE FileHandle, 
                        KXMFileOffset Frag[], int NumEntries, 
                        LPOVERLAPPED Overlapped)
{
    NTSTATUS status;
    KXMFileHandle::SPtr handle;

    handle = ResolveHandle<KXMFileHandle>(FileHandle);  
    
    if ((handle->UseFileSystemOnly()) || (DevFD == KXM_USER_EMULATION_FD))
    {
        status = KXMReadFileScatterAsyncU(DevFD, *handle, Frag, NumEntries, Overlapped);
    } else {
        status = KXMReadFileScatterAsyncK(DevFD, handle->GetKxmHandle(), Frag, NumEntries, Overlapped);
    }

    return(status);
}

NTSTATUS KXMWriteFileGatherAsync(HANDLE DevFD, HANDLE FileHandle, 
                        KXMFileOffset Frag[], int NumEntries, 
                        LPOVERLAPPED Overlapped)
{
    NTSTATUS status;
    KXMFileHandle::SPtr handle;

    handle = ResolveHandle<KXMFileHandle>(FileHandle);  
    
    if ((handle->UseFileSystemOnly()) || (DevFD == KXM_USER_EMULATION_FD))
    {
        status = KXMWriteFileGatherAsyncU(DevFD, *handle, Frag, NumEntries, Overlapped);
    } else {
        status = KXMWriteFileGatherAsyncK(DevFD, handle->GetKxmHandle(), Frag, NumEntries, Overlapped);
    }

    return(status);
}

NTSTATUS KXMFlushBuffersFile(
    HANDLE DevFD,                            
    HANDLE FileHandle
)
{
    NTSTATUS status;
    KXMFileHandle::SPtr handle;

    handle = ResolveHandle<KXMFileHandle>(FileHandle);  
    
    if ((handle->UseFileSystemOnly()) || (DevFD == KXM_USER_EMULATION_FD))
    {
        status = KXMFlushBuffersFileU(DevFD, *handle);
    } else {
        status = KXMFlushBuffersFileK(DevFD, handle->GetKxmHandle());
    }

    return(status);
}

NTSTATUS KXMCloseFile(HANDLE DevFD, HANDLE FileHandle)
{
    NTSTATUS status;
    KXMFileHandle::SPtr handle;

    handle = ResolveHandle<KXMFileHandle>(FileHandle);  
    
    if ((handle->UseFileSystemOnly()) || (DevFD == KXM_USER_EMULATION_FD))
    {
        status = KXMCloseFileU(DevFD, *handle);
    } else {
        status = KXMCloseFileK(DevFD, handle->GetKxmHandle());
    }

	handle->Close();
    
    return(status);
}

