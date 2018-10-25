/*++

Copyright (c) Microsoft Corporation

Module Name:

    KXMApi.cpp

Abstract:

    Implementation of user space APIs to acces KXM driver.

--*/

#include <ktl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "KXMApi.h"
#include <palio.h>
#include <palerr.h>
#include <ktrace.h>

#include <palhandle.h>
#include "KXMUKApi.h"

#define Message(a, b...) //printf("[%s@%d] "a"\n", __FUNCTION__, __LINE__, ##b)

//////////////////////////////////////
//////////////////////////////////////
////////CompletionPort APIs///////////
//////////////////////////////////////
//////////////////////////////////////

//Kernel Emulation.
static NTSTATUS SetupFileCompletionPort(HANDLE DevFD, HANDLE BlockHandle,
                        HANDLE CompHandle, unsigned long long CompletionKey)
{
    NTSTATUS status;
    KXMFileAssociateCmd setup;
    KXMAssociationCommand cmd;
    int ret;

    cmd.Handle = (int)(long)BlockHandle;
    cmd.CPortHandle = (int)(long)CompHandle;
    cmd.Args = (void *)&setup;

    //Set CompletionKey. This would be returned along with all completed events
    setup.CompletionKey = CompletionKey;

    ret = ioctl((int)(long)DevFD, KXM_ASSOCIATE_FILE_WITH_COMP_PORT, &cmd);
    if(ret < 0) {
        status = LinuxError::LinuxErrorToNTStatus(errno);
        KTraceFailedAsyncRequest(status, NULL, errno, (ULONGLONG)BlockHandle);
        return status;
    }
    return cmd.Status;
}

HANDLE CreateIoCompletionPortK(HANDLE DevFD, HANDLE FileHandle, 
                        HANDLE CompHandle, ULONG_PTR CompletionKey, 
                        DWORD MaxConcurrentThreads)
{
    KXMOpenCommand cmd;
    KXMCompletionPortOpen cport;
    int ret;
   
    if (FileHandle == INVALID_HANDLE_VALUE && CompHandle != NULL)
    {
        KTraceFailedAsyncRequest(STATUS_INVALID_HANDLE, NULL, EBADF, (ULONGLONG)FileHandle);
        SetLastError(LinuxError::LinuxErrorToWinError(EBADF));
        return NULL;
    }

    if(CompHandle == NULL)
    {
        memset(&cmd, 0, sizeof(KXMOpenCommand));
        cmd.Type = CompletionPort;
        cport.MaxConcurrentThreads = MaxConcurrentThreads;

        cmd.Args = (void *)&cport;
        ret = ioctl((int)(long)DevFD, KXM_CREATE_DESCRIPTOR, &cmd);

        if(ret<0)
        {
            KTraceFailedAsyncRequest(LinuxError::LinuxErrorToNTStatus(errno), NULL, errno, (ULONGLONG)FileHandle);
            SetLastError(LinuxError::LinuxErrorToWinError(errno));
            return NULL;
        }
        else if(cmd.Status != STATUS_SUCCESS)
        {
            KTraceFailedAsyncRequest(cmd.Status, NULL, errno, (ULONGLONG)FileHandle);
            SetLastError(WinError::NTStatusToWinError(cmd.Status));
            return NULL;
        }
    }

    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        NTSTATUS status;

        status  = SetupFileCompletionPort(DevFD, FileHandle,
                        CompHandle == NULL ? (HANDLE)(long)cmd.Handle : CompHandle,
                        CompletionKey);
        if(status != STATUS_SUCCESS)
        {
            //Close completion port if it was opened in this call.
            if(CompHandle == NULL)
            {
                KXMCloseIoCompletionPort(DevFD, (HANDLE)cmd.Handle);  // TODO: Check for errors ?
            }
            KTraceFailedAsyncRequest(status, NULL, errno, (ULONGLONG)FileHandle);
            SetLastError(WinError::NTStatusToWinError(status));
            return NULL;
        }
    }
    return CompHandle == NULL ? (HANDLE)(long)cmd.Handle : CompHandle;
}


