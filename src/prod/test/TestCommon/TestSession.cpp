// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace TestCommon
{
    using namespace std;
    using namespace Common;

    wstring const TestSession::InternalCommandPrefix = L"!";
    wstring const TestSession::AbortCommand = L"!abort";
    wstring const TestSession::ExitCommand = L"!exit";
    wstring const TestSession::PauseCommand = L"!pause";
    wstring const TestSession::RandomPauseCommand = L"!rpause";
    wstring const TestSession::SaveCommand = L"!save";
    wstring const TestSession::LabelCommand = L"!label";
    wstring const TestSession::ResetCommand = L"!reset";
    wstring const TestSession::AutoCommand = L"!auto";
    wstring const TestSession::WaitCommand = L"!wait";
    wstring const TestSession::InterceptCommand = L"!intercept";
    wstring const TestSession::TraceCommand = L"!trace";
    wstring const TestSession::QuitCommand = L"!q";
    wstring const TestSession::LoadCommand = L"!load";
    wstring const TestSession::ExpectCommand = L"!expect";
    wstring const TestSession::ClearCommand = L"!clear";
    wstring const TestSession::PrintCommand = L"!?";
    wstring const TestSession::StateCommand = L"!state";
    wstring const TestSession::SaveStateCommand = L"!var";
    wstring const TestSession::SaveStringCommand = L"!string";
    wstring const TestSession::WaitForStateCommand = L"!waitforstate";
    wstring const TestSession::PendingCommand = L"!pending";
    wstring const TestSession::SetConfigCommand = L"!setcfg";
    wstring const TestSession::UpdateConfigCommand = L"!updatecfg";
    wstring const TestSession::AssertCommand = L"!assert";
    wstring const TestSession::HelpCommand = L"!help";

    string const TestSession::NewLine = "\r\n";
    wstring const TestSession::Comment = L"#";

    wchar_t const TestSession::Space = ' ';

    bool TestSession::assertOnFailTest_ = false;

    StringLiteral const TraceOutput("Output");

    TestSession::TestSession(wstring const& label, bool autoMode, TestDispatcher& dispatcher, wstring const& dispatcherHelpFileName)
        : ComponentRoot(),
        dispatcher_(dispatcher),
        verifier_(),
        label_(),
        logFile_(),
        logWriter_(),
        commands_(),
        result_(0),
        autoMode_(autoMode),
        intercept_(0),
        console_(),
        externalHelpFile_(),
        lastGotLogicalTime_(0),
        pendingItemManager_()
    {
        Label = label;
        TraceProvider::GetSingleton()->AddFilter(TraceSinkType::Console, L"TestSession.Command:0");
        ReadFileToText(dispatcherHelpFileName, externalHelpFile_);
        TestSession::assertOnFailTest_ = autoMode_;
    }

    void TestSession::ReadFileToText(wstring const& fileName, wstring & text)
    {

        if (!fileName.empty())
        {
            wstring fileNameLocal = Path::Combine(Directory::GetCurrentDirectory(), fileName);
            if (!File::Exists(fileNameLocal))
            {
                fileNameLocal = Path::Combine(Environment::GetExecutablePath(), fileName);
                if (!File::Exists(fileNameLocal))
                {
                    WriteWarning("FileError", "File does not exist: {0}", fileName);
                    return;
                }
            }

            File fileReader;
            auto error = fileReader.TryOpen(fileNameLocal, Common::FileMode::Open, Common::FileAccess::Read, Common::FileShare::Read);
            if (!error.IsSuccess())
            {
                WriteWarning("FileError", "Could not read file: {0} error={1}", fileNameLocal, error);
                return;
            }

            //Currently we asume the files to be pretty small and so we store the entire text as a wstring
            //for convinience. If the file size becomes large enough then we can consider reading on the fly
            int64 fileSize = fileReader.size();
            TestSession::FailTestIf(fileSize > 1000000, "{0} File size is {1}", fileNameLocal, fileSize);

            size_t size = static_cast<size_t>(fileSize);
            string helpText;
            helpText.resize(size);
            fileReader.Read(&helpText[0], static_cast<int>(size));
            fileReader.Close();

            StringWriter(text).Write(helpText);
        }
    }

    // Execute the session.
    // return value: 0 if the test is successful.
    //              -1 if the test fails.
    int TestSession::Execute()
    {
        Run();

        if (result_ > 0)
        {
            result_ = 0;
        }

        return result_;
    }

    void TestSession::Run()
    {
        if (!OpenSession())
        {
            result_ = -1;
        }

        while (result_ == 0)
        {
            Command command = GetNextCommand();
            wstring commandText = command.Text;

            WriteInfo("Command", "{0}", command);

            if (StringUtility::StartsWith(commandText, TestSession::QuitCommand))
            {
                LogWriterWriteLine(TestSession::QuitCommand);
                result_ = 1;
            }
            else if (!StringUtility::StartsWith(commandText, TestSession::Comment))
            {
                if (StringUtility::StartsWith(commandText, TestSession::LoadCommand))
                {
                    // Such commands cannot be put into the commandText buffer
                    // because they will load the actual commands to the
                    // buffer instead.

                    LogWriterWriteLine(TestSession::Comment + commandText);
                }
                else
                {
                    LogWriterWriteLine(commandText);
                }

                logWriter_.Flush();

                EvaluateCommand(commandText);

                if (autoMode_)
                {
                    ExecuteCommand(commandText);
                }
                else
                {
                    try
                    {
                        ExecuteCommand(commandText);
                    }
                    catch (exception const& e)
                    {
                        string stringMessage = e.what();
                        wstring wstringMessage(stringMessage.begin(), stringMessage.end());
                        WriteError("Execute", "Error executing external command: {0} with error {1}", command, wstringMessage);
                        OnError(true/*Need to crash the test if it is auto mode to collect crash dump*/);
                    }
                }
            }
        }

        if (result_ > 0)
        {
            CloseSession();
        }
    }

    void TestSession::ExecuteCommand(std::wstring const & command)
    {
        if (dispatcher_.ShouldSkipCommand(command))
        {
            return;
        }

        if (StringUtility::StartsWith(command, TestSession::InternalCommandPrefix))
        {
            ExecuteInternalCommand(command);
        }
        else if (command.size() != 0)
        {
            ExecuteTestCommand(command);
        }
    }

    Command TestSession::GetNextCommand()
    {
        if (!autoMode_)
        {
            wstring commandText = CheckForInterceptCommand();
            if (!commandText.empty())
            {
                return Command::CreateCommand(move(commandText));
            }
        }

        if (commands_.empty())
        {
            wstring commandText = GetInput();
            if (commandText.empty())
            {
                return Command::CreateEmptyCommand();
            }

            commands_.push(Command::CreateCommand(move(commandText)));
        }

        Command command = commands_.front();
        TraceConsoleSink::Write(0x0a, command.Text);
        commands_.pop();

        return command;
    }

    wstring TestSession::CheckForInterceptCommand()
    {
        wstring command;
        if (_kbhit() || intercept_ > 0)
        {
            command = GetConsoleInput();
            intercept_ = 1;
        }

        return command;
    }

    void TestSession::SetTrace(std::wstring const & filter)
    {
        bool result = TraceProvider::GetSingleton()->AddFilter(TraceSinkType::Console, filter);
        if (!result)
        {
            WriteWarning("Dispatch", "Invalid trace filter setting");
        }
    }

    wstring TestSession::GetConsoleInput()
    {
        console_.Write(">");
        wstring command;
        console_.ReadLine(command);
        return command;
    }

    wstring TestSession::GetInput()
    {
        return GetConsoleInput();
    }

    void TestSession::ExecuteTestCommand(wstring const& command)
    {
        if (!dispatcher_.ExecuteCommand(command))
        {
            WriteWarning("Dispatch", "Invalid command entered: {0}", command);
            OnError(false);
        }
    }

    void TestSession::ExecuteInternalCommand(wstring const& command)
    {
        wstring commandType;
        wstring parameter;
        wstring::size_type index = command.find_first_of(L" ,");

        if (index > 0)
        {
            commandType = command.substr(0, index);
            if (index != wstring::npos && index + 1 < command.size())
            {
                parameter = command.substr(index + 1);
            }
        }
        else
        {
            commandType = command;
        }

        wstring::iterator wstringIterator = remove(commandType.begin(), commandType.end(), TestSession::Space);
        commandType.erase(wstringIterator, commandType.end());

        if (commandType.compare(TestSession::AbortCommand) == 0)
        {
            result_ = -1;
        }
        else if (commandType.compare(TestSession::ExitCommand) == 0)
        {
            result_ = Int32_Parse(parameter);
        }
        else if (commandType.compare(TestSession::PauseCommand) == 0)
        {
            Pause(parameter);
        }
        else if (commandType.compare(TestSession::RandomPauseCommand) == 0)
        {
            RandomPause(parameter);
        }
        else if (commandType.compare(TestSession::LoadCommand) == 0)
        {
            LoadCommands(parameter);
        }
        else if (commandType.compare(TestSession::SaveCommand) == 0)
        {
            SaveCommands(parameter, wstring());
        }
        else if (commandType.compare(TestSession::LabelCommand) == 0)
        {
            Label = parameter;
        }
        else if (commandType.compare(TestSession::ResetCommand) == 0)
        {
            Reset();
        }
        else if (commandType.compare(TestSession::AutoCommand) == 0)
        {
            set_IsAutoMode(Int32_Parse(parameter) == 0);
        }
        else if (commandType.compare(TestSession::WaitCommand) == 0)
        {
            int secondsToWait = 60;
            if (!parameter.empty())
            {
                secondsToWait = Int32_Parse(parameter);
            }

            WaitForExpectationEquilibrium(TimeSpan::FromSeconds(secondsToWait));
        }
        else if (commandType.compare(TestSession::InterceptCommand) == 0)
        {
            intercept_ = Int32_Parse(parameter);
        }
        else if (commandType.compare(TestSession::TraceCommand) == 0)
        {
            SetTrace(parameter);
        }
        else if (commandType.compare(TestSession::ExpectCommand) == 0)
        {
            verifier_.Expect(parameter);
        }
        else if (commandType.compare(TestSession::ClearCommand) == 0)
        {
            if (parameter.empty())
            {
                verifier_.Clear();
            }
            else
            {
                verifier_.Clear(Int32_Parse(parameter));
            }
        }
        else if (commandType.compare(TestSession::PrintCommand) == 0)
        {
            Print(parameter);
        }
        else if (commandType.compare(TestSession::StateCommand) == 0)
        {
            VerifyState(parameter);
        }
        else if (commandType.compare(TestSession::SaveStateCommand) == 0)
        {
            SaveState(parameter);
        }
        else if (commandType.compare(TestSession::SaveStringCommand) == 0)
        {
            SaveString(parameter);
        }
        else if (commandType.compare(TestSession::WaitForStateCommand) == 0)
        {
            WaitForState(parameter);
        }
        else if (commandType.compare(TestSession::PendingCommand) == 0)
        {
            ShowPendingItems(parameter);
        }
        else if (commandType.compare(TestSession::SetConfigCommand) == 0)
        {
            SetConfig(parameter);
        }
        else if (commandType.compare(TestSession::UpdateConfigCommand) == 0)
        {
            UpdateConfig(parameter);
        }
        else if (commandType.compare(TestSession::AssertCommand) == 0)
        {
            Assert::CodingError("Test assert triggered");
        }
        else if (commandType.compare(TestSession::HelpCommand) == 0)
        {
            PrintHelp(parameter);
        }
        else
        {
            WriteWarning("Dispatch", "Invalid command entered: {0}", command);
        }
    }

    bool TestSession::OpenSession()
    {
        bool result = this->OnOpenSession();
        if (result)
        {
            return dispatcher_.Open();
        }

        return false;
    }

    void TestSession::CloseSession()
    {
        if (TestCommonConfig::GetConfig().DumpCoreAtExit)
        {
            Assert::CodingError("dump core at test exit");
        }

        this->OnCloseSession();
        dispatcher_.Close();
        logWriter_.Close();
    }

    void TestSession::OnError(bool crashTestOnAutoMode)
    {
        if (autoMode_)
        {
            ASSERT_IF(crashTestOnAutoMode, "Test failed with an exception. Crashing test to encure crash dump is generated");
            result_ = -1;
        }
        else
        {
            intercept_ = 1;
        }
    }

    void TestSession::Pause(wstring const & time)
    {
        int waitTime;
        if (!time.empty())
        {
            if (StringUtility::EndsWithCaseInsensitive(time, wstring(L"ms")))
            {
                waitTime = Int32_Parse(time.substr(0, time.size() - 2));
            }
            else
            {
                waitTime = Int32_Parse(time) * 1000;
            }
            Sleep(waitTime);
        }
        else
        {
            console_.WriteLine("Press enter to continue...");
            wstring s;
            console_.ReadLine(s);
        }
    }

    void TestSession::RandomPause(wstring const& interval)
    {
        Random r;
        int waitTime = r.Next(Int32_Parse(interval) * 1000);
        Sleep(waitTime);
    }

    void TestSession::LoadCommands(wstring const & fileName)
    {
        wstring localFileName = fileName;
        bool fileExists = File::Exists(localFileName);
        if (!fileExists && !Path::IsPathRooted(localFileName))
        {
            // Look if file exists at executable path
            localFileName = Path::Combine(Environment::GetExecutablePath(), fileName);
            fileExists = File::Exists(localFileName);
        }

        if (autoMode_)
        {
            TestSession::FailTestIfNot(fileExists, "Cannot load file {0} since it does not exist.", fileName);
        }
        else if (!fileExists)
        {
            WriteWarning("Execute", "Invalid file name: {0}", fileName);
            return;
        }

        bool isLast = commands_.empty();
        queue<Command> tempCommandQueue = move(commands_);
        commands_ = queue<Command>();

        File fileReader;
        auto error = fileReader.TryOpen(localFileName, FileMode::OpenOrCreate, FileAccess::Read, FileShare::ReadWrite);
        if (error.IsSuccess())
        {
            bool done = false;
            int lineNumber = 0;
            while (!done)
            {
                // It is not completely correct to assume that each command spans a line
                // However, the line number is used for diagnostics only so it should be very close to the
                // actual line number from the file and the user can look at the line number reported, the text
                // and the file to find the approximate location in the file
                string line = FileReadLine(fileReader, done);
                lineNumber++;

                wstring commandText(line.begin(), line.end());
                Command command = Command::CreateCommandFromSourceFile(move(commandText), lineNumber);

                if (command.IsEmpty)
                {
                    // Skip empty commands
                    continue;
                }

                // Insert the commands read from the file.
                // Ignore session termination commands unless there is no more
                // command in the buffer.
                bool isTerminateCommand = StringUtility::StartsWith(command.Text, TestSession::QuitCommand) ||
                    StringUtility::StartsWith(command.Text, TestSession::AbortCommand);
                if (isLast || !isTerminateCommand)
                {
                    AddInput(command);
                }
            }

            fileReader.Close();

            intercept_ = 0;
        }

        while (tempCommandQueue.size() > 0)
        {
            auto tempCommand = tempCommandQueue.front();
            tempCommandQueue.pop();
            commands_.push(tempCommand);
        }
    }

    void TestSession::SaveCommands(wstring const & fileName, wstring const & errorMsg)
    {
        FileWriter fileWriter;
        auto error = fileWriter.TryOpen(fileName);
        if (error.IsSuccess())
        {
            File logFileReader;
            error = logFileReader.TryOpen(logFile_, FileMode::Open, FileAccess::Read, FileShare::ReadWrite);
            if (error.IsSuccess())
            {
                bool done = false;
                while (!done)
                {
                    string line = FileReadLine(logFileReader, done);
                    fileWriter << line << TestSession::NewLine;
                }

                if (!errorMsg.empty())
                {
                    string ansiErrorMsg;
                    StringUtility::UnicodeToAnsi(errorMsg, ansiErrorMsg);
                    fileWriter << "#" << ansiErrorMsg << TestSession::NewLine;
                }
            }
        }
    }

    void TestSession::set_Label(wstring const& label)
    {
        label_ = label;
        if (logWriter_.IsValid())
        {
            logWriter_.Close();
        }

        logFile_ = label_ + L".log";
        auto error = logWriter_.TryOpen(logFile_, Common::FileShare::ReadWrite);
        TestSession::FailTestIfNot(
            error.IsSuccess(),
            "Failed to open log file {0} : {1}",
            logFile_,
            error);
        WriteInfo("Log", "Command log file is at {0}", logFile_);
    }

    void TestSession::Reset()
    {
        dispatcher_.Reset();
    }

    void TestSession::AddInput(Command const& command)
    {
        commands_.push(command);
    }

    void TestSession::AddInput(wstring const & command)
    {
        AddInput(Command::CreateCommand(command));
    }

    string TestSession::FileReadLine(File& fileReader, __out bool& eof)
    {
        string line;
        eof = false;
        char charReadFromFile;
        char newLine = '\n';
        char backslash = '\\';
        char space = ' ';
        char tab = '\t';

        bool done = false;
        bool ignoreNewline = false;
        bool ignoreSpace = false;

        do
        {
            int read = fileReader.TryRead(&charReadFromFile, 1);
            if (read == 0)
            {
                eof = true;
                break;
            }

            if (charReadFromFile == '\r')
            {
                continue;
            }

            if (ignoreSpace && (charReadFromFile == (space) || charReadFromFile == (tab)))
            {
                continue;
            }

            if (charReadFromFile == (backslash))
            {
                if (ignoreNewline)
                {
                    line.append(&backslash, 1);
                }
                else
                {
                    ignoreNewline = true;
                }

                continue;
            }

            if (charReadFromFile == (newLine))
            {
                done = !ignoreNewline;
                ignoreSpace = ignoreNewline;
                ignoreNewline = false;
                continue;
            }

            if (charReadFromFile != (newLine))
            {
                if (ignoreNewline)
                {
                    line.append(&backslash, 1);
                    ignoreNewline = false;
                }

                ignoreSpace = false;
            }

            if (charReadFromFile != 0)
            {
                line.append(&charReadFromFile, 1);
            }
        } while (!done);

        return line;
    }

    wstring TestSession::GetCurrentDateTime()
    {
        return wformatString(DateTime::Now().Ticks);
    }

    void TestSession::LogWriterWriteLine(wstring line)
    {
        string lineString;
        StringUtility::UnicodeToAnsi(line, lineString);
        logWriter_ << lineString << TestSession::NewLine;
    }

    void TestSession::WaitForExpectationEquilibrium(Common::TimeSpan timeout)
    {
        TestSession::FailTestIfNot(
            verifier_.WaitForExpectationEquilibrium(timeout),
            "Still expecting events after timeout occurred.");
    }

    void TestSession::Print(std::wstring const & param)
    {
        wstring temp = param;
        if (StringUtility::StartsWith<wstring>(temp, L"<") &&
            StringUtility::EndsWith<wstring>(temp, L">"))
        {
            temp = temp.substr(1, temp.size() - 2);
        }

        WriteInfo(TraceOutput, "{0} = {1}", temp, GetState(temp));
    }

    void TestSession::SaveState(std::wstring const & param)
    {
        vector<wstring> params;
        StringUtility::Split<wstring>(param, params, L" ");

        if (params.size() != 2)
        {
            WriteError(TraceOutput, "SaveState requires two parameters");
            return;
        }

        wstring temp = params[1];
        if (StringUtility::StartsWith<wstring>(temp, L"<") &&
            StringUtility::EndsWith<wstring>(temp, L">"))
        {
            temp = temp.substr(1, temp.size() - 2);
        }

        savedStates_[params[0]] = GetState(temp);
    }

    void TestSession::SaveString(std::wstring const & param)
    {
        vector<wstring> params;
        StringUtility::Split<wstring>(param, params, L" ");

        if (params.size() != 2)
        {
            WriteError(TraceOutput, "SaveString requires two parameters");
            return;
        }

        savedStrings_[params[0]] = params[1];
    }

    wstring TestSession::GetState(std::wstring const & param)
    {
        wstring name = param;
        if (StringUtility::StartsWith<wstring>(param, L"var."))
        {
            name = param.substr(4);
            auto it = savedStates_.find(name);
            if (it != savedStates_.end())
            {
                return it->second;
            }

            return L"";
        }

        return dispatcher_.GetState(name);
    }

    bool TestSession::TryGetString(std::wstring const & param, __out std::wstring & result)
    {
        if (StringUtility::StartsWith<wstring>(param, L"string."))
        {
            wstring key = param.substr(7);
            auto it = savedStrings_.find(key);
            if (it != savedStrings_.end())
            {
                result = it->second;
                return true;
            }
        }

        return false;
    }

    void TestSession::EvaluateCommand(std::wstring & command)
    {
        bool changed = false;
        for (;;)
        {
            size_t i = command.find(L'<');
            if (i == wstring::npos || ((i != wstring::npos) && (command[i - 1] == L'\\')))
            {
                break;
            }

            size_t j = command.find(L'>', i);
            if (j == wstring::npos || j - i <= 1 || ((j != wstring::npos) && (command[j - 1] == L'\\')))
            {
                break;
            }

            wstring stateName = command.substr(i + 1, j - i - 1);
            wstring state;

            if (!TryGetString(stateName, state))
            {
                state = GetState(stateName);
            }

            command = command.substr(0, i) + state + command.substr(j + 1);

            changed = true;
        }

        if (changed)
        {
            WriteInfo(TraceOutput, "Command evaluated: {0}", command);
        }
    }

    void TestSession::WaitForState(std::wstring const & param)
    {
        vector<wstring> params;
        StringUtility::Split<wstring>(param, params, L" ,");

        TestSession::FailTestIf(params.size() < 2, "Invalid number of parameters");

        wstring const& state = params[0];
        wstring const& expectedState = params[1];
        TimeSpan timeout = params.size() > 2 ? TimeSpan::FromSeconds(Double_Parse(params[2])) : TimeSpan::FromSeconds(100);
        VerifyState(state, expectedState, timeout);
    }

    void TestSession::VerifyState(std::wstring const & param)
    {
        vector<wstring> params;
        StringUtility::Split<wstring>(param, params, L" ,");

        if (params.size() >= 2)
        {
            wstring const& state = params[0];
            wstring const& expectedState = params[1];

            VerifyState(state, expectedState, TimeSpan::Zero);
        }
        else
        {
            WriteInfo(TraceOutput, "{0}", GetState(params[0]));
        }
    }

    void TestSession::VerifyState(std::wstring const & state, std::wstring const & expectedState, Common::TimeSpan timeout)
    {
        wstring stateName = state;
        if (StringUtility::StartsWith<wstring>(stateName, L"<") &&
            StringUtility::EndsWith<wstring>(stateName, L">"))
        {
            stateName = stateName.substr(1, stateName.size() - 2);
        }

        vector<wstring> expectedStates;
        StringUtility::Split<wstring>(expectedState, expectedStates, L"|");

        bool matched = false;
        DateTime deadline = DateTime::Now() + timeout;

        while (!matched)
        {
            wstring stateStr = GetState(stateName);

            for (wstring const & value : expectedStates)
            {
                if (stateStr == value)
                {
                    matched = true;
                }
            }

            if (matched)
            {
                WriteInfo(TraceOutput,
                    "State {0} is in expected state {1}",
                    stateName, stateStr);
            }
            else
            {
                if (deadline > DateTime::Now())
                {
                    WriteInfo(TraceOutput,
                        "State {0} expected: {1} / actual: {2}",
                        stateName, expectedState, stateStr);

                    Sleep(1000);
                }
                else
                {
                    TestSession::FailTest(
                        "State {0} expected: {1} / actual: {2}",
                        stateName, expectedState, stateStr);                  
                }
            }
        }
    }

    void TestSession::AddPendingItem(std::wstring const & name, ComponentRootWPtr const & weakRoot)
    {
        pendingItemManager_.AddPendingItem(name, weakRoot);
    }

    bool TestSession::ItemExists(std::wstring const & name)
    {
        return pendingItemManager_.ItemExists(name);
    }

    void TestSession::RemovePendingItem(std::wstring const & name, bool skipExistenceCheck)
    {
        pendingItemManager_.RemovePendingItem(name, skipExistenceCheck);
    }

    void TestSession::ClosePendingItem(std::wstring const & name)
    {
        pendingItemManager_.ClosePendingItem(name);
    }

    void TestSession::PrintPendingItem(StringWriter & w, std::pair<std::wstring, TimeSpan> const & pendingItem) const
    {
        if (pendingItem.second == TimeSpan::MinValue)
        {
            w.Write("{0}\r\n", pendingItem.first);
        }
        else
        {
            w.Write("{0} ===> {1}\r\n", pendingItem.first, pendingItem.second);
        }
    }

    void TestSession::ShowPendingItems(std::wstring const & param) const
    {
        wstring filter;
        int64 seconds;

        size_t index = param.find_last_of(L" ,");
        if (index != wstring::npos)
        {
            filter = param.substr(0, index);
            seconds = Int64_Parse(param.substr(index + 1));
        }
        else
        {
            filter = param;
            seconds = -1;
        }

        wstring output;
        StringWriter w(output);

        auto pendingItems = seconds == -1 ? pendingItemManager_.GetPendingItems(filter) : pendingItemManager_.GetPendingItems(filter, seconds);
        
        for (auto currentItem : pendingItems)
        {
            PrintPendingItem(w, currentItem);
        }

        TestSession::WriteInfo("Output", "Count={0}\r\n{1}", pendingItems.size(), output);
    }
    
    void TestSession::SetConfig(std::wstring const & param)
    {
        StringCollection params;
        StringUtility::Split<wstring>(param, params, L" ");
        params = TestDispatcher::CompactParameters(params);

        TestConfigStore* configStore = dynamic_cast<TestConfigStore*>(Config::Test_DefaultConfigStore().get());
        bool result = configStore->SetConfig(params);
        if (!result)
        {
            TestSession::WriteError(TraceOutput, "Configuration can't be dynamically updated");
        }
    }

    void TestSession::UpdateConfig(std::wstring const & param)
    {
        StringCollection params;
        StringUtility::Split<wstring>(param, params, L" ");
        params = TestDispatcher::CompactParameters(params);

        TestConfigStore* configStore = dynamic_cast<TestConfigStore*>(Config::Test_DefaultConfigStore().get());
        bool result = configStore->UpdateConfig(params);
        if (!result)
        {
            TestSession::WriteError(TraceOutput, "Configuration can't be dynamically updated");
        }
    }

    void TestSession::InternalFailTest(std::string explanation)
    {
        std::string finalMessage;
        Common::StringWriterA sw(finalMessage);
        sw.Write(explanation);

        Common::StackTrace currentStack;
        currentStack.CaptureCurrentPosition();

        TestSession::WriteError("TestFailure", "{0}:\n{1}", finalMessage, currentStack.ToString());
        if (TestSession::assertOnFailTest_)
        {
            if (TestCommonConfig::GetConfig().AssertOnVerifyTimeout)
            {                
                Common::Assert::CodingError("{0}", finalMessage);
            }
            else
            {
                ExitProcess(1);
            }            
        }
        else
        {
            throw std::system_error(microsoft::MakeWindowsErrorCode(ERROR_ASSERTION_FAILURE), finalMessage);
        }
    }

    void TestSession::PrintHelp(wstring const& name)
    {
        TestSession::WriteInfo("Help", " ");
        PrintInternalHelp(name);
        if (name.empty())
        {
            TestSession::WriteInfo("Help", "{0}", externalHelpFile_);
        }
        else
        {
            size_t startIndex = externalHelpFile_.find(L"Command:" + name);
            if (startIndex != wstring::npos)
            {
                size_t endIndex = externalHelpFile_.find(L"Command:", startIndex + 1);
                wstring helpText = externalHelpFile_.substr(startIndex, endIndex != wstring::npos ? endIndex - startIndex : wstring::npos);
                TestSession::WriteInfo("Help", "{0}", helpText);
            }
        }
    }

    void TestSession::PrintInternalHelp(wstring const& name)
    {
        if (name.compare(TestSession::AbortCommand) == 0 || name.empty())
        {
            TestSession::WriteInfo("Help", "{0}: Exit immediately with return code -1\n", TestSession::AbortCommand);
        }
        if (name.compare(TestSession::ExitCommand) == 0 || name.empty())
        {
            TestSession::WriteInfo("Help", "{0}: Exit immediately with given return code. e.g !exit -99\n", TestSession::ExitCommand);
        }
        if (name.compare(TestSession::PauseCommand) == 0 || name.empty())
        {
            TestSession::WriteInfo("Help", "{0}: Pause for specified seconds or milliseconds. e.g. !pause 5 or !pause 5000ms both will pause for 5seconds\n", TestSession::PauseCommand);
        }
        if (name.compare(TestSession::LoadCommand) == 0 || name.empty())
        {
            TestSession::WriteInfo("Help", "{0}: Load a script file. e.g !load MyScriptFile\n", TestSession::LoadCommand);
        }
        if (name.compare(TestSession::SaveCommand) == 0 || name.empty())
        {
            TestSession::WriteInfo("Help", "{0}: Save current commands to a file. e.g !save MySaveFile\n", TestSession::SaveCommand);
        }
        if (name.compare(TestSession::LabelCommand) == 0 || name.empty())
        {
            TestSession::WriteInfo("Help", "{0}: Rename the TestSession label. e.g !label NewLabel\n", TestSession::LabelCommand);
        }
        if (name.compare(TestSession::ResetCommand) == 0 || name.empty())
        {
            TestSession::WriteInfo("Help", "{0}: Resets the test dispatcher. e.g !reset\n", TestSession::ResetCommand);
        }
        if (name.compare(TestSession::AutoCommand) == 0 || name.empty())
        {
            TestSession::WriteInfo("Help", "{0}: Puts the session in auto mode. This means the test will exit if the processing of the external command fails. e.g !auto\n", TestSession::AutoCommand);
        }
        if (name.compare(TestSession::WaitCommand) == 0 || name.empty())
        {
            TestSession::WriteInfo("Help", "{0}: Wait for pending async events to be validated for the specified nhumber of seconds. e.g !wait 20\n", TestSession::WaitCommand);
        }
        if (name.compare(TestSession::InterceptCommand) == 0 || name.empty())
        {
            TestSession::WriteInfo("Help", "{0}: Toggles in and out of interactive mode. e.g !intercept 0  to switch to interactive or !intercept -1 to switch out\n", TestSession::InterceptCommand);
        }
        if (name.compare(TestSession::TraceCommand) == 0 || name.empty())
        {
            TestSession::WriteInfo("Help", "{0}: Set trace filters for the console sink during runtime\n", TestSession::TraceCommand);
        }
        if (name.compare(TestSession::ExpectCommand) == 0 || name.empty())
        {
            TestSession::WriteInfo("Help", "{0}: Add an event which needs to be validated. e.g !expect NodeOpened\n", TestSession::ExpectCommand);
        }
        if (name.compare(TestSession::ClearCommand) == 0 || name.empty())
        {
            TestSession::WriteInfo("Help", "{0}: Clear the last n expected events. e.g !clear 5\n", TestSession::ClearCommand);
        }
        if (name.compare(TestSession::PrintCommand) == 0 || name.empty())
        {
        }
        if (name.compare(TestSession::StateCommand) == 0 || name.empty())
        {
        }
        if (name.compare(TestSession::WaitForStateCommand) == 0 || name.empty())
        {
        }
        if (name.compare(TestSession::PendingCommand) == 0 || name.empty())
        {
        }
        if (name.compare(TestSession::HelpCommand) == 0 || name.empty())
        {
            TestSession::WriteInfo("Help", "{0}: Prints out help. e.g !help CommandName\n", TestSession::HelpCommand);
        }
    }

}
