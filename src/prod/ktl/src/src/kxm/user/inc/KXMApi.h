/*++

Copyright (c) Microsoft Corporation

Module Name:

    KXMApi.h

Abstract:

    Declaration of user space KXM APIs.

--*/

#pragma once
#include <sys/ioctl.h>
#include <KUShared.h>

#define KXM_DEVICE_FILE	"/dev/sfkxm"

/////////////////////////////////////
///////////KXM Device API////////////
/////////////////////////////////////

#define KXM_USER_EMULATION_FD  (HANDLE)(-2)

//Opens KXM device and returns a file descriptor, non-negative integer,
//for use in subsequent calls to access completionport, blockfile and syncfile.
//Return:
//  On Success, updates FD with +ve integer representing file descriptor and
//  returns STATUS_SUCCESS.
//  On Failure, returns appropriate NTSTATUS error code..
HANDLE KXMOpenDev(void);

//Closes previously opened device file. Any completionport, blockfile or
//syncfile descriptors associated with this device descriptor will be closed
//automatically and will no longer be available for use.
//Return:
//  On Success, 0
//  On Failure, returns -1.
NTSTATUS KXMCloseDev(HANDLE DeviceFD);



/////////////////////////////////////
/////COMPLETION PORT APIs////////////
/////////////////////////////////////


//Creates an IO completion port and associates it with a specified FileHandle,
//or creates an I/O completion port that is not yet associated with a
//FileHandle, allowing association at a later time. Associating an instance of
//an opened file handle with an I/O completion port allows a process to receive
//notification of the completion of async IO operation involving that 
//FileHandle. If INVALID_HANDLE_VALUE is specified for FileHandle, the function
//creates an I/O completion port without associating it with a file handle. In
//this case, the ExistingCompHandle parameter must be NULL and the
//CompletionKey paramter is ignored.
//Return:
// On Success, 
//    If the ExistingCompHandle paramter was NULL the return value is new
//    handle.
//    If the ExistingCompHandle was valid IO comp handle, return value is that
//    same handle.
// On Failure, returns NULL.
HANDLE KXMCreateIoCompletionPort(HANDLE DevFD, HANDLE FileHandle, 
                        HANDLE ExistingCompHandle, ULONG_PTR CompletionKey,
                        DWORD NumberOfConcurrentThreads);


//Attempts to dequeue an I/O completion packet from the specified CompHandle.
//If there is no completion packet queued, the function waits for the pending
//IO operation associated with the completion port to complete for the
//specified timeout. If MilliSecondsTimeout is set to INFINITE, call blocks
//until atleast one event gets completed. 
//Return:
// On Success, returns true and CompletionKey, NumberOfBytes and 
//             Overlappaed are updated based on the event completed. 
// On Failure, returns false, use GetLastError to get the extended error
// information.
BOOL KXMGetQueuedCompletionStatus(HANDLE DevFD, HANDLE CompHandle, 
                        LPDWORD NumberOfBytes,
                        PULONG_PTR CompletionKey, 
                        LPOVERLAPPED *Overlapped,
                        DWORD MilliSecondsTimeout);


//Blocking call to post the completion event on IOCompletionPort specified by
//CompHandle. If a thread is waiting on this completionport for an event it
//would be woken up.
//Return:
// On Success, returns true.
// On Failure, returns false, use GetLastError to get the extended error
// information.
BOOL KXMPostQueuedCompletionStatus(HANDLE DevFD, HANDLE CompHandle, 
                        DWORD NumberOfBytesXfred, ULONG_PTR CompletionKey, 
                        LPOVERLAPPED Overlapped);

//Closes completion port specified by CompHandle. This call would fail if there
//are BlockFiles/EventFiles descriptors using this completion port open. User
//must close all BlockFiles/EventFiles using this completion port before calling
//this API.
//Return:
// On Success, returns STATUS_SUCCESS.
// On Failure, returns NTSTATUS failure code based on the type of error.
NTSTATUS KXMCloseIoCompletionPort(HANDLE DevFD, HANDLE CompHandle);


//All threads calling GetQueuedCompletionStatus must call this API to enable
//thread monitoring. 
BOOL KXMRegisterCompletionThread(HANDLE DevFD, HANDLE CompHandle);