BOOL GetQueuedCompletionStatusK(HANDLE DevFD, HANDLE CompHandle, 
                        LPDWORD DataXfred,
                        PULONG_PTR CompletionKey, 
                        LPOVERLAPPED *Poverlapped, DWORD MilliSecondsTimeout)
{
    KXMExecuteCommand cmd;
    KXMCompletionPortExecute cport;
    int ret;
    LPOVERLAPPED overlap = NULL;

    //Generic command structure
    cmd.Handle = (int)(long)CompHandle;
    cmd.Args = (void *)&cport;

    //CompletionPort specific command
    cport.Command = GetCompletedEvent;
    cport.MilliSecondsTimeout = MilliSecondsTimeout;

    *Poverlapped = NULL;

    ret = ioctl((int)(long)DevFD, KXM_EXECUTE_COMMAND, &cmd);
    if(ret < 0)
    {
        KTraceFailedAsyncRequest(LinuxError::LinuxErrorToNTStatus(errno), NULL, errno, (ULONGLONG)CompHandle);
        SetLastError(LinuxError::LinuxErrorToWinError(errno));
        return false;
    }

    //IOCTL has succeeded, check cmd.status to see if any event was dequeued or
    //not.
    
    if(cmd.Status == STATUS_SUCCESS)
    {
        //Get the pointer to overlap of completed request. 
        overlap = cport.CompletionEvent.Overlapped;
        if(overlap)
        {
            //Copy status from completion event to overlap.
            overlap->Internal = cport.CompletionEvent.Status;
            if(overlap->Internal != STATUS_SUCCESS)
                overlap->InternalHigh = 0;
            else
                overlap->InternalHigh = cport.CompletionEvent.DataXfred;
        }

        //Update the overlap.
        *Poverlapped = overlap;
        //Update number of bytes xfred.
        *DataXfred = cport.CompletionEvent.DataXfred;
        //Update completion key.
        *CompletionKey = cport.CompletionEvent.CompletionKey;

        if(cport.CompletionEvent.Status == STATUS_SUCCESS)
        {
            //Operation completed successfully.
            return true;
        }
        else
        {
            KTraceFailedAsyncRequest(LinuxError::LinuxErrorToNTStatus(EIO), NULL, EIO, (ULONGLONG)CompHandle);
            SetLastError(LinuxError::LinuxErrorToWinError(EIO));
            return false;
        }
    } else {
        if (! NT_SUCCESS(cmd.Status))
        {
            KTraceFailedAsyncRequest(cmd.Status, NULL, errno, (ULONGLONG)CompHandle);
        }
        SetLastError(WinError::NTStatusToWinError(cmd.Status));
        return false;
    }

    return false;    
}

BOOL PostQueuedCompletionStatusK(HANDLE DevFD, HANDLE CompHandle, 
                        DWORD DataXfred, ULONG_PTR CompletionKey, 
                        LPOVERLAPPED Overlapped)
{
    KXMExecuteCommand cmd;
    KXMCompletionPortExecute cport;
    int ret;
    KXMCompletionEvent compEvent;

    cmd.Handle = (int)(long)CompHandle;
    cmd.Args = (void *)&cport;

    cport.Command = PostCompletionEvent;

    //Setup KXMCompletionEvent fort this request.
    cport.CompletionEvent.Status = Overlapped ? Overlapped->Internal : STATUS_SUCCESS;
    cport.CompletionEvent.CompletionKey = CompletionKey;

    //
    // Note that Overlapped is a pointer to memory in the application
    // and opaque to completion port apis.
    //
    cport.CompletionEvent.Overlapped = Overlapped;
    cport.CompletionEvent.DataXfred = DataXfred;

    //Update InternalHigh with the number of bytes xfred.
    if(Overlapped)
        Overlapped->InternalHigh = DataXfred;

    ret = ioctl((int)(long)DevFD, KXM_EXECUTE_COMMAND, &cmd);
    if(ret < 0)
    {
        KTraceFailedAsyncRequest(LinuxError::LinuxErrorToNTStatus(errno), NULL, errno, (ULONGLONG)CompHandle);
        SetLastError(LinuxError::LinuxErrorToWinError(errno));
        return false;
    }

    //Check if we could successfully post the event.
    if(cmd.Status == STATUS_SUCCESS)
    {
        return true;
    }
    else
    { 
        KTraceFailedAsyncRequest(cmd.Status, NULL, errno, (ULONGLONG)CompHandle);
        SetLastError(WinError::NTStatusToWinError(cmd.Status));
        return false; 
    }
}


BOOL RegisterCompletionThreadK(HANDLE DevFD, HANDLE CompHandle)
{
    KXMExecuteCommand cmd;
    KXMCompletionPortExecute cport;
    int ret;

    cmd.Handle = (int)(long)CompHandle;
    cmd.Args = (void *)&cport;

    cport.Command = RegisterThread;

    ret = ioctl((int)(long)DevFD, KXM_EXECUTE_COMMAND, &cmd);
    if(ret < 0)
    {
        KTraceFailedAsyncRequest(LinuxError::LinuxErrorToNTStatus(errno), NULL, errno, (ULONGLONG)CompHandle);
        SetLastError(LinuxError::LinuxErrorToWinError(errno));
        return false;
    }

    if(cmd.Status == STATUS_SUCCESS)
    {
        return true;
    }
    else
    {    
        KTraceFailedAsyncRequest(cmd.Status, NULL, errno, (ULONGLONG)CompHandle);
        SetLastError(WinError::NTStatusToWinError(cmd.Status));
        return false;
    }
}

