/*++

Copyright (c) Microsoft Corporation

Module Name:

    KXMUKApi.cpp

Abstract:

    Implementation of user space APIs to acces KXM driver.

--*/

#include "KXMKApi.h"

//////////////////////////////////////
//////////////////////////////////////
////////CompletionPort APIs///////////
//////////////////////////////////////
//////////////////////////////////////

// User
BOOL
WINAPI
GetQueuedCompletionStatusU(
    _In_  HANDLE       CompletionPort,
    _Out_ LPDWORD      lpNumberOfBytes,
    _Out_ PULONG_PTR   lpCompletionKey,
    _Out_ LPOVERLAPPED *lpOverlapped,
    _In_  DWORD        dwMilliseconds
    );

BOOL
WINAPI
PostQueuedCompletionStatusU(
    _In_     HANDLE       CompletionPort,
    _In_     DWORD        dwNumberOfBytesTransferred,
    _In_     ULONG_PTR    dwCompletionKey,
    _In_opt_ LPOVERLAPPED lpOverlapped
    );

HANDLE
CreateIoCompletionPortU(
    _In_     HANDLE      FileHandle,
    _In_opt_ HANDLE      ExistingCompletionPort,
    _In_     ULONG_PTR   CompletionKey,
    _In_     DWORD       NumberOfConcurrentThreads
    );

BOOL RegisterCompletionThreadU(HANDLE CompHandle);

BOOL UnregisterCompletionThreadU(HANDLE CompHandle);

BOOL CloseIoCompletionPortU(HANDLE CompHandle);


//////////////////////////////////////
//////////////////////////////////////
////////////KXM File APIs/////////////
//////////////////////////////////////
//////////////////////////////////////

template<class T> class SystemThreadWorker
{
    public:
        
    NTSTATUS StartThread()
    {
        NTSTATUS status = STATUS_SUCCESS;
        int error;
        pthread_t thread;
        pthread_attr_t attr;
        int stack_size = 64 * 1024;

        error = pthread_attr_init(&attr);
        if (error != 0)
        {
            status = LinuxError::LinuxErrorToNTStatus(error);           
            KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)this, (ULONGLONG)error);
            return(status);
        }
		KFinally([&]() {
			pthread_attr_destroy(&attr);
		});

        error = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (error != 0)
        {
            status = LinuxError::LinuxErrorToNTStatus(error);           
            KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)this, (ULONGLONG)error);
            return(status);
        }

	    error = pthread_attr_setstacksize(&attr, stack_size);
        if (error != 0)
        {
            status = LinuxError::LinuxErrorToNTStatus(error);           
            KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)this, (ULONGLONG)error);
            return(status);
        }
        
        error = pthread_create(&thread, &attr, T::ThreadProcA, (T*)this);
        if (error != 0)
        {
            status = LinuxError::LinuxErrorToNTStatus(error);           
            KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)this, (ULONGLONG)error);
            return(status);
        }

        return(status);
    }
    
    protected:
    static void* ThreadProcA(void* context)
    {
        ((T*)context)->Execute();
        return(nullptr);
    }
};


#define KXM_FILEHANDLE_TAG 'KXFH'
class KXMFileHandle : public IHandleObject
{
    K_RUNTIME_TYPED(KXMFileHandle);
    K_FORCE_SHARED(KXMFileHandle);

public:
    static KXMFileHandle::SPtr Create();

    BOOL Close() override;

    inline VOID SetOpen()
    {
        _IsOpen = 1;
    }
    
    inline BOOLEAN IsOpen()
    {
        return(_IsOpen == 1);
    }
    
    inline VOID SetKxmHandle(HANDLE KxmHandle)
    {
        _KxmHandle = KxmHandle;
    }

    inline HANDLE GetKxmHandle()
    {
        return(_KxmHandle);
    }

    inline VOID SetUseFileSystemOnly()
    {
        _UseFileSystemOnly = TRUE;
    }
    
    inline BOOLEAN UseFileSystemOnly()
    {
        return(_UseFileSystemOnly);
    }

    inline BOOLEAN SetCompletionPort(HANDLE CompletionPortHandle)
    {
        if (_CompletionPort != NULL)
        {
            return(TRUE);
        }
        _CompletionPort = CompletionPortHandle;
        
        return(FALSE);
    }
    
    inline HANDLE GetCompletionPort()
    {
        return(_CompletionPort);
    }

    inline VOID SetCompletionKey(ULONG_PTR CompletionKey)
    {
        _CompletionKey = CompletionKey;
    }
    
    inline ULONG_PTR GetCompletionKey()
    {
        return(_CompletionKey);
    }

    inline VOID SetMaxConcurrentThreads(ULONG MaxConcurrentThreads)
    {
        _MaxConcurrentThreads = MaxConcurrentThreads;
    }

    inline ULONG GetMaxConcurrentThreads()
    {
        return(_MaxConcurrentThreads);
    }
    
private:
    volatile LONG _IsOpen;
    BOOLEAN _UseFileSystemOnly;
    HANDLE _KxmHandle;
    HANDLE _CompletionPort;
    ULONG_PTR _CompletionKey;
    ULONG _MaxConcurrentThreads;
};


// User
HANDLE KXMCreateFileUFS(int FileFD, BOOLEAN IsEventFile, FileMode Mode,
                            pid_t RemotePID, ULONG Flags);

HANDLE KXMCreateFileUDW(const char *FileName, BOOLEAN IsEventFile, FileMode Mode,
                            pid_t RemotePID, ULONG Flags);

NTSTATUS KXMReadFileAsyncU(HANDLE DevFD, KXMFileHandle& FileHandle, void *UserAddress,
                        int NumPages, LPOVERLAPPED Overlapped);

NTSTATUS KXMWriteFileAsyncU(HANDLE DevFD, KXMFileHandle& FileHandle, void *UserAddress,
                        int NumPages, LPOVERLAPPED Overlapped);

NTSTATUS KXMReadFileScatterAsyncU(HANDLE DevFD, KXMFileHandle& FileHandle, 
                        KXMFileOffset Frag[], int NumEntries, 
                        LPOVERLAPPED Overlapped);

NTSTATUS KXMWriteFileGatherAsyncU(HANDLE DevFD, KXMFileHandle& FileHandle, 
                        KXMFileOffset Frag[], int NumEntries, 
                        LPOVERLAPPED Overlapped);

NTSTATUS KXMCloseFileU(HANDLE DevFD, KXMFileHandle& FileHandle);

NTSTATUS KXMFlushBuffersFileU(HANDLE DevFD, KXMFileHandle& FileHandle);

