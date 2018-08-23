/*++

Copyright (c) Microsoft Corporation

Module Name:

    KUShared.h

Abstract:

    Shared header file between KXM driver and user space library.
--*/

#pragma once

#if defined(KTL_USER_MODE)
#include <ktlpal.h>
#endif

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//Common enums/structures used by layer maintaining file descriptor table //
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
#define KXM_CREATE_DESCRIPTOR   _IOWR(0, 1, KXMOpenCommand)
#define KXM_CLOSE_DESCRIPTOR    _IOWR(0, 2, KXMCloseCommand) 
#define KXM_EXECUTE_COMMAND     _IOWR(0, 3, KXMExecuteCommand)
#define KXM_ASSOCIATE_FILE_WITH_COMP_PORT   _IOWR(0, 4, KXMAssociationCommand)

typedef enum
{
    //This desctype is used to read/write into blockfile bypassing the file
    //system.
    BlockFile=0,

    //This desctype is used for sending notification between processes.
    EventFile,

    //This desctype is used to create IOCompletionPort. Semantics of this
    //descriptor is similar to that of Windows.
    CompletionPort,

    MaxDescType
}KXMDescType;

typedef struct _KXMOpenCommand
{
    //BlockFile, EventFile, CompletionPort
    KXMDescType Type;

    //Kernel fills up this value with new Handle.
    int Handle;

    //Status of command.
    NTSTATUS Status;

    //DescType specific Parameters.
    void *Args;
}KXMOpenCommand;

typedef struct _KXMCloseCommand
{
    //Handle to close
    int Handle;

    //Status of command.
    NTSTATUS Status;

    //DescType specific Parameters.
    void *Args;
}KXMCloseCommand;

typedef struct _KXMExecuteCommand
{
    //Handle to close
    int Handle;

    //Status of command
    NTSTATUS Status;

    //DescType specific Parameters.
    void *Args;
}KXMExecuteCommand;

typedef struct _KXMAssociationCommand
{
    //Handle to FileHandle
    int Handle;

    //Handle to CompletionPort 
    int CPortHandle;

    //Status of command
    NTSTATUS Status;

    //DescType specific Parameters.
    void *Args;
}KXMAssociationCommand;


///////////////////////////////////////////////////////////////////////////////
//////////////////Completion port specific structures./////////////////////////
///////////////////////////////////////////////////////////////////////////////

//Completion Port specific structures.
typedef struct _KXMCompletionEvent
{
    NTSTATUS Status;
    //Completion key associated with the File Handle.
    ULONG_PTR CompletionKey;
    //Data transfered in this completion.
    DWORD DataXfred;
    //Pointer to overlapped structure passed when request was submitted.
    LPOVERLAPPED Overlapped;
}KXMCompletionEvent;

typedef struct _KXMCompletionPortOpenCmd
{
    DWORD MaxConcurrentThreads;
}KXMCompletionPortOpen;

typedef struct _KXMCompletionPortCloseCmd
{
    unsigned int Unused;
}KXMCompletionPortClose;

typedef enum
{
    PostCompletionEvent=1,
    GetCompletedEvent,
    RegisterThread,
    UnregisterThread
}CPortExecuteCommand;

typedef struct _KXMCompletionPortExecuteCmd
{
    CPortExecuteCommand Command;
    DWORD MilliSecondsTimeout;
    KXMCompletionEvent CompletionEvent;
}KXMCompletionPortExecute;


///////////////////////////////////////////////////////////////////////////////
/////////////////////Block File specific structures.///////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define NUM_CACHE_LIST_BUCKETS      9
#define MAX_FILE_NAME_LEN           512

//
// Note that KXM_MAX_FILE_SG must equal KBlockFile::MaxBlocksPerIo
//
#define KXM_MAX_FILE_SG             256
#define KXM_MAX_FILE_SG_ON_STACK    16

typedef enum _FileMode
{
    //Below modes are applicable to BlockFile.

    //When blcok file is opened with FLUSH mode, writes are submitted to disk 
    //with FUA flag set.
    BLOCK_FLUSH,

    //When block file is opened with NOFLUSH mode, writes are submitted without
    //FUA flag set.
    BLOCK_NOFLUSH,

    //Below modes are applicable to EventFile.
    EVENT_PRODUCER,

    EVENT_CONSUMER
}FileMode;

typedef struct _KXMFileOpenCmd
{
    //Specifies FileName to work on.
    unsigned char FileName[MAX_FILE_NAME_LEN];

    //Number of cached BIOs per list.
    unsigned long NumCacheEntries[NUM_CACHE_LIST_BUCKETS];

    FileMode Mode;

    int RemotePID;
}KXMFileOpen;

typedef struct _KXMFileCloseCmd
{
    unsigned int Unused;
}KXMFileClose;

typedef enum
{
    ReadKXMFile=1,
    WriteKXMFile 
}FileExecuteCommand;


typedef struct _KXMFileOffset
{
    unsigned long long UserAddress;
    unsigned long long PhysicalAddress;
    unsigned long long NumPages;
}KXMFileOffset;

typedef struct _KXMFileExecuteCmd
{
    FileExecuteCommand Command;

    //This value is returned back in completion event.
    LPOVERLAPPED Overlapped;
   
    //Number of valid IO vectors. This parameter is ignored for EventFile. 
    int NumIov;   

    //Iov containting user address, sector offset and number of pages for
    //read/write. This parameter is ignored for EventFile. 
    KXMFileOffset *Iov;
}KXMFileExecute;

//AssocioateCmd shared by both BlockFile and EventFile
typedef struct _KXMFileAssociateCmd
{
    //Key to be returned along with Completion event.
    ULONG_PTR CompletionKey;
}KXMFileAssociateCmd;
