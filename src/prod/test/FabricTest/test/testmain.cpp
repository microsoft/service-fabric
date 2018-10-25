// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#ifdef PLATFORM_UNIX

#include "Common/CryptoUtility.Linux.h"
#include "Common/CryptoTest.h"

namespace FabricTest
{
    Common::Global<Common::InstallTestCertInScope> defaultClusterCert;
    Common::Global<Common::InstallTestCertInScope> defaultClientCert;
}

#endif

#include <ktl.h>
#include <ktrace.h>

using namespace TestCommon;
using namespace FederationTestCommon;
using namespace FabricTest;
using namespace std;
using namespace Common;

wstring Label = L"FabricTest";
wstring LoadFile = L"";
wstring RandomTestType = L"Failover";
bool Auto = false;
wstring CredentialType = L"None";
Transport::SecurityProvider::Enum SecurityProvider = Transport::SecurityProvider::None;
bool RandomSession = false;
int MaxIterations = std::numeric_limits<int>::max();
int Timeout = 0;
int PauseWhenStart = 0;
USHORT StartPortRange = 0;
USHORT EndPortRange = 0;
wstring LogOption = L"min:30,f:5";
bool NoTestDir = false;
bool EnableNativeImageStore = false;
bool EnableTStoreSystemServices = false;
bool EnableEseSystemServices = false;
bool ToggleNativeStoreOnDate = false;
bool EnableContainers = false;

wstring LoadArg = L"-load";
wstring AutoArg = L"-auto";
wstring PauseArg = L"-pause";
wstring RandomTestIterationArg = L"-r";
wstring RandomTestTimeArg = L"-t";
wstring RandomTestTypeArg = L"-type";
wstring CredentialTypeArg = L"-security";
wstring EtwEnabledArg = L"-etw";
wstring PortRangeArg = L"-port";
wstring CommandSign = L"-";
wstring LogArg = L"-log";
wstring NoTestDirArg = L"-notestdir";
wstring EnableNativeImageStoreArg = L"-enableNativeImageStore";
wstring EnableTStoreSystemServicesArg = L"-tstore";
wstring EnableNativeStoreSystemServicesArg = L"-nativestore";
wstring EnableEseSystemServicesArg = L"-ese";
wstring ToggleNativeStoreOnDateArg = L"-togglestore";
wstring EnableContainersArg = L"-enableContainers";

#define ISARGKEYWORD(X,Y) (Common::StringUtility::AreEqualCaseInsensitive(X, Y) || Common::StringUtility::StartsWith(X, Y+L":"))

