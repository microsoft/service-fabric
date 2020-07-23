// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#define LEASE_APP_DIRECTORY "/dev/shm/"
#define LEASE_APP_FILE_PREFIX "ServiceFabricLeaseApp_"
#define LEASE_APP_FILE_PREFIX_LENGTH sizeof(LEASE_APP_FILE_PREFIX)-1
#define LEASE_APP_CLEANUP_TIME_THRESHOLD_IN_SECONDS (60*30) //30 minutes

using namespace Common;
using namespace std;

namespace
{
    LeaseLayerEventSource LeaseLayerEvents;
    const StringLiteral TraceType("TTLFile");


    Global<LEASING_APPLICATION_REF_COUNT_HASH_TABLE> LeasingApplicationRefCountHashTable = make_global<LEASING_APPLICATION_REF_COUNT_HASH_TABLE>(); 

    Global<RwLock> LeaseLayerLock = make_global<RwLock>();
    volatile BOOLEAN IsInitializeCalled = FALSE;
    // Global<LEASING_APPLICATION_HANDLE_MAP> leaseAppWriteMap = make_global<LEASING_APPLICATION_HANDLE_MAP>();
    // bool shouldStartCleanup = true;
    // Global<RwLock> leaseAppWriteMapLock = make_global<RwLock>();
    // Global<LEASING_APPLICATION_HANDLE_MAP> leaseAppReadMap = make_global<LEASING_APPLICATION_HANDLE_MAP>();
    // Global<RwLock> leaseAppReadMapLock = make_global<RwLock>();
};
/*
class TTLFile
{
    DENY_COPY(TTLFile);

public:
    TTLFile(HANDLE handle);
    virtual ~TTLFile(); // made virtual so that fabdbg can count

    bool Initialize(bool writable);
    PLONGLONG Addr() const;

    static constexpr size_t FileSize = sizeof(LONGLONG);

private:
    HANDLE handle_;
    PLONGLONG addr_;
};

TTLFile::TTLFile(HANDLE handle) : handle_(handle), addr_(nullptr)
{
}

TTLFile::~TTLFile()
{
    if (!addr_) return;

    auto retval = munmap(addr_, FileSize);
    if (retval < 0)
    {
        LeaseTrace::WriteError(TraceType, "dtor: munmap failed: {0}", errno);
    }
}

PLONGLONG TTLFile::Addr() const
{
    return addr_;
}

bool TTLFile::Initialize(bool writable)
{
    auto fileName = formatString("ServiceFabricLeaseApp_{0}",(int64)handle_);
    auto fd =
        writable ?
        shm_open(fileName.c_str(), O_RDWR | O_CREAT, S_IRWXU | S_IRGRP | S_IROTH) :
        shm_open(fileName.c_str(), O_RDONLY, NULL);

    if (fd < 0)
    {
        return false; 
    }

    if (writable && ftruncate(fd, FileSize) != 0)
    {
        return false;
    }

    addr_ = (PLONGLONG)mmap(
        NULL,
        FileSize,
        writable ? (PROT_READ | PROT_WRITE) : PROT_READ,
        MAP_SHARED,
        fd,
        0);

    // close fd as early as possible to avoid running into file descriptor count limit.
    // It is okay to close fd here as mmap adds an extra reference to the file
    close(fd);
    return addr_ != MAP_FAILED;
}

void RemoveLeasingAppDueTimeFile(string const & fileName)
{
    if (shm_unlink(fileName.c_str()) < 0)
    {
        auto errNo = errno;
        LeaseTrace::WriteInfo(TraceType, "shm_unlink({0}) failed: {1}", fileName, errNo); 
        return;
    }

    LeaseTrace::WriteInfo(TraceType, "shm_unlink({0}): success", fileName); 
}

void RemoveLeasingAppDueTimeFileIfNeeded(string const & fileName)
{
    auto fd = shm_open(fileName.c_str(), O_RDONLY, NULL);
    KFinally([fd] { if (fd >= 0) close(fd); });
    if (fd < 0)
    {
        auto err = errno;
        LeaseTrace::WriteInfo(TraceType, "RemoveLeasingAppDueTimeFileIfNeeded: shm_open({0}) failed: {1}", fileName, err); 
        return;
    }

    LONGLONG dueTime = 0;
    auto retval = read(fd, &dueTime, sizeof(dueTime));
    if (retval < 0)
    {
        auto err = errno;
        LeaseTrace::WriteInfo(TraceType, "RemoveLeasingAppDueTimeFileIfNeeded: read({0}) failed: {1}", fileName, err); 
        return;
    }

    if ((retval != sizeof(dueTime)) || (dueTime == 0))
    {
        if (retval != sizeof(dueTime))
        {
            LeaseTrace::WriteInfo(TraceType, "RemoveLeasingAppDueTimeFileIfNeeded: read({0}) returned {1}, {2} expected", fileName, retval, sizeof(dueTime));
        }
        else if (dueTime == 0)
        {
            LeaseTrace::WriteInfo(TraceType, "RemoveLeasingAppDueTimeFileIfNeeded: file {0} contains 0", fileName);
        }

        // file was abandoned during initialization?
        struct stat statBuf = {};
        retval = fstat(fd, &statBuf);
        if (retval < 0)
        {
            auto err = errno;
            LeaseTrace::WriteWarning(TraceType, "RemoveLeasingAppDueTimeFileIfNeeded: fstat({0}) failed: {1}", fileName, err); 
            return;
        }

        auto timeElapsedSinceLastUpdate = time(nullptr) - statBuf.st_mtime;
        if (timeElapsedSinceLastUpdate > LEASE_APP_CLEANUP_TIME_THRESHOLD_IN_SECONDS)
        {
            RemoveLeasingAppDueTimeFile(fileName);
            return;
        }

        LeaseTrace::WriteInfo(
            TraceType,
            "RemoveLeasingAppDueTimeFileIfNeeded: skipping file {0}, as timeElapsedSinceLastUpdate = {1}, < {2}",
            fileName, timeElapsedSinceLastUpdate, LEASE_APP_CLEANUP_TIME_THRESHOLD_IN_SECONDS);
        return;
    }

    auto now = ((LONGLONG) Stopwatch::Now().Ticks)/TICK_PER_MILLISECOND;
    if ((now - dueTime) >  (LEASE_APP_CLEANUP_TIME_THRESHOLD_IN_SECONDS * 1000))
    {
        RemoveLeasingAppDueTimeFile(fileName);
    }
}

void CleanUpLeaseApplicationTTLFile()
{
    DIR *dfd = opendir(LEASE_APP_DIRECTORY);
    Invariant(dfd);
    KFinally([dfd] { closedir(dfd); });
            
    vector<string> filesToBeCleaned;
    struct dirent *dp;
    while ((dp = readdir(dfd)) != NULL)
    {
        size_t stringLength = strlen(dp->d_name);
        if (dp->d_type == DT_REG 
            && stringLength > LEASE_APP_FILE_PREFIX_LENGTH
            && strncmp(dp->d_name, LEASE_APP_FILE_PREFIX, LEASE_APP_FILE_PREFIX_LENGTH) == 0)
        {
            filesToBeCleaned.push_back(dp->d_name);
        }
    }
		
    for(string const& fileName: filesToBeCleaned)
    {
        RemoveLeasingAppDueTimeFileIfNeeded(fileName);
    }

    Threadpool::Post([] { CleanUpLeaseApplicationTTLFile(); }, TimeSpan::FromSeconds(LEASE_APP_CLEANUP_TIME_THRESHOLD_IN_SECONDS));
}

void ClearLeasingApplicationTTLFileMap(HANDLE handle)
{
    {
        AcquireExclusiveLock lock(*leaseAppWriteMapLock);
        leaseAppWriteMap->erase(handle);
    }

    {
        AcquireExclusiveLock lock(*leaseAppReadMapLock);
        leaseAppReadMap->erase(handle);
    }
}

BOOL WriteToLeaseApplicationExpirationTTLFile(HANDLE handle, LONGLONG ttl)
{
    shared_ptr<TTLFile> ttlFile;
    {
        AcquireExclusiveLock lock(*leaseAppWriteMapLock);

        auto iter = leaseAppWriteMap->find(handle);
        if(iter == leaseAppWriteMap->end())
        {
            ttlFile = make_shared<TTLFile>(handle);
            if (!ttlFile->Initialize(true)) return FALSE;

            leaseAppWriteMap->emplace(handle, ttlFile);        
            if (shouldStartCleanup)
            {
                shouldStartCleanup = false;
                Threadpool::Post([] { CleanUpLeaseApplicationTTLFile(); });
            }
        }
        else
        {
            ttlFile = iter->second;    
        }    
    }
 

    *(ttlFile->Addr()) = ttl;
    return TRUE;
}

BOOL ReadFromLeaseApplicationExpirationTTLFile(HANDLE handle, PLONGLONG ttl)
{
    shared_ptr<TTLFile> ttlFile;
    {
        AcquireExclusiveLock lock(*leaseAppReadMapLock);

        auto iter = leaseAppReadMap->find(handle);
        if(iter == leaseAppReadMap->end())
        {
            ttlFile = make_shared<TTLFile>(handle);
            if (!ttlFile->Initialize(false)) return FALSE;

            leaseAppReadMap->emplace(handle, ttlFile);
        }
        else
        {
            ttlFile = iter->second;    
        }     
    }
    
    *ttl = *(ttlFile->Addr());
    return TRUE;
}
*/

BOOL WINAPI
AddRef(
    PLEASING_APPLICATION_REF_COUNT LeasingApplicationRefCount
    )

/*++
 
Routine Description:
 
    Internal routine. Increments a leasing application ref count.

Parameters Description:
 
    LeasingApplicationRefCount - ref count.
 
Return Value:
 
    TRUE if the leasing application is not already closed.
 
--*/

{
    if (LeasingApplicationRefCount->IsClosed) {

        //
        // The leasing application has been closed already.
        // 
        return FALSE;
    }

    LeasingApplicationRefCount->Count++;

    LeaseLayerEvents.LeasingApplicationChangeRef(
        LONGLONG(LeasingApplicationRefCount->LeasingApplicationHandle),
        1,
        LeasingApplicationRefCount->Count);

    return TRUE;
}

