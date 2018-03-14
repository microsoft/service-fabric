// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#if defined(PLATFORM_UNIX)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#endif

using namespace Common;
using namespace FabricHost;
using namespace ServiceModel;
using namespace std;

void RunAsService(bool, bool);
void RunAsConsoleApplication(Common::StringCollection const &, bool, bool);
void PrintUsage();
void InstallService();
void UninstallService();
void SetConfigStoreEnv(bool);

static RealConsole console;

#if !defined(PLATFORM_UNIX)
int __cdecl wmain(int argc, __in_ecount(argc) wchar_t* argv[])
{
#else
int main(int argc, char* argva[])
{
    int argv_num = 0;
    vector<wstring> vec;
    for(int i = 0; argva[i]; i++)
    {
        string astr(argva[i]);
        wstring wstr;
        StringUtility::Utf8ToUtf16(astr, wstr);
        vec.push_back(wstr);
        argv_num++;
    }
    wchar_t **argv = NULL;
    if (argv_num)
    {
        argv = new wchar_t*[argv_num + 1];
        int i = 0;
        for(auto &wstr : vec)
        {
            argv[i++] = (wchar_t*)wstr.c_str();
        }
        argv[argv_num] = 0;
    }
#endif
    HeapSetInformation(GetProcessHeap(), HeapEnableTerminationOnCorruption, NULL, 0);

    try
    {
        Common::StringCollection args(argv, argv+argc);
        
        size_t count = 1;
        
        bool activateHidden = false;
        bool runAsConsoleApp = false;
        bool skipFabricSetup = false;

        while (count < args.size()) 
        {
            if (Common::StringUtility::AreEqualCaseInsensitive(args[count], FabricHost::Constants::CommandLineOptionService) ||
                Common::StringUtility::AreEqualCaseInsensitive(args[count], FabricHost::Constants::CommandLineOptionServiceShort))
            {
                runAsConsoleApp = false;
            }
            else if (Common::StringUtility::AreEqualCaseInsensitive(args[count], FabricHost::Constants::CommandLineOptionConsole) ||
                Common::StringUtility::AreEqualCaseInsensitive(args[count], FabricHost::Constants::CommandLineOptionConsoleShort))
            {
                runAsConsoleApp = true;
            }
            else if (Common::StringUtility::AreEqualCaseInsensitive(args[count], FabricHost::Constants::CommandLineOptionActivateHidden) ||
                Common::StringUtility::AreEqualCaseInsensitive(args[count], FabricHost::Constants::CommandLineOptionActivateHiddenShort))
            {
                activateHidden = true;
            }
            else if (Common::StringUtility::AreEqualCaseInsensitive(args[count], FabricHost::Constants::CommandLineOptionSkipFabricSetup) ||
                Common::StringUtility::AreEqualCaseInsensitive(args[count], FabricHost::Constants::CommandLineOptionSkipFabricSetupShort))
            {
                skipFabricSetup = true;
            }
#if DBG && !defined(PLATFORM_UNIX)
            else if (Common::StringUtility::AreEqualCaseInsensitive(args[count], FabricHost::Constants::CommandLineOptionInstall) ||
                Common::StringUtility::AreEqualCaseInsensitive(args[count], FabricHost::Constants::CommandLineOptionInstallShort))
            {
                InstallService();
                return 0;
            }
            else if (Common::StringUtility::AreEqualCaseInsensitive(args[count], FabricHost::Constants::CommandLineOptionUninstall) ||
                Common::StringUtility::AreEqualCaseInsensitive(args[count], FabricHost::Constants::CommandLineOptionUninstallShort))
            {
                UninstallService();
                return 0;
            }
#endif
            else if (Common::StringUtility::AreEqualCaseInsensitive(args[count], FabricHost::Constants::CommandLineOptionHelp) ||
                Common::StringUtility::AreEqualCaseInsensitive(args[count],  FabricHost::Constants::CommandLineOptionHelpShort))
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
            RunAsConsoleApplication(args, activateHidden, skipFabricSetup);
            return 0;
        }
        else
        {
            RunAsService(activateHidden, skipFabricSetup);
            return 0;
        }
    }
    catch (std::exception const& e)
    {
        console.WriteLine("{0} {1}", StringResource::Get(IDS_FABRICHOST_EXCEPTION), e);
        return FabricHost::Constants::ExitCodeUnhandledException;
    } 
}

void PrintUsage()
{
    console.WriteLine("{0} [options]", Path::GetFileNameWithoutExtension(Environment::GetExecutableFileName()));
    console.WriteLine("    {0}/{1} : run as console application", FabricHost::Constants::CommandLineOptionConsoleShort, FabricHost::Constants::CommandLineOptionConsole);
#if DBG
    console.WriteLine("    {0}/{1} : install as service", FabricHost::Constants::CommandLineOptionInstallShort, FabricHost::Constants::CommandLineOptionInstall);
    console.WriteLine("    {0}/{1} : uninstall service", FabricHost::Constants::CommandLineOptionUninstallShort, FabricHost::Constants::CommandLineOptionUninstall);
#endif
    console.WriteLine("    {0}/{1} : print usage", FabricHost::Constants::CommandLineOptionHelpShort, FabricHost::Constants::CommandLineOptionHelp);
}

void RunAsConsoleApplication(Common::StringCollection const & args, bool activateHidden, bool skipFabricSetup)
{
    RealConsole consoleLocal;
    consoleLocal.WriteLine(StringResource::Get(IDS_FABRICHOST_STARTING));
    Directory::SetCurrentDirectoryW(Common::Environment::GetExecutablePath());
#if defined(PLATFORM_UNIX)
    CertDirMonitor::GetSingleton().StartWatch("/var/lib/waagent", "/var/lib/sfcerts");
#endif
    FabricActivatorService service(false, activateHidden, skipFabricSetup);
    service.OnStart(args);
#if !defined(PLATFORM_UNIX)
    consoleLocal.WriteLine(StringResource::Get(IDS_FABRIC_SERVICE_EXIT_PROMPT));
    wchar_t singleChar;
    wcin.getline(&singleChar, 1);
#else
    do
    {
        pause();
    } while(errno == EINTR);
#endif
    service.OnStop(true);
}

void RunAsService(bool activateHidden, bool skipFabricSetup)
{
    Directory::SetCurrentDirectory(Common::Environment::GetExecutablePath());
#if !defined(PLATFORM_UNIX)
    FabricActivatorService service(true, activateHidden, skipFabricSetup);
    ServiceController serviceController(FabricHost::Constants::ServiceName);
    if (Hosting2::FabricHostConfig::GetConfig().EnableRestartManagement)
    {
        serviceController.SetPreshutdownTimeout(FabricHost::Constants::PreShutdownBlockTimeInMinutes * 60 * 1000);
    }

    FabricActivatorService::Run(service);
#else
    CertDirMonitor::GetSingleton().StartWatch("/var/lib/waagent", "/var/lib/sfcerts");
    FabricActivatorService service(false, activateHidden, skipFabricSetup);

    //Daemonize the process. There is no fork, as PAL will get messed up.
    if (getpgid(0) == getpid())
    {
        int fd = open("/dev/tty", O_RDWR);
        ioctl(fd, TIOCNOTTY, NULL);
    }
    else
    {
        pid_t sid = setsid();
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    open("/dev/null", O_RDONLY);
    dup(0);
    dup(0);

    Common::StringCollection args;
    service.OnStart(args);
    do
    {
        pause();
    } while(errno == EINTR);
    service.OnStop(true);
#endif
}

#if !defined(PLATFORM_UNIX)
void InstallService()
{
    console.WriteLine("Installing as service");
    console.WriteLine("Service name: {0}", FabricHost::Constants::ServiceName);
    console.WriteLine("Service displanname: {0}", FabricHost::Constants::ServiceDisplayName);
    wstring serviceCommandLine = Common::Environment::GetExecutableFileName();
    console.WriteLine("Service executable commandline: {0}", serviceCommandLine);
    ServiceController serviceController(FabricHost::Constants::ServiceName);
    serviceController.Install(FabricHost::Constants::ServiceDisplayName, serviceCommandLine);
}

void UninstallService()
{
    ServiceController serviceController(FabricHost::Constants::ServiceName);

    try
    {
        console.WriteLine("Uninstalling service");
        serviceController.Uninstall();
    }
    catch(std::exception const &)
    {
    }
}
#endif