bool ParseArguments(Common::StringCollection const& args)
{
    for(size_t i = 1; i < args.size(); ++i)
    {
        wstring arg = args[i];
        size_t index = arg.find_first_of(L":");
        if (arg.empty()) continue;
        else if (ISARGKEYWORD(arg, LoadArg)) 
        {
            if(index == wstring::npos)
            {
                return false;
            }

            LoadFile = arg.substr(index + 1, arg.size());
        }
        else if (ISARGKEYWORD(arg, AutoArg)) 
        {
            Auto = true;
        }
        else if (ISARGKEYWORD(arg, PauseArg))
        {
            if(index == wstring::npos)
            {
                return false;
            }

            wstring value = arg.substr(index + 1, arg.size());
            PauseWhenStart = _wtoi(value.c_str());
        }
        else if (ISARGKEYWORD(arg, CredentialTypeArg))
        {
            if(index == wstring::npos)
            {
                CredentialType = L"X509"; // Set default credential type when type value is missing
                SecurityProvider = Transport::SecurityProvider::Ssl;
            }
            else
            {
                CredentialType = arg.substr(index + 1, arg.size());
                auto error = Transport::SecurityProvider::FromCredentialType(CredentialType, SecurityProvider);
                if (!error.IsSuccess())
                {
                    TestSession::WriteError("FabricTest", "Unknown credential type: {0}", CredentialType);
                    return false;
                }
            }

            TestSession::WriteInfo("FabricTest", "Security provider set to {0} based on credential type {1}", SecurityProvider, CredentialType);
        }
        else if (ISARGKEYWORD(arg, EtwEnabledArg))
        {
            FabricTestSessionConfig::GetConfig ().UseEtw = true;
        }
        else if (ISARGKEYWORD(arg, RandomTestTypeArg)) 
        {
            if(index == wstring::npos)
            {
                return false;
            }

            RandomTestType = arg.substr(index + 1, arg.size());
        }
        else if (ISARGKEYWORD(arg, RandomTestIterationArg)) 
        {
            RandomSession = true;
            if(index != wstring::npos)
            {
                wstring value = arg.substr(index + 1, arg.size());
                MaxIterations = _wtoi(value.c_str());
            }
            else
            {
                MaxIterations = std::numeric_limits<int>::max();
            }
        }
        else if (ISARGKEYWORD(arg, RandomTestTimeArg)) 
        {
            RandomSession = true;
            if(index != wstring::npos)
            {
                wstring value = arg.substr(index + 1, arg.size());
                Timeout = _wtoi(value.c_str()) * 60;
            }
            else
            {
                Timeout = 0;
            }
        }
        else if (ISARGKEYWORD(arg, PortRangeArg))
        {
            if(index == wstring::npos)
            {
                return false;
            }

            wstring value = arg.substr(index + 1, arg.size());
            StringCollection portRange;
            StringUtility::Split<wstring>(value, portRange, L",");
            if(portRange.size() != 2)
            {
                return false;
            }

            StartPortRange = CommandLineParser::ParsePort(portRange[0], "start");
            EndPortRange = CommandLineParser::ParsePort(portRange[1], "end");
            TestSession::FailTestIfNot(EndPortRange - StartPortRange >= 500, "StartPortRange - EndPortRange should be greater than 500 {0}", arg);
        }
        else if(ISARGKEYWORD(arg, EnableNativeImageStoreArg))
        {
            EnableNativeImageStore = true;
        }
        else if(ISARGKEYWORD(arg, EnableTStoreSystemServicesArg) || ISARGKEYWORD(arg, EnableNativeStoreSystemServicesArg))
        {
            EnableTStoreSystemServices = true;
        }
        else if (ISARGKEYWORD(arg, EnableEseSystemServicesArg))
        {
            EnableEseSystemServices = true;
        }
        else if (ISARGKEYWORD(arg, EnableContainersArg))
        {
            EnableContainers = true;
        }
        else if (ISARGKEYWORD(arg, ToggleNativeStoreOnDateArg))
        {
            ToggleNativeStoreOnDate = true;
        }
        else if (Common::StringUtility::StartsWith(arg, LogArg))
        {
            if(index != wstring::npos)
            {
                LogOption = args[i].substr(index + 1);
            }
        }
        else if (Common::StringUtility::StartsWith(arg, NoTestDirArg))
        {
            NoTestDir = true;
        }
        else if (i == args.size() - 1 && LoadFile.size() == 0 && !Common::StringUtility::StartsWith(arg, CommandSign))
        {
            LoadFile = args[i];
        }
        else
        {
            TestSession::WriteInfo("FabricTest", "Unknown command line option {0}", args[i]);
            return false;
        }
    }

    if (EnableEseSystemServices && EnableTStoreSystemServices)
    {
        TestSession::WriteError("FabricTest", "Both -tsore and -ese set which is not allowed.");
    }

    if(RandomSession && (LoadFile.size() != 0))
    {
        return false;
    }
    else
    {
        return true;
    }
}