VOID WINAPI
ReleaseRef(
    PLEASING_APPLICATION_REF_COUNT LeasingApplicationRefCount
    )

/*++
 
Routine Description:
 
    Internal routine. Decrements a leasing application ref count. If the
    ref count is zero and the application is closed, then signals that
    the application can now be unregistered.

Parameters Description:
 
    LeasingApplicationRefCount - ref count.
 
Return Value:
 
    n/a
 
--*/

{
    LeasingApplicationRefCount->Count--;
    ASSERT_IFNOT(0 <= LeasingApplicationRefCount->Count, "Leasing Application RefCount less than 0");

    LeaseLayerEvents.LeasingApplicationChangeRef(
        LONGLONG(LeasingApplicationRefCount->LeasingApplicationHandle),
        -1,
        LeasingApplicationRefCount->Count);

    if (LeasingApplicationRefCount->IsClosed && 0 == LeasingApplicationRefCount->Count) {

        //
        // If the leasing application has been closed the set the
        // closed event so UnregisterLeasingApplication can continue.
        //
        LeasingApplicationRefCount->CloseEvent->Set();
    }
}

BOOL WINAPI
CloseRef(
    PLEASING_APPLICATION_REF_COUNT LeasingApplicationRefCount
    )

/*++
 
Routine Description:
 
    Internal routine. Marks a leasing application as closing/closed.

Parameters Description:
 
    LeasingApplicationRefCount - ref count.
 
Return Value:
 
    n/a
 
--*/

{
    //
    // Mark the leasing application as closing.
    //
    ASSERT_IFNOT(!LeasingApplicationRefCount->IsClosed, "Leasing Application RefCount is already closed");

    LeaseLayerEvents.LeasingApplicationClosed(
        LONGLONG(LeasingApplicationRefCount->LeasingApplicationHandle),
        LeasingApplicationRefCount->Count);

    LeasingApplicationRefCount->IsClosed = TRUE;

    return (0 == LeasingApplicationRefCount->Count);
}

BOOL WINAPI RegisterLeasingApplicationForRefCounting(
    POVERLAPPED_LEASE_LAYER_EXTENSION Overlapped,
    HANDLE LeasingApplicationHandle)

/*++
 
Routine Description:
 
    Internal routine. A leasing application ref count is created for this
        leasing application handle.

Parameters Description:
 
    LeasingApplicationHandle - leasing application handle to register.
 
Return Value:
 
    TRUE if the leasing application ref count was inserted successfully.
 
--*/

{
    LEASING_APPLICATION_REF_COUNT LeasingApplicationRefCount;
    ZeroMemory(&LeasingApplicationRefCount, sizeof(LEASING_APPLICATION_REF_COUNT));
    BOOL HashTableRegistrationSuccess = FALSE;

    //
    // Check arguments.
    //
    if (NULL == Overlapped) {
        return FALSE;
    }

    if (NULL == LeasingApplicationHandle || 
        INVALID_HANDLE_VALUE == LeasingApplicationHandle) {

        return FALSE;
    }

    LeaseLayerEvents.RegisterForRefCount(LONGLONG(LeasingApplicationHandle));

    LeasingApplicationRefCount.CloseEvent = make_shared<ManualResetEvent>();
    LeasingApplicationRefCount.LeasingApplicationHandle = LeasingApplicationHandle;
    LeasingApplicationRefCount.Overlapped = Overlapped;

    //
    // Register the handle with the ref count.
    //
    {
        AcquireExclusiveLock lock(*LeaseLayerLock);
    
        try {

            //
            // Insert new entry for ref counting this leasing application.
            //
            LeasingApplicationRefCountHashTable->insert(
                LEASING_APPLICATION_REF_COUNT_HASH_TABLE_ENTRY(
                    LeasingApplicationHandle,
                    LeasingApplicationRefCount
                    )
                );

            HashTableRegistrationSuccess = TRUE;
        }
        catch(...) {
        
            LeaseLayerEvents.FailedToRegisterRefCount(
                LONGLONG(LeasingApplicationHandle));
        }
    
    }

    return HashTableRegistrationSuccess;
}

BOOL WINAPI
UnregisterLeasingApplicationFromRefCounting(
    HANDLE LeasingApplicationHandle
    )

/*++
 
Routine Description:
 
    Internal routine. Removes a leasing application ref count.

Parameters Description:
 
    LeasingApplicationHandle - leasing application handle to unregister.
 
Return Value:
 
    TRUE if the leasing application ref count was removed.
 
--*/

{
    //
    // Check arguments.
    //
    if (NULL == LeasingApplicationHandle || 
        INVALID_HANDLE_VALUE == LeasingApplicationHandle) {

        return FALSE;
    }

    LeaseLayerEvents.UnregisterFromRefCount(LONGLONG(LeasingApplicationHandle));

    {
        AcquireExclusiveLock lock(*LeaseLayerLock);

        {
        LEASING_APPLICATION_REF_COUNT_HASH_TABLE_ITERATOR IsFound;

        //
        // Make sure the entry is there.
        //
        IsFound = LeasingApplicationRefCountHashTable->find(LeasingApplicationHandle);
        ASSERT_IFNOT(IsFound != LeasingApplicationRefCountHashTable->end(), 
            "UnregisterLeasingApplicationFromRefCounting leasing application {0} is not in the refcount hash table ",
            LeasingApplicationHandle);
        ASSERT_IFNOT(TRUE == IsFound->second.IsClosed, "leasing application is not closed ");

        //
        // Close event.
        //
        IsFound->second.CloseEvent.reset();

        //
        // Erase the entry.
        //
        LeasingApplicationRefCountHashTable->erase(IsFound);
        }

    }

    return TRUE;
}

BOOL WINAPI
UnregisterLeasingApplicationWaitForZeroRefCount(
    HANDLE LeasingApplicationHandle
    )

/*++
 
Routine Description:
 
    Internal routine. Waits for a leasing application ref count to go to 0.

Parameters Description:
 
    LeasingApplicationHandle - leasing applicaiton handle to wait on.
 
Return Value:
 
    TRUE if the leasing application is not closed.
 
--*/

{
    BOOL IsZeroRefCount = FALSE;
    BOOL RetVal = FALSE;
    shared_ptr<ManualResetEvent> CloseRefCountEvent;

    //
    // Check arguments.
    //
    if (NULL == LeasingApplicationHandle || 
        INVALID_HANDLE_VALUE == LeasingApplicationHandle) {

        SetLastError(ERROR_INVALID_HANDLE);
        
        return RetVal;
    }

    LeaseLayerEvents.UnregisterWaitForZeroRefCount(LONGLONG(LeasingApplicationHandle));

    {
        AcquireExclusiveLock lock(*LeaseLayerLock);

        LEASING_APPLICATION_REF_COUNT_HASH_TABLE_ITERATOR IsFound;

        //
        // Make sure the entry is there.
        //
        IsFound = LeasingApplicationRefCountHashTable->find(LeasingApplicationHandle);
        RetVal = (IsFound != LeasingApplicationRefCountHashTable->end());

        if (RetVal) {

                //
                // Retrieve event.
                //
                CloseRefCountEvent = IsFound->second.CloseEvent;

                //
                // Mark the leasing application as closing.
                //
                IsZeroRefCount = CloseRef(&IsFound->second);
        }

    }

    //
    // Wait for the last callback to complete.
    //
    if (!IsZeroRefCount) {

        ASSERT_IFNOT(TRUE == RetVal, 
            "UnregisterLeasingApplicationWaitForZeroRefCount leasing application {0} is not in the refcount hash table", 
            LeasingApplicationHandle);
        auto closed = CloseRefCountEvent->WaitOne(TimeSpan::FromSeconds(30));
        ASSERT_IFNOT(closed, "WaitForSingleObject does not reach zero ref count");
    }

    return RetVal;
}

BOOL WINAPI
LeasingApplicationChangeRefCount(
    POVERLAPPED_LEASE_LAYER_EXTENSION Overlapped,
    HANDLE LeasingApplicationHandle,
    BOOL IsIncrement
    )

/*++
 
Routine Description:
 
    Internal routine. Increment or decrements a leasing application ref count.

Parameters Description:
 
    LeasingApplicationHandle - leasing applicaiton handle to work on.

    IsIncrement - TRUE for increment, FALSE for decrement.
 
Return Value:
 
    TRUE if the leasing application changed ref count appropriately.
 
--*/

{
    BOOL RefCountIncreased = TRUE;

    //
    // Check arguments.
    //
    if (NULL == Overlapped) {
        LeaseLayerEvents.ChangeRefCountNullOverlapped(LONGLONG(0), IsIncrement);
        return FALSE;
    }

    if (NULL == LeasingApplicationHandle || 
        INVALID_HANDLE_VALUE == LeasingApplicationHandle) {

        LeaseLayerEvents.ChangeRefCountInvalidHandle(LONGLONG(LeasingApplicationHandle), IsIncrement);

        return FALSE;
    }

    {
        AcquireExclusiveLock lock(*LeaseLayerLock);

        LEASING_APPLICATION_REF_COUNT_HASH_TABLE_ITERATOR IsFound;

        //
        // Make sure the entry is there.
        //
        IsFound = LeasingApplicationRefCountHashTable->find(LeasingApplicationHandle);

        if (IsFound == LeasingApplicationRefCountHashTable->end()) {

            LeaseLayerEvents.ChangeRefCountNotFound(LONGLONG(LeasingApplicationHandle), IsIncrement);
            RefCountIncreased = FALSE;

        }
//TODO shuxu check this to see if it is needed
//else if (IsFound->second.Overlapped != Overlapped) {
//
//            LeaseLayerEvents.ChangeRefCountOverlappedMismatch(LONGLONG(LeasingApplicationHandle), IsIncrement);
//            RefCountIncreased = FALSE;
//
//        }
        else {

            //
            // Change ref count.
            //
            if (IsIncrement) {

                RefCountIncreased = AddRef(&IsFound->second);

            } else {

                ReleaseRef(&IsFound->second); 
            }

        }
    }

    return RefCountIncreased;
}