BOOL UnregisterCompletionThreadK(HANDLE DevFD, HANDLE CompHandle)
{
    KXMExecuteCommand cmd;
    KXMCompletionPortExecute cport;
    int ret;

    cmd.Handle = (int)(long)CompHandle;
    cmd.Args = (void *)&cport;

    cport.Command = UnregisterThread;

    ret = ioctl((int)(long)DevFD, KXM_EXECUTE_COMMAND, &cmd);
    if(ret < 0)
    {
        KTraceFailedAsyncRequest(LinuxError::LinuxErrorToNTStatus(errno), NULL, errno, (ULONGLONG)CompHandle);
        SetLastError(LinuxError::LinuxErrorToWinError(errno));
        return false;
    }

    if(cmd.Status == STATUS_SUCCESS)
    {
        return true;
    }
    else
    {    
        KTraceFailedAsyncRequest(cmd.Status, NULL, errno, (ULONGLONG)CompHandle);
        SetLastError(WinError::NTStatusToWinError(cmd.Status));
        return false;
    }
}

BOOL CloseIoCompletionPortK(HANDLE DevFD, HANDLE CompHandle)
{
    KXMCloseCommand cmd;
    KXMCompletionPortClose cport;
    int ret;

    cmd.Handle = (int)(long)CompHandle; 
    //Unused.
    cmd.Args = &cport;

    ret = ioctl((int)(long)DevFD, KXM_CLOSE_DESCRIPTOR, &cmd);
    if(ret < 0)
    {
        KTraceFailedAsyncRequest(LinuxError::LinuxErrorToNTStatus(errno), NULL, errno, (ULONGLONG)CompHandle);
        SetLastError(LinuxError::LinuxErrorToWinError(errno));
        return false;
    }

    if(cmd.Status == STATUS_SUCCESS)
    {
        return true;
    }
    else
    {    
        KTraceFailedAsyncRequest(cmd.Status, NULL, errno, (ULONGLONG)CompHandle);
        SetLastError(WinError::NTStatusToWinError(cmd.Status));
        return false;
    }
}



//////////////////////////////////////
//////////////////////////////////////
////////////KXM File APIs/////////////
//////////////////////////////////////
//////////////////////////////////////

// Kernel

