// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace TestIPC
{
    class TestIPCController
    {
        public:
            TestIPCController();
            ~TestIPCController();

            static const PWCHAR IPCControllerName;
            static const PWCHAR IPCControllerOffsetName;

            //
            // Initialize the named memory section using base name and
            // the section is at least SizeInBytes large
            //
            ULONG Initialize(
                __in PWCHAR BaseName,
                __in SIZE_T SizeInBytes
            );

            //
            // Retrieves the connection string used by the host to connect
            // to the memory section
            //
            PWCHAR GetConnectionString(
            )
            {
                return(_ConnectionString);
            }

            //
            // Get the pointer and size of the shared memory section
            //
            PVOID GetPointer(
            )
            {
                return(_Pointer);
            }

            SIZE_T GetSizeInBytes(
            )
            {
                return(_SizeInBytes);
            }

            //
            // Close access to the shared section
            //
            VOID Reset();
            
        private:
            HANDLE _FileMapping;
            PVOID _Pointer;
            SIZE_T _SizeInBytes;
            WCHAR _ConnectionString[MAX_PATH];

            // CONSIDER: Add a pair of named events that can be used as
            // notifications between controller and host
    };

    class TestIPCHost
    {
        public:
            TestIPCHost();
            ~TestIPCHost();

            //
            // Connect to the shared memory section created by the
            // controller and return the pointer and size
            //
            ULONG Connect(
                __in LPCWSTR ConnectionString
            );

            PVOID GetPointer()
            {
                return(_Pointer);
            }

            SIZE_T GetSizeInBytes()
            {
                return(_SizeInBytes);
            }

            VOID Reset();

        private:
            HANDLE _FileMapping;
            PVOID _Pointer;
            SIZE_T _SizeInBytes;
    };

    //
    // This struct is used for the host to return test completion
    // status to the controller
    //
    typedef struct
    {
        ULONG CompletionStatus;
        ULONG ErrorLineNumber;
        CHAR ErrorFileName[256];
        WCHAR ErrorMessage[256];
    } TestIPCResult;

    VOID TestIPCReturnError(
        TestIPCResult* Result,
        ULONG CompletionStatus,
        PCHAR ErrorFileName,
        ULONG ErrorLineNumber,
        PWCHAR ErrorMessage
    );
}

//
// If RETURN_VERIFY_RESULTS_TO_CONTROLLER is set then VERIFY_IS_TRUE()
// is redefined to fill the results back to the controller before
// allowing TAEF to see the failed condition. Note that this macro
// assumes that there is a pointer to TestIPCResult named ipcResult.
//
#ifdef RETURN_VERIFY_RESULTS_TO_CONTROLLER
#undef VERIFY_IS_TRUE
#define VERIFY_IS_TRUE(__condition, ...) \
    if (! (__condition)) {\
        TestIPC::TestIPCReturnError(ipcResult, ERROR_GEN_FAILURE, __FILE__, __LINE__, (L#__condition));\
        Log::Comment(String().Format(L"Error: Verify(%d): IsTrue(%ws): at %hs line %d", GetCurrentProcessId(), (L#__condition), __FILE__, __LINE__));\
        ExitProcess(ERROR_GEN_FAILURE);\
    }\
    Log::Comment(String().Format(L"VERIFY(%d): IsTrue(%ws)", GetCurrentProcessId(), (L#__condition)));
#endif
