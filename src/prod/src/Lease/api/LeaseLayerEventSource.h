// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

class LeaseLayerEventSource
{
public:

    Common::TraceEventWriter<LONGLONG> RegisterForRefCount;
    Common::TraceEventWriter<LONGLONG> UnregisterFromRefCount;
    Common::TraceEventWriter<LONGLONG> UnregisterWaitForZeroRefCount;
    Common::TraceEventWriter<LONGLONG, int, int> LeasingApplicationChangeRef;
    Common::TraceEventWriter<LONGLONG, uint32> CompletionCallbackIOError;
    Common::TraceEventWriter<LONGLONG> EventPumpIOError;
    Common::TraceEventWriter<LONGLONG, int> LeasingApplicationClosed;
    Common::TraceEventWriter<LONGLONG> FailedToRegisterRefCount;
    Common::TraceEventWriter<LONGLONG, int> ChangeRefCountNullOverlapped;
    Common::TraceEventWriter<LONGLONG, int> ChangeRefCountInvalidHandle;
    Common::TraceEventWriter<LONGLONG, int> ChangeRefCountNotFound;
    Common::TraceEventWriter<LONGLONG, int> ChangeRefCountOverlappedMismatch;
    //Security traces
    Common::TraceEventWriter<std::wstring, BOOLEAN, std::wstring> RegisterLeasingApplicationInvalidPara;
    Common::TraceEventWriter<std::wstring, BOOLEAN, std::wstring> CreateLeaseAgentForSecurity;
    Common::TraceEventWriter<LONGLONG, int> SendCertVerifyResultToKernel;
    Common::TraceEventWriter<LONGLONG> RemoteCertEventFromKernel;
    Common::TraceEventWriter<LONGLONG, int> LeasingApplicationChangeRefEventType;
    Common::TraceEventWriter<int, int> UserKernelVersionMismatch;
    //V2
    Common::TraceEventWriter<std::wstring, uint32, std::wstring, uint32, BOOLEAN> BlockLeaseConnection;
    Common::TraceEventWriter<int> InvalidSID;
    Common::TraceEventWriter<int, int> CopySidFaild;
    Common::TraceEventWriter<LONGLONG, int, int> UpdateLeaseDuration;
    Common::TraceEventWriter<std::wstring, uint32, std::wstring, uint32, int> QueryLeaseDuration;

    Common::TraceEventWriter<LONGLONG, int> GetContainerLeasingTTL;
    Common::TraceEventWriter<int, int> UpdateLeaseGlobalConfig;

    LeaseLayerEventSource() :