HANDLE KXMCreateFileK(HANDLE DevFD, const char *FileName,
                            unsigned long *NumCacheEntries, BOOLEAN IsEventFile, FileMode Mode,
                            pid_t RemotePID, ULONG Flags)
{
    KXMOpenCommand cmd;
    KXMFileOpen openargs;
    int ret;
    int i = 0;

    if(strnlen(FileName, MAX_FILE_NAME_LEN) >= MAX_FILE_NAME_LEN)
    {
        KTraceFailedAsyncRequest(STATUS_INVALID_PARAMETER, NULL, MAX_FILE_NAME_LEN, (ULONGLONG)0);
        return INVALID_HANDLE_VALUE;
    }

    memset(&cmd, 0, sizeof(KXMOpenCommand));
    cmd.Handle = -1;  //KXM will update this with handle. 
    cmd.Args = (void *)&openargs;
    openargs.Mode = Mode;
    if(! IsEventFile)
    {
        //Setup BlockFile parameter.
        if(Mode != BLOCK_FLUSH && Mode != BLOCK_NOFLUSH)
        {
            KTraceFailedAsyncRequest(STATUS_INVALID_PARAMETER, NULL, Mode, (ULONGLONG)0);
            SetLastError(ERROR_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
        }
        cmd.Type = BlockFile;

        strncpy((char *)openargs.FileName, FileName, sizeof(openargs.FileName));

        for(i=0; i<NUM_CACHE_LIST_BUCKETS; i++)
        {
            if(NumCacheEntries)
                openargs.NumCacheEntries[i] = NumCacheEntries[i];
            else
                openargs.NumCacheEntries[i] = 0;
        }
    }
    else 
    {
        //Setup Eventfile parameter.
        if(Mode != EVENT_PRODUCER && Mode != EVENT_CONSUMER)
        {
            KTraceFailedAsyncRequest(STATUS_INVALID_PARAMETER, NULL, Mode, (ULONGLONG)0);
            SetLastError(ERROR_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
        }

        strncpy((char *)openargs.FileName, FileName, sizeof(openargs.FileName));

        openargs.RemotePID = (int)RemotePID;
        cmd.Type = EventFile;
    }

    //Open BlockFile descriptor.
    ret = ioctl((int)(long)DevFD, KXM_CREATE_DESCRIPTOR, &cmd);
    if (ret < 0)
    {
        KTraceFailedAsyncRequest(LinuxError::LinuxErrorToNTStatus(errno), NULL, errno, (ULONGLONG)0);
        SetLastError(LinuxError::LinuxErrorToWinError(errno));
        return INVALID_HANDLE_VALUE;
    }

    if (cmd.Status != STATUS_SUCCESS)
    {
        KTraceFailedAsyncRequest(cmd.Status, NULL, errno, (ULONGLONG)0);
        SetLastError(WinError::NTStatusToWinError(cmd.Status));
        return INVALID_HANDLE_VALUE;        
    }

    return (HANDLE)(long)cmd.Handle;
}

NTSTATUS KXMReadFileAsyncK(HANDLE DevFD, HANDLE FileHandle, void *UserAddress,
                        int NumPages, LPOVERLAPPED Overlapped)
{
    KXMExecuteCommand cmd;
    KXMFileExecute execargs;
    int ret;
    KXMFileOffset iov;

    if(!Overlapped)
        return STATUS_INVALID_PARAMETER;

    cmd.Handle = (int)(long)FileHandle;
    cmd.Args = (void *)&execargs;

    //Setup UserAddress, NumPages and Offset information.
    execargs.Command = ReadKXMFile;
    execargs.Iov = &iov;
    execargs.Iov[0].PhysicalAddress = (((unsigned long long)Overlapped->OffsetHigh)<<32) | 
                        ((unsigned long long)Overlapped->Offset & 0xffffffffU);
    execargs.Iov[0].UserAddress = (unsigned long long)UserAddress;
    execargs.Iov[0].NumPages = NumPages;
    execargs.NumIov = 1;

    //This value is returned back in GetQueuedCompletionStatus call.
    execargs.Overlapped = (LPOVERLAPPED)Overlapped;

    ret = ioctl((int)(long)DevFD, KXM_EXECUTE_COMMAND, &cmd);
    if(ret < 0)
    {
        KTraceFailedAsyncRequest(LinuxError::LinuxErrorToNTStatus(errno), NULL, errno, (ULONGLONG)FileHandle);
        return LinuxError::LinuxErrorToNTStatus(errno);
    }

    if (cmd.Status != STATUS_SUCCESS)
    {
        KTraceFailedAsyncRequest(cmd.Status, NULL, errno, (ULONGLONG)FileHandle);
    }

    return cmd.Status;
}

NTSTATUS KXMWriteFileAsyncK(HANDLE DevFD, HANDLE FileHandle, void *UserAddress,
                        int NumPages, LPOVERLAPPED Overlapped)
{
    KXMExecuteCommand cmd;
    KXMFileExecute execargs;
    int ret;
    KXMFileOffset iov;

    if(!Overlapped)
        return STATUS_INVALID_PARAMETER;

    cmd.Handle = (int)(long)FileHandle;
    cmd.Args = (void *)&execargs;

    //Setup UserAddress, NumPages and Offset information.
    execargs.Command = WriteKXMFile;
    execargs.Iov = &iov;
    execargs.Iov[0].PhysicalAddress = (((unsigned long long)Overlapped->OffsetHigh)<<32) | 
                        ((unsigned long long)Overlapped->Offset & 0xffffffffU);
    execargs.Iov[0].UserAddress = (unsigned long long)UserAddress;
    execargs.Iov[0].NumPages = NumPages;
    execargs.NumIov = 1;

    //This value is returned back in GetQueuedCompletionStatus call.
    execargs.Overlapped = Overlapped;

    ret = ioctl((int)(long)DevFD, KXM_EXECUTE_COMMAND, &cmd);

    if(ret < 0)
    {
        KTraceFailedAsyncRequest(LinuxError::LinuxErrorToNTStatus(errno), NULL, errno, (ULONGLONG)FileHandle);
        return LinuxError::LinuxErrorToNTStatus(errno);
    }

    if (cmd.Status != STATUS_SUCCESS)
    {
        KTraceFailedAsyncRequest(cmd.Status, NULL, errno, (ULONGLONG)FileHandle);
    }

    return cmd.Status;  
}

NTSTATUS KXMReadFileScatterAsyncK(HANDLE DevFD, HANDLE FileHandle, 
                        KXMFileOffset Frag[], int NumEntries, 
                        LPOVERLAPPED Overlapped)
{
    KXMExecuteCommand cmd;
    KXMFileExecute execargs;
    int ret;
    int i;

    if(!Overlapped)
        return STATUS_INVALID_PARAMETER;

    cmd.Handle = (int)(long)FileHandle;
    cmd.Args = (void *)&execargs;

    if(NumEntries <= 0 || NumEntries > KXM_MAX_FILE_SG)
        return STATUS_INVALID_PARAMETER;

    //Setup UserAddress, NumPages and Offset information.
    execargs.Command = ReadKXMFile;
	
    execargs.Iov = Frag;
    execargs.NumIov = NumEntries;

    //This value is returned back in GetQueuedCompletionStatus call.
    execargs.Overlapped = (LPOVERLAPPED)Overlapped;
	
    ret = ioctl((int)(long)DevFD, KXM_EXECUTE_COMMAND, &cmd);

    if(ret < 0)
    {
        KTraceFailedAsyncRequest(LinuxError::LinuxErrorToNTStatus(errno), NULL, errno, (ULONGLONG)FileHandle);
        return LinuxError::LinuxErrorToNTStatus(errno);
    }

    if (cmd.Status != STATUS_SUCCESS)
    {
        KTraceFailedAsyncRequest(cmd.Status, NULL, errno, (ULONGLONG)FileHandle);
    }

    if (cmd.Status == STATUS_SUCCESS)
    {
        cmd.Status = STATUS_PENDING;
    }

    return cmd.Status;
    
}

NTSTATUS KXMWriteFileGatherAsyncK(HANDLE DevFD, HANDLE FileHandle, 
                        KXMFileOffset Frag[], int NumEntries, 
                        LPOVERLAPPED Overlapped)
{
    KXMExecuteCommand cmd;
    KXMFileExecute execargs;
    int ret;
    int i;

    if(!Overlapped)
        return STATUS_INVALID_PARAMETER;

    cmd.Handle = (int)(long)FileHandle;
    cmd.Args = (void *)&execargs;

    if(NumEntries <= 0 || NumEntries > KXM_MAX_FILE_SG)
        return STATUS_INVALID_PARAMETER;

    //Setup UserAddress, NumPages and Offset information.
    execargs.Command = WriteKXMFile;

    execargs.Iov = Frag;
    execargs.NumIov = NumEntries;

    //This value is returned back in GetQueuedCompletionStatus call.
    execargs.Overlapped = (LPOVERLAPPED)Overlapped;

    ret = ioctl((int)(long)DevFD, KXM_EXECUTE_COMMAND, &cmd);

    if(ret < 0)
    {
        KTraceFailedAsyncRequest(LinuxError::LinuxErrorToNTStatus(errno), NULL, errno, (ULONGLONG)FileHandle);
        return LinuxError::LinuxErrorToNTStatus(errno);
    }

    if (cmd.Status != STATUS_SUCCESS)
    {
        KTraceFailedAsyncRequest(cmd.Status, NULL, errno, (ULONGLONG)FileHandle);
    }

    if (cmd.Status == STATUS_SUCCESS)
    {
        cmd.Status = STATUS_PENDING;
    }

    return cmd.Status;
    
}


NTSTATUS KXMFlushBuffersFileK(HANDLE DevFD, HANDLE FileHandle)
{
    // BLOCK_NOFLUSH is to differentiate between FUA and NONFUA 
    // KXM always by passes filesystem cache
    // we can add SYNC to send an empty FUA operation to disk to flush disk buffer
   
    return STATUS_SUCCESS;
}

NTSTATUS KXMCloseFileK(HANDLE DevFD, HANDLE FileHandle)
{
    KXMFileClose closeargs;
    KXMCloseCommand cmd;
    int ret;

    cmd.Handle = (int)(long) FileHandle; 
    //Unused.
    cmd.Args = (void *)&closeargs;
    ret = ioctl((int)(long)DevFD, KXM_CLOSE_DESCRIPTOR, &cmd);

    if(ret < 0)
    {
        KTraceFailedAsyncRequest(LinuxError::LinuxErrorToNTStatus(errno), NULL, errno, (ULONGLONG)FileHandle);
        return LinuxError::LinuxErrorToNTStatus(errno);
    }

    if (cmd.Status != STATUS_SUCCESS)
    {
        KTraceFailedAsyncRequest(cmd.Status, NULL, errno, (ULONGLONG)FileHandle);
    }

    return cmd.Status;
}
