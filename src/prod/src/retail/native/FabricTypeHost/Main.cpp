// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#if defined(PLATFORM_UNIX)
#include <signal.h>
#include <sys/stat.h>

struct sigaction act;
#endif

using namespace std;
using namespace Common;
using namespace FabricTypeHost;


GlobalWString HelpOption1 = make_global<wstring>(L"/?");
GlobalWString HelpOption2 = make_global<wstring>(L"-?");
GlobalWString TypeOption1 = make_global<wstring>(L"/type:");
GlobalWString TypeOption2 = make_global<wstring>(L"-type:");

std::shared_ptr<StatelessServiceTypeHost> host_;
RealConsole console_;

void PrintUsage()
{
    console_.WriteLine();
    console_.WriteLine("Usage: FabricTypeHost.exe /type:<ServiceTypeName> [/type:<ServiceTypeName>]");
    console_.WriteLine();
}

bool OpenServiceHost()
{
    console_.WriteLine("Opening StatelessServiceTypeHost ...");

    auto error = host_->Open();
    if (!error.IsSuccess())
    {
        console_.WriteLine("Failed. ErrorCode={0}", error);
        return false;
    }

    console_.WriteLine("StatelessServiceTypeHost Opened, waiting for Microsoft Azure Service Fabric to place services ...");
    console_.WriteLine("Press [Enter] to exit.");
    return true;
}

bool CloseServiceHost()
{
    console_.WriteLine("Closing StatelessServiceTypeHost ...");
    auto error = host_->Close();
    if (!error.IsSuccess())
    {
        console_.WriteLine("Failed. ErrorCode={0}", error);
        return false;
    }
    else
    {
        console_.WriteLine("StatelessServiceTypeHost Closed.");
    }

    return true;
}

#if !defined(PLATFORM_UNIX)

bool ParseArguments(__in int argc, __in_ecount(argc) wchar_t* argv[], __out vector<wstring> & typesToHost)
{
	if (argc < 2)
	{
		PrintUsage();
		return false;
	}

	wstring argument = wstring(argv[1]);
	if ((StringUtility::CompareCaseInsensitive(argument, *HelpOption1) == 0) ||
		(StringUtility::CompareCaseInsensitive(argument, *HelpOption2) == 0))
	{
		PrintUsage();
		return false;
	}

	for (int i = 1; i < argc; i++)
	{
		argument = wstring(argv[i]);
		if (StringUtility::StartsWithCaseInsensitive(argument, *TypeOption1) ||
			StringUtility::StartsWithCaseInsensitive(argument, *TypeOption2))
		{
			wstring value = argument.substr(TypeOption1->size());
			if (!value.empty())
			{
				typesToHost.push_back(value);
			}
		}
		else
		{
			// unknown argument
			return false;
		}
	}

	return !typesToHost.empty();
}

BOOL CtrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType)
    {
        // Handle the CTRL-C signal. 
    case CTRL_C_EVENT:
        CloseServiceHost();
#if defined(PLATFORM_UNIX)
        ExitProcess(ERROR_SUCCESS);
#else
        ExitProcess(ERROR_CONTROL_C_EXIT);
#endif

        // Pass other signals to the next handler.
    case CTRL_BREAK_EVENT:
        CloseServiceHost();
#if defined(PLATFORM_UNIX)
        ExitProcess(ERROR_SUCCESS);
#else
        ExitProcess(ERROR_CONTROL_C_EXIT);
#endif

    default:
        return FALSE;
    }
}

int __cdecl wmain(int argc, __in_ecount(argc) wchar_t* argv[])
{
    HeapSetInformation(GetProcessHeap(), HeapEnableTerminationOnCorruption, NULL, 0);

    ::FabricDisableIpcLeasing(); 

    vector<wstring> typesToHost;
    auto success = ParseArguments(argc, argv, typesToHost);
    if (!success) return -1;

    host_ = make_shared<StatelessServiceTypeHost>(move(typesToHost));
    success = OpenServiceHost();
    if (!success) return -1;
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, true);

    wchar_t singleChar;
    wcin.getline(&singleChar, 1);

    success = CloseServiceHost();
    if (!success) return -1;

    return 0;
}


#else

void sighandler(int signum, siginfo_t *info, void *ptr)
{
    printf(">> Signal: %d\n", signum);
    printf(">> From process: %lu\n", (unsigned long)info->si_pid);
    CloseServiceHost();
}



bool ParseArguments(__in int argc, char* argv[], __out vector<wstring> & typesToHost)
{
    if (argc < 2)
    {
        PrintUsage();
        return false;
    }

    string arg = argv[1];
    wstring argument;
    StringUtility::Utf8ToUtf16(arg, argument);
    if ((StringUtility::CompareCaseInsensitive(argument, *HelpOption1) == 0) ||
        (StringUtility::CompareCaseInsensitive(argument, *HelpOption2) == 0))
    {
        PrintUsage();
        return false;
    }
    for(int i = 1; i < argc; i++)
    {
        arg = argv[i];
        StringUtility::Utf8ToUtf16(arg, argument);
        if (StringUtility::StartsWithCaseInsensitive(argument, *TypeOption1) ||
            StringUtility::StartsWithCaseInsensitive(argument, *TypeOption2))
        {
            wstring value = argument.substr(TypeOption1->size());
            if (!value.empty())
            {
                typesToHost.push_back(value);
            }
        }
        else
        {
            // unknown argument
            return false;
        }
    }

    return !typesToHost.empty();
}

int main(int argc, char* argv[])
{
    HeapSetInformation(GetProcessHeap(), HeapEnableTerminationOnCorruption, NULL, 0);

    ::FabricDisableIpcLeasing(); 

    vector<wstring> typesToHost;
    auto success = ParseArguments(argc, argv, typesToHost);
    if (!success) return -1;

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = sighandler;
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &act, NULL);

    umask(0002);

    host_ = make_shared<StatelessServiceTypeHost>(move(typesToHost));
    success = OpenServiceHost();
    if (!success) return -1;
        
    do
    {
        pause();
    } while (errno == EINTR);

    success = CloseServiceHost();
    if (!success) return -1;

    return 0;
}

#endif
