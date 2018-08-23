//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

/* Note: How to add a new event to linux lttng
 *
 * 1. Assume this event has been added on the Windows side, with EID_NEWEVENT
 *
 * 2. Add a line in "// Event Descriptors" section like this:
 *
 *   EXTERN_C __declspec(selectany) const LttngEventTemplate KEID_NEWEVENT = {5, "NewEvent", "%s"};
 *
 *   Prefix a "K" to avoid confliction with Windows event name, i.e. KEID_NEWEVENT
 *   The first field is severity: 5 verbose, 4 info, 3 warning, 2 error, 1 critical
 *   The second field is event name: The "Type" field in TraceViewer.exe would show up as KTL.NewEvent for this event.
 *   The third field is message template, it supports "printf" style format, but no %ws or %S support yet.
 *
 * 3. Add a replacement macro for Windows equivalent, in "// Event Macros" section
 *    // Event Macro for EID_NEWEVENT
 *    #define TRACE_EID_NEWEVENT(Value)\
 *           EventEnabledEID_NEWEVENT() ?\
 *           KTraceWrite(&KEID_EVENT, Value)\
 *           : ERROR_SUCCESS\
 *
 *    This example uses one parameter, but it could be multiple.
 */

extern "C" {

#define KTRACE_LEVEL_CRITICAL 1
#define KTRACE_LEVEL_ERROR    2
#define KTRACE_LEVEL_WARNING  3
#define KTRACE_LEVEL_INFO     4
#define KTRACE_LEVEL_VERBOSE  5

    extern void LttngTraceWrapper(
            const char * taskName,
            const char * eventName,
            int level,
            const char * id,
            const char * data);

    typedef struct _LttngEventTemplate
    {
        int level;
        char* name;
        char* format;
    } LttngEventTemplate;

    #define KTRACE_TASKNAME_KTL "KTL"

    // Only ANSI is supported for now.
    inline ULONG KTraceWrite(const LttngEventTemplate *event, ...)
    {
        char DestA[1024];
        wchar_t DestW[1024];
        va_list args;
        va_start(args, event);
        vsnprintf(DestA, 1024, event->format, args);

        int si = 0;
        for (; DestA[si]; si++)
        {
            DestW[si] = (DestA[si] < 0 ? '?' : DestA[si]);
        }
        DestW[si] = 0;

        LttngTraceWrapper(KTRACE_TASKNAME_KTL,
                      (const char*)event->name,
                      (int)event->level,
                      (const char*)L"",
                      (const char*)DestW);
        return ERROR_SUCCESS;
    }

    // Event Descriptors
    EXTERN_C __declspec(selectany) const LttngEventTemplate KEID_DBG_PRINTF = {KTRACE_LEVEL_VERBOSE, "Debug.Printf", "%s"};
    EXTERN_C __declspec(selectany) const LttngEventTemplate KEID_DBG_FUNCTION_ENTRY = {KTRACE_LEVEL_VERBOSE, "Debug.FunctionEntry", "Entry(Function:%s) (Line:%d) (File:%s)"};
    EXTERN_C __declspec(selectany) const LttngEventTemplate KEID_DBG_CHECKPOINT = {KTRACE_LEVEL_VERBOSE, "Debug.Checkpoint", "Checkpoint (Id:%d) (Message:=%s) (Function:%s) (Line:%d) (File:%s)"};
    EXTERN_C __declspec(selectany) const LttngEventTemplate KEID_DBG_FUNCTION_EXIT = {KTRACE_LEVEL_VERBOSE, "Debug.FunctionExit", "Exit (Function:%s) (NTSTATUS=%8.1X) (Message=%s)"};
    EXTERN_C __declspec(selectany) const LttngEventTemplate KEID_MY_MODULE_CHECKPOINTEXAMPLE = {KTRACE_LEVEL_VERBOSE, "MyModule.MyCheckpoint", "MyCheckpoint (CHECPOINTID=%d (NTSTATUS=%8.1X) (Message=%s)"};
    EXTERN_C __declspec(selectany) const LttngEventTemplate KEID_DBG_CHECKPOINT_WDATA = {KTRACE_LEVEL_VERBOSE, "Debug.CheckpointWData", "Checkpoint (Id:%d) (Message:=%s) (Status:=%8.1X) (D1:=%16.1llX) (D2:=%16.1llX) (D3:=%16.1llX) (D4:=%16.1llX) (Function:%s) (Line:%d) (File:%s)"};
    EXTERN_C __declspec(selectany) const LttngEventTemplate KEID_DBG_CHECKPOINT_WSTATUS = {KTRACE_LEVEL_VERBOSE, "Debug.CheckpointWStatus", "Checkpoint (Id:%d) (Message:=%s) (Status:=%8.1X) (Function:%s) (Line:%d) (File:%s)"};
    EXTERN_C __declspec(selectany) const LttngEventTemplate KEID_DBG_ERROR_WDATA = {KTRACE_LEVEL_ERROR, "Debug.ErrorWData", "Error (Activity Id: %16.1llX) (Message:=%s) (Status:=%8.1X) (D1:=%16.1llX) (D2:=%16.1llX) (D3:=%16.1llX) (D4:=%16.1llX) (Function:%s) (Line:%d) (File:%s)"};
    EXTERN_C __declspec(selectany) const LttngEventTemplate KEID_DBG_ERROR_WSTATUS = {KTRACE_LEVEL_ERROR, "Debug.ErrorWStatus", "Error (Activity Id: %16.1llX) (Message:=%s) (Status:=%8.1X) (Function:%s) (Line:%d) (File:%s)"};
    EXTERN_C __declspec(selectany) const LttngEventTemplate KThreadHeldTooLong = {KTRACE_LEVEL_ERROR, "Debug.ThreadHeldTooLong", "Work item held thread too long: %16.1llX"};
    EXTERN_C __declspec(selectany) const LttngEventTemplate KSecurityNegotiationSucceeded = {KTRACE_LEVEL_INFO, "Debug.SecurityNegotiationSucceeded", "session %16.1llX(inbound:%d): negotiation succeeded: package:%s, capabilities:%8.1X"};
    EXTERN_C __declspec(selectany) const LttngEventTemplate KSecurityNegotiationFailed = {KTRACE_LEVEL_WARNING, "Debug.SecurityNegotiationFailed", "session %16.1llX(inbound:%d): negotiation failed: %8.1X"};
    EXTERN_C __declspec(selectany) const LttngEventTemplate KEID_DBG_CHECKPOINT_AI = {KTRACE_LEVEL_VERBOSE, "Debug.CheckpointAI", "Checkpoint (Activity Id: %16.1llX) (Message:=%s) (Function:%s) (Line:%d) (File:%s)"};
    EXTERN_C __declspec(selectany) const LttngEventTemplate KEID_DBG_CHECKPOINT_WDATA_AI = {KTRACE_LEVEL_VERBOSE, "Debug.CheckpointWDataAI", "Checkpoint (Activity Id: %16.1llX) (Message:=%s) (Status:=%8.1X) (D1:=%16.1llX) (D2:=%16.1llX) (D3:=%16.1llX) (D4:=%16.1llX) (Function:%s) (Line:%d) (File:%s)"};
    EXTERN_C __declspec(selectany) const LttngEventTemplate KEID_DBG_CHECKPOINT_WSTATUS_AI = {KTRACE_LEVEL_VERBOSE, "Debug.CheckpointWStatusAI", "Checkpoint (Activity Id: %16.1llX) (Message:=%s) (Status:=%8.1X) (Function:%s) (Line:%d) (File:%s)"};
    EXTERN_C __declspec(selectany) const LttngEventTemplate KEID_DBG_PRINTF_INFORMATIONAL = {KTRACE_LEVEL_INFO, "Debug.PrintfInformational", "%s"};
    EXTERN_C __declspec(selectany) const LttngEventTemplate KEID_DBG_CHECKPOINT_INFORMATIONAL = {KTRACE_LEVEL_INFO, "Debug.CheckpointInformational", "Checkpoint (Activity Id: %16.1llX) (Message:=%s) (Function:%s) (Line:%d) (File:%s)"};
    EXTERN_C __declspec(selectany) const LttngEventTemplate KEID_DBG_CHECKPOINT_WDATA_INFORMATIONAL = {KTRACE_LEVEL_INFO, "Debug.CheckpointWDataInformational", "Checkpoint (Activity Id: %16.1llX) (Message:=%s) (Status:=%8.1X) (D1:=%16.1llX) (D2:=%16.1llX) (D3:=%16.1llX) (D4:=%16.1llX) (Function:%s) (Line:%d) (File:%s)"};
    EXTERN_C __declspec(selectany) const LttngEventTemplate KEID_DBG_CHECKPOINT_WSTATUS_INFORMATIONAL = {KTRACE_LEVEL_INFO, "Debug.CheckpointWStatusInformational", "Checkpoint (Activity Id: %16.1llX) (Message:=%s) (Status:=%8.1X) (Function:%s) (Line:%d) (File:%s)"};
    EXTERN_C __declspec(selectany) const LttngEventTemplate KEID_ERR_OUT_OF_MEMORY = {KTRACE_LEVEL_WARNING, "Error.OutOfMemory", "Out of memory. (Activity Id: %16.1llX) (Status:=%8.1X) (Context:=%16.1llX) (D1:=%16.1llX) (D2:=%16.1llX)(Function:%s) (Line:%d) (File:%s)"};
    EXTERN_C __declspec(selectany) const LttngEventTemplate KEID_FAILED_ASYNC_REQUEST = {KTRACE_LEVEL_WARNING, "Error.FailedAsyncRequest", "An Async request was failed. (Activity Id: %16.1llX) (Status:=%8.1X) (AsyncContext:=%16.1llX) (D1:=%16.1llX) (D2:=%16.1llX)(Function:%s) (Line:%d) (File:%s)"};
    EXTERN_C __declspec(selectany) const LttngEventTemplate KEID_TRACE_CANCEL_CALLED = {KTRACE_LEVEL_INFO, "Trace.CancelCalled", "Cancel called (Activity Id: %16.1llX) (AsyncContext:=%16.1llX) (Operation Cancelable:=%d) (Operation Completed:=%d) (D1:=%16.1llX)(Function:%s) (Line:%d) (File:%s)"};
    EXTERN_C __declspec(selectany) const LttngEventTemplate KEID_TRACE_CONFIG_CHANGE = {KTRACE_LEVEL_INFO, "Trace.ConfigChange", "Configuration parameter changed in class:= %s (Context:=%16.1llX) (Parameter:= %s) (OldValue:=%lld) (NewValue:=%lld)"};


    // Event Macros

    // Event Macro for EID_DBG_PRINTF
    #define TRACE_EID_DBG_PRINTF(Value)\
            EventEnabledEID_DBG_PRINTF() ?\
            KTraceWrite(&KEID_DBG_PRINTF, Value)\
            : ERROR_SUCCESS\

    // Event Macro for EID_DBG_FUNCTION_ENTRY
    #define TRACE_EID_DBG_FUNCTION_ENTRY(Function, Line, File)\
            EventEnabledEID_DBG_FUNCTION_ENTRY() ?\
            KTraceWrite(&KEID_DBG_FUNCTION_ENTRY, Function, Line, File)\
            : ERROR_SUCCESS\

    // Event Macro for EID_DBG_CHECKPOINT
    #define TRACE_EID_DBG_CHECKPOINT(CheckpointId, Message, Function, Line, File)\
            EventEnabledEID_DBG_CHECKPOINT() ?\
            KTraceWrite(&KEID_DBG_CHECKPOINT, CheckpointId, Message, Function, Line, File)\
            : ERROR_SUCCESS\

    // Event Macro for EID_DBG_FUNCTION_EXIT
    #define TRACE_EID_DBG_FUNCTION_EXIT(Function, Status, Message)\
            EventEnabledEID_DBG_FUNCTION_EXIT() ?\
            KTraceWrite(&KEID_DBG_FUNCTION_EXIT, Function, Status, Message)\
            : ERROR_SUCCESS\

    // Event Macro for EID_MY_MODULE_CHECKPOINTEXAMPLE
    #define TRACE_EID_MY_MODULE_CHECKPOINTEXAMPLE(CheckpointId, Status, Message)\
            EventEnabledEID_MY_MODULE_CHECKPOINTEXAMPLE() ?\
            KTraceWrite(&KEID_MY_MODULE_CHECKPOINTEXAMPLE, CheckpointId, Status, Message)\
            : ERROR_SUCCESS\

    // Event Macro for EID_DBG_CHECKPOINT_WDATA
    #define TRACE_EID_DBG_CHECKPOINT_WDATA(CheckpointId, Message, Status, D1, D2, D3, D4, Function, Line, File)\
            EventEnabledEID_DBG_CHECKPOINT_WDATA() ?\
            KTraceWrite(&KEID_DBG_CHECKPOINT_WDATA, CheckpointId, Message, Status, D1, D2, D3, D4, Function, Line, File)\
            : ERROR_SUCCESS\

    // Event Macro for EID_DBG_CHECKPOINT_WSTATUS
    #define TRACE_EID_DBG_CHECKPOINT_WSTATUS(CheckpointId, Message, Status, Function, Line, File)\
            EventEnabledEID_DBG_CHECKPOINT_WSTATUS() ?\
            KTraceWrite(&KEID_DBG_CHECKPOINT_WSTATUS, CheckpointId, Message, Status, Function, Line, File)\
            : ERROR_SUCCESS\

    // Event Macro for EID_DBG_ERROR_WDATA
    #define TRACE_EID_DBG_ERROR_WDATA(ActivityId, Message, Status, D1, D2, D3, D4, Function, Line, File)\
            EventEnabledEID_DBG_ERROR_WDATA() ?\
            KTraceWrite(&KEID_DBG_ERROR_WDATA, ActivityId, Message, Status, D1, D2, D3, D4, Function, Line, File)\
            : ERROR_SUCCESS\

    //
    // Event Macro for EID_DBG_ERROR_WSTATUS
    //
    #define TRACE_EID_DBG_ERROR_WSTATUS(ActivityId, Message, Status, D1, D2, D3, D4, Function, Line, File)\
            EventEnabledEID_DBG_ERROR_WSTATUS() ?\
            KTraceWrite(&KEID_DBG_ERROR_WSTATUS, ActivityId, Message, Status, Function, Line, File)\
            : ERROR_SUCCESS\

    // Event Macro for ThreadHeldTooLong
    #define TRACE_ThreadHeldTooLong(Value)\
            EventEnabledThreadHeldTooLong() ?\
            KTraceWrite(&KThreadHeldTooLong, Value)\
            : ERROR_SUCCESS\

    // Event Macro for SecurityNegotiationSucceeded
    #define TRACE_SecurityNegotiationSucceeded(traceId, inbound, packageName, capabilities)\
            EventEnabledSecurityNegotiationSucceeded() ?\
            KTraceWrite(&KSecurityNegotiationSucceeded, traceId, inbound, packageName, capabilities)\
            : ERROR_SUCCESS\

    // Event Macro for SecurityNegotiationFailed
    #define TRACE_SecurityNegotiationFailed(traceId, inbound, error)\
            EventEnabledSecurityNegotiationFailed() ?\
            KTraceWrite(&KSecurityNegotiationFailed, traceId, inbound, error)\
            : ERROR_SUCCESS\

    // Event Macro for EID_DBG_CHECKPOINT_AI
    #define TRACE_EID_DBG_CHECKPOINT_AI(ActivityId, Message, Function, Line, File)\
            EventEnabledEID_DBG_CHECKPOINT_AI() ?\
            KTraceWrite(&KEID_DBG_CHECKPOINT_AI, ActivityId, Message, Function, Line, File)\
            : ERROR_SUCCESS\

    // Event Macro for EID_DBG_CHECKPOINT_WDATA_AI
    #define TRACE_EID_DBG_CHECKPOINT_WDATA_AI(ActivityId, Message, Status, D1, D2, D3, D4, Function, Line, File)\
            EventEnabledEID_DBG_CHECKPOINT_WDATA_AI() ?\
            KTraceWrite(&KEID_DBG_CHECKPOINT_WDATA_AI, ActivityId, Message, Status, D1, D2, D3, D4, Function, Line, File)\
            : ERROR_SUCCESS\

    // Event Macro for EID_DBG_CHECKPOINT_WSTATUS_AI
    #define TRACE_EID_DBG_CHECKPOINT_WSTATUS_AI(ActivityId, Message, Status, Function, Line, File)\
            EventEnabledEID_DBG_CHECKPOINT_WSTATUS_AI() ?\
            KTraceWrite(&KEID_DBG_CHECKPOINT_WSTATUS_AI, ActivityId, Message, Status, Function, Line, File)\
            : ERROR_SUCCESS\

    // Event Macro for EID_DBG_PRINTF_INFORMATIONAL
    #define TRACE_EID_DBG_PRINTF_INFORMATIONAL(Value)\
            EventEnabledEID_DBG_PRINTF_INFORMATIONAL() ?\
            KTraceWrite(&KEID_DBG_PRINTF_INFORMATIONAL, Value)\
            : ERROR_SUCCESS\

    // Event Macro for EID_DBG_CHECKPOINT_INFORMATIONAL
    #define TRACE_EID_DBG_CHECKPOINT_INFORMATIONAL(ActivityId, Message, Function, Line, File)\
            EventEnabledEID_DBG_CHECKPOINT_INFORMATIONAL() ?\
            KTraceWrite(&KEID_DBG_CHECKPOINT_INFORMATIONAL, ActivityId, Message, Function, Line, File)\
            : ERROR_SUCCESS\

    // Event Macro for EID_DBG_CHECKPOINT_WDATA_INFORMATIONAL
    #define TRACE_EID_DBG_CHECKPOINT_WDATA_INFORMATIONAL(ActivityId, Message, Status, D1, D2, D3, D4, Function, Line, File)\
            EventEnabledEID_DBG_CHECKPOINT_WDATA_INFORMATIONAL() ?\
            KTraceWrite(&KEID_DBG_CHECKPOINT_WDATA_INFORMATIONAL, ActivityId, Message, Status, D1, D2, D3, D4, Function, Line, File)\
            : ERROR_SUCCESS\

    // Event Macro for EID_DBG_CHECKPOINT_WSTATUS_INFORMATIONAL
    #define TRACE_EID_DBG_CHECKPOINT_WSTATUS_INFORMATIONAL(ActivityId, Message, Status, Function, Line, File)\
            EventEnabledEID_DBG_CHECKPOINT_WSTATUS_INFORMATIONAL() ?\
            KTraceWrite(&KEID_DBG_CHECKPOINT_WSTATUS_INFORMATIONAL, ActivityId, Message, Status, Function, Line, File)\
            : ERROR_SUCCESS\

    // Event Macro for EID_ERR_OUT_OF_MEMORY
    #define TRACE_EID_ERR_OUT_OF_MEMORY(ActivityId, Status, ContextPointer, D1, D2, Function, Line, File)\
            EventEnabledEID_ERR_OUT_OF_MEMORY() ?\
            KTraceWrite(&KEID_ERR_OUT_OF_MEMORY, ActivityId, Status, ContextPointer, D1, D2, Function, Line, File)\
            : ERROR_SUCCESS\

    // Event Macro for EID_FAILED_ASYNC_REQUEST
    #define TRACE_EID_FAILED_ASYNC_REQUEST(ActivityId, Status, ContextPointer, D1, D2, Function, Line, File)\
            EventEnabledEID_FAILED_ASYNC_REQUEST() ?\
            KTraceWrite(&KEID_FAILED_ASYNC_REQUEST, ActivityId, Status, ContextPointer, D1, D2, Function, Line, File)\
            : ERROR_SUCCESS\

    // Event Macro for EID_TRACE_CANCEL_CALLED
    #define TRACE_EID_TRACE_CANCEL_CALLED(ActivityId, ContextPointer, Cancelable, Completed, D1, Function, Line, File)\
            EventEnabledEID_TRACE_CANCEL_CALLED() ?\
            KTraceWrite(&KEID_TRACE_CANCEL_CALLED, ActivityId, ContextPointer, Cancelable, Completed, D1, Function, Line, File)\
            : ERROR_SUCCESS\

    // Event Macro for EID_TRACE_CONFIG_CHANGE
    #define TRACE_EID_TRACE_CONFIG_CHANGE(ClassName, ContextPointer, ParameterName, OldValue, NewValue)\
            EventEnabledEID_TRACE_CONFIG_CHANGE() ?\
            KTraceWrite(&KEID_TRACE_CONFIG_CHANGE, ClassName, ContextPointer, ParameterName, OldValue, NewValue)\
            : ERROR_SUCCESS\

} // extern "C"