void
CrashLeasingApplication()
{
    DWORD BytesReturned = 0;

    //
    // Create a device IOCTL and send it to the device.
    //
    DeviceIoControl(
        IOCTL_CRASH_LEASING_APPLICATION,
        NULL,
        0,
        NULL,
        0,
        &BytesReturned
        );
}

BOOL WINAPI
InternalRegisterLeasingApplication(
    __in POVERLAPPED_LEASE_LAYER_EXTENSION OverlappedDeviceIoctlRegister
    )
/*
Routine Description:
 
    Internal routine. Creates or renews a leasing application registration.

Parameters Description:
 
    OverlappedDeviceIoctlRegister - overlapped to renew.
 
Return Value:
 
    TRUE if the I/O has been posted successfully.
*/
{
    DWORD BytesReturned = 0;
    DWORD LastError = ERROR_SUCCESS;

    //
    // Must call this function before initiating each asynchronous
    // device I/O operation on the handle when using thread pool I/O.
    //

    //
    // Send the register leasing application Device IOCTL.
    // This IOCTL will be delayed in the kernel. It will be completed
    // at a later time when an event of interest to the application
    // will be raised.
    //
    auto DeviceIoctlReturn = DeviceIoControl(
        IOCTL_REGISTER_LEASING_APPLICATION,
        &OverlappedDeviceIoctlRegister->EventBuffer,
        sizeof(LEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER),
        &OverlappedDeviceIoctlRegister->EventBuffer,
        sizeof(LEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER),
        &BytesReturned);

    //
    // Check return code of DeviceIoControl.
    //
    if (!NT_SUCCESS(DeviceIoctlReturn)) {

        //
        // Retrieve last error.
        //
        LastError = GetLastError();

        //
        // Check this exact error code. Any other error code is a failure.
        //
        if (ERROR_IO_PENDING != LastError) {
            return FALSE;
        }
    }

    Common::Assert::SetCrashLeasingApplicationCallback(CrashLeasingApplication);

    return TRUE;
}

HANDLE WINAPI 
CreateLeaseAgent(
    __in PTRANSPORT_LISTEN_ENDPOINT SocketAddress,
    __in PLEASE_CONFIG_DURATIONS LeaseConfigDurations,
    __in LONG LeaseSuspendDurationMilliseconds,
    __in LONG ArbitrationDurationMilliseconds,
    __in LONG NumberOfLeaseRetry,
    __in LONG StartOfLeaseRetry,
    Transport::SecuritySettings const* securitySettings,
    __inout_opt PLONGLONG Instance
    )

/*++
 
Routine Description:
 
    Creates a lease agent. A lease agent is uniquely identified on a machine by the PTRANSPORT_LISTEN_ENDPOINT 
    information. Local leasing applications will be bound always to a lease agent. If 
    the PTRANSPORT_LISTEN_ENDPOINT information has been used previously to create a lease agent, the already 
    existent lease agent will be returned.
 
Parameters Description:
 
    SocketAddress - local network address information used to bind the lease agent. 
        It contains information about the listening address and socket used to receive 
        incoming lease requests.
 
Return Value:
 
    HANDLE to a successfully created lease agent structure, NULL 
    otherwise. Call GetLastError to retrieve the actual error that occured.
 
--*/

{
    DWORD BytesReturned = 0;
    HRESULT WcharStringResult = E_FAIL;

    CREATE_LEASE_AGENT_INPUT_BUFFER_DEVICE_IOCTL DeviceIoctlInputBuffer;
    ZeroMemory(&DeviceIoctlInputBuffer, sizeof(CREATE_LEASE_AGENT_INPUT_BUFFER_DEVICE_IOCTL));

    LEASE_AGENT_BUFFER_DEVICE_IOCTL DeviceIoctlOutputBuffer;
    ZeroMemory(&DeviceIoctlOutputBuffer, sizeof(LEASE_AGENT_BUFFER_DEVICE_IOCTL));

    //
    // Check to see if the user mode lease layer 
    // has been successfully initialized.
    //
    if (!IsLeaseLayerInitialized()) {

        return NULL;
    }

    //
    // Check arguments.
    //
    if (NULL == SocketAddress) {

        SetLastError(ERROR_INVALID_PARAMETER);

        return NULL;
    }

    //
    // Initialize device IOCTL input buffer.
    //
    if (memcpy_s(
        &DeviceIoctlInputBuffer.SocketAddress,
        sizeof(TRANSPORT_LISTEN_ENDPOINT),
        SocketAddress,
        sizeof(TRANSPORT_LISTEN_ENDPOINT)
        ))
    {
        return NULL;
    }

    // Initialize lease config durations struct
    if (memcpy_s(
        &DeviceIoctlInputBuffer.LeaseConfigDurations,
        sizeof(LEASE_CONFIG_DURATIONS),
        LeaseConfigDurations,
        sizeof(LEASE_CONFIG_DURATIONS)
        ))
    {
        return NULL;
    }

    DeviceIoctlInputBuffer.LeaseSuspendDurationMilliseconds = LeaseSuspendDurationMilliseconds;
    DeviceIoctlInputBuffer.ArbitrationDurationMilliseconds = ArbitrationDurationMilliseconds;
    DeviceIoctlInputBuffer.NumberOfLeaseRetry = NumberOfLeaseRetry;
    DeviceIoctlInputBuffer.StartOfLeaseRetry = StartOfLeaseRetry;
    DeviceIoctlInputBuffer.SecuritySettings = securitySettings;
    DeviceIoctlInputBuffer.UserModeVersion = LEASE_DRIVER_VERSION;

    //
    // Create a device IOCTL and send it to the device.
    //
    auto DeviceIoctlReturn = DeviceIoControl(
        IOCTL_LEASING_CREATE_LEASE_AGENT,
        &DeviceIoctlInputBuffer,
        sizeof(CREATE_LEASE_AGENT_INPUT_BUFFER_DEVICE_IOCTL),
        &DeviceIoctlOutputBuffer,
        sizeof(LEASE_AGENT_BUFFER_DEVICE_IOCTL),
        &BytesReturned);

    //
    // Check return code of DeviceIoControl.
    //
    if (!NT_SUCCESS(DeviceIoctlReturn)) {

        //
        // check the returned version number
        //
        if (BytesReturned != 0 && DeviceIoctlOutputBuffer.KernelModeVersion != LEASE_DRIVER_VERSION)
        {
            LeaseLayerEvents.UserKernelVersionMismatch(LEASE_DRIVER_VERSION, DeviceIoctlOutputBuffer.KernelModeVersion);
            SetLastError(ERROR_REVISION_MISMATCH);
        }

        ASSERT_IFNOT(NULL == DeviceIoctlOutputBuffer.LeaseAgentHandle, "CreateLeaseAgent LeaseAgentHandle is not NULL ");

        return NULL;
    }

    ASSERT_IFNOT(BytesReturned == sizeof(LEASE_AGENT_BUFFER_DEVICE_IOCTL), "CreateLeaseAgent BytesReturned is wrong");

    if (Instance != NULL) {
        *Instance = DeviceIoctlOutputBuffer.LeaseAgentInstance;
    }

    //
    // Return lease agent handle.
    //
    return DeviceIoctlOutputBuffer.LeaseAgentHandle;
}

BOOL WINAPI
CloseLeaseAgentInternal(
    __in PTRANSPORT_LISTEN_ENDPOINT SocketAddress,
    __in BOOL CleanUpImmideately
    )

/*++
 
Routine Description:
 
    Closes and frees all resources associated with a lease agent. 
    This will unregister all applications previously registered for this lease agent.
    The timing of the cleanup would depend on the flag passed. If the flag is true
    the clean up would be immideate. If not the clean up will happen when the 
    leases expir.

Parameters Description:
 
    SocketAddress - socket address of lease agent to be closed.
    CleanUpImmideately - Whether to clean up state immideately.
 
Return Value:
 
    TRUE if lease agent was successfully uninitialized, FALSE otherwise. 
    Call GetLastError to retrieve the actual error that occured.
 
--*/

{
    DWORD BytesReturned = 0;

    CLOSE_LEASE_AGENT_BUFFER_DEVICE_IOCTL DeviceIoctlInputBuffer;
    ZeroMemory(&DeviceIoctlInputBuffer, sizeof(CLOSE_LEASE_AGENT_BUFFER_DEVICE_IOCTL));

    //
    // Check to see if the user mode lease layer 
    // has been successfully initialized.
    //
    if (!IsLeaseLayerInitialized()) {

        return FALSE;
    }

    //
    // Check arguments.
    //
    if (NULL == SocketAddress) {

        SetLastError(ERROR_INVALID_PARAMETER);

        return FALSE;
    }

    //
    // Initialize device IOCTL input buffer.
    //
    if (memcpy_s(
        &DeviceIoctlInputBuffer.SocketAddress,
        sizeof(TRANSPORT_LISTEN_ENDPOINT),
        SocketAddress,
        sizeof(TRANSPORT_LISTEN_ENDPOINT)
        ))
    {
        return FALSE;
    }

    //
    // Create a device IOCTL and send it to the device.
    //
    if (CleanUpImmideately) {
        return NT_SUCCESS(DeviceIoControl(
            IOCTL_LEASING_CLOSE_LEASE_AGENT,
            &DeviceIoctlInputBuffer,
            sizeof(CLOSE_LEASE_AGENT_BUFFER_DEVICE_IOCTL),
            NULL,
            0,
            &BytesReturned))? TRUE : FALSE;
    }
    else {
        return NT_SUCCESS(DeviceIoControl(
            IOCTL_LEASING_BLOCK_LEASE_AGENT,
            &DeviceIoctlInputBuffer,
            sizeof(CLOSE_LEASE_AGENT_BUFFER_DEVICE_IOCTL),
            NULL,
            0,
            &BytesReturned))? TRUE : FALSE;
    }
}



BOOL WINAPI 
CloseLeaseAgent(
    __in PTRANSPORT_LISTEN_ENDPOINT SocketAddress
    )

