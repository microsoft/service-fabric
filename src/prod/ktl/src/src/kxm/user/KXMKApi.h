/*++

Copyright (c) Microsoft Corporation

Module Name:

    KXMKApi.cpp

Abstract:

    Implementation of user space APIs to acces KXM driver.

--*/

//////////////////////////////////////
//////////////////////////////////////
////////CompletionPort APIs///////////
//////////////////////////////////////
//////////////////////////////////////

// Kernel
HANDLE CreateIoCompletionPortK(HANDLE DevFD, HANDLE FileHandle, 
                        HANDLE CompHandle, ULONG_PTR CompletionKey, 
                        DWORD MaxConcurrentThreads);

BOOL GetQueuedCompletionStatusK(HANDLE DevFD, HANDLE CompHandle, 
                        LPDWORD DataXfred,
                        PULONG_PTR CompletionKey, 
                        LPOVERLAPPED *Poverlapped, DWORD MilliSecondsTimeout);

BOOL PostQueuedCompletionStatusK(HANDLE DevFD, HANDLE CompHandle, 
                        DWORD DataXfred, ULONG_PTR CompletionKey, 
                        LPOVERLAPPED Overlapped);

BOOL RegisterCompletionThreadK(HANDLE DevFD, HANDLE CompHandle);

BOOL UnregisterCompletionThreadK(HANDLE DevFD, HANDLE CompHandle);

BOOL CloseIoCompletionPortK(HANDLE DevFD, HANDLE CompHandle);

//////////////////////////////////////
//////////////////////////////////////
////////////KXM File APIs/////////////
//////////////////////////////////////
//////////////////////////////////////


// Kernel
HANDLE KXMCreateFileK(HANDLE DevFD, const char *FileName,
                            unsigned long *NumCacheEntries, BOOLEAN IsEventFile, FileMode Mode,
                            pid_t RemotePID, ULONG Flags);

NTSTATUS KXMReadFileAsyncK(HANDLE DevFD, HANDLE FileHandle, void *UserAddress,
                        int NumPages, LPOVERLAPPED Overlapped);

NTSTATUS KXMWriteFileAsyncK(HANDLE DevFD, HANDLE FileHandle, void *UserAddress,
                        int NumPages, LPOVERLAPPED Overlapped);

NTSTATUS KXMReadFileScatterAsyncK(HANDLE DevFD, HANDLE FileHandle, 
                        KXMFileOffset Frag[], int NumEntries, 
                        LPOVERLAPPED Overlapped);

NTSTATUS KXMWriteFileGatherAsyncK(HANDLE DevFD, HANDLE FileHandle, 
                        KXMFileOffset Frag[], int NumEntries, 
                        LPOVERLAPPED Overlapped);

NTSTATUS KXMCloseFileK(HANDLE DevFD, HANDLE FileHandle);

NTSTATUS KXMFlushBuffersFileK(HANDLE DevFD, HANDLE FileHandle);