//Unregisters thread from monitoring.
BOOL KXMUnregisterCompletionThread(HANDLE DevFD, HANDLE CompHandle);

/////////////////////////////////////
/////////BLOCK FILE APIs/////////////
/////////////////////////////////////


//Blocking call to Open BlockFile. 
//Parameters:
//  DevFD:              Device File descriptor returned in KXMOpenDev()
//  FileFD:             File descriptor for the file as opened via the file system
//  FileName:           Path of a file to open.  
//  NumCacheEntreis:    Contains the array of integeres representing number of
//                      preallocated BIOs for specified block size. If 
//                      NumCacheEntries is set to NULL preallocation is
//                      disabled.
//
//                      Entry     BlockSize       DefaultEntries      Memory Reserved To Store Pages
//                      0         4K              1024                1024*(8Bytes per Page) = 8K
//                      1         8K              1024                1024*(8Bytes per Page) = 16K
//                      2         16K             512                 512*32  = 16K
//                      3         32K             512                 512*64  = 32K
//                      4         64K             128                 128*128 = 16K
//                      5         128K            128                 128*256 = 32K
//                      6         256K            64                  64*512  = 32K
//                      7         512K            64                  64*1024 = 64K
//                      8         1024K           64                  64*2048 = 128K
//
//                      Fill up the array with all zeros to use the above
//                      default values. 
//
//  IsEventFile         If TRUE then file is opened as an EventFile, if
//                      FALSE then as a block file
//  FileMode:           For block file, possible values are 
//                        BLOCK_FLUSH - Use FUA/Writethrough
//                        BLOCK_NOFLUSH - No FUA or Writethrough
//
//                      For event file, possible values are
//                      "EVENT_PRODUCER/EVENT_CONSUMER".
//
//                      See KUShared.h for description of each of this mode.
//  RemotePID:          PID of the remote process allowed to open event file.
//                      This parameter is valid only for EventFile when file 
//                      is opened in PRODUCER  mode. Setting thsi value to -1
//                      ignores the PID comparison.
//
// Flags:               See KXMFLAG_ below
//Return:
// On Success, returns the Handle representing File.
// On Failure, returns INVALID_HANDLE_VALUE.


//
// KXMFLAG_USE_FILESYSTEM_ONLY is TRUE to indicate that all IO go via the file
// system. Note that this can be set on return in the case that KXM
// needs to fallback to file system only
//
// KXMFLAG_AVOID_FILESYSTEM is TRUE to indicate that all IO go through
// KXM
//
#define KXMFLAG_USE_FILESYSTEM_ONLY           0x00000001
#define KXMFLAG_AVOID_FILESYSTEM              0x00000002

HANDLE KXMCreateFile(
    HANDLE DevFD,
    int FileFD,
	const char *FileName,
	unsigned long *NumCacheEntries,
    BOOLEAN IsEventFile,
	FileMode Mode,
    pid_t RemotePID,
    ULONG& Flags
	);


//
// This api will return the infomration used for the completion port
// associated with the handle
//
//Parameters:
//  FileHandle:             Block/Event file handle returned in CreateKXMFile()
//  CompletionPort          Returns completion port handle associated with FileHandle
//  CompletionKey           Returns completion key associated with FileHandle
//  MaxConcurrentThreads    Returns max concurent threads handle associated with FileHandle
VOID KXMGetCompletionPortInfo(
	HANDLE FileHandle,
	HANDLE& CompletionPort,
	ULONG_PTR& CompletionKey,
	ULONG& MaxConcurrentThreads
	);

//
// This will force flushing of any unsaved data to the disk
//
//Parameters:
//  DevFD:              Device File descriptor returned in KXMOpenDev()
//  FileHandle:         Block/Event file handle returned in CreateKXMFile()
NTSTATUS KXMFlushBuffersFile(
    HANDLE DevFD, 							 
	HANDLE FileHandle
);

