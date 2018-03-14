// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// FabricRuntimeHelloWorld.cpp : Defines the entry point for the console application.
#include "stdafx.h"
#include "MyStatelessServiceFactory.h"

using namespace std;

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("Usage: LinuxRunAsTest.exe acct|root\n");
        return -1;
    }

    uid_t euid = geteuid();
    struct passwd *pw = getpwuid(euid);
    string uname(pw->pw_name);

    wstring serviceType;
    if (strcasecmp(argv[1], "acct") == 0)
    {
        if (uname.find("WF-") == string::npos)
        {
            string msg("'LinuxRunAsTest.exe root' is supposed to run as dynamic account like WF-Kg3Pm8VrEawJf2O, but actually as ");
            msg += uname;
            throw runtime_error(msg.c_str());
        }
        serviceType = L"RunAsAcctServiceType";
    }
    else if (strcasecmp(argv[1], "root") == 0)
    {
        if (uname.compare("root") != 0)
        {
            string msg("'LinuxRunAsTest.exe root' is supposed to run as root, but actually as ");
            msg += uname;
            throw runtime_error(msg.c_str());
        }
        serviceType = L"RunAsRootServiceType";
    }
    else
    {
        printf("Invalid ServiceType: %s\n", argv[1]);
        return -1;
    }

    IFabricRuntime * fabricRuntime;
    HRESULT hr = FabricCreateRuntime(IID_IFabricRuntime, reinterpret_cast<void**>(&fabricRuntime));
    if (!SUCCEEDED(hr))
    {
        printf("FabricCreateRuntime failed: hr=%d\n", hr);
        return hr;
    }

    IFabricStatelessServiceFactory * factory = new MyStatelessServiceFactory();
    hr = fabricRuntime->RegisterStatelessServiceFactory(serviceType.c_str(), factory);
    if (!SUCCEEDED(hr))
    {
        printf("RegisterStatelessServiceFactory failed: hr=%d\n", hr);
        return hr;
    }

    printf("LinuxRunAsTest Host is running on %s as %s.\n", getenv("Fabric_NodeName"), pw->pw_name);

    do
    {
        pause();
    } while(errno == EINTR);

    return 0;
}

