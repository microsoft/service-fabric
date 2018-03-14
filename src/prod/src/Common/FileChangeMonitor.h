// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Common
{
    class FileChangeMonitor2;
    typedef std::shared_ptr<FileChangeMonitor2> FileChangeMonitor2SPtr;
    typedef std::weak_ptr<FileChangeMonitor2> FileChangeMonitor2WPtr;

    class FileChangeMonitor2 :  
        public Common::StateMachine,
        public std::enable_shared_from_this<FileChangeMonitor2>
    {
        DENY_COPY(FileChangeMonitor2)

        STATEMACHINE_STATES_05(Created, Opening, Opened, Updating, Failed);
        STATEMACHINE_TRANSITION(Opening, Created);
        STATEMACHINE_TRANSITION(Opened, Opening|Updating);
        STATEMACHINE_TRANSITION(Updating, Opened);
        STATEMACHINE_TRANSITION(Failed, Opening|Updating);
        STATEMACHINE_ABORTED_TRANSITION(Created|Opened|Failed);
        STATEMACHINE_TERMINAL_STATES(Aborted);

    public:
        typedef Common::EventT<Common::ErrorCode> FailedEvent;
        typedef FailedEvent::EventHandler FailedEventHandler;
        typedef FailedEvent::HHandler FailedEventHHandler;

        typedef std::function<void (FileChangeMonitor2SPtr const & monitor)> ChangeCallback;
        static FileChangeMonitor2SPtr Create(std::wstring const & filePath, bool const supportFileDelete = FALSE);

    public:
        FileChangeMonitor2(std::wstring const & filePath, bool const supportFileDelete = FALSE);
        virtual ~FileChangeMonitor2();

        FailedEventHHandler RegisterFailedEventHandler(FailedEventHandler const & handler);
        bool UnregisterFailedEventHandler(FailedEventHHandler const & hHandler);

        Common::ErrorCode Open(ChangeCallback const & callback);
        void Close();

#ifndef PLATFORM_UNIX
        static void Callback(PTP_CALLBACK_INSTANCE, void* lpParameter, PTP_WAIT, TP_WAIT_RESULT);
#endif

    protected:
        virtual void OnAbort();

    private:
        Common::ErrorCode InitializeChangeHandle();
        void CloseChangeHandle();
        void DispatchChange();
        void OnFailure(Common::ErrorCode const error);

    private:
        ChangeCallback callback_;
        std::wstring traceId_;
        FailedEvent failedEvent_;
        bool supportFileDelete_;

#ifdef PLATFORM_UNIX
        std::string filePath_;
        int wd_;
        bool wdInitialized_;

        class INotify;
#else
        static Common::ErrorCode GetLastWriteTime(std::wstring const & filePath, __out Common::DateTime & lastWriteTime);
        Common::ErrorCode UpdateLastWriteTime();
        Common::ErrorCode IsMyChange(__out bool & myChange);
        Common::ErrorCode OnWin32Error(std::wstring const & win32FunctionName);
        void CancelThreadpoolWait();
        Common::ErrorCode WaitForNextChange();
        void OnWaitCompleted();

        std::wstring const filePath_;
        std::wstring const directory_;
        HANDLE directoryChangeHandle_;
        PTP_WAIT wait_;
        Common::DateTime lastWriteTime_;

        static const int FileReadMaxRetryIntervalInMillis;
        static const int FileReadMaxRetryCount;
#endif
    };
}