/*++
 
Routine Description:
 
    Closes and frees all resources associated with a lease agent. 
    This will unregister all applications previously registered for this lease agent.

Parameters Description:
 
    SocketAddress - socket address of lease agent to be closed.
 
Return Value:
 
    TRUE if lease agent was successfully uninitialized, FALSE otherwise. 
    Call GetLastError to retrieve the actual error that occured.
 
--*/

{
    return CloseLeaseAgentInternal(SocketAddress,TRUE);
}

BOOL WINAPI
FaultLeaseAgent(
    __in PTRANSPORT_LISTEN_ENDPOINT SocketAddress
    )
    

/*++
 
Routine Description:
 
    Faults a lease agent by closing it.

Parameters Description:
 
    SocketAddress - socket address of lease agent to be closed.
 
Return Value:
 
    TRUE if lease agent was successfully uninitialized, FALSE otherwise. 
    Call GetLastError to retrieve the actual error that occured.
 
--*/

{
    return CloseLeaseAgent(SocketAddress);
}

BOOL WINAPI
BlockLeaseAgent(
    __in PTRANSPORT_LISTEN_ENDPOINT SocketAddress
    )

/*++
 
Routine Description:
 
    Faults a lease agent eventually by blocking it. The channels are disabled so as to ensure
    a lease failure.

Parameters Description:
 
    SocketAddress - socket address of lease agent to be closed.
 
Return Value:
 
    TRUE if lease agent was successfully uninitialized, FALSE otherwise. 
    Call GetLastError to retrieve the actual error that occured.
 
--*/

{
    return CloseLeaseAgentInternal(SocketAddress, FALSE);
}

VOID WINAPI
DefaultLeasingApplicationLeaseEstablished(
    __in HANDLE Lease,
    __in LPCWSTR RemoteLeasingApplicationIdentifier,
    __in PVOID Context
    )

/*++
 
Routine Description:
 
    Empty implementation of the lease establihs callback. If the application
    choose to not set its lease establish callback, this routine will be used.
 
Parameters Description:

    Lease - lease handle.

    RemoteLeasingApplicationIdentifier � remot eleasing application (unused).

    Context - leasing application context (unused).

Return Value:
 
    n/a
 
--*/

{
    //
    // Does nothing.
    //
    UNREFERENCED_PARAMETER(Lease);
    UNREFERENCED_PARAMETER(RemoteLeasingApplicationIdentifier);
    UNREFERENCED_PARAMETER(Context);
}

HANDLE WINAPI 
RegisterLeasingApplication(
    __in PTRANSPORT_LISTEN_ENDPOINT SocketAddress,
    __in LPCWSTR LeasingApplicationIdentifier,
    __in PLEASE_CONFIG_DURATIONS LeaseConfigDurations,
    __in LONG LeaseSuspendDurationMilliseconds,
    __in LONG ArbitrationDurationMilliseconds,
    __in LONG NumOfLeaseRetry,
    __in LONG StartOfLeaseRetry,
    void const* securitySettings,
    __in LONG LeasingAppExpiryTimeoutMilliseconds,
    __in LEASING_APPLICATION_EXPIRED_CALLBACK LeasingApplicationExpired,
    __in REMOTE_LEASING_APPLICATION_EXPIRED_CALLBACK RemoteLeasingApplicationExpired,
    __in_opt LEASING_APPLICATION_ARBITRATE_CALLBACK LeasingApplicationArbitrate,
    __in_opt LEASING_APPLICATION_LEASE_ESTABLISHED_CALLBACK LeasingApplicationLeaseEstablished,
    __in_opt PVOID CallbackContext,
    __out_opt PLONGLONG Instance,
    __out_opt PHANDLE leaseAgentHandle
    )

/*++
 
Routine Description:
 
    Registers a local leasing application with the lease layer.
 
Parameters Description:
 
    LeaseAgent � handle to a registered lease agent.

    LeasingApplicationIdentifier - unique identifier of the leasing application. This ideintifier 
        is unique to the lease agent (and not the machine or cluster).

    LeasingApplicationExpired - callback routine invoked when the lease has expired.

    RemoteLeasingApplicationExpired - callback routine invoked when the remote application has expired.

    LeasingApplicationArbitrate - callback routine used for arbitration.

    LeasingApplicationLeaseEstablished - callback routine used for lease establishment.

    CallbackContext - context for the callback routines.

Return Value:
 
    HANDLE to a successfully created leasing application structure, NULL otherwise. Call GetLastError 
    to retrieve the actual error that occured.
 
--*/

{
    HRESULT WcharStringResult = E_FAIL;
    size_t WcharLength = 0;
    BOOL RefCountRegistered = FALSE;
    DWORD BytesReturned = 0;
    HANDLE LeaseAgent = NULL;

    if (Instance != NULL) 
    {
        *Instance = 0;
    }

    ASSERT_IF(leaseAgentHandle == NULL, "RegisterLeasingApplication lease agent handle is NULL");

    POVERLAPPED_LEASE_LAYER_EXTENSION OverlappedDeviceIoctlRegister = NULL;

    CREATE_LEASING_APPLICATION_INPUT_BUFFER_DEVICE_IOCTL DeviceIoctlCreateInputBuffer;
    ZeroMemory(&DeviceIoctlCreateInputBuffer, sizeof(CREATE_LEASING_APPLICATION_INPUT_BUFFER_DEVICE_IOCTL));

    LEASING_APPLICATION_BUFFER_DEVICE_IOCTL DeviceIoctlCreateOutputBuffer;
    ZeroMemory(&DeviceIoctlCreateOutputBuffer, sizeof(LEASING_APPLICATION_BUFFER_DEVICE_IOCTL));

    //
    // Check to see if the user mode lease layer 
    // has been successfully initialized.
    //

    if (!IsInitializeCalled)
    {
        AcquireExclusiveLock lock(*LeaseLayerLock);
        {
            if (!IsInitializeCalled)
            {
                InitializeLeaseLayer();
                IsInitializeCalled = TRUE;
            }
        }
    }

    if (!IsLeaseLayerInitialized()) {

        return NULL;
    }

    //
    // Check arguments.
    //
    if (NULL == SocketAddress ||
        NULL == LeasingApplicationIdentifier ||
        INVALID_HANDLE_VALUE == LeasingApplicationIdentifier ||
        NULL == LeasingApplicationExpired ||
        NULL == RemoteLeasingApplicationExpired ||
        S_OK != StringCchLength(
            LeasingApplicationIdentifier,
            MAX_PATH + 1,
            &WcharLength
            ))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    //
    // Create lease agent first.
    //
    LeaseAgent = CreateLeaseAgent(
        SocketAddress,
        LeaseConfigDurations,
        LeaseSuspendDurationMilliseconds,
        ArbitrationDurationMilliseconds,
        NumOfLeaseRetry,
        StartOfLeaseRetry,
        (Transport::SecuritySettings const*)securitySettings,
        Instance
        );

    if (NULL == LeaseAgent) {

        return NULL;
    }

    *leaseAgentHandle = LeaseAgent;
    //
    // Populate device IOCTL input buffer.
    //
    DeviceIoctlCreateInputBuffer.LeaseAgentHandle = LeaseAgent;
    WcharStringResult = StringCchCopy(
        DeviceIoctlCreateInputBuffer.LeasingApplicationIdentifier,
        MAX_PATH + 1,
        LeasingApplicationIdentifier
        );
    ASSERT_IFNOT(S_OK == WcharStringResult, "RegisterLeasingApplication StringCchCopy failed ");
    DeviceIoctlCreateInputBuffer.IsArbitrationEnabled = (NULL != LeasingApplicationArbitrate);
    DeviceIoctlCreateInputBuffer.LeasingAppExpiryTimeoutMilliseconds = LeasingAppExpiryTimeoutMilliseconds;

    //Setup callback related
    OverlappedDeviceIoctlRegister = &(DeviceIoctlCreateInputBuffer.Callbacks);
    OverlappedDeviceIoctlRegister->EventBuffer.LeaseAgentHandle = LeaseAgent;
    OverlappedDeviceIoctlRegister->LeasingApplicationExpired = LeasingApplicationExpired;
    OverlappedDeviceIoctlRegister->RemoteLeasingApplicationExpired = RemoteLeasingApplicationExpired;
    OverlappedDeviceIoctlRegister->LeasingApplicationArbitrate = LeasingApplicationArbitrate;
    OverlappedDeviceIoctlRegister->CallbackContext = CallbackContext;
    OverlappedDeviceIoctlRegister->LeasingApplicationLeaseEstablished = (NULL != LeasingApplicationLeaseEstablished) ?
                                                                        LeasingApplicationLeaseEstablished :
                                                                        DefaultLeasingApplicationLeaseEstablished;

    //
    // Send the create leasing application IOCTL.
    //
    auto DeviceIoctlReturn = DeviceIoControl(
        IOCTL_CREATE_LEASING_APPLICATION,
        &DeviceIoctlCreateInputBuffer,
        sizeof(CREATE_LEASING_APPLICATION_INPUT_BUFFER_DEVICE_IOCTL),
        &DeviceIoctlCreateOutputBuffer, 
        sizeof(LEASING_APPLICATION_BUFFER_DEVICE_IOCTL),
        &BytesReturned);

    OverlappedDeviceIoctlRegister->EventBuffer.LeasingApplicationHandle = DeviceIoctlCreateOutputBuffer.LeasingApplicationHandle;
    //
    // Check return code of DeviceIoControl.
    //
    if (!NT_SUCCESS(DeviceIoctlReturn)) {

        //
        // The output buffer should be untouched.
        //
        ASSERT_IFNOT(NULL == DeviceIoctlCreateOutputBuffer.LeasingApplicationHandle, "RegisterLeasingApplication LeasingApplicationHandle is not NULL ");

        return NULL;
    }

    //
    // We are expecting non-empty output buffer.
    //
    ASSERT_IFNOT(sizeof(LEASING_APPLICATION_BUFFER_DEVICE_IOCTL) == BytesReturned, "RegisterLeasingApplication BytesReturned is wrong ");
    ASSERT_IFNOT(NULL != DeviceIoctlCreateOutputBuffer.LeasingApplicationHandle, "RegisterLeasingApplication is NULL ");


    //
    // Register the leasing application handle with the ref counting hash table.
    //
    RefCountRegistered = RegisterLeasingApplicationForRefCounting(OverlappedDeviceIoctlRegister, DeviceIoctlCreateOutputBuffer.LeasingApplicationHandle);
    if (!RefCountRegistered) {

        goto Error;
    }

    //
    // Register application.
    //
    if (!InternalRegisterLeasingApplication(OverlappedDeviceIoctlRegister)) {


        LeaseLayerEvents.EventPumpIOError(LONGLONG(DeviceIoctlCreateOutputBuffer.LeasingApplicationHandle));
        goto Error;
    }

    //
    // Success.
    //
    return DeviceIoctlCreateOutputBuffer.LeasingApplicationHandle;

Error:

    if (NULL != DeviceIoctlCreateOutputBuffer.LeasingApplicationHandle) {

        //
        // Unregister application from the ref counting hash table.
        //
        if (RefCountRegistered) {

            UnregisterLeasingApplicationFromRefCounting(
                DeviceIoctlCreateOutputBuffer.LeasingApplicationHandle
                );
        }

        //
        // Unregister application.
        //
        UnregisterLeasingApplication(
            DeviceIoctlCreateOutputBuffer.LeasingApplicationHandle,
            FALSE
            );
    }

    return NULL;
}