        RegisterForRefCount(Common::TraceTaskCodes::LeaseLayer, 4, "RegisterForRefCount", Common::LogLevel::Info, "Leasing application {0:x} is registered for refcount", "a1") ,
        UnregisterFromRefCount(Common::TraceTaskCodes::LeaseLayer, 5, "UnregisterFromRefCount", Common::LogLevel::Info, "Leasing application {0:x} is unregistered from refcount table", "a1"),
        UnregisterWaitForZeroRefCount(Common::TraceTaskCodes::LeaseLayer, 6, "UnregisterWaitForZeroRefCount", Common::LogLevel::Info, "Leasing application {0:x} wait for ref count to go to 0.", "a1"),
        LeasingApplicationChangeRef(Common::TraceTaskCodes::LeaseLayer, 7, "LeasingApplicationChangeRef", Common::LogLevel::Info, "Leasing application {0:x} change reference count by {1} to {2}", "a1", "a2", "a3"),
        CompletionCallbackIOError(Common::TraceTaskCodes::LeaseLayer, 8, "CompletionCallbackIOError", Common::LogLevel::Error, "Leasing application {0:x} IO completion callback return error {1}", "a1", "a2"),
        EventPumpIOError(Common::TraceTaskCodes::LeaseLayer, 9, "EventPumpIOError", Common::LogLevel::Error, "Leasing application {0:x} I/O event pump error", "a1"),
        LeasingApplicationClosed(Common::TraceTaskCodes::LeaseLayer, 10, "LeasingApplicationClosed", Common::LogLevel::Info, "Leasing application {0:x} closed with {1} references remaining", "a1", "a2"),
        FailedToRegisterRefCount(Common::TraceTaskCodes::LeaseLayer, 11, "FailedToRegisterRefCount", Common::LogLevel::Error, "Leasing application {0:x} failed to register for ref counting", "a1"),
        ChangeRefCountNullOverlapped(Common::TraceTaskCodes::LeaseLayer, 12, "ChangeRefCountNullOverlapped", Common::LogLevel::Error, "Leasing application {0:x} failed to change ref count: null overlapped; is increment? {1}.", "a1", "a2"),
        ChangeRefCountInvalidHandle(Common::TraceTaskCodes::LeaseLayer, 13, "ChangeRefCountInvalidHandle", Common::LogLevel::Error, "Leasing application {0:x} failed to change ref count: invalid handle; is increment? {1}.", "a1", "a2"),
        ChangeRefCountNotFound(Common::TraceTaskCodes::LeaseLayer, 14, "ChangeRefCountNotFound", Common::LogLevel::Info, "Leasing application {0:x} failed to change ref count: handle not found in table; is increment? {1}.", "a1", "a2"),
        ChangeRefCountOverlappedMismatch(Common::TraceTaskCodes::LeaseLayer, 15, "ChangeRefCountOverlappedMismatch", Common::LogLevel::Error, "Leasing application {0:x} failed to change ref count: registered overlapped does not match; is increment? {1}.", "a1", "a2"),
        // Security traces
        RegisterLeasingApplicationInvalidPara(Common::TraceTaskCodes::LeaseLayer, 16, "RegisterLeasingApplicationInvalidPara", Common::LogLevel::Error, "Leasing application {0:x} failed to register with invalid para: security enabled? {1}, cert store name {2}.", "a1", "a2", "a3"),
        CreateLeaseAgentForSecurity(Common::TraceTaskCodes::LeaseLayer, 17, "CreateLeaseAgentForSecurity", Common::LogLevel::Info, "Leasing application {0:x} call IOCTL to create lease agent: security enabled? {1}, cert store name {2}.", "a1", "a2", "a3"),
        SendCertVerifyResultToKernel(Common::TraceTaskCodes::LeaseLayer, 18, "SendCertVerifyResultToKernel", Common::LogLevel::Info, "Call IOCTL for cert verify result: operation handle {0:x}, verify result {1}.", "a1", "a2"),
        RemoteCertEventFromKernel(Common::TraceTaskCodes::LeaseLayer, 19, "RemoteCertEventFromKernel", Common::LogLevel::Info, "Recv remote cert verify event from kernel: operation handle {0:x}.", "a1"),
        LeasingApplicationChangeRefEventType(Common::TraceTaskCodes::LeaseLayer, 20, "LeasingApplicationChangeRefEventType", Common::LogLevel::Info, "Leasing application {0:x} change reference count EventType: {1}.", "a1", "a2"),
        UserKernelVersionMismatch(Common::TraceTaskCodes::LeaseLayer, 21, "UserKernelVersionMismatch", Common::LogLevel::Error, "User mode (V: %1) and Kernel model (V: %2) version number do not match.", "a1", "a2"),
        BlockLeaseConnection(Common::TraceTaskCodes::LeaseLayer, 22, "BlockLeaseConnection", Common::LogLevel::Warning, "Block lease connection for {0}:{1} to {2}:{3}, is blocking? {4}.", "a1", "a2", "a3", "a4", "a5"),
        InvalidSID(Common::TraceTaskCodes::LeaseLayer, 23, "InvalidSID", Common::LogLevel::Warning, "SID {0} is invalid.", "a1"),
        CopySidFaild(Common::TraceTaskCodes::LeaseLayer, 24, "CopySidFaild", Common::LogLevel::Warning, "Copy SID {0} failed with {1:x}.", "a1", "a2"),
        UpdateLeaseDuration(Common::TraceTaskCodes::LeaseLayer, 25, "UpdateLeaseDuration", Common::LogLevel::Info, "Update lease duration for lease agent {0:x}; {1}:{2}.", "a1", "a2", "a3"),
        QueryLeaseDuration(Common::TraceTaskCodes::LeaseLayer, 26, "QueryLeaseDuration", Common::LogLevel::Info, "Query lease duration for {0}:{1} to {2}:{3}; current duration in use is: {4}.", "a1", "a2", "a3", "a4", "a5"),
        GetContainerLeasingTTL(Common::TraceTaskCodes::LeaseLayer, 27, "GetContainerLeasingTTL", Common::LogLevel::Info, "Get TTL for Leasing application {0} with request TTL {1}", "a1", "a2"),
        UpdateLeaseGlobalConfig(Common::TraceTaskCodes::LeaseLayer, 28, "UpdateLeaseGlobalConfig", Common::LogLevel::Info, "Update lease global config, MaintenanceInterval = {0}, ProcessAssertExitTimeout = {1}.", "a1", "a2")
        {
        }
};

 
