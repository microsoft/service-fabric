// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestCommon/TestSession.h"
#include "FederationTest/FederationTestDispatcher.h"
#include "FederationTest/FederationTestSession.h"
#include "FederationTest/RandomFederationTestSession.h"
#include "FederationTest/TestFederation.h"

#ifdef PLATFORM_UNIX

#include "Common/CryptoUtility.Linux.h"
#include "Common/CryptoTest.h"

namespace FederationTest
{
    Common::Global<Common::InstallTestCertInScope> testCert;
}

#endif

using namespace TestCommon;
using namespace FederationTestCommon;
using namespace FederationTest;
using namespace std;
using namespace Common;

wstring Label = L"FederationTest";
wstring LoadFile = L"";
bool Auto = false;
bool RandomSession = false;
int Iterations = 0;
int Timeout = 0;
USHORT StartPortRange = 0;
USHORT EndPortRange = 0;
wstring LogOption = L"min:30,f:3";
int RingCount = 1;

wstring LabelArg = L"-label";
wstring LoadArg = L"-load";
wstring AutoArg = L"-auto";
wstring RandomTestIterationArg = L"-r";
wstring RandomTestTimeArg = L"-t";
wstring PortRangeArg = L"-port";
wstring SecurityArg = L"-security";
wstring LogArg = L"-log";
wstring MultiRingArg = L"-m";

bool ParseArguments(Common::StringCollection const& args)
{
    for(size_t i = 1; i < args.size(); ++i)
    {
        wstring arg = args[i];
        size_t index = arg.find_first_of(L":");
        if (args[i].empty()) continue;
        else if (Common::StringUtility::StartsWith(arg, LabelArg)) 
        {
            TestSession::FailTestIf(index == wstring::npos, "Invalid args {0}", arg);
            Label = arg.substr(index + 1, arg.size());
        }
        else if (Common::StringUtility::StartsWith(arg, LoadArg)) 
        {
            TestSession::FailTestIf(index == wstring::npos, "Invalid args {0}", arg);
            LoadFile = arg.substr(index + 1, arg.size());
        }
        else if (Common::StringUtility::StartsWith(arg, AutoArg)) 
        {
            Auto = true;
        }
        else if (Common::StringUtility::StartsWith(arg, RandomTestIterationArg)) 
        {
            RandomSession = true;
            if(index != wstring::npos)
            {
                wstring value = arg.substr(index + 1, arg.size());
                Iterations = Int32_Parse(value);
            }
            else
            {
                Iterations = std::numeric_limits<int>::max();
            }
        }
        else if (Common::StringUtility::StartsWith(arg, RandomTestTimeArg)) 
        {
            RandomSession = true;
            if(index != wstring::npos)
            {
                wstring value = arg.substr(index + 1, arg.size());
                Timeout = Int32_Parse(value) * 60;
            }
            else
            {
                Timeout = 0;
            }
        }
        else if (Common::StringUtility::StartsWith(arg, SecurityArg))
        {
            vector<wstring> splitted;
            StringUtility::Split<wstring>(arg, splitted, L":");

            if (splitted.size() == 1)
            {
                // Set to default security provider
                FederationTestDispatcher::SecurityProvider = Transport::SecurityProvider::Ssl;
                RandomFederationTestConfig::GetConfig().MaxAllowedMemoryInMB = RandomFederationTestConfig::GetConfig().MaxAllowedMemoryInMBForSsl;
            }
            else
            {
                auto error = Transport::SecurityProvider::FromCredentialType(splitted[1], FederationTestDispatcher::SecurityProvider);
                if (!error.IsSuccess())
                {
                    RealConsole().WriteLine("Invalid security credential type: {0}", splitted[1]);
                    return false;
                }

                if (Transport::SecurityProvider::IsWindowsProvider(FederationTestDispatcher::SecurityProvider))
                {
                    RandomFederationTestConfig::GetConfig().MaxAllowedMemoryInMB = RandomFederationTestConfig::GetConfig().MaxAllowedMemoryInMBForKerberos;
                    FederationTestDispatcher::SetClusterSpnIfNeeded();
                }
            }

#ifdef PLATFORM_UNIX
            if (FederationTestDispatcher::SecurityProvider == Transport::SecurityProvider::Ssl)
            {
                testCert = make_global<InstallTestCertInScope>();
            }
#endif
        }
        else if (Common::StringUtility::StartsWith(arg, PortRangeArg))
        {
            TestSession::FailTestIf(index == wstring::npos, "Invalid args {0}", arg);
            wstring value = arg.substr(index + 1, arg.size());
            StringCollection portRange;
            StringUtility::Split<wstring>(value, portRange, L",");
            TestSession::FailTestIfNot(portRange.size() == 2, "Invalid args {0}", arg);
            StartPortRange = CommandLineParser::ParsePort(portRange[0], "start");
            EndPortRange = CommandLineParser::ParsePort(portRange[1], "end");
            TestSession::FailTestIfNot(EndPortRange - StartPortRange >= 50, "StartPortRange - EndPortRange should be greater than 50 {0}", arg);
        }
        else if (Common::StringUtility::StartsWith(arg, LogArg))
        {
            if(index != wstring::npos)
            {
                LogOption = args[i].substr(index + 1);
            }
        }
        else if (Common::StringUtility::StartsWith(arg, MultiRingArg))
        {
            if(index != wstring::npos)
            {
                wstring value = arg.substr(index + 1, arg.size());
                RingCount = Int32_Parse(value);
            }
        }
        else if (i == args.size() - 1 && LoadFile.size() == 0)
        {
            LoadFile = args[i];
        }
        else
        {
            RealConsole().WriteLine("Unknown command line option {0}", args[i]);
            return false;
        }
    }

    return true;
}

