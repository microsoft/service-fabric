// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestCommon
{
    class TestSession : public Common::ComponentRoot, public Common::TextTraceComponent<Common::TraceTaskCodes::TestSession>
    {
        DENY_COPY(TestSession)
    public:
        TestSession(std::wstring const& label, bool autoMode, TestDispatcher& dispatcher, std::wstring const& dispatcherHelpFileName = L"");

        __declspec (property(get = get_Dispatcher))                         TestDispatcher& Dispatcher;
        __declspec (property(get = get_IsAutoMode, put = set_IsAutoMode))     bool IsAutoMode;
        __declspec (property(get = get_Label, put = set_Label))               std::wstring Label;
        __declspec (property(get = get_ConsoleRef))               Common::RealConsole & ConsoleRef;
        __declspec (property(get = get_LastGotLogicalTime, put = set_LastGotLogicalTime))   int64 LastGotLogicalTime;

        int Execute();
        void FinishExecution(int result)
        {
            result_ = result;
        }

        Common::RealConsole & get_ConsoleRef()
        {
            return console_;
        }

        TestDispatcher& get_Dispatcher() const
        {
            return dispatcher_;
        }

        std::wstring get_Label() const
        {
            return label_;
        }

        void set_Label(std::wstring const& label);

        bool get_IsAutoMode() const
        {
            return autoMode_;
        }

        void set_IsAutoMode(bool autoMode)
        {
            autoMode_ = autoMode;
        }

        int64 get_LastGotLogicalTime()
        {
            return lastGotLogicalTime_;
        }

        void set_LastGotLogicalTime(int64 lastGotLogicalTime)
        {
            lastGotLogicalTime_ = lastGotLogicalTime;
        }

        void WaitForExpectationEquilibrium(Common::TimeSpan timeout);

        void Expect(std::wstring const & message)
        {
            verifier_.Expect(message);
        }

        void PrintHelp(std::wstring const & name);

        void AddPendingItem(std::wstring const & name, Common::ComponentRootWPtr const & weakRoot = Common::ComponentRootWPtr());
        bool ItemExists(std::wstring const & name);
        void RemovePendingItem(std::wstring const & name, bool skipExistenceCheck = false);
        void ClosePendingItem(std::wstring const & name);
        void ShowPendingItems(std::wstring const & param) const;

        std::wstring GetState(std::wstring const & param);
        bool TryGetString(std::wstring const & param, __out std::wstring & result);

        template <class t0>
        void Expect(Common::literal_holder<char> const & message, t0 const & a0)
        {
            std::wstring finalMessage;
            Common::StringWriter sw(finalMessage);
            sw.Write(message, a0);
            Expect(finalMessage);
        }

        template <class t0, class t1>
        void Expect(Common::literal_holder<char> const & message, t0 const & a0, t1 const & a1)
        {
            std::wstring finalMessage;
            Common::StringWriter sw(finalMessage);
            sw.Write(message, a0, a1);
            Expect(finalMessage);
        }

        template <class t0, class t1, class t2>
        void Expect(Common::literal_holder<char> const & message, t0 const & a0, t1 const & a1, t2 const & a2)
        {
            std::wstring finalMessage;
            Common::StringWriter sw(finalMessage);
            sw.Write(message, a0, a1, a2);
            Expect(finalMessage);
        }

        template <class t0, class t1, class t2, class t3>
        void Expect(Common::literal_holder<char> const & message, t0 const & a0, t1 const & a1, t2 const & a2, t3 const & a3)
        {
            std::wstring finalMessage;
            Common::StringWriter sw(finalMessage);
            sw.Write(message, a0, a1, a2, a3);
            Expect(finalMessage);
        }

        void Validate(std::wstring const & message, bool ignoreUnmatched = false)
        {
            verifier_.Validate(message, ignoreUnmatched);
        }

        template <class t0>
        void Validate(Common::literal_holder<char> const & message, t0 const & a0)
        {
            std::wstring finalMessage;
            Common::StringWriter sw(finalMessage);
            sw.Write(message, a0);
            Validate(finalMessage);
        }

        template <class t0, class t1>
        void Validate(Common::literal_holder<char> const & message, t0 const & a0, t1 const & a1)
        {
            std::wstring finalMessage;
            Common::StringWriter sw(finalMessage);
            sw.Write(message, a0, a1);
            Validate(finalMessage);
        }

        template <class t0, class t1, class t2>
        void Validate(Common::literal_holder<char> const & message, t0 const & a0, t1 const & a1, t2 const & a2)
        {
            std::wstring finalMessage;
            Common::StringWriter sw(finalMessage);
            sw.Write(message, a0, a1, a2);
            Validate(finalMessage);
        }

        template <class t0, class t1, class t2, class t3>
        void Validate(Common::literal_holder<char> const & message, t0 const & a0, t1 const & a1, t2 const & a2, t3 const & a3)
        {
            std::wstring finalMessage;
            Common::StringWriter sw(finalMessage);
            sw.Write(message, a0, a1, a2, a3);
            Validate(finalMessage);
        }

        static __declspec(noreturn) void FailTest(Common::literal_holder<char> const & explanation)
        {
            std::string finalMessage;
            Common::StringWriterA sw(finalMessage);
            sw.Write(explanation);
            InternalFailTest(finalMessage);
        }

        template<class t0>
        static __declspec(noreturn) void FailTest(Common::literal_holder<char> const & explanation, t0 const & a0)
        {
            std::string finalMessage;
            Common::StringWriterA sw(finalMessage);
            sw.Write(explanation, a0);
            InternalFailTest(finalMessage);
        }

        template<class t0, class t1>
        static __declspec(noreturn) void FailTest(Common::literal_holder<char> const & explanation, t0 const & a0, t1 const & a1)
        {
            std::string finalMessage;
            Common::StringWriterA sw(finalMessage);
            sw.Write(explanation, a0, a1);
            InternalFailTest(finalMessage);
        }

        template<class t0, class t1, class t2>
        static __declspec(noreturn) void FailTest(Common::literal_holder<char> const & explanation, t0 const & a0, t1 const & a1, t2 const & a2)
        {
            std::string finalMessage;
            Common::StringWriterA sw(finalMessage);
            sw.Write(explanation, a0, a1, a2);
            InternalFailTest(finalMessage);
        }

        template<class t0, class t1, class t2, class t3>
        static __declspec(noreturn) void FailTest(Common::literal_holder<char> const & explanation, t0 const & a0, t1 const & a1, t2 const & a2, t3 const & a3)
        {
            std::string finalMessage;
            Common::StringWriterA sw(finalMessage);
            sw.Write(explanation, a0, a1, a2, a3);
            InternalFailTest(finalMessage);
        }

        template<class t0, class t1, class t2, class t3, class t4>
        static __declspec(noreturn) void FailTest(Common::literal_holder<char> const & explanation, t0 const & a0, t1 const & a1, t2 const & a2, t3 const & a3, t4 const & a4)
        {
            std::string finalMessage;
            Common::StringWriterA sw(finalMessage);
            sw.Write(explanation, a0, a1, a2, a3, a4);
            InternalFailTest(finalMessage);
        }

        template<class t0, class t1, class t2, class t3, class t4, class t5>
        static __declspec(noreturn) void FailTest(Common::literal_holder<char> const & explanation, t0 const & a0, t1 const & a1, t2 const & a2, t3 const & a3, t4 const & a4, t5 const & a5)
        {
            std::string finalMessage;
            Common::StringWriterA sw(finalMessage);
            sw.Write(explanation, a0, a1, a2, a3, a4, a5);
            InternalFailTest(finalMessage);
        }

        static void FailTestIf(bool condition, Common::literal_holder<char> const & explanation)
        {
            if (condition)
            {
                FailTest(explanation);
            }
        }

        template<class t0>
        static void FailTestIf(bool condition, Common::literal_holder<char> const & explanation, t0 const & a0)
        {
            if (condition)
            {
                FailTest(explanation, a0);
            }
        }

        template<class t0, class t1>
        static void FailTestIf(bool condition, Common::literal_holder<char> const & explanation, t0 const & a0, t1 const & a1)
        {
            if (condition)
            {
                FailTest(explanation, a0, a1);
            }
        }

        template<class t0, class t1, class t2>
        static void FailTestIf(
            bool condition,
            Common::literal_holder<char> const & explanation,
            t0 const & a0,
            t1 const & a1,
            t2 const & a2)
        {
            if (condition)
            {
                FailTest(explanation, a0, a1, a2);
            }
        }

        template<class t0, class t1, class t2, class t3>
        static void FailTestIf(
            bool condition,
            Common::literal_holder<char> const & explanation,
            t0 const & a0,
            t1 const & a1,
            t2 const & a2,
            t3 const & a3)
        {
            if (condition)
            {
                FailTest(explanation, a0, a1, a2, a3);
            }
        }

        template<class t0, class t1, class t2, class t3, class t4>
        static void FailTestIf(
            bool condition,
            Common::literal_holder<char> const & explanation,
            t0 const & a0,
            t1 const & a1,
            t2 const & a2,
            t3 const & a3,
            t4 const & a4)
        {
            if (condition)
            {
                FailTest(explanation, a0, a1, a2, a3, a4);
            }
        }

        template<class t0, class t1, class t2, class t3, class t4, class t5>
        static void FailTestIf(
            bool condition,
            Common::literal_holder<char> const & explanation,
            t0 const & a0,
            t1 const & a1,
            t2 const & a2,
            t3 const & a3,
            t4 const & a4,
            t5 const & a5)
        {
            if (condition)
            {
                FailTest(explanation, a0, a1, a2, a3, a4, a5);
            }
        }

        template<class t0, class t1, class t2, class t3, class t4, class t5, class t6>
        static void FailTestIf(
            bool condition,
            Common::literal_holder<char> const & explanation,
            t0 const & a0,
            t1 const & a1,
            t2 const & a2,
            t3 const & a3,
            t4 const & a4,
            t5 const & a5,
            t6 const & a6)
        {
            if (condition)
            {
                FailTest(explanation, a0, a1, a2, a3, a4, a5, a6);
            }
        }

        static void FailTestIfNot(bool condition, Common::literal_holder<char> const & explanation)
        {
            if (!condition)
            {
                FailTest(explanation);
            }
        }

        template<class t0>
        static void FailTestIfNot(bool condition, Common::literal_holder<char> const & explanation, t0 const & a0)
        {
            if (!condition)
            {
                FailTest(explanation, a0);
            }
        }

        template<class t0, class t1>
        static void FailTestIfNot(bool condition, Common::literal_holder<char> const & explanation, t0 const & a0, t1 const & a1)
        {
            if (!condition)
            {
                FailTest(explanation, a0, a1);
            }
        }

        template<class t0, class t1, class t2>
        static void FailTestIfNot(bool condition, Common::literal_holder<char> const & explanation, t0 const & a0, t1 const & a1, t2 const & a2)
        {
            if (!condition)
            {
                FailTest(explanation, a0, a1, a2);
            }
        }

        template<class t0, class t1, class t2, class t3>
        static void FailTestIfNot(bool condition, Common::literal_holder<char> const & explanation, t0 const & a0, t1 const & a1, t2 const & a2, t3 const & a3)
        {
            if (!condition)
            {
                FailTest(explanation, a0, a1, a2, a3);
            }
        }

        static wchar_t const Space;
        static std::string const NewLine;

    protected:
        virtual std::wstring GetInput();
        virtual void ExecuteTestCommand(std::wstring const& command);
        void AddInput(Command const& command);
        void AddInput(std::wstring const& command);

        static std::wstring const InternalCommandPrefix;
        static std::wstring const CommentPrefix;
        static std::wstring const QuitCommand;
        static std::wstring const LoadCommand;
        static std::wstring const AbortCommand;
        static std::wstring const ExitCommand;
        static std::wstring const PauseCommand;
        static std::wstring const RandomPauseCommand;
        static std::wstring const SaveCommand;
        static std::wstring const LabelCommand;
        static std::wstring const ResetCommand;
        static std::wstring const AutoCommand;
        static std::wstring const WaitCommand;
        static std::wstring const ExpectCommand;
        static std::wstring const ClearCommand;
        static std::wstring const InterceptCommand;
        static std::wstring const TraceCommand;
        static std::wstring const PrintCommand;
        static std::wstring const StateCommand;
        static std::wstring const SaveStateCommand;
        static std::wstring const SaveStringCommand;
        static std::wstring const WaitForStateCommand;
        static std::wstring const PendingCommand;
        static std::wstring const UpdateConfigCommand;
        static std::wstring const SetConfigCommand;
        static std::wstring const AssertCommand;
        static std::wstring const HelpCommand;
        static std::wstring const Comment;

        virtual bool OnOpenSession() { return true; }
        virtual void OnCloseSession() {}

    private:
        void Run();
        bool OpenSession();
        void CloseSession();

        Command GetNextCommand();
        std::wstring CheckForInterceptCommand();
        std::wstring GetConsoleInput();
        void ExecuteInternalCommand(std::wstring const& command);
        void ExecuteCommand(std::wstring const & command);
        void OnError(bool crashTestOnAutoMode);
        void Pause(std::wstring const & time);
        void RandomPause(std::wstring const& interval);
        void LoadCommands(std::wstring const & fileName);
        void SaveCommands(std::wstring const & fileName, std::wstring const & errorMsg);
        void Reset();
        void SaveErrorFile(std::wstring const & errorMsg);
        std::wstring GetCurrentDateTime();
        std::string FileReadLine(Common::File& fileReader, __out bool& eof);
        void LogWriterWriteLine(std::wstring);
        void SetTrace(std::wstring const & filter);
        void Print(std::wstring const & param);
        void SaveState(std::wstring const & param);
        void SaveString(std::wstring const & param);
        void SetConfig(std::wstring const & param);
        void UpdateConfig(std::wstring const & param);
        void VerifyState(std::wstring const & param);
        void WaitForState(std::wstring const & param);
        void VerifyState(std::wstring const & state, std::wstring const & expectedState, Common::TimeSpan timeout);

        void EvaluateCommand(std::wstring & command);
        void ReadFileToText(std::wstring const& fileName, std::wstring & text);

        void PrintInternalHelp(std::wstring const& command);
        void PrintPendingItem(Common::StringWriter & w, std::pair<std::wstring, Common::TimeSpan> const & pendingItem) const;

        static void InternalFailTest(std::string explanation);
        static bool assertOnFailTest_;

        TestDispatcher& dispatcher_;
        TestVerifier verifier_;
        std::wstring label_;
        std::wstring logFile_;
        Common::FileWriter logWriter_;
        std::queue<Command> commands_;
        int result_;
        bool autoMode_;
        int intercept_;
        Common::RealConsole console_;
        std::wstring externalHelpFile_;

        PendingItemManager pendingItemManager_;
        std::map<std::wstring, std::wstring> savedStates_;
        std::map<std::wstring, std::wstring> savedStrings_;
        int64 lastGotLogicalTime_;
    };
};