void PrintHelp()
{
    TestSession::WriteInfo("FabricTest", "\nThis exe invokes FaricTest which is the E2E developer test.");
    TestSession::WriteInfo("FabricTest", "Just invoking the exe with no arguments starts FabricTest in the interactive mode waiting for user input.\n");
    TestSession::WriteInfo("FabricTest", "FabricTest.exe <TestScriptFileName> with no additional options is a convenience option to run the specified test script .\n");
    TestSession::WriteInfo("FabricTest", "Other options are specified below\n");

    TestSession::WriteInfo("FabricTest", "\t-load:TestScriptFileName        Runs specified script file\n");

    TestSession::WriteInfo("FabricTest", "\t-auto                           Sets auto mode to true meaning test exits on any error rather than going into interactive mode\n");

    TestSession::WriteInfo("FabricTest", "\t-pause:secs                     Number of seconds to pause at before starting script file or longhaul mode\n");

    TestSession::WriteInfo("FabricTest", "\t-r:iterations                   Enables longhaul mode & specifies number of iterations to run for. Defaults to int.Max\n");

    TestSession::WriteInfo("FabricTest", "\t-t:minutes                      Enables longhaul mode & specifies number of minutes to run for. Defaults to TimeSpan.MaxValue\n");

    TestSession::WriteInfo("FabricTest", "\t-type:RandomTestType            If longhaul mode enabled it specifies the mode (Upgrade, Failover, Rebuild, Unreliable etc). Defaults to Failover\n");

    TestSession::WriteInfo("FabricTest", "\t-security[:X509/Windows]        Enable security\n");

    TestSession::WriteInfo("FabricTest", "\t-etw                            Test will use Etw instead of file tracing. Only applies to longhaul(random) runs\n");

    TestSession::WriteInfo("FabricTest", "\t-port:StartPort,EndPort         Specifies the port range for the test\n");

    TestSession::WriteInfo("FabricTest", "\t-log:TextTraceFileOption        Specifies the text trace file option\n");

    TestSession::WriteInfo("FabricTest", "\t-notestdir                      Used when running these test in ETCM so that the directory path is small (i.e. < 256 chars)\n");

    TestSession::WriteInfo("FabricTest", "\t-enableNativeImageStore        Use NativeImageStore\n");
    TestSession::WriteInfo("FabricTest", "\t-enableContainers              Enables testing containers. This is disabled by default. A test using containers should pass this argument. Note that container based tests should run sequentially.\n");
    TestSession::WriteInfo("FabricTest", "\t-nativestore                   Use TStore for system services. Cannot be set with -ese\n");
    TestSession::WriteInfo("FabricTest", "\t-ese                           Use Ese for system services. Cannot be set with -tstore\n");
    TestSession::WriteInfo("FabricTest", "\t-togglestore                   Switches the cluster deployed by the test to use native store stack or ese based on date. If even it is Native Store and if odd it uses Ese. If -ses or -tstore set those will take preference\n");
}

