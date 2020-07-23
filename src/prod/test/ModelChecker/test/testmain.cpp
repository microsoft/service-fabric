// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestDispatcher.h"
#include "TestSession.h"
//#include "MyStack.h"

using namespace std;
using namespace Common;
using namespace ModelChecker;

wstring Label = L"ModelChecker";
static wstring LoadFile = L"";
bool Auto = false;

void OutputUsage()
{
    RealConsole().WriteLine("Windows fabric model checker");
    RealConsole().WriteLine("");
    RealConsole().WriteLine("ModelChecker [-auto] -load filename");
}

bool ParseArguments(StringCollection const& args)
{
    for(size_t i = 1; i < args.size(); ++i)
    {
        if (args[i].empty())
        {
            continue;
        }
        else if (args[i] == L"-load") 
        {
            LoadFile = args[++i];
        }
        else if (args[i] == L"-auto") 
        {
            Auto = true;
        }
        else
        {
            RealConsole().WriteLine("Unknown command line option {0}", args[i]);
            return false;
        }
    }

    return true;
}

int __cdecl wmain(int argc, __in_ecount( argc ) wchar_t ** argv) 
{
    Common::StringCollection args(argv+0, argv+argc);
    if (!ParseArguments(args))
    {
        OutputUsage();
        return -1;
    }

    TestDispatcher testDispatcher;
    shared_ptr<TestSession> session; 

    ModelCheckerConfig & config = testDispatcher.ModelCheckerConfigObj;
    config.VerifyTimeout = TimeSpan::FromMinutes(1);
    // Reliability::Failove & config = testDispatcher.PLBConfigObj;

    session = make_shared<TestSession>(Label, Auto, testDispatcher);
    if(!LoadFile.empty())
    {
        wstring loadCommand = L"!load,";
        loadCommand = loadCommand.append(LoadFile);
        session->AddCommand(loadCommand);
    }

    DateTime startTime = DateTime::Now();
    DWORD pid = GetCurrentProcessId();
    TestSession::WriteInfo("ModelChecker", "ModelChecker started with process id {0}....", pid);
    int result = session->Execute();
    TestSession::WriteInfo("ModelChecker",
        "ModelChecker session exited with result {0}, time spent: {1} ",
        result, DateTime::Now() - startTime);

    return result;
}