//For BlockFile, this function is used to asynchronously read certain number 
//of pages from the blockfile starting at specified file offset. Request
//completion would be reported in the associated completion port. This
//function would return failure if,
//  1. Invalid handle is passed.
//  2. UserAddress is invalid or not page size aligned.
//  3. Kernel couldn't allocate memory to submit the request.
//
//For EventFile, this function submits an async read. Request completion would
//be reported in the associated completion port. 
//This function would return failure if,
//  1. Invalid handle is passed.
//  2. If previous read is still pending.
//  3. If file was opened in ProducerMode. 
//
//
//Parameters:
//  DevFD:              Device File descriptor returned in KXMOpenDev()
//  Handle:             Block/Event file handle returned in CreateKXMFile()
//  UserAddress:        PageAligned user space address. This parameter is
//                      ignored for EventFIle.
//  NumPages:           Number of pages to read. This parameter is ignored for
//                      EventFile.
//  Overlapped:         This overlapped pointer would be returned through the
//                      GetQueuedCompletionStatus. Overlapped.Offset & 
//                      OffsetHigh should contain the file offset. 
//Return:
// On Success, returns STATUS_SUCCESS
// On Failure, returns NTSTATUS failure code based on the type of error.
NTSTATUS KXMReadFileAsync(HANDLE DevFD, HANDLE FileHandle, void *UserAddress,
                        int NumPages, LPOVERLAPPED Overlapped);

//For BlockFile, 
//This function is used to asynchronously write certain number 
//of pages from the blockfile starting at specified file offset. Request
//completion would be reported in the associated completion port.
//This function would fail if,
//  1. Invalid handle is passed.
//  2. UserAddress is invalid or not page size aligned.
//  3. Kernel couldn't allocate memory to submit the request.
//
//For EventFile,
//This function is used to submit an event to the EventFile opened in Producer
//mode. If a consumer has an unsatisfied read pending it would complete in 
//this write context. Request completion would be reported in the associated
//CompletionPort.
//UserAddress, NumPages and Offset parameters are ignored for EventFile.
//
//This function would fail if,
// 1. Invalid handle is passed.
// 2. Previous write is still pending.
// 3. If file was opened in Consumer mode.
//
//Parameters:
//  DevFD:              Device File descriptor returned in KXMOpenDev()
//  BlockHandle:        Block file handle returned in CreateBlockFile()
//  UserAddress:        PageAligned user space address.
//  NumPages:           Number of pages to write.
//  Overlapped:         This overlapped pointer would be returned through the
//                      GetQueuedCompletionStatus. Overlapped.Offset & 
//                      OffsetHigh should contain the file offset. 
//Return:
// On Success, returns STATUS_SUCCESS
// On Failure, returns NTSTATUS failure code based on the type of error.
NTSTATUS KXMWriteFileAsync(HANDLE DevFD, HANDLE FileHandle, void *UserAddress,
                        int NumPages, LPOVERLAPPED Overlapped);

//Closes file opend using CreateBlockFile. This function blocks until all
//in-flight read/write requests are completed.
//Parameters:
//  DevFD:              Device File descriptor returned in KXMOpenDev()
//  BlockHandle:        File handle returned in CreateKXMFile()
//                      on this block file. Currently this value is not used.
//Return:
// On Success, returns STATUS_SUCCESS 
// On Failure, returns NTSTATUS failure code based on the type of error.
NTSTATUS KXMCloseFile(HANDLE DevFD, HANDLE FileHandle);

//For BlockFile, 
//This API takes multiple fragements of user space address and reads data from
//the specified offset. 
//This function would fail if,
//  1  FileHandle is not a valid block handle. 
//  2. Invalid DevFD is passed. 
//  3. UserAddress of any fragment is invalid or not page size aligned.
//  4. Kernel couldn't allocate memory to submit the request.
NTSTATUS KXMReadFileScatterAsync(HANDLE DevFD, HANDLE FileHandle, 
                        KXMFileOffset Frags[], int NumEntries,
                        LPOVERLAPPED Overlapped);

//For BlockFile, 
//This API takes multiple fragements of user space address and writes data at
//the specified offset. 
//This function would fail if,
//  1  FileHandle is not a valid block handle. 
//  2. Invalid DevFD is passed. 
//  3. UserAddress of any fragment is invalid or not page size aligned.
//  4. Kernel couldn't allocate memory to submit the request.
NTSTATUS KXMWriteFileGatherAsync(HANDLE DevFD, HANDLE FileHandle, 
                        KXMFileOffset Frags[], int NumEntries,
                        LPOVERLAPPED Overlapped);

/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////
