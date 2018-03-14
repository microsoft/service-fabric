// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

GlobalWString HelpOption1 = make_global<wstring>(L"/?");
GlobalWString HelpOption2 = make_global<wstring>(L"-?");
GlobalWString TestNameOption1 = make_global<wstring>(L"/testName:");
GlobalWString TestNameOption2 = make_global<wstring>(L"-testName:");
GlobalWString SetupEntryPointOption1 = make_global<wstring>(L"/setupEntryPoint");
GlobalWString SetupEntryPointOption2 = make_global<wstring>(L"-setupEntryPoint");

std::shared_ptr<WorkingFolderTestHost> host_;
RealConsole console_;

void PrintUsage()
{
    console_.WriteLine();
    console_.WriteLine("Usage: WorkingFolderTestHost.exe /testName:<TestName> [-setupEntryPoint]");
    console_.WriteLine("Registers TestName as the service type if not SetupEntrypoint.");
    console_.WriteLine();
}

bool OpenServiceHost()
{
    console_.WriteLine("Opening WorkingFolderTestHost ...");
    console_.WriteLine("OpenServiceHost() is called...");
    auto error = host_->Open();
    if (!error.IsSuccess())
    {
        console_.WriteLine("Failed. ErrorCode={0}", error);
        return false;
    }

    console_.WriteLine("WorkingFolderTestHost Opened, waiting for WindowsFabric to place services ...");
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
bool ParseArguments(__in int argc, __in_ecount(argc) wchar_t* argv[], __out wstring & testName, __out bool & isSetupEntryPoint)
{
    if (argc < 2)
    {
        PrintUsage();
        return false;
    }    

    isSetupEntryPoint = false;
    testName = L"";
    for(int i = 1; i < argc; i++)
    {
        wstring argument = wstring(argv[i]);
        if ((StringUtility::CompareCaseInsensitive(argument, *HelpOption1) == 0) ||
            (StringUtility::CompareCaseInsensitive(argument, *HelpOption2) == 0))
        {
            PrintUsage();
            return false;
        }

        argument = wstring(argv[i]);
        if (StringUtility::StartsWithCaseInsensitive(argument, *TestNameOption1) ||
            StringUtility::StartsWithCaseInsensitive(argument, *TestNameOption2))
        {
            wstring value = argument.substr(TestNameOption1->size());
            if (value.empty()) 
            { 
                return false; 
            }
            else
            {
                testName = value;
            }
        } 
        else if (StringUtility::StartsWithCaseInsensitive(argument, *SetupEntryPointOption1) ||
            StringUtility::StartsWithCaseInsensitive(argument, *SetupEntryPointOption2))
        {
            isSetupEntryPoint = true;
        }
        else
        {
            return false;
        }
    }

    return true;
}

int __cdecl wmain(int argc, __in_ecount(argc) wchar_t* argv[])
{
    HeapSetInformation(GetProcessHeap(), HeapEnableTerminationOnCorruption, NULL, 0);

    wstring testName;
    bool isSetupEntryPoint;
    auto success = ParseArguments(argc, argv, testName, isSetupEntryPoint);
    if (!success) return -1;

    console_.WriteLine("Arguments: TestName=[{0}], IsSetupEntryPoint=[{1}]", testName, isSetupEntryPoint);

    host_ = make_shared<WorkingFolderTestHost>(testName, isSetupEntryPoint);
    success = OpenServiceHost();
    if (!success) return -1;
        
    if (!isSetupEntryPoint)
    {
        wchar_t singleChar;
        wcin.getline(&singleChar, 1);
    }

    success = CloseServiceHost();
    if (!success) return -1;

    return 0;
}

#else

bool ParseArguments(__in int argc, char* argv[], __out wstring & testName, __out bool & isSetupEntryPoint)
{
    if (argc < 2)
    {
        PrintUsage();
        return false;
    }    

    isSetupEntryPoint = false;
    testName = L"";
    for(int i = 1; i < argc; i++)
    {
        string arg = argv[i];
        wstring argument;
        StringUtility::Utf8ToUtf16(arg, argument);
        if ((StringUtility::CompareCaseInsensitive(argument, *HelpOption1) == 0) ||
            (StringUtility::CompareCaseInsensitive(argument, *HelpOption2) == 0))
        {
            PrintUsage();
            return false;
        }
        
        //argument = wstring(argv[i]);
        arg = argv[i];
        StringUtility::Utf8ToUtf16(arg, argument);
        if (StringUtility::StartsWithCaseInsensitive(argument, *TestNameOption1) ||
            StringUtility::StartsWithCaseInsensitive(argument, *TestNameOption2))
        {
            wstring value = argument.substr(TestNameOption1->size());
            if (value.empty()) 
            { 
                return false; 
            }
            else
            {
                testName = value;
            }
        } 
        else if (StringUtility::StartsWithCaseInsensitive(argument, *SetupEntryPointOption1) ||
            StringUtility::StartsWithCaseInsensitive(argument, *SetupEntryPointOption2))
        {
            isSetupEntryPoint = true;
        }
        else
        {
            return false;
        }
    }

    return true;
}

int main(int argc, char* argv[])
{
    HeapSetInformation(GetProcessHeap(), HeapEnableTerminationOnCorruption, NULL, 0);

    wstring testName;
    bool isSetupEntryPoint;
    auto success = ParseArguments(argc, argv, testName, isSetupEntryPoint);
    if (!success) return -1;

    console_.WriteLine("Arguments: TestName=[{0}], IsSetupEntryPoint=[{1}]", testName, isSetupEntryPoint);

    host_ = make_shared<WorkingFolderTestHost>(testName, isSetupEntryPoint);
    success = OpenServiceHost();
    if (!success) return -1;
        
    if (!isSetupEntryPoint)
    {
        wchar_t singleChar;
        wcin.getline(&singleChar, 1);
    }

    success = CloseServiceHost();
    if (!success) return -1;

    return 0;
}

#endif
