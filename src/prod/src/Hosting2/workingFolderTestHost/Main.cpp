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
GlobalWString GuestExeOption1 = make_global<wstring>(L"/guestExe");
GlobalWString GuestExeOption2 = make_global<wstring>(L"-guestExe");
GlobalWString ActivatorOption1 = make_global<wstring>(L"/activator");
GlobalWString ActivatorOption2 = make_global<wstring>(L"-activator");

std::shared_ptr<WorkingFolderTestHost> host_;
RealConsole console_;

void PrintUsage()
{
    console_.WriteLine();
    console_.WriteLine("Usage: WorkingFolderTestHost.exe /testName:<TestName> [-setupEntryPoint]");
    console_.WriteLine("Registers TestName as the service type if not SetupEntrypoint.");
    console_.WriteLine("Usage: WorkingFolderTestHost.exe -guestExe");
    console_.WriteLine("Behaves as a guest executable.");
    console_.WriteLine("Usage: WorkingFolderTestHost.exe /testName:<TestName> -activator");
    console_.WriteLine("Registers TestName as the service type and behaves as activator code package.");
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

bool ParseArguments(
    _In_ vector<wstring> const & arguments,
    _Out_ wstring & testName, 
    _Out_ bool & isSetupEntryPoint,
    _Out_ bool & isGuestExe,
    _Out_ bool & isActivator)
{
    if (arguments.empty())
    {
        PrintUsage();
        return false;
    }

    isActivator = false;
    isGuestExe = false;
    isSetupEntryPoint = false;
    testName = L"";

    for(auto const & argument : arguments)
    {
        if (StringUtility::AreEqualCaseInsensitive(argument, *HelpOption1) ||
            StringUtility::AreEqualCaseInsensitive(argument, *HelpOption2))
        {
            PrintUsage();
            return false;
        }

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
        else if (StringUtility::AreEqualCaseInsensitive(argument, *SetupEntryPointOption1) ||
            StringUtility::AreEqualCaseInsensitive(argument, *SetupEntryPointOption2))
        {
            isSetupEntryPoint = true;
        }
        else if (StringUtility::AreEqualCaseInsensitive(argument, *GuestExeOption1) ||
            StringUtility::AreEqualCaseInsensitive(argument, *GuestExeOption2))
        {
            isGuestExe = true;
        }
        else if (StringUtility::AreEqualCaseInsensitive(argument, *ActivatorOption1) ||
            StringUtility::AreEqualCaseInsensitive(argument, *ActivatorOption2))
        {
            isActivator = true;
        }
        else
        {
            return false;
        }
    }

    return true;
}

#if defined(PLATFORM_UNIX)
int main(int argc, char* argv[])
#else
int __cdecl wmain(int argc, __in_ecount(argc) wchar_t* argv[])
#endif
{
    HeapSetInformation(GetProcessHeap(), HeapEnableTerminationOnCorruption, NULL, 0);

    vector<wstring> arguments;
    for (int i = 1; i < argc; i++)
    {
#if defined(PLATFORM_UNIX)
        string arg(argv[i]);
        wstring argument;
        StringUtility::Utf8ToUtf16(arg, argument);

        arguments.push_back(argument);
#else
        arguments.push_back(wstring(argv[i]));
#endif
    }

    wstring testName;
    bool isSetupEntryPoint;
    bool isGuestExe;
    bool isActivator;
    auto success = ParseArguments(arguments, testName, isSetupEntryPoint, isGuestExe, isActivator);
    if (!success) return -1;

    console_.WriteLine(
        "Arguments: TestName=[{0}], IsSetupEntryPoint=[{1}], IsGuestExe=[{2}], IsActivator=[{3}].",
        testName, 
        isSetupEntryPoint,
        isGuestExe,
        isActivator);

    if (isGuestExe)
    {
        wchar_t singleChar;
        wcin.getline(&singleChar, 1);
    }
    else
    {
        auto fileName = Path::GetFileNameWithoutExtension(Environment::GetExecutableFileName());
        auto pid = ::GetCurrentProcessId();
        auto traceFileName = wformatString("{0}_{1}.trace", fileName, pid);
        TraceTextFileSink::SetPath(traceFileName);

        host_ = make_shared<WorkingFolderTestHost>(testName, isSetupEntryPoint, isActivator);
        success = OpenServiceHost();
        if (!success) return -1;

        if (!isSetupEntryPoint)
        {
            wchar_t singleChar;
            wcin.getline(&singleChar, 1);
        }

        success = CloseServiceHost();
        if (!success) return -1;
    }

    return 0;
}