int main(int argc, __in_ecount( argc ) char** argv) 
{
#ifndef PLATFORM_UNIX 
    CRTAbortBehavior::DisableAbortPopup();    
#endif

    ConfigStoreSPtr configStore = make_shared<TestConfigStore>(Config::InitConfigStore());
    Config::SetConfigStore(configStore);

    { Common::Config config; } // trigger tracing config loading
    TraceTextFileSink::SetOption(L""); // Empty options since the one in the config are used by the ImageBuilder.exe
    TraceTextFileSink::SetPath(L""); // Empty trace file path so that nothing is traced until the actual path is set

#ifdef PLATFORM_UNIX
    SecurityTestSetup SecurityTestSetup;
#endif

    StringCollection args;
    for (int i = 0; i < argc; i++)
    {
        args.push_back(wstring(argv[i], argv[i] + strlen(argv[i])));
    }
    
    if (!ParseArguments(args))
    {
        PrintHelp();
        return -1;
    }

    if(StartPortRange > 0 && EndPortRange > 0)
    {
        AddressHelper::SeedNodeRangeStart = StartPortRange;
        AddressHelper::SeedNodeRangeEnd = StartPortRange + 30;
        AddressHelper::StartPort = AddressHelper::SeedNodeRangeEnd + 1;
        AddressHelper::EndPort = EndPortRange - 1;
        AddressHelper::ServerListenPort = EndPortRange;
    }
    else
    {
        USHORT startRange = 0;
        TestPortHelper::GetPorts(500, startRange);
        AddressHelper::SeedNodeRangeStart = startRange;
        AddressHelper::SeedNodeRangeEnd = startRange + 30;
        AddressHelper::StartPort = AddressHelper::SeedNodeRangeEnd + 1;
        AddressHelper::EndPort = startRange + 498;
        AddressHelper::ServerListenPort = startRange + 499;
    }

    
    if(RandomSession)
    {
        Label = RandomTestType + L"Random";

        RandomFabricTestSession::CreateSingleton(MaxIterations, Timeout, Label, Auto);
        if(NoTestDir)
        {
            FABRICSESSION.FabricDispatcher.TestDataDirectory = Path::Combine(Directory::GetCurrentDirectoryW(), L"Data");
        }
        else
        {
            FABRICSESSION.FabricDispatcher.TestDataDirectory = Path::Combine(Directory::GetCurrentDirectoryW(), Label + L".Data");
        }

        if(FabricTestSessionConfig::GetConfig ().UseEtw)
        {
            TraceProvider::GetSingleton()->SetDefaultLevel(TraceSinkType::ETW, LogLevel::Noise);
            TraceProvider::GetSingleton()->AddFilter(TraceSinkType::ETW, L"EseStore:4");
            TraceProvider::GetSingleton()->AddFilter(TraceSinkType::ETW, L"Transport:4");
        }
        else
        {
            wstring traceFileName = Label + L".trace";
            TraceTextFileSink::SetPath(traceFileName);
            TraceTextFileSink::SetOption(LogOption);
        }

        if (PauseWhenStart > 0)
        {
            wstring pauseCommand = L"!pause ";
            pauseCommand = pauseCommand.append(StringUtility::ToWString(PauseWhenStart));
            FABRICSESSION.AddCommand(pauseCommand);
        }
    }
    else
    {
        if(!LoadFile.empty())
        {
            Label = Path::GetFileName(LoadFile);
        }

        FabricTestSession::CreateSingleton(Label, Auto);
        if(NoTestDir)
        {
            FABRICSESSION.FabricDispatcher.TestDataDirectory = Path::Combine(Directory::GetCurrentDirectoryW(), L"Data");
        }
        else
        {
            FABRICSESSION.FabricDispatcher.TestDataDirectory = Path::Combine(Directory::GetCurrentDirectoryW(), Label + L".Data");
        }

        TraceTextFileSink::SetPath(FabricTestSessionConfig::GetConfig().TraceFileName);
        TraceTextFileSink::SetOption(L"");
		
        if (PauseWhenStart > 0)
        {
            wstring pauseCommand = L"!pause ";
            pauseCommand = pauseCommand.append(StringUtility::ToWString(PauseWhenStart));
            FABRICSESSION.AddCommand(pauseCommand);
        }

        if(!LoadFile.empty())
        {
            Label = LoadFile + L"Label";
            wstring traceFileName = Path::GetFileName(LoadFile) + L".trace";
            TraceTextFileSink::SetPath(traceFileName);
            wstring loadCommand = L"!load,";
            loadCommand = loadCommand.append(LoadFile);
            FABRICSESSION.AddCommand(loadCommand);
        }
    }

    if(SecurityProvider == Transport::SecurityProvider::Ssl)
    {
#ifdef PLATFORM_UNIX
        FABRICSESSION.FabricDispatcher.NewTestCerts();
#else
        wstring clusterCredentials = L"X509:EncryptAndSign:WinFabricRing.Rings.WinFabricTestDomain.com:WinFabricRing.Rings.WinFabricTestDomain.com:WinFabricRing.Rings.WinFabricTestDomain.com";
        wstring defaultNodeCredentials = wformatString(
            "FindByThumbprint,78:12:20:5a:39:d2:23:76:da:a0:37:f0:5a:ed:e3:60:1a:7e:64:bf,{0},{1}",
            X509Default::StoreName(), X509Default::StoreLocation());

        wstring clientCredentials = ClientSecurityHelper::DefaultSslClientCredentials;

        FABRICSESSION.FabricDispatcher.SetClusterWideCredentials(clusterCredentials);
        FABRICSESSION.FabricDispatcher.SetDefaultNodeCredentials(defaultNodeCredentials);
        FABRICSESSION.FabricDispatcher.SetClientCredentials(clientCredentials);
#endif
    }
    else if (IsWindowsProvider(SecurityProvider))
    {
        wstring clusterCredentials = L"Windows:EncryptAndSign";
        FABRICSESSION.FabricDispatcher.SetClusterWideCredentials(clusterCredentials);

        wstring clientCredentials = ClientSecurityHelper::DefaultWindowsCredentials;
        FABRICSESSION.FabricDispatcher.SetClientCredentials(clientCredentials);
    }

    if(EnableNativeImageStore)
    {
        FABRICSESSION.FabricDispatcher.EnableNativeImageStore();
    }

#if !defined(PLATFORM_UNIX)
    // Default is Ese
    bool enableNativeStack = false;
    if (EnableTStoreSystemServices)
    {
        enableNativeStack = true;
    }
    else if (EnableEseSystemServices)
    {
        enableNativeStack = false;
    }
    else if (ToggleNativeStoreOnDate)
    {
        auto systemTime = DateTime::Now().GetSystemTime();
        auto currentDay = systemTime.wDay;
        enableNativeStack = currentDay % 2 == 0 ? true : false;
    }

    if (enableNativeStack)
    {
        TestSession::WriteInfo("FabricTest", "******** SystemService/KVS Store set to: Native Stack ********");
        FABRICSESSION.FabricDispatcher.EnableTStoreSystemServices();
    }
    else
    {
        TestSession::WriteInfo("FabricTest", "******** SystemService/KVS Store set to: ESE ********");
    }
#endif

    if (FabricTestSessionConfig::GetConfig().HealthFullVerificationEnabled || RandomSession)
    {
        FabricTestSessionConfig::GetConfig().HealthVerificationEnabled = true;
    }

    if (FabricTestSessionConfig::GetConfig().HealthVerificationEnabled)
    {
        Reliability::FailoverConfig & failoverConfig = Reliability::FailoverConfig::GetConfig();
        failoverConfig.ReconfigurationTimeLimit = TimeSpan::FromSeconds(10);
        failoverConfig.PlacementTimeLimit = TimeSpan::FromSeconds(10);
        failoverConfig.BuildReplicaTimeLimit = TimeSpan::FromSeconds(10);
        failoverConfig.CreateInstanceTimeLimit = TimeSpan::FromSeconds(10);
        Management::ManagementConfig & healthConfig = Management::ManagementConfig::GetConfig();
        healthConfig.HealthStoreCleanupInterval = TimeSpan::FromSeconds(10);
        Client::ClientConfig & healthClientConfig = Client::ClientConfig::GetConfig();
        healthClientConfig.HealthOperationTimeout = TimeSpan::FromSeconds(15);
        healthClientConfig.HealthReportSendInterval = TimeSpan::FromSeconds(10);
    }
    if (EnableContainers)
    {
        auto & hostingConfig = Hosting2::HostingConfig::GetConfig();
        hostingConfig.DisableContainers = false;
    }

    //
    // Set FabricCodePath and FabricLogRoot environment variable. This is needed by
    // FabricCAS to set binds while launching containers. These do not really get used
    // and value could be any non-empty paths. Settings it to current directory as these
    // paths do not really apply to FabricTest.
    // 
    auto currentDir = Directory::GetCurrentDirectoryW();
    auto codePathEnvVar = *(FabricEnvironment::FabricCodePathEnvironmentVariable);
    auto logRootEnvVar = *(FabricEnvironment::FabricLogRootEnvironmentVariable);

    if (!Environment::SetEnvironmentVariable(codePathEnvVar, currentDir))
    {
        TestSession::WriteError("FabricTest", "Failed to set FabricCodePath environment variable.");
    }

    if (!Environment::SetEnvironmentVariable(logRootEnvVar, currentDir))
    {
        TestSession::WriteError("FabricTest", "Failed to set FabricLogRoot environment variable.");
    }
    
    //
    // Enable KTL tracing into the SF file log
    //
    RegisterKtlTraceCallback(Common::ServiceFabricKtlTraceCallback);

    DateTime startTime = DateTime::Now();
    DWORD pid = GetCurrentProcessId();
    TestSession::WriteInfo("FabricTest", "Fabric test started with process id {0}....", pid);
    int result = FABRICSESSION.Execute();

    TestSession::WriteInfo("FabricTest",
        "Fabric session exited with result {0}, time spent: {1} ",
        result, DateTime::Now() - startTime);

    return result;
}