BOOL WINAPI 
UnregisterLeasingApplication(
    __in HANDLE LeasingApplicationHandle,
    __in BOOL IsDelayed
    )

/*++
 
Routine Description:
 
    Unregisters a local application with the lease layer and frees all resources associated with it.
    This will terminate all leases associated with this application.
 
Parameters Description:
 
    LeasingApplicationHandle - leasing application handle to unregister.
 
Return Value:
 
    TRUE if the local application was successfully unregistered with the lease agent. Call GetLastError 
    to retrieve the actual error that occured.
 
--*/

{
    DWORD BytesReturned = 0;

    UNREGISTER_LEASING_APPLICATION_INPUT_BUFFER_DEVICE_IOCTL DeviceIoctlInputBuffer;
    ZeroMemory(&DeviceIoctlInputBuffer, sizeof(UNREGISTER_LEASING_APPLICATION_INPUT_BUFFER_DEVICE_IOCTL));

    //
    // Check to see if the user mode lease layer 
    // has been successfully initialized.
    //
    if (!IsLeaseLayerInitialized()) {

        return FALSE;
    }

    //
    // Check arguments.
    //
    if (NULL == LeasingApplicationHandle || 
        INVALID_HANDLE_VALUE == LeasingApplicationHandle) {

        SetLastError(ERROR_INVALID_PARAMETER);

        return FALSE;
    }

    //
    // Wait for the leasing application ref count to go to zero.
    //
    if (!UnregisterLeasingApplicationWaitForZeroRefCount(LeasingApplicationHandle)) {

        SetLastError(ERROR_INVALID_PARAMETER);

        return FALSE;
    }

    //
    // Unregister the leasing application form the ref counting hash table.
    //
    auto boolRetval = UnregisterLeasingApplicationFromRefCounting(LeasingApplicationHandle);
    ASSERT_IFNOT(TRUE == boolRetval, "UnregisterLeasingApplication FromRefCounting failed");

    //
    // Initialize device IOCTL input buffer.
    //
    DeviceIoctlInputBuffer.LeasingApplicationHandle = LeasingApplicationHandle;
    DeviceIoctlInputBuffer.IsDelayed = (BOOLEAN) IsDelayed;

    //
    // Create a device IOCTL and send it to the device.
    //
    auto DeviceIoControlReturn = DeviceIoControl(
        IOCTL_UNREGISTER_LEASING_APPLICATION,
        &DeviceIoctlInputBuffer,
        sizeof(UNREGISTER_LEASING_APPLICATION_INPUT_BUFFER_DEVICE_IOCTL),
        NULL,
        0,
        &BytesReturned);

    return NT_SUCCESS(DeviceIoControlReturn)? TRUE : FALSE;
}

__success(return != NULL)
HANDLE WINAPI 
EstablishLease(
    __in HANDLE LeasingApplication,
    __in LPCWSTR RemoteApplicationIdentifier,
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress,
    __in LONGLONG RemoteLeaseAgentInstance,
    __in LEASE_DURATION_TYPE LeaseDurationType,
    __out PBOOL IsEstablished
    )

/*++
 
Routine Description:
 
    Establishes a lease relationship between a local and a remote application.
 
Parameters Description:
 
    LeasingApplication - leasing application handle.

    RemoteApplicationIdentifier - unique identifier of the remote application.

    RemoteSocketAddress - network address information used to bind the remote lease agent to.
 
Return Value:
 
    HANDLE to a successfully created lease structure, NULL otherwise. Call GetLastError 
    to retrieve the actual error that occured.
 
--*/

{
    HRESULT WcharStringResult = E_FAIL;
    DWORD BytesReturned = 0;
    size_t WcharLength = 0;

    ESTABLISH_LEASE_INPUT_BUFFER_DEVICE_IOCTL DeviceIoctlEstablishInputBuffer;
    ZeroMemory(&DeviceIoctlEstablishInputBuffer, sizeof(ESTABLISH_LEASE_INPUT_BUFFER_DEVICE_IOCTL));

    ESTABLISH_LEASE_OUTPUT_BUFFER_DEVICE_IOCTL DeviceIoctlEstablishOutputBuffer;
    ZeroMemory(&DeviceIoctlEstablishOutputBuffer, sizeof(ESTABLISH_LEASE_OUTPUT_BUFFER_DEVICE_IOCTL));

    //
    // Check to see if the user mode lease layer 
    // has been successfully initialized.
    //
    if (!IsLeaseLayerInitialized()) {

        return NULL;
    }

    //
    // Check arguments.
    //
    if (NULL == LeasingApplication ||
        NULL == RemoteSocketAddress ||
        INVALID_HANDLE_VALUE == LeasingApplication ||
        NULL == RemoteApplicationIdentifier ||
        S_OK != StringCchLength(
            RemoteApplicationIdentifier,
            MAX_PATH + 1,
            &WcharLength
            )
        ) {

        SetLastError(ERROR_INVALID_PARAMETER);

        return NULL;
    }

    //
    // Populate establish input buffer.
    //
    
    DeviceIoctlEstablishInputBuffer.LeasingApplicationHandle = LeasingApplication;
    if (memcpy_s(
        &DeviceIoctlEstablishInputBuffer.RemoteSocketAddress,
        sizeof(TRANSPORT_LISTEN_ENDPOINT),
        RemoteSocketAddress,
        sizeof(TRANSPORT_LISTEN_ENDPOINT)
        ))
    {
        return NULL;
    }

    WcharStringResult = StringCchCopy(
        DeviceIoctlEstablishInputBuffer.RemoteLeasingApplicationIdentifier,
        MAX_PATH + 1,
        RemoteApplicationIdentifier
        );
    ASSERT_IFNOT(S_OK == WcharStringResult, "EstablishLease StringCchCopy failed ");
    
    DeviceIoctlEstablishInputBuffer.RemoteLeaseAgentInstance = RemoteLeaseAgentInstance;
    DeviceIoctlEstablishInputBuffer.LeaseDurationType = LeaseDurationType;

    //
    // Send the create lease Device IOCTL.
    //
    auto DeviceIoctlReturn = DeviceIoControl(
        IOCTL_ESTABLISH_LEASE,
        &DeviceIoctlEstablishInputBuffer,
        sizeof(ESTABLISH_LEASE_INPUT_BUFFER_DEVICE_IOCTL),
        &DeviceIoctlEstablishOutputBuffer, 
        sizeof(ESTABLISH_LEASE_OUTPUT_BUFFER_DEVICE_IOCTL),
        &BytesReturned);

    //
    // Check return code of DeviceIoControl.
    //
    if (!NT_SUCCESS(DeviceIoctlReturn)) {

        //
        // The output buffer should be untouched.
        //
        ASSERT_IFNOT(NULL == DeviceIoctlEstablishOutputBuffer.LeaseHandle, "EstablishLease LeaseHandle is not NULL ");

        return NULL;
    }

    //
    // We are expecting non-empty output buffer.
    //
    ASSERT_IFNOT(sizeof(ESTABLISH_LEASE_OUTPUT_BUFFER_DEVICE_IOCTL) == BytesReturned, "EstablishLease BytesReturned is wrong ");
    ASSERT_IFNOT(NULL != DeviceIoctlEstablishOutputBuffer.LeaseHandle, "EstablishLease LeaseHandle is NULL ");

    *IsEstablished = DeviceIoctlEstablishOutputBuffer.IsEstablished;

    return DeviceIoctlEstablishOutputBuffer.LeaseHandle;
}
 
BOOL WINAPI 
TerminateLease(
    __in HANDLE LeasingApplication,
    __in HANDLE Lease,
    __in LPCWSTR RemoteApplicationIdentifier
    )

/*++
 
Routine Description:
 
    Terminates a lease relationship between a local and a remote application and 
    frees all resources associated with it.
 
Parameters Description:
 
    Lease - lease handle.
 
Return Value:
 
    TRUE if the lease relationship has successfully been terminated. Call GetLastError 
    to retrieve the actual error that occured.
 
--*/

{
    DWORD BytesReturned = 0;
    HRESULT WcharStringResult = E_FAIL;
    size_t WcharLength = 0;

    TERMINATE_LEASE_BUFFER_DEVICE_IOCTL DeviceIoctlInputBuffer;
    ZeroMemory(&DeviceIoctlInputBuffer, sizeof(TERMINATE_LEASE_BUFFER_DEVICE_IOCTL));

    //
    // Check to see if the user mode lease layer 
    // has been successfully initialized.
    //
    if (!IsLeaseLayerInitialized()) {

        return FALSE;
    }

    //
    // Check arguments.
    //
    if (NULL == Lease ||
        INVALID_HANDLE_VALUE == Lease ||
        S_OK != StringCchLength(
            RemoteApplicationIdentifier,
            MAX_PATH + 1,
            &WcharLength)
        )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Initialize device IOCTL input buffer.
    //
    DeviceIoctlInputBuffer.LeasingApplicationHandle = LeasingApplication;
    
    DeviceIoctlInputBuffer.LeaseHandle = Lease;

    WcharStringResult = StringCchCopy(
        DeviceIoctlInputBuffer.RemoteLeasingApplicationIdentifier,
        MAX_PATH + 1,
        RemoteApplicationIdentifier
        );
    ASSERT_IFNOT(S_OK == WcharStringResult, "TerminateLease StringCchCopy failed ");

    //
    // Create a device IOCTL and send it to the device.
    //
    return NT_SUCCESS(DeviceIoControl(
        IOCTL_TERMINATE_LEASE,
        &DeviceIoctlInputBuffer,
        sizeof(TERMINATE_LEASE_BUFFER_DEVICE_IOCTL),
        NULL,
        0,
        &BytesReturned))? TRUE : FALSE;
}