int main(int argc, __in_ecount( argc ) char** argv) 
{
    ConfigStoreSPtr configStore = make_shared<TestConfigStore>(Config::InitConfigStore());
    Config::SetConfigStore(configStore);
    { Config config; } // trigger tracing config loading

#ifdef PLATFORM_UNIX
    SecurityTestSetup SecurityTestSetup;
#endif

    Common::StringCollection args;
    for (int i = 0; i < argc; i++)
    {
        args.push_back(wstring(argv[i], argv[i] + strlen(argv[i])));
    }
    
    if (!ParseArguments(args))
    {
        return -1;
    }

    if(StartPortRange > 0 && EndPortRange > 0)
    {
        AddressHelper::SeedNodeRangeStart = StartPortRange;
        AddressHelper::SeedNodeRangeEnd = StartPortRange + 30;
    }
    else
    {
        USHORT startRange = 0;
        TestPortHelper::GetPorts(100, startRange);
        AddressHelper::SeedNodeRangeStart = startRange;
        AddressHelper::SeedNodeRangeEnd = startRange + 30;
    }

    if(RandomSession)
    {
        Label = L"FederationRandom";
        TraceTextFileSink::SetOption(LogOption);
        RandomFederationTestSession::CreateSingleton(Iterations, Timeout, Label, Auto, RingCount);
        FEDERATIONSESSION.FederationDispatcher.TestDataDirectory = Label + L".Data";
    }
    else
    {
        if(!LoadFile.empty())
        {
            Label = Path::GetFileName(LoadFile);
        }

        FederationTestSession::CreateSingleton(Label, Auto);
        FEDERATIONSESSION.FederationDispatcher.TestDataDirectory = Label + L".Data";

        if(!LoadFile.empty())
        {
            wstring traceFileName = Path::GetFileName(LoadFile) + L".trace";
            TraceTextFileSink::SetPath(traceFileName);
            wstring loadCommand = L"!load,";
            loadCommand = loadCommand.append(LoadFile);
            FEDERATIONSESSION.AddCommand(loadCommand);
        }
    }

    DateTime startTime = DateTime::Now();
    DWORD pid = GetCurrentProcessId();
    TestSession::WriteInfo("Federationtest", "Federation test started with process id {0}....", pid);
    TestSession::WriteInfo("Federationtest", "Security provider: {0}", FederationTestDispatcher::SecurityProvider);

    int result = FEDERATIONSESSION.Execute();

    TestSession::WriteInfo("Federationtest",
        "Federation session exited with result {0}, time spent: {1} ",
        result, DateTime::Now() - startTime);

#ifdef PLATFORM_UNIX
    if (testCert)
    {
        testCert->Uninstall();
    }
#endif

    return result;
}
