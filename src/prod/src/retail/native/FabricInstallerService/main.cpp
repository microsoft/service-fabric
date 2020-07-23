// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricInstallerService;
using namespace ServiceModel;
using namespace std;

void RunAsService();
void RunAsConsoleApplication(Common::StringCollection const &);
void PrintUsage();
void SetupTextTracing();

static RealConsole console;
static bool autocleanEnabled = false;

int __cdecl wmain(int argc, __in_ecount(argc) wchar_t* argv[])
{
    HeapSetInformation(GetProcessHeap(), HeapEnableTerminationOnCorruption, NULL, 0);

    try
    {
        SetupTextTracing();
        Common::StringCollection args(argv, argv+argc);
        size_t count = 1;
        bool runAsConsoleApp = false;
        while (count < args.size()) 
        {
            if (Common::StringUtility::AreEqualCaseInsensitive(args[count], FabricInstallerService::Constants::CommandLineOptionService) ||
                Common::StringUtility::AreEqualCaseInsensitive(args[count], FabricInstallerService::Constants::CommandLineOptionServiceShort))
            {
                runAsConsoleApp = false;
            }
            else if (Common::StringUtility::AreEqualCaseInsensitive(args[count], FabricInstallerService::Constants::CommandLineOptionConsole) ||
                Common::StringUtility::AreEqualCaseInsensitive(args[count], FabricInstallerService::Constants::CommandLineOptionConsoleShort))
            {
                runAsConsoleApp = true;
            }
            else if (Common::StringUtility::AreEqualCaseInsensitive(args[count], FabricInstallerService::Constants::CommandLineOptionAutoclean) ||
                Common::StringUtility::AreEqualCaseInsensitive(args[count], FabricInstallerService::Constants::CommandLineOptionAutocleanShort))
            {
                autocleanEnabled = true;
            }
            else if (Common::StringUtility::AreEqualCaseInsensitive(args[count], FabricInstallerService::Constants::CommandLineOptionHelp) ||
                Common::StringUtility::AreEqualCaseInsensitive(args[count], FabricInstallerService::Constants::CommandLineOptionHelpShort))
            {
                PrintUsage();
                return 1;
            }
            else
            {
                PrintUsage();
                return 1;
            }

            count++;
        }

        if(runAsConsoleApp)
        {
            RunAsConsoleApplication(args);
            return 0;
        }
        else
        {
            RunAsService();
            return 0;
        }
    }
    catch (std::exception const& e)
    {
        console.WriteLine("{0} {1}", StringResource::Get(IDS_FABRICHOST_EXCEPTION), e);
        return FabricInstallerService::Constants::ExitCodeUnhandledException;
    }
}

void PrintUsage()
{
    console.WriteLine("{0} [options]", Path::GetFileNameWithoutExtension(Environment::GetExecutableFileName()));
    console.WriteLine("    {0}/{1} : run as console application", FabricInstallerService::Constants::CommandLineOptionConsoleShort, FabricInstallerService::Constants::CommandLineOptionConsole);
    console.WriteLine("    {0}/{1} : clean Fabric files for removal scenario", FabricInstallerService::Constants::CommandLineOptionAutocleanShort, FabricInstallerService::Constants::CommandLineOptionAutoclean);

    console.WriteLine("    {0}/{1} : print usage", FabricInstallerService::Constants::CommandLineOptionHelpShort, FabricInstallerService::Constants::CommandLineOptionHelp);
}

void SetupTextTracing()
{
    wstring logRoot;
    wstring traceFileDir;
    auto error = FabricEnvironment::GetFabricLogRoot(logRoot);
    if (!error.IsSuccess())
    {
        console.WriteLine("Unable to get fabric log root because of {0}, Using current directory", error);
        traceFileDir = Directory::GetCurrentDirectoryW();
    }
    else
    {
        traceFileDir = Path::Combine(logRoot, L"Traces");
    }

    wstring traceFileName = wformatString(FabricInstallerService::Constants::TraceFileFormat, Environment::GetCurrentModuleFileVersion(), DateTime::Now().Ticks);
    TraceTextFileSink::SetPath(Path::Combine(traceFileDir, traceFileName));
    TraceProvider::StaticInit_Singleton()->SetDefaultLevel(TraceSinkType::TextFile, LogLevel::Noise);
    TraceProvider::StaticInit_Singleton()->SetDefaultLevel(TraceSinkType::Console, LogLevel::Info);
}

void RunAsConsoleApplication(Common::StringCollection const & args)
{
    Directory::SetCurrentDirectoryW(Common::Environment::GetExecutablePath());
    FabricInstallerServiceImpl service(false, autocleanEnabled);
    service.OnStart(args);
    wchar_t singleChar;
    wcin.getline(&singleChar, 1);
    service.OnStop(true);
}

void RunAsService()
{
    Directory::SetCurrentDirectoryW(Common::Environment::GetExecutablePath());
    FabricInstallerServiceImpl service(true, autocleanEnabled);
    FabricInstallerServiceImpl::Run(service);
}