__success(return == TRUE)
BOOL WINAPI 
GetLeasingApplicationExpirationTime(
    __in HANDLE LeasingApplication,
    __in LONG RequestTimeToLiveMilliseconds,
    __out PLONG RemainingTimeToLiveMilliseconds,
    __out PLONGLONG KernelCurrentTime
    )

/*++
 
Routine Description:
 
    Gets the time for which the local application is guaranteed to have valid leases.
 
Parameters Description:
 
    LeaseApplication - lease application handle.

    RemainingTimeToLiveMilliseconds - on return, it contains the current lease 
        expiration time in milliseconds.

    KernelCurrentTime - on return, contains the kernel system time
 
Return Value:
 
    TRUE if arguments are valid, FALSE otherwise.
 
--*/

{
    DWORD BytesReturned = 0;

    GET_LEASING_APPLICATION_EXPIRATION_INPUT_BUFFER DeviceIoctlGetTTLInputBuffer;
    ZeroMemory(&DeviceIoctlGetTTLInputBuffer, sizeof(GET_LEASING_APPLICATION_EXPIRATION_INPUT_BUFFER));
    GET_LEASING_APPLICATION_EXPIRATION_OUTPUT_BUFFER DeviceIoctlGetTTLOutputBuffer;
    ZeroMemory(&DeviceIoctlGetTTLOutputBuffer, sizeof(GET_LEASING_APPLICATION_EXPIRATION_OUTPUT_BUFFER));

    //
    // Check to see if the user mode lease layer 
    // has been successfully initialized.
    //

    if (!IsInitializeCalled)
    {
        AcquireExclusiveLock lock(*LeaseLayerLock);
        {
            if (!IsInitializeCalled)
            {
                InitializeLeaseLayer();
                IsInitializeCalled = TRUE;
            }
        }
    }

    if (!IsLeaseLayerInitialized()) {

        return FALSE;
    }

    //
    // Check arguments.
    //
    if (NULL == RemainingTimeToLiveMilliseconds ||
        NULL == KernelCurrentTime ||
        NULL == LeasingApplication ||
        INVALID_HANDLE_VALUE == LeasingApplication
        ) {

        SetLastError(ERROR_INVALID_PARAMETER);

        return FALSE;
    }

    //
    // Populate device IOCTL input buffer.
    //
    DeviceIoctlGetTTLInputBuffer.LeasingApplicationHandle = LeasingApplication;
    DeviceIoctlGetTTLInputBuffer.RequestTimeToLive = RequestTimeToLiveMilliseconds;

    if (RequestTimeToLiveMilliseconds > 0)
    {
        LeaseLayerEvents.GetContainerLeasingTTL(LONGLONG(LeasingApplication), RequestTimeToLiveMilliseconds);
    }

    //
    // Send the add leasing application Device IOCTL.
    //
    auto DeviceIoctlReturn = DeviceIoControl(
        IOCTL_GET_LEASING_APPLICATION_EXPIRATION_TIME,
        &DeviceIoctlGetTTLInputBuffer,
        sizeof(GET_LEASING_APPLICATION_EXPIRATION_INPUT_BUFFER),
        &DeviceIoctlGetTTLOutputBuffer, 
        sizeof(GET_LEASING_APPLICATION_EXPIRATION_OUTPUT_BUFFER),
        &BytesReturned);
    *KernelCurrentTime = DeviceIoctlGetTTLOutputBuffer.KernelSystemTime;
    *RemainingTimeToLiveMilliseconds = DeviceIoctlGetTTLOutputBuffer.TimeToLive;
    //
    // Check return code of DeviceIoControl.
    //
    if (NT_SUCCESS(DeviceIoctlReturn)) {
        //
        // We are expecting non-empty output buffer.
        //
        ASSERT_IFNOT(
            sizeof(GET_LEASING_APPLICATION_EXPIRATION_OUTPUT_BUFFER) == BytesReturned,
            "GetLeasingApplicationExpirationTime IOCTL return size is wrong ");
        ASSERT_IFNOT(0 <= *RemainingTimeToLiveMilliseconds, "GetLeasingApplicationExpirationTime RemainingTimeToLiveMilliseconds < 0 ");

        return TRUE;
    }

    return FALSE;
}


BOOL WINAPI 
SetGlobalLeaseExpirationTime(
    __in HANDLE LeasingApplication,
    __in LONGLONG ExpireTime
    )

/*++
 
Routine Description:
 
    Set the time for global lease expired.
 
Parameters Description:
 
    LeaseApplication - lease application handle.

    ExpireTime - global lease expire time
.
Return Value:
 
    TRUE if arguments are valid, FALSE otherwise.
 
--*/

{
    DWORD BytesReturned = 0;

    GLOBAL_LEASE_EXPIRATION_INPUT_BUFFER DeviceIoctlGlobalLeaseInputBuffer;
    ZeroMemory(&DeviceIoctlGlobalLeaseInputBuffer, sizeof(GLOBAL_LEASE_EXPIRATION_INPUT_BUFFER));

    //
    // Check to see if the user mode lease layer 
    // has been successfully initialized.
    //
    if (!IsLeaseLayerInitialized()) {

        return FALSE;
    }

    //
    // Check arguments.
    //
    if (NULL == ExpireTime ||
        NULL == LeasingApplication ||
        INVALID_HANDLE_VALUE == LeasingApplication
        ) {

        SetLastError(ERROR_INVALID_PARAMETER);

        return FALSE;
    }

    //
    // Populate device IOCTL input buffer.
    //
    DeviceIoctlGlobalLeaseInputBuffer.LeasingApplicationHandle = LeasingApplication;
    DeviceIoctlGlobalLeaseInputBuffer.LeaseExpireTime = ExpireTime;
    //
    // Send the add leasing application Device IOCTL.
    //
    auto DeviceIoctlReturn = DeviceIoControl(
        IOCTL_SET_GLOBAL_LEASE_EXPIRATION_TIME,
        &DeviceIoctlGlobalLeaseInputBuffer,
        sizeof(DeviceIoctlGlobalLeaseInputBuffer),
        NULL, 
        0,
        &BytesReturned);

    return NT_SUCCESS(DeviceIoctlReturn)? TRUE : FALSE;
}

BOOL WINAPI
GetRemoteLeaseExpirationTime(
    __in HANDLE LeasingApplication,
    __in LPCWSTR RemoteApplicationIdentifier,
    __out PLONG MonitorExpireTTL,
    __out PLONG SubjectExpireTTL
    )
{
    HRESULT WcharStringResult = E_FAIL;
    DWORD BytesReturned = 0;

    REMOTE_LEASE_EXPIRATION_RESULT_INPUT_BUFFER DeviceIoctlGetRemoteExpirationTimeInputBuffer;
    ZeroMemory(&DeviceIoctlGetRemoteExpirationTimeInputBuffer, sizeof(REMOTE_LEASE_EXPIRATION_RESULT_INPUT_BUFFER));

    REMOTE_LEASE_EXPIRATION_RESULT_OUTPUT_BUFFER DeviceIoctlGetRemoteExpirationTimeOutputBuffer;
    ZeroMemory(&DeviceIoctlGetRemoteExpirationTimeOutputBuffer, sizeof(REMOTE_LEASE_EXPIRATION_RESULT_OUTPUT_BUFFER));

    DeviceIoctlGetRemoteExpirationTimeInputBuffer.LeasingApplicationHandle = LeasingApplication;
    WcharStringResult = StringCchCopy(
        DeviceIoctlGetRemoteExpirationTimeInputBuffer.RemoteLeasingApplicationIdentifier,
        MAX_PATH + 1,
        RemoteApplicationIdentifier
        );
    ASSERT_IFNOT(S_OK == WcharStringResult, "GetRemoteLeaseExpirationTime StringCchCopy failed ");

    auto DeviceIoctlReturn = DeviceIoControl(
        IOCTL_GET_REMOTE_LEASE_EXPIRATION_TIME,
        &DeviceIoctlGetRemoteExpirationTimeInputBuffer,
        sizeof(REMOTE_LEASE_EXPIRATION_RESULT_INPUT_BUFFER),
        &DeviceIoctlGetRemoteExpirationTimeOutputBuffer,
        sizeof(REMOTE_LEASE_EXPIRATION_RESULT_OUTPUT_BUFFER),
        &BytesReturned
        );
    //
    // Check return code of DeviceIoControl.
    //
    if (!NT_SUCCESS(DeviceIoctlReturn)) {
        return DeviceIoctlReturn;
    }

    *MonitorExpireTTL = DeviceIoctlGetRemoteExpirationTimeOutputBuffer.MonitorExpireTTL;
    *SubjectExpireTTL = DeviceIoctlGetRemoteExpirationTimeOutputBuffer.SubjectExpireTTL;

    return NT_SUCCESS(DeviceIoctlReturn) ? TRUE : FALSE;
}

BOOL WINAPI
CompleteArbitrationSuccessProcessing(
    __in HANDLE localApplicationHandle,
    __in LONGLONG localInstance,
    __in PTRANSPORT_LISTEN_ENDPOINT remoteSocketAddress,
    __in LONGLONG remoteInstance,
    __in LONG localTTL,
    __in LONG remoteTTL,
    __in BOOL isDelayed
    )

/*++
Routine Description:

     Completes a successful arbitration with relation to the remote lease agent.

Parameters Description:

    LeaseApplicationHandle - The handle of the lease application that completed aribitration successfully.

    TimeToLive - time to live for the remote lease agent.

    LeaseAgentInstance - lease agent instance involved in arbitration.

    RemoteLeaseAgentInstance - remote lease agent instance involved in arbitration.

    RemoteSocketAddress - remote socket address.
 
Return Value:

    TRUE if arguments are valid, FALSE otherwise.   

--*/

{
    DWORD BytesReturned = 0;

    ARBITRATION_RESULT_INPUT_BUFFER DeviceIoctlInputBuffer;
    ZeroMemory(&DeviceIoctlInputBuffer, sizeof(ARBITRATION_RESULT_INPUT_BUFFER));

    //
    // Check to see if the user mode lease layer 
    // has been successfully initialized.
    //
    if (!IsLeaseLayerInitialized()) {

        return FALSE;
    }

    //
    // Check arguments.
    //
    if (NULL == localApplicationHandle ||
        INVALID_HANDLE_VALUE == localApplicationHandle ||
        0 > localInstance ||
        0 > remoteInstance ||
        0 > remoteTTL ||
        NULL == remoteSocketAddress) {

        SetLastError(ERROR_INVALID_PARAMETER);

        return FALSE;
    }

    //
    // Initialize device IOCTL input buffer.
    //
    DeviceIoctlInputBuffer.LeasingApplicationHandle = localApplicationHandle;
    DeviceIoctlInputBuffer.RemoteTimeToLive = remoteTTL;
    DeviceIoctlInputBuffer.LocalTimeToLive = localTTL;
    DeviceIoctlInputBuffer.IsDelayed = (BOOLEAN)isDelayed;

    DeviceIoctlInputBuffer.LeaseAgentInstance = localInstance;
    DeviceIoctlInputBuffer.RemoteLeaseAgentInstance = remoteInstance;
    if (memcpy_s(
        &DeviceIoctlInputBuffer.RemoteSocketAddress,
        sizeof(TRANSPORT_LISTEN_ENDPOINT),
        remoteSocketAddress,
        sizeof(TRANSPORT_LISTEN_ENDPOINT)
        ))
    {
        return FALSE;
    }

    //
    // Create a device IOCTL and send it to the device.
    //
    return NT_SUCCESS(DeviceIoControl(
        IOCTL_ARBITRATION_RESULT,
        &DeviceIoctlInputBuffer,
        sizeof(ARBITRATION_RESULT_INPUT_BUFFER),
        NULL,
        0,
        &BytesReturned))? TRUE : FALSE;
}

BOOL WINAPI
CompleteRemoteCertificateVerification(
    __in HANDLE LeasingApplication,
    __in PVOID RemoteCertVerifyCallbackOperation,
    __in NTSTATUS verifyResult
    )

/*++
Routine Description:

     Completes a remote certification verification

Parameters Description:

    RemoteCertVerifyCallbackOperation - The operation handle in the kernel to complete the verify result
 
Return Value:

    TRUE if arguments are valid, FALSE otherwise.   

--*/

{
    DWORD BytesReturned = 0;

    REMOTE_CERT_VERIFY_CALLBACK_OPERATION_INPUT_BUFFER DeviceIoctlInputBuffer;
    ZeroMemory(&DeviceIoctlInputBuffer, sizeof(REMOTE_CERT_VERIFY_CALLBACK_OPERATION_INPUT_BUFFER));

    LeaseLayerEvents.SendCertVerifyResultToKernel((LONGLONG)RemoteCertVerifyCallbackOperation, verifyResult);

    //
    // Check to see if the user mode lease layer 
    // has been successfully initialized.
    //
    if (!IsLeaseLayerInitialized())
    {
        return FALSE;
    }

    //
    // Check arguments.
    //
    if (NULL == RemoteCertVerifyCallbackOperation)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Initialize device IOCTL input buffer.
    //
    DeviceIoctlInputBuffer.LeasingApplicationHandle = LeasingApplication;
    DeviceIoctlInputBuffer.CertVerifyOperation = RemoteCertVerifyCallbackOperation;
    DeviceIoctlInputBuffer.VerifyResult = verifyResult;

    //
    // Create a device IOCTL and send it to the device.
    //
    auto status = DeviceIoControl(
        IOCTL_REMOTE_CERT_VERIFY_RESULT,
        &DeviceIoctlInputBuffer,
        sizeof(REMOTE_CERT_VERIFY_CALLBACK_OPERATION_INPUT_BUFFER),
        NULL,
        0,
        &BytesReturned);

    return NT_SUCCESS(status)? TRUE : FALSE;
}

BOOL WINAPI 
UpdateSecuritySettings(
    HANDLE LeasingApplication,
    Transport::SecuritySettings const* securitySettings)
{
    DWORD BytesReturned = 0;
    HRESULT WcharStringResult = E_FAIL;

    UPDATE_SECURITY_SETTINGS_INPUT_BUFFER updateSecuritySettingsInputBuffer;
    ZeroMemory(&updateSecuritySettingsInputBuffer, sizeof(UPDATE_SECURITY_SETTINGS_INPUT_BUFFER));

    //
    // Check to see if the user mode lease layer 
    // has been successfully initialized.
    //
    if (!IsLeaseLayerInitialized()) {

        return FALSE;
    }

    //
    // Check arguments.
    //
    if (NULL == LeasingApplication || INVALID_HANDLE_VALUE == LeasingApplication)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Populate device IOCTL input buffer.
    //
    updateSecuritySettingsInputBuffer.LeasingApplicationHandle = LeasingApplication;
    updateSecuritySettingsInputBuffer.SecuritySettings = securitySettings;

    //
    // Send the update certificate Device IOCTL.
    //
    auto DeviceIoctlReturn = DeviceIoControl(
        IOCTL_UPDATE_CERTIFICATE,
        &updateSecuritySettingsInputBuffer,
        sizeof(UPDATE_SECURITY_SETTINGS_INPUT_BUFFER),
        NULL, 
        0,
        &BytesReturned);

    return NT_SUCCESS(DeviceIoctlReturn)? TRUE : FALSE;
}

_Use_decl_annotations_
BOOL WINAPI 
SetListenerSecurityDescriptor(
    HANDLE LeaseAgentHandle,
    PSECURITY_DESCRIPTOR selfRelativeSecurityDescriptor
    )
/*++
Routine Description:
    Pass self-relative security descriptor to driver for lease transport connection authorization
 
Parameters Description:
    LeaseApplication - kernel lease agent handle.

Return Value:
    TRUE if arguments are valid, FALSE otherwise.
--*/

{
    Invariant(false);
/*
    BOOL ret = FALSE;
    if (!IsLeaseLayerInitialized())
    {
        return ret;
    }

    if (NULL == LeaseAgentHandle || 
        INVALID_HANDLE_VALUE == LeaseAgentHandle || 
        IsValidSecurityDescriptor(selfRelativeSecurityDescriptor) == FALSE) // lease driver code will verify if the input is in self-relative format
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return ret;
    }

    ULONG securityDescriptorSize = GetSecurityDescriptorLength(selfRelativeSecurityDescriptor);
    ASSERT_IF(securityDescriptorSize < sizeof(SECURITY_DESCRIPTOR), "Invalid security descriptor size");

    ULONG sdStartOffset = (ULONG)(&(((PSET_SECURITY_DESCRIPTOR_INPUT_BUFFER)nullptr)->SecurityDescriptor));
    DWORD inputBufferSize = sdStartOffset + securityDescriptorSize;

    PBYTE inputBuffer = new BYTE[inputBufferSize];
    if (NULL == inputBuffer)
    {
        SetLastError(ERROR_NO_SYSTEM_RESOURCES);
        return ret;
    }

    KFinally([inputBuffer] { delete inputBuffer; });

    PSET_SECURITY_DESCRIPTOR_INPUT_BUFFER DeviceIoctlSetSecurityDescriptorInputBufferPtr = 
        reinterpret_cast<PSET_SECURITY_DESCRIPTOR_INPUT_BUFFER>( inputBuffer );

    DeviceIoctlSetSecurityDescriptorInputBufferPtr->LeaseAgentHandle = LeaseAgentHandle;
    DeviceIoctlSetSecurityDescriptorInputBufferPtr->SecurityDescriptorSize = securityDescriptorSize;
    if (memcpy_s(
        &(DeviceIoctlSetSecurityDescriptorInputBufferPtr->SecurityDescriptor),
        securityDescriptorSize,
        selfRelativeSecurityDescriptor,
        securityDescriptorSize))
    {
        return ret;
    }

    DWORD BytesReturned = 0;

    return DeviceIoControl(
        IOCTL_SET_SECURITY_DESCRIPTOR,
        DeviceIoctlSetSecurityDescriptorInputBufferPtr,
        inputBufferSize,
        NULL, 
        0,
        &BytesReturned,
        NULL
        ); */
}


BOOL WINAPI 
BlockLeaseConnection(
    __in PTRANSPORT_LISTEN_ENDPOINT LocalSocketAddress,
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress,
    __in BOOL IsBlocking
    )
/*++
 
Routine Description:
 
    Blocking the connectiosn between two lease agents by dropping all messages
    Unblocking by set the flag 'IsBlocking' to false

Parameters Description:
 
    SocketAddress - socket addresss of involved lease agents.
 
Return Value:
 
    TRUE if inputs are valid, FALSE otherwise. 
    Call GetLastError to retrieve the actual error that occured.
 
--*/

{
    DWORD BytesReturned = 0;

    BLOCK_LEASE_CONNECTION_BUFFER_DEVICE_IOCTL DeviceIoctlInputBuffer;
    ZeroMemory(&DeviceIoctlInputBuffer, sizeof(BLOCK_LEASE_CONNECTION_BUFFER_DEVICE_IOCTL));

    //
    // Check to see if the user mode lease layer has been successfully initialized.
    //
    if (!IsLeaseLayerInitialized())
    {
        return FALSE;
    }

    //
    // Check input arguments.
    //
    if (NULL == LocalSocketAddress || NULL == RemoteSocketAddress)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Initialize device IOCTL input buffer.
    //
    if (memcpy_s(
        &DeviceIoctlInputBuffer.LocalSocketAddress,
        sizeof(TRANSPORT_LISTEN_ENDPOINT),
        LocalSocketAddress,
        sizeof(TRANSPORT_LISTEN_ENDPOINT)
        ))
    {
        return FALSE;
    }

    if (memcpy_s(
        &DeviceIoctlInputBuffer.RemoteSocketAddress,
        sizeof(TRANSPORT_LISTEN_ENDPOINT),
        RemoteSocketAddress,
        sizeof(TRANSPORT_LISTEN_ENDPOINT)
        ))
    {
        return FALSE;
    }

    DeviceIoctlInputBuffer.IsBlocking = BOOLEAN(IsBlocking);
    //
    // Create a device IOCTL and send it to the device.
    //

    LeaseLayerEvents.BlockLeaseConnection(
        LocalSocketAddress->Address,
        LocalSocketAddress->Port,
        RemoteSocketAddress->Address,
        RemoteSocketAddress->Port,
        BOOLEAN(IsBlocking)
        );

    return NT_SUCCESS(DeviceIoControl(
        IOCTL_BLOCK_LEASE_CONNECTION,
        &DeviceIoctlInputBuffer,
        sizeof(BLOCK_LEASE_CONNECTION_BUFFER_DEVICE_IOCTL),
        NULL,
        0,
        &BytesReturned))? TRUE : FALSE;


}

BOOL WINAPI
UpdateLeaseDuration(
    __in HANDLE LeaseAgentHandle,
    __in PLEASE_CONFIG_DURATIONS LeaseDurations
    )
/*++
Routine Description:
    Pass lease duration with its index to driver lease agent when there is user mode federation configuration update
 
Parameters Description:
    LeaseAgentHandle - kernel lease agent handle.

Return Value:
    TRUE if arguments are valid, FALSE otherwise.
--*/

{
    BOOL ret = FALSE;
    if (!IsLeaseLayerInitialized())
    {
        return ret;
    }

    if (NULL == LeaseAgentHandle || 
        INVALID_HANDLE_VALUE == LeaseAgentHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return ret;
    }

    UPDATE_LEASE_DURATION_INPUT_BUFFER DeviceIoctlUpdateLeaseDurationInputBuffer;
    ZeroMemory(&DeviceIoctlUpdateLeaseDurationInputBuffer, sizeof(UPDATE_LEASE_DURATION_INPUT_BUFFER));

    DeviceIoctlUpdateLeaseDurationInputBuffer.LeaseAgentHandle = LeaseAgentHandle;

    if (memcpy_s(
        &DeviceIoctlUpdateLeaseDurationInputBuffer.UpdatedLeaseDurations,
        sizeof(LEASE_CONFIG_DURATIONS),
        LeaseDurations,
        sizeof(LEASE_CONFIG_DURATIONS)
        ))
    {
        return ret;
    }

    LeaseLayerEvents.UpdateLeaseDuration(
        LONGLONG(LeaseAgentHandle),
        LeaseDurations->LeaseDuration,
        LeaseDurations->LeaseDurationAcrossFD);

    DWORD BytesReturned = 0;

    return NT_SUCCESS(DeviceIoControl(
        IOCTL_UPDATE_LEASE_DURATION,
        &DeviceIoctlUpdateLeaseDurationInputBuffer,
        sizeof(UPDATE_LEASE_DURATION_INPUT_BUFFER),
        NULL, 
        0,
        &BytesReturned))? TRUE : FALSE;

}

LONG WINAPI 
QueryLeaseDuration(
    __in PTRANSPORT_LISTEN_ENDPOINT LocalSocketAddress,
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress
    )
/*++
Routine Description: Retrieve the existing lease relationship duration from driver
--*/
{
    DWORD BytesReturned = 0;
    // Reuse the block lease connection buffer
    BLOCK_LEASE_CONNECTION_BUFFER_DEVICE_IOCTL DeviceIoctlQueryDurationInputBuffer;
    ZeroMemory(&DeviceIoctlQueryDurationInputBuffer, sizeof(BLOCK_LEASE_CONNECTION_BUFFER_DEVICE_IOCTL));
    // Reuse the get lease application expiration output buffer
    GET_LEASING_APPLICATION_EXPIRATION_OUTPUT_BUFFER DeviceIoctlQueryDurationOutputBuffer;
    ZeroMemory(&DeviceIoctlQueryDurationOutputBuffer, sizeof(GET_LEASING_APPLICATION_EXPIRATION_OUTPUT_BUFFER));

    if (!IsLeaseLayerInitialized())
    {
        return FALSE;
    }

    if (NULL == LocalSocketAddress || NULL == RemoteSocketAddress)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (memcpy_s(
        &DeviceIoctlQueryDurationInputBuffer.LocalSocketAddress,
        sizeof(TRANSPORT_LISTEN_ENDPOINT),
        LocalSocketAddress,
        sizeof(TRANSPORT_LISTEN_ENDPOINT)
        ))
    {
        return FALSE;
    }

    if (memcpy_s(
        &DeviceIoctlQueryDurationInputBuffer.RemoteSocketAddress,
        sizeof(TRANSPORT_LISTEN_ENDPOINT),
        RemoteSocketAddress,
        sizeof(TRANSPORT_LISTEN_ENDPOINT)
        ))
    {
        return FALSE;
    }

    auto DeviceIoctlReturn = DeviceIoControl(
        IOCTL_QUERY_LEASE_DURATION,
        &DeviceIoctlQueryDurationInputBuffer,
        sizeof(BLOCK_LEASE_CONNECTION_BUFFER_DEVICE_IOCTL),
        &DeviceIoctlQueryDurationOutputBuffer, 
        sizeof(GET_LEASING_APPLICATION_EXPIRATION_OUTPUT_BUFFER),
        &BytesReturned);

    if (NT_SUCCESS(DeviceIoctlReturn))
    {
        LeaseLayerEvents.QueryLeaseDuration(
            LocalSocketAddress->Address,
            LocalSocketAddress->Port,
            RemoteSocketAddress->Address,
            RemoteSocketAddress->Port,
            DeviceIoctlQueryDurationOutputBuffer.TimeToLive);
        
        return DeviceIoctlQueryDurationOutputBuffer.TimeToLive;
    }

    return 0;

}

BOOL WINAPI
addLeaseBehavior(
__in PTRANSPORT_LISTEN_ENDPOINT LocalSocketAddress,
__in BOOL FromAny,
__in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress,
__in BOOL ToAny,
__in LEASE_BLOCKING_ACTION_TYPE BlockingType,
__in std::wstring alias
)
/*++

Routine Description:

This is test API that helps to simulate network loss. It
can block a message type from source address to destination address.

Parameters Description:

SocketAddress - socket addresss of involved lease agents.

Return Value:

TRUE if inputs are valid, FALSE otherwise.
Call GetLastError to retrieve the actual error that occured.

--*/

{
    DWORD BytesReturned = 0;
    TRANSPORT_BEHAVIOR_BUFFER DeviceIoctlInputBuffer;
    ZeroMemory(&DeviceIoctlInputBuffer, sizeof(TRANSPORT_BEHAVIOR_BUFFER));

    DeviceIoctlInputBuffer.BlockingType = BlockingType;
    //
    // Check to see if the user mode lease layer has been successfully initialized.
    //
    if (!IsLeaseLayerInitialized())
    {
        return FALSE;
    }

    //
    // Check input arguments.
    //
    if (NULL == LocalSocketAddress || NULL == RemoteSocketAddress)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Initialize device IOCTL input buffer.
    //
    if (memcpy_s(
        &DeviceIoctlInputBuffer.LocalSocketAddress,
        sizeof(TRANSPORT_LISTEN_ENDPOINT),
        LocalSocketAddress,
        sizeof(TRANSPORT_LISTEN_ENDPOINT)
        ))
    {
        return FALSE;
    }

    if (memcpy_s(
        &DeviceIoctlInputBuffer.RemoteSocketAddress,
        sizeof(TRANSPORT_LISTEN_ENDPOINT),
        RemoteSocketAddress,
        sizeof(TRANSPORT_LISTEN_ENDPOINT)
        ))
    {
        return FALSE;
    }

    DeviceIoctlInputBuffer.FromAny = (BOOLEAN)FromAny;
    DeviceIoctlInputBuffer.ToAny = (BOOLEAN)ToAny;
    int i = 0;
    for (; i < alias.size(); ++i)
    {
        DeviceIoctlInputBuffer.Alias[i] = alias[i];
    }
    DeviceIoctlInputBuffer.Alias[i] = L'\0';

    //
    // Create a device IOCTL and send it to the device.
    //

    return DeviceIoControl(
        IOCTL_ADD_TRANSPORT_BEHAVIOR,
        &DeviceIoctlInputBuffer,
        sizeof(TRANSPORT_BEHAVIOR_BUFFER),
        NULL,
        0,
        &BytesReturned
        );
}

BOOL WINAPI
removeLeaseBehavior(
__in std::wstring alias
)
/*++

Routine Description:

This is test API that clear the blocking rules that 
addleaseBebavior add.

Parameters Description:

Return Value:

TRUE if inputs are valid, FALSE otherwise.
Call GetLastError to retrieve the actual error that occured.

--*/
{
    DWORD BytesReturned = 0;

    TRANSPORT_BEHAVIOR_BUFFER DeviceIoctlInputBuffer;
    ZeroMemory(&DeviceIoctlInputBuffer, sizeof(TRANSPORT_BEHAVIOR_BUFFER));

    int i = 0;
    for (; i < alias.size(); ++i)
    {
        DeviceIoctlInputBuffer.Alias[i] = alias[i];
    }

    DeviceIoctlInputBuffer.Alias[i] = L'\0';

    if (!IsInitializeCalled)
    {
        AcquireExclusiveLock lock(*LeaseLayerLock);
        {
            if (!IsInitializeCalled)
            {
                InitializeLeaseLayer();
                IsInitializeCalled = TRUE;
            }
        }
    }

    return DeviceIoControl(
        IOCTL_CLEAR_TRANSPORT_BEHAVIOR,
        &DeviceIoctlInputBuffer,
        sizeof(TRANSPORT_BEHAVIOR_BUFFER),
        NULL,
        0,
        &BytesReturned
        );
}
