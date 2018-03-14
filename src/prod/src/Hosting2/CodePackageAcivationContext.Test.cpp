// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include "FabricRuntime.h"
#include "Common/Common.h"


using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;
using namespace Transport;
using namespace Reliability::ReconfigurationAgentComponent;

#define VERIFY( condition, fmt, ...) \
{ \
    wstring tmp; \
    StringWriter writer(tmp); \
    writer.Write(fmt, __VA_ARGS__); \
    VERIFY_IS_TRUE( condition, tmp.c_str() ); \
} \

const StringLiteral TraceType("CodePackageActivationContextTest");

class CodePackageActivationContextTest
{
protected:
    void PrintStatefulServiceType(FABRIC_STATEFUL_SERVICE_TYPE_DESCRIPTION & type, StringWriter & testOutput);
    void PrintStatelessServiceType(FABRIC_STATELESS_SERVICE_TYPE_DESCRIPTION & type, StringWriter & testOutput);

    static const wstring TestDirectory;
    static const wstring DeploymentRoot;
    static const wstring ApplicationId;
    static const wstring ApplicationId_App1;
    static const wstring PackageName;
    static const wstring TestStatefulConfigPackage;
    static const wstring TestStatelessConfigPackage;
    static const wstring ManifestName;
    static const wstring Version;
    static const RolloutVersion ApplicationRolloutVersion;
    static const wstring CurrentCodePackageName;
    static const wstring ComSecurityCertIssuerThumbprint1;
    static const wstring ComSecurityCertIssuerThumbprint2;

    static wstring GetTestFilePath(wstring testFileName);
    static wstring GetUnitTestFolder();
    ComPointer<ComCodePackageActivationContext> TestGetComActivationContext();
    ComPointer<ComCodePackageActivationContext> TestGetComActivationContext(FabricNodeContextSPtr & config);
    ComPointer<ComCodePackageActivationContext> TestGetComActivationContextSecurityCredentials();
    static void VerifyComSecurityCredentials(SecuritySettings credentials);
};
#if !defined(PLATFORM_UNIX)
const wstring CodePackageActivationContextTest::TestDirectory = Path::Combine(CodePackageActivationContextTest::GetUnitTestFolder(), L"Hosting.Test.Data\\ActivationContextTest");
#else
const wstring CodePackageActivationContextTest::TestDirectory = Path::Combine(CodePackageActivationContextTest::GetUnitTestFolder(), L"Hosting.Test.Data/ActivationContextTest");
#endif
const wstring CodePackageActivationContextTest::DeploymentRoot = Path::Combine(TestDirectory, L"Applications");
const wstring CodePackageActivationContextTest::ApplicationId = L"Application_App0";
const wstring CodePackageActivationContextTest::ApplicationId_App1 = L"Application_App1";
const wstring CodePackageActivationContextTest::PackageName = L"ServicePackage";
const wstring CodePackageActivationContextTest::TestStatefulConfigPackage = L"ConfigA";
const wstring CodePackageActivationContextTest::TestStatelessConfigPackage = L"ConfigB";
const wstring CodePackageActivationContextTest::ManifestName = L"ServicePackage";
const wstring CodePackageActivationContextTest::Version = L"1.0";
const RolloutVersion CodePackageActivationContextTest::ApplicationRolloutVersion = RolloutVersion(1, 0);
const wstring CodePackageActivationContextTest::CurrentCodePackageName = L"CodeA";

const wstring CodePackageActivationContextTest::ComSecurityCertIssuerThumbprint1 = L"b3 44 9b 01 8d 0f 68 39 a2 c5 d6 2b 5b 6c 6a c8 22 b6 f6 62";
const wstring CodePackageActivationContextTest::ComSecurityCertIssuerThumbprint2 = L"bc 21 ae 9f 0b 88 cf 6e a9 b4 d6 23 3f 97 2a 60 63 b2 25 a9";

BOOST_FIXTURE_TEST_SUITE(CodePackageActivationContextTestSuite, CodePackageActivationContextTest)

// LINUXTO: disable this test case due to xmlReader issue. We may need to change this test case.
#if !defined(PLATFORM_UNIX)
BOOST_AUTO_TEST_CASE(TestComSecurityCredentials)
{
    ComPointer<ComCodePackageActivationContext> comCodePackageActivationContextCPtr = TestGetComActivationContextSecurityCredentials();

    ComPointer<IFabricCodePackageActivationContext2> activationContext2CPtr;
    auto hr = comCodePackageActivationContextCPtr->QueryInterface(
        IID_IFabricCodePackageActivationContext2,
        activationContext2CPtr.VoidInitializationAddress());
    VERIFY_IS_TRUE(SUCCEEDED(hr));

    ComPointer<IFabricCodePackageActivationContext> activationContextCPtr;
    hr = comCodePackageActivationContextCPtr->QueryInterface(
        IID_IFabricCodePackageActivationContext,
        activationContextCPtr.VoidInitializationAddress());
    VERIFY_IS_TRUE(SUCCEEDED(hr));

    ComPointer<IFabricStringListResult> configStringListResult;
    hr = activationContext2CPtr->GetConfigurationPackageNames(configStringListResult.InitializationAddress());
    VERIFY_IS_TRUE(SUCCEEDED(hr));
    if (configStringListResult.GetRawPointer() != NULL)
    {
        ULONG count;
        const LPCWSTR * configPackageName;
        hr = configStringListResult->GetStrings(&count, &configPackageName);
        VERIFY_IS_TRUE(SUCCEEDED(hr));
        if (count > 0)
        {
            VERIFY_IS_FALSE(configPackageName == NULL);
            IFabricConfigurationPackage *configPackage = nullptr;
            hr = activationContext2CPtr->GetConfigurationPackage(*configPackageName, &configPackage);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(configPackage != NULL);

            const FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION * configPackageDescription = configPackage->get_Description();
            const FABRIC_CONFIGURATION_SETTINGS* settings = configPackage->get_Settings();

            for (ULONG j = 0; j < settings->Sections->Count; ++j)
            {
                const FABRIC_CONFIGURATION_SECTION * section = settings->Sections->Items + j;
                ComPointer<IFabricSecurityCredentialsResult> securityCredentialsResult;
                ComSecurityCredentialsResult::FromConfig(activationContextCPtr, configPackageDescription->Name, section->Name, securityCredentialsResult.InitializationAddress());
                const FABRIC_SECURITY_CREDENTIALS *credentials = securityCredentialsResult->get_SecurityCredentials();
                VERIFY_IS_TRUE(credentials != NULL);
                SecuritySettings securitySettings;
                auto error = SecuritySettings::FromPublicApi(*credentials, securitySettings);
                VERIFY_IS_TRUE(error.IsSuccess());

                if (StringUtility::AreEqualCaseInsensitive(section->Name, L"DummySecurityConfigWithIssuerStore"))
                {
                    VerifyComSecurityCredentials(securitySettings);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(TestActivationContext)
{
    ComPointer<ComCodePackageActivationContext> comCodePackageActivationContextCPtr = TestGetComActivationContext();

    ComPointer<IFabricCodePackageActivationContext2> activationContext2CPtr;
    auto hr = comCodePackageActivationContextCPtr->QueryInterface(
        IID_IFabricCodePackageActivationContext2,
        activationContext2CPtr.VoidInitializationAddress());
    VERIFY_IS_TRUE(SUCCEEDED(hr));

    wstring testOutputString;
    StringWriter testOutput(testOutputString);

    testOutput.WriteLine("Done Creating CodePackageActivationContext.");

    wstring contextId = wstring(activationContext2CPtr->get_ContextId());
    testOutput.WriteLine("ContextId = {0}", contextId);

    wstring codePackageName = wstring(activationContext2CPtr->get_CodePackageName());
    testOutput.WriteLine("CodePackageName = {0}", codePackageName);

    wstring codePackageVersion = wstring(activationContext2CPtr->get_CodePackageVersion());
    testOutput.WriteLine("CodePackageVersion = {0}", codePackageVersion);

    wstring workingDir = wstring(activationContext2CPtr->get_WorkDirectory());
    testOutput.WriteLine("WorkDirectory = {0}", workingDir);

    wstring logDir = wstring(activationContext2CPtr->get_LogDirectory());
    testOutput.WriteLine("LogDirectory = {0}", logDir);

    wstring tempDir = wstring(activationContext2CPtr->get_TempDirectory());
    testOutput.WriteLine("TempDirectory = {0}", tempDir);

    testOutput.WriteLine("ApplicationName = {0}", wstring(activationContext2CPtr->get_ApplicationName()));
    testOutput.WriteLine("ApplicationTypeName = {0}", wstring(activationContext2CPtr->get_ApplicationTypeName()));

    ComPointer<IFabricStringResult> serviceManifestName;
    ComPointer<IFabricStringResult> serviceManifestVersion;

    hr = activationContext2CPtr->GetServiceManifestName(serviceManifestName.InitializationAddress());
    VERIFY_IS_TRUE(SUCCEEDED(hr));

    hr = activationContext2CPtr->GetServiceManifestVersion(serviceManifestVersion.InitializationAddress());
    VERIFY_IS_TRUE(SUCCEEDED(hr));

    testOutput.WriteLine("ServiceManifestName = {0}", wstring(serviceManifestName->get_String()));
    testOutput.WriteLine("ServiceManifestVersion = {0}", wstring(serviceManifestVersion->get_String()));

    const FABRIC_SERVICE_TYPE_DESCRIPTION_LIST * serviceTypes = activationContext2CPtr->get_ServiceTypes();
    if (serviceTypes != NULL)
    {
        if (serviceTypes->Count > 0)
        {
            VERIFY_IS_FALSE(serviceTypes->Items == NULL);

            testOutput.WriteLine();
            testOutput.WriteLine("Enumerating ServiceTypes...");
            for (ULONG i = 0; i < serviceTypes->Count; ++i)
            {
                const FABRIC_SERVICE_TYPE_DESCRIPTION* typeDescription = serviceTypes->Items + i;

                if (typeDescription->Kind == FABRIC_SERVICE_KIND_STATELESS)
                {
                    auto stateless = reinterpret_cast<FABRIC_STATELESS_SERVICE_TYPE_DESCRIPTION*>(
                        typeDescription->Value);
                    PrintStatelessServiceType(*stateless, testOutput);
                }
                else if (typeDescription->Kind == FABRIC_SERVICE_KIND_STATEFUL)
                {
                    auto stateless = reinterpret_cast<FABRIC_STATEFUL_SERVICE_TYPE_DESCRIPTION*>(
                        typeDescription->Value);
                    PrintStatefulServiceType(*stateless, testOutput);
                }
                else
                {
                    VERIFY_FAIL(L"Invalid type");
                }
            }
        }
    }

    const FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST * serviceGroupTypes = activationContext2CPtr->get_ServiceGroupTypes();
    if (serviceGroupTypes != NULL)
    {
        if (serviceGroupTypes->Count > 0)
        {
            VERIFY_IS_FALSE(serviceGroupTypes->Items == NULL);

            testOutput.WriteLine();
            testOutput.WriteLine("Enumerating ServiceGroupTypes...");
            for (ULONG i = 0; i < serviceGroupTypes->Count; ++i)
            {
                const FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION* typeDescription = serviceGroupTypes->Items + i;

                if (typeDescription->Description->Kind == FABRIC_SERVICE_KIND_STATELESS)
                {
                    auto stateless = reinterpret_cast<FABRIC_STATELESS_SERVICE_TYPE_DESCRIPTION*>(
                        typeDescription->Description->Value);
                    PrintStatelessServiceType(*stateless, testOutput);
                }
                else if (typeDescription->Description->Kind == FABRIC_SERVICE_KIND_STATEFUL)
                {
                    auto stateless = reinterpret_cast<FABRIC_STATEFUL_SERVICE_TYPE_DESCRIPTION*>(
                        typeDescription->Description->Value);
                    PrintStatefulServiceType(*stateless, testOutput);
                }
                else
                {
                    VERIFY_FAIL(L"Invalid type");
                }

                testOutput.WriteLine("UseImplicitFactory = {0}", typeDescription->UseImplicitFactory);
                if (typeDescription->Members != NULL)
                {
                    if (typeDescription->Members->Count > 0)
                    {
                        testOutput.Write("ServiceGroupTypeMembers: [");
                        for (ULONG j = 0; j < typeDescription->Members->Count; ++j)
                        {
                            const FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION* typeMember = (typeDescription->Members->Items + j);
                            testOutput.Write("({0} ", typeMember->ServiceTypeName);
                            testOutput.Write("LoadMetrics: [");
                            for (ULONG k = 0; k < typeMember->LoadMetrics->Count; ++k)
                            {
                                const FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION* metricDescription = typeMember->LoadMetrics->Items + k;
                                testOutput.Write(" (Name = {0}, PrimaryDefaultLoad = '{1}', SecondaryDefaultLoad = '{2}', Weight = '{3}') ",
                                    metricDescription->Name,
                                    metricDescription->PrimaryDefaultLoad,
                                    metricDescription->SecondaryDefaultLoad,
                                    metricDescription->Weight);
                            }
                            testOutput.Write("])");
                        }
                        testOutput.WriteLine("]");
                    }
                }

                testOutput.WriteLine();
            }
        }
    }

    const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST * endpointResources = activationContext2CPtr->get_ServiceEndpointResources();
    if (endpointResources != NULL)
    {
        if (endpointResources->Count > 0)
        {
            VERIFY_IS_FALSE(endpointResources->Items == NULL);
            testOutput.WriteLine("Enumerating Endpoints...");
            for (ULONG i = 0; i < endpointResources->Count; ++i)
            {
                const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION* endpointResource = endpointResources->Items + i;

                auto endpointResourceEx1 = (FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_EX1*)(endpointResource->Reserved);
                auto endpointResourceEx2 = (FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_EX2*)(endpointResourceEx1->Reserved);

                testOutput.WriteLine(
                    "Endpoint: Name={0}, Protocol={1}, Type={2}, Port={3}, Certificate='{4}', UriScheme='{5}', PathSuffix='{6}', CodePackageName='{7}', IpAddressOrFqdn='{8}'",
                    endpointResource->Name,
                    endpointResource->Protocol,
                    endpointResource->Type,
                    endpointResource->Port,
                    endpointResource->CertificateName,
                    endpointResourceEx1->UriScheme,
                    endpointResourceEx1->PathSuffix,
                    endpointResourceEx2->CodePackageName,
                    endpointResourceEx2->IpAddressOrFqdn);

                const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION* endpointResourceToRead = nullptr;
                hr = activationContext2CPtr->GetServiceEndpointResource(endpointResource->Name, &endpointResourceToRead);
                VERIFY_IS_TRUE(SUCCEEDED(hr));

                wstring endpointResourceName(endpointResource->Name);
                StringUtility::ToLower(endpointResourceName);
                hr = activationContext2CPtr->GetServiceEndpointResource(endpointResourceName.c_str(), &endpointResourceToRead);
                VERIFY_IS_TRUE(SUCCEEDED(hr));
            }
        }
    }

    const FABRIC_APPLICATION_PRINCIPALS_DESCRIPTION * applicationPrincipals = activationContext2CPtr->get_ApplicationPrincipals();
    if (applicationPrincipals != NULL)
    {
        testOutput.WriteLine();
        if (applicationPrincipals->Users->Count > 0)
        {
            VERIFY_IS_FALSE(applicationPrincipals->Users->Items == NULL);

            testOutput.WriteLine("Enumerating Users...");
            for (ULONG i = 0; i < applicationPrincipals->Users->Count; ++i)
            {
                const FABRIC_SECURITY_USER_DESCRIPTION* userDescription = applicationPrincipals->Users->Items + i;
                testOutput.WriteLine(
                    "User: Name = {0}, SID = {1}",
                    userDescription->Name,
                    userDescription->Sid);

                if (userDescription->ParentApplicationGroups->Count > 0)
                {
                    VERIFY_IS_FALSE(userDescription->ParentApplicationGroups->Items == NULL);

                    testOutput.Write("ParentApplicationGroups:");
                    for (ULONG j = 0; j < userDescription->ParentApplicationGroups->Count; ++j)
                    {
                        testOutput.Write(" {0}", *(userDescription->ParentApplicationGroups->Items + j));
                    }

                    testOutput.WriteLine();
                }

                if (userDescription->ParentSystemGroups->Count > 0)
                {
                    VERIFY_IS_FALSE(userDescription->ParentSystemGroups->Items == NULL);

                    testOutput.Write("ParentSystemGroups:");
                    for (ULONG j = 0; j < userDescription->ParentSystemGroups->Count; ++j)
                    {
                        testOutput.Write(" {0}", *(userDescription->ParentSystemGroups->Items + j));
                    }

                    testOutput.WriteLine();
                }

                testOutput.WriteLine();
            }
        }

        if (applicationPrincipals->Groups->Count > 0)
        {
            VERIFY_IS_FALSE(applicationPrincipals->Groups->Items == NULL);

            testOutput.WriteLine("Enumerating Groups...");
            for (ULONG i = 0; i < applicationPrincipals->Groups->Count; ++i)
            {
                const FABRIC_SECURITY_GROUP_DESCRIPTION* groupDescription = applicationPrincipals->Groups->Items + i;
                testOutput.WriteLine(
                    "Group: Name = {0}, SID = {1}",
                    groupDescription->Name,
                    groupDescription->Sid);

                if (groupDescription->DomainGroupMembers->Count > 0)
                {
                    VERIFY_IS_FALSE(groupDescription->DomainGroupMembers->Items == NULL);

                    testOutput.Write("DomainGroupMembers:");
                    for (ULONG j = 0; j < groupDescription->DomainGroupMembers->Count; ++j)
                    {
                        testOutput.Write(" {0}", *(groupDescription->DomainGroupMembers->Items + j));
                    }

                    testOutput.WriteLine();
                }

                if (groupDescription->DomainUserMembers->Count > 0)
                {
                    VERIFY_IS_FALSE(groupDescription->DomainUserMembers->Items == NULL);

                    testOutput.Write("DomainUserMembers:");
                    for (ULONG j = 0; j < groupDescription->DomainUserMembers->Count; ++j)
                    {
                        testOutput.Write(" {0}", *(groupDescription->DomainUserMembers->Items + j));
                    }

                    testOutput.WriteLine();
                }

                if (groupDescription->SystemGroupMembers->Count > 0)
                {
                    VERIFY_IS_FALSE(groupDescription->SystemGroupMembers->Items == NULL);

                    testOutput.Write("SystemGroupMembers:");
                    for (ULONG j = 0; j < groupDescription->SystemGroupMembers->Count; ++j)
                    {
                        testOutput.Write(" {0}", *(groupDescription->SystemGroupMembers->Items + j));
                    }

                    testOutput.WriteLine();
                }
            }
        }
    }

    ComPointer<IFabricStringListResult> stringListResult;
    hr = activationContext2CPtr->GetCodePackageNames(stringListResult.InitializationAddress());
    VERIFY_IS_TRUE(SUCCEEDED(hr));
    if (stringListResult.GetRawPointer() != NULL)
    {
        ULONG count;
        const LPCWSTR * strings;
        hr = stringListResult->GetStrings(&count, &strings);
        VERIFY_IS_TRUE(SUCCEEDED(hr));
        if (count > 0)
        {
            VERIFY_IS_FALSE(strings == NULL);

            testOutput.WriteLine();
            testOutput.WriteLine("Enumerating CodePackages...");
            for (ULONG i = 0; i < count; ++i)
            {
                const LPCWSTR * codePackageName1 = strings + i;
                IFabricCodePackage *codePackage = nullptr;
                hr = activationContext2CPtr->GetCodePackage(*codePackageName1, &codePackage);
                VERIFY_IS_TRUE(SUCCEEDED(hr));
                VERIFY_IS_TRUE(codePackage != NULL);
                const FABRIC_CODE_PACKAGE_DESCRIPTION * codePackageDescription = codePackage->get_Description();
                testOutput.WriteLine(
                    "CodePackageDescription: Name = {0}, Version = {1}, ServiceManifestName = {2}, ServiceManifestVersion = {3}, Path = {4}",
                    codePackageDescription->Name,
                    codePackageDescription->Version,
                    codePackageDescription->ServiceManifestName,
                    codePackageDescription->ServiceManifestVersion,
                    codePackage->get_Path());

                wstring entryPointType;
                wstring entryPoint;

                CodePackageIsolationPolicyType::Enum isolationPolicyType = CodePackageIsolationPolicyType::Invalid;

                if (codePackageDescription->EntryPoint->Kind == FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_EXEHOST)
                {
                    isolationPolicyType = CodePackageIsolationPolicyType::DedicatedProcess;

                    auto exeHost = reinterpret_cast<FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION*>(
                        codePackageDescription->EntryPoint->Value);
                    entryPointType = L"ExeHost";
                    entryPoint = L" " + wstring(exeHost->Program);
                }
                else if (codePackageDescription->EntryPoint->Kind == FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_DLLHOST)
                {
                    auto dllHost = reinterpret_cast<FABRIC_DLLHOST_ENTRY_POINT_DESCRIPTION*>(codePackageDescription->EntryPoint->Value);
                    entryPointType = L"DllHost";

                    auto error = CodePackageIsolationPolicyType::FromPublicApi(dllHost->IsolationPolicyType, isolationPolicyType);
                    VERIFY_IS_TRUE(error.IsSuccess());

                    for (ULONG j = 0; j < dllHost->HostedDlls->Count; ++j)
                    {
                        auto dllHostItem = dllHost->HostedDlls->Items[j];
                        if (dllHostItem.Kind == FABRIC_DLLHOST_HOSTED_DLL_KIND_UNMANAGED)
                        {
                            auto unmanagedDll = reinterpret_cast<FABRIC_DLLHOST_HOSTED_UNMANAGED_DLL_DESCRIPTION*>(dllHostItem.Value);
                            entryPoint = entryPoint + wformatString(" {0}.{1}", DllHostHostedDllKind::Unmanaged, unmanagedDll->DllName);
                        }
                        else if (dllHostItem.Kind == FABRIC_DLLHOST_HOSTED_DLL_KIND_MANAGED)
                        {
                            auto managedAssembly = reinterpret_cast<FABRIC_DLLHOST_HOSTED_MANAGED_DLL_DESCRIPTION*>(dllHostItem.Value);
                            entryPoint = entryPoint + wformatString(" {0}.{1}", DllHostHostedDllKind::Managed, managedAssembly->AssemblyName);
                        }
                        else
                        {
                            VERIFY_FAIL(L"Invalid DllHostHostedDllKind");
                        }
                    }
                }
                else
                {
                    VERIFY_FAIL(L"Invalid EntryPointKind");
                }

                testOutput.WriteLine(
                    "IsShared = {0}, IsolationPolicy = {1}, EntryPointType = {2}, EntryPoints ={3}, SetupEntryPoint = {4}",
                    codePackageDescription->IsShared,
                    isolationPolicyType,
                    entryPointType,
                    entryPoint,
                    codePackageDescription->SetupEntryPoint->Program);
                wstring packageName(*codePackageName1);
                StringUtility::ToLower(packageName);
                hr = activationContext2CPtr->GetCodePackage(packageName.c_str(), &codePackage);
                VERIFY_IS_TRUE(SUCCEEDED(hr));
                VERIFY_IS_TRUE(codePackage != NULL);
            }
        }
    }

    ComPointer<IFabricStringListResult> configStringListResult;
    hr = activationContext2CPtr->GetConfigurationPackageNames(configStringListResult.InitializationAddress());
    VERIFY_IS_TRUE(SUCCEEDED(hr));
    if (configStringListResult.GetRawPointer() != NULL)
    {
        ULONG count;
        const LPCWSTR * strings;
        hr = configStringListResult->GetStrings(&count, &strings);
        VERIFY_IS_TRUE(SUCCEEDED(hr));
        if (count > 0)
        {
            VERIFY_IS_FALSE(strings == NULL);

            testOutput.WriteLine();
            testOutput.WriteLine("Enumerating ConfigPackages...");
            for (ULONG i = 0; i < count; ++i)
            {
                const LPCWSTR * configPackageName = strings + i;
                IFabricConfigurationPackage *configPackage = nullptr;
                hr = activationContext2CPtr->GetConfigurationPackage(*configPackageName, &configPackage);
                VERIFY_IS_TRUE(SUCCEEDED(hr));
                VERIFY_IS_TRUE(configPackage != NULL);
                const FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION * configPackageDescription = configPackage->get_Description();
                testOutput.WriteLine(
                    "ConfigPackageDescription: Name = {0}, Version = {1}, Path = {2}",
                    configPackageDescription->Name,
                    configPackageDescription->Version,
                    configPackage->get_Path());

                const FABRIC_CONFIGURATION_SETTINGS* settings = configPackage->get_Settings();
                for (ULONG j = 0; j < settings->Sections->Count; ++j)
                {
                    const FABRIC_CONFIGURATION_SECTION * section = settings->Sections->Items + j;
                    testOutput.Write("ConfigurationSection {0} : [", section->Name);
                    for (ULONG k = 0; k < section->Parameters->Count; ++k)
                    {
                        const FABRIC_CONFIGURATION_PARAMETER * param = section->Parameters->Items + k;
                        testOutput.Write(" (Name = {0}, Value = {1}) ", param->Name, param->Value);
                    }

                    testOutput.WriteLine("]");
                }
                wstring packageName(*configPackageName);
                StringUtility::ToLower(packageName);
                hr = activationContext2CPtr->GetConfigurationPackage(packageName.c_str(), &configPackage);
                VERIFY_IS_TRUE(SUCCEEDED(hr));
                VERIFY_IS_TRUE(configPackage != NULL);
            }
        }
    }

    ComPointer<IFabricStringListResult> dataStringListResult;
    hr = activationContext2CPtr->GetDataPackageNames(dataStringListResult.InitializationAddress());
    VERIFY_IS_TRUE(SUCCEEDED(hr));
    if (dataStringListResult.GetRawPointer() != NULL)
    {
        ULONG count;
        const LPCWSTR * strings;
        hr = dataStringListResult->GetStrings(&count, &strings);
        VERIFY_IS_TRUE(SUCCEEDED(hr));
        if (count > 0)
        {
            VERIFY_IS_FALSE(strings == NULL);

            testOutput.WriteLine();
            testOutput.WriteLine("Enumerating DataPackages...");
            for (ULONG i = 0; i < count; ++i)
            {
                const LPCWSTR * dataPackageName = strings + i;
                IFabricDataPackage *dataPackage = nullptr;
                hr = activationContext2CPtr->GetDataPackage(*dataPackageName, &dataPackage);
                VERIFY_IS_TRUE(SUCCEEDED(hr));
                VERIFY_IS_TRUE(dataPackage != NULL);
                const FABRIC_DATA_PACKAGE_DESCRIPTION * dataPackageDescription = dataPackage->get_Description();
                testOutput.WriteLine(
                    "DataPackageDescription: Name = {0}, Version = {1}, Path = {2}",
                    dataPackageDescription->Name,
                    dataPackageDescription->Version,
                    dataPackage->get_Path());

                wstring packageName(*dataPackageName);
                StringUtility::ToLower(packageName);
                hr = activationContext2CPtr->GetDataPackage(packageName.c_str(), &dataPackage);
                VERIFY_IS_TRUE(SUCCEEDED(hr));
                VERIFY_IS_TRUE(dataPackage != NULL);
            }
        }
    }

#if !defined(PLATFORM_UNIX)
    wstring resultsFile = GetTestFilePath(L"Hosting.Test.Data\\ActivationContextTest\\ActivationContextTestResults.txt");
#else
    wstring resultsFile = GetTestFilePath(L"Hosting.Test.Data/ActivationContextTest/ActivationContextTestResults.txt");
#endif

    wstring resultString;
    try
    {
        File fileReader;
        auto error = fileReader.TryOpen(resultsFile, Common::FileMode::Open, Common::FileAccess::Read, Common::FileShare::Read);
        VERIFY_IS_TRUE(error.IsSuccess());

        int64 fileSize = fileReader.size();
        size_t size = static_cast<size_t>(fileSize);

        string text;
        text.resize(size);
        fileReader.Read(&text[0], static_cast<int>(size));
        fileReader.Close();

        StringWriter(resultString).Write(text);
    }
    catch (std::exception const&)
    {
        VERIFY_FAIL(L"File read failed");
    }

    StringUtility::Replace<wstring>(resultString, L"{0}", Path::GetDirectoryName(resultsFile));

    VERIFY(resultString.size() == testOutputString.size(), "Size does not match. Expected '{0}' Actual '{1}'", resultString.size(), testOutputString.size());
    wstring compareString;
    for (size_t i = 0; i < testOutputString.size(); ++i)
    {
        wchar_t expected = resultString[i];
        wchar_t actual = testOutputString[i];
        if (actual != 65279)
        {
            if (StringUtility::AreEqualCaseInsensitive(wstring(1, expected), wstring(1, actual)))
            {
                compareString = compareString + resultString[i];
            }
            else
            {
                VERIFY(StringUtility::AreEqualCaseInsensitive(wstring(1, resultString[i]), wstring(1, testOutputString[i])), "Mismatch: {0}: Expected '{1}' Actual '{2}'", compareString, resultString[i], testOutputString[i]);
            }
        }
    }
}
#endif

BOOST_AUTO_TEST_CASE(TestActivationContextErrors)
{
    ComPointer<ComCodePackageActivationContext> comCodePackageActivationContextCPtr = TestGetComActivationContext();

    ComPointer<IFabricCodePackageActivationContext2> activationContext2CPtr;
    auto hr = comCodePackageActivationContextCPtr->QueryInterface(
        IID_IFabricCodePackageActivationContext2,
        activationContext2CPtr.VoidInitializationAddress());
    VERIFY_IS_TRUE(SUCCEEDED(hr));

    wstring invalidPackage(L"Invalid");
    IFabricCodePackage *codePackage = nullptr;
    IFabricConfigurationPackage *configPackage = nullptr;
    IFabricDataPackage *dataPackage = nullptr;
    FABRIC_ENDPOINT_RESOURCE_DESCRIPTION const* resource = nullptr;

    VERIFY_IS_TRUE(activationContext2CPtr->GetCodePackage(invalidPackage.c_str(), &codePackage) == FABRIC_E_CODE_PACKAGE_NOT_FOUND);
    VERIFY_IS_TRUE(activationContext2CPtr->GetConfigurationPackage(invalidPackage.c_str(), &configPackage) == FABRIC_E_CONFIGURATION_PACKAGE_NOT_FOUND);
    VERIFY_IS_TRUE(activationContext2CPtr->GetDataPackage(invalidPackage.c_str(), &dataPackage) == FABRIC_E_DATA_PACKAGE_NOT_FOUND);
    VERIFY_IS_TRUE(activationContext2CPtr->GetServiceEndpointResource(invalidPackage.c_str(), &resource) == FABRIC_E_SERVICE_ENDPOINT_RESOURCE_NOT_FOUND);

    VERIFY_IS_TRUE(activationContext2CPtr->GetCodePackage(NULL, &codePackage) == E_POINTER);
    VERIFY_IS_TRUE(activationContext2CPtr->GetConfigurationPackage(NULL, &configPackage) == E_POINTER);
    VERIFY_IS_TRUE(activationContext2CPtr->GetDataPackage(NULL, &dataPackage) == E_POINTER);
    VERIFY_IS_TRUE(activationContext2CPtr->GetServiceEndpointResource(NULL, &resource) == E_POINTER);

    VERIFY_IS_TRUE(activationContext2CPtr->GetCodePackage(invalidPackage.c_str(), NULL) == E_POINTER);
    VERIFY_IS_TRUE(activationContext2CPtr->GetConfigurationPackage(invalidPackage.c_str(), NULL) == E_POINTER);
    VERIFY_IS_TRUE(activationContext2CPtr->GetDataPackage(invalidPackage.c_str(), NULL) == E_POINTER);
    VERIFY_IS_TRUE(activationContext2CPtr->GetServiceEndpointResource(invalidPackage.c_str(), NULL) == E_POINTER);
}

BOOST_AUTO_TEST_CASE(TestActivationRuntimeWorkDirectories)
{
    // Test Case to test the Work Directories in Code Package Activation Context
    // We create some directories inside the work folder and then set the code package activation context.
    // If the directories in the FabricNodeConfig LogicalApplicationDirectories Map are created by the DownloadManager
    // the returned path from CPAC to these directories is a symbolic link to the root directories. In this test case,
    // we created AppDedicatedDataLog and Foo directories inside work and when requested from CPAC
    // will return work\AppDedicatedDataLog and work\Foo directories.
    // If you try to GetApplicationDirectory and the directory doesn't exist error will be thrown.

    FabricNodeContextSPtr config = make_shared<FabricNodeContext>();
    Common::FabricNodeConfig::KeyStringValueCollectionMap logicalApplicationDirectoryDirMap;

    ComPointer<ComCodePackageActivationContext> comCodePackageActivationContextCPtr = TestGetComActivationContext();

    ComPointer<IFabricCodePackageActivationContext6> activationContext2CPtr;
    ComPointer<IFabricStringResult> directoryPath;

    auto hr = comCodePackageActivationContextCPtr->QueryInterface(
        IID_IFabricCodePackageActivationContext6,
        activationContext2CPtr.VoidInitializationAddress());
    VERIFY_IS_TRUE(SUCCEEDED(hr));

    std::wstring workDirectory = comCodePackageActivationContextCPtr->get_WorkDirectory();

    //This is a negative test case.
    hr = activationContext2CPtr->GetDirectory(L"Random", directoryPath.InitializationAddress());
    //shoudnot succeed as the directory is not present
    VERIFY_IS_TRUE(!SUCCEEDED(hr));

    //Add some directories into the map
    logicalApplicationDirectoryDirMap.insert(make_pair(L"ApplicationDedicatedDataLog", L"."));
    logicalApplicationDirectoryDirMap.insert(make_pair(L"Foo", L"."));

    //Create directories in work folder
    wstring appDedicatedDataLogDirectoryPath = Path::Combine(workDirectory, L"ApplicationDedicatedDataLog");
    auto error = Directory::Create2(appDedicatedDataLogDirectoryPath);
    VERIFY_IS_TRUE(error.IsSuccess(), L"Unable to create directory work\\ApplicationDedicatedDataLog");

    wstring fooDirectory = Path::Combine(workDirectory, L"Foo");
    error = Directory::Create2(fooDirectory);
    VERIFY_IS_TRUE(error.IsSuccess(), L"Unable to create directory work\\Foo");

    config->Test_SetLogicalApplicationDirectories(move(logicalApplicationDirectoryDirMap));
    comCodePackageActivationContextCPtr = TestGetComActivationContext(config);

    ComPointer<IFabricCodePackageActivationContext6> activationContext2CPtr2;
    ComPointer<IFabricStringResult> fooDirectoryPath;
    ComPointer<IFabricStringResult> appDedicatedDataLogDirectoryPathResult;
    ComPointer<IFabricStringResult> unknownDirectoryPath;

    hr = comCodePackageActivationContextCPtr->QueryInterface(
        IID_IFabricCodePackageActivationContext6,
        activationContext2CPtr2.VoidInitializationAddress());
    VERIFY_IS_TRUE(SUCCEEDED(hr));

    hr = activationContext2CPtr2->GetDirectory(L"Foo", fooDirectoryPath.InitializationAddress());
    VERIFY_IS_TRUE(SUCCEEDED(hr));

    hr = activationContext2CPtr2->GetDirectory(L"ApplicationDedicatedDataLog", appDedicatedDataLogDirectoryPathResult.InitializationAddress());
    VERIFY_IS_TRUE(SUCCEEDED(hr));

    //Try to get some UnknownDirectory. Should return error. This is a negative test case.
    hr = activationContext2CPtr2->GetDirectory(L"Unknown", unknownDirectoryPath.InitializationAddress());
    VERIFY_IS_TRUE(!SUCCEEDED(hr));

    VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(wstring(fooDirectoryPath->get_String()), fooDirectory));

    //Delete the created work directories
    error = Directory::Delete(workDirectory, true, true/*deleteReadOnlyFiles*/);
    VERIFY_IS_TRUE(error.IsSuccess());
}

BOOST_AUTO_TEST_SUITE_END()

wstring CodePackageActivationContextTest::GetUnitTestFolder()
{
    wstring unitTestsFolder;

#if !defined(PLATFORM_UNIX)
    ASSERT_IF(Environment::ExpandEnvironmentStringsW(L"%_NTTREE%\\FabricUnitTests", unitTestsFolder) == 0, "GetUnitTestFolder() failed.");
#else
    ASSERT_IF(Environment::ExpandEnvironmentStringsW(L"%_NTTREE%/test", unitTestsFolder) == 0, "GetUnitTestFolder() failed.");
#endif

    return unitTestsFolder;
}

wstring CodePackageActivationContextTest::GetTestFilePath(wstring testFileName)
{
    wstring folder = Directory::GetCurrentDirectoryW();
    wstring filePath = Path::Combine(folder, testFileName);

    if (!File::Exists(filePath))
    {
        wstring unitTestsFolder = GetUnitTestFolder();
        return Path::Combine(unitTestsFolder, testFileName);
    }
    else
    {
        return filePath;
    }
}

ComPointer<ComCodePackageActivationContext> CodePackageActivationContextTest::TestGetComActivationContext()
{
    CodePackageActivationContextSPtr activationContextSPtr;
    Client::IpcHealthReportingClientSPtr ipcHealthReportingClientSPtr;
    std::map<wstring, wstring> testMap;
    Hosting2::FabricNodeContextSPtr fabricNodeContextSPtr = make_shared<FabricNodeContext>(
        L"TestNode",
        L"TestNodeId",
        0,
        L"TestNodeTyp",
        L"TestAddress",
        L"TestRuntimeAddress",
        L"TestDirectory",
        L"TestIP",
        L"TestNodeWorkFolder",
        testMap,
        testMap); ;

    ServicePackageIdentifier servicePackageId(CodePackageActivationContextTest::ApplicationId, CodePackageActivationContextTest::PackageName);
    ServicePackageInstanceIdentifier servicePackageInstanceId(servicePackageId, ServicePackageActivationContext(), L"");
    CodePackageInstanceIdentifier codePackageInstanceId(servicePackageInstanceId, CodePackageActivationContextTest::CurrentCodePackageName);

    auto error = CodePackageActivationContext::CreateCodePackageActivationContext(
        codePackageInstanceId,
        ApplicationRolloutVersion,
        CodePackageActivationContextTest::DeploymentRoot,
        ipcHealthReportingClientSPtr,
        fabricNodeContextSPtr,
        false, /*Not container host*/
        activationContextSPtr);

    VERIFY(
        error.IsSuccess(),
        "Failed to read code package activation context {0}:{1}",
        CodePackageActivationContextTest::ApplicationId,
        CodePackageActivationContextTest::CurrentCodePackageName);

    auto comCodePackageActivationContextCPtr = make_com<ComCodePackageActivationContext>(activationContextSPtr);

    return move(comCodePackageActivationContextCPtr);

}

ComPointer<ComCodePackageActivationContext> CodePackageActivationContextTest::TestGetComActivationContextSecurityCredentials()
{
    CodePackageActivationContextSPtr activationContextSPtr;
    Client::IpcHealthReportingClientSPtr ipcHealthReportingClientSPtr;
    std::map<wstring, wstring> testMap;
    Hosting2::FabricNodeContextSPtr fabricNodeContextSPtr = make_shared<FabricNodeContext>(
        L"TestNode",
        L"TestNodeId",
        0,
        L"TestNodeTyp",
        L"TestAddress",
        L"TestRuntimeAddress",
        L"TestDirectory",
        L"TestIP",
        L"TestNodeWorkFolder",
        testMap,
        testMap); ;

    ServicePackageIdentifier servicePackageId(CodePackageActivationContextTest::ApplicationId_App1, CodePackageActivationContextTest::PackageName);
    ServicePackageInstanceIdentifier servicePackageInstanceId(servicePackageId, ServicePackageActivationContext(), L"");
    CodePackageInstanceIdentifier codePackageInstanceId(servicePackageInstanceId, CodePackageActivationContextTest::CurrentCodePackageName);

    auto error = CodePackageActivationContext::CreateCodePackageActivationContext(
        codePackageInstanceId,
        ApplicationRolloutVersion,
        CodePackageActivationContextTest::DeploymentRoot,
        ipcHealthReportingClientSPtr,
        fabricNodeContextSPtr,
        false, /*Not container host*/
        activationContextSPtr);

    VERIFY(
        error.IsSuccess(),
        "Failed to read code package activation context {0}:{1}",
        CodePackageActivationContextTest::ApplicationId,
        CodePackageActivationContextTest::CurrentCodePackageName);

    auto comCodePackageActivationContextCPtr = make_com<ComCodePackageActivationContext>(activationContextSPtr);

    return move(comCodePackageActivationContextCPtr);

}

ComPointer<ComCodePackageActivationContext> CodePackageActivationContextTest::TestGetComActivationContext(FabricNodeContextSPtr & config)
{
    CodePackageActivationContextSPtr activationContextSPtr;
    Client::IpcHealthReportingClientSPtr ipcHealthReportingClientSPtr;
    Hosting2::FabricNodeContextSPtr fabricNodeContextSPtr;

    ServicePackageIdentifier servicePackageId(CodePackageActivationContextTest::ApplicationId, CodePackageActivationContextTest::PackageName);
    ServicePackageInstanceIdentifier servicePackageInstanceId(servicePackageId, ServicePackageActivationContext(), L"");
    CodePackageInstanceIdentifier codePackageInstanceId(servicePackageInstanceId, CodePackageActivationContextTest::CurrentCodePackageName);

    auto error = CodePackageActivationContext::CreateCodePackageActivationContext(
        codePackageInstanceId,
        ApplicationRolloutVersion,
        CodePackageActivationContextTest::DeploymentRoot,
        ipcHealthReportingClientSPtr,
        config ? config: fabricNodeContextSPtr,
        false, /*Not container host*/
        activationContextSPtr);

    VERIFY(
        error.IsSuccess(),
        "Failed to read code package activation context {0}:{1}",
        CodePackageActivationContextTest::ApplicationId,
        CodePackageActivationContextTest::CurrentCodePackageName);

    auto comCodePackageActivationContextCPtr = make_com<ComCodePackageActivationContext>(activationContextSPtr);

    return move(comCodePackageActivationContextCPtr);

}

void CodePackageActivationContextTest::PrintStatefulServiceType(FABRIC_STATEFUL_SERVICE_TYPE_DESCRIPTION & type, StringWriter & testOutput)
{
    testOutput.WriteLine(
        "StatefulTypeDescription: ServiceTypeName = {0}, PlacementConstraints = '{1}', HasPersistedState = {2}",
        type.ServiceTypeName,
        type.PlacementConstraints,
        type.HasPersistedState);

    if (type.LoadMetrics->Count > 0)
    {
        VERIFY_IS_FALSE(type.LoadMetrics->Items == NULL);

        testOutput.Write("LoadMetrics: [");
        for (ULONG i = 0; i < type.LoadMetrics->Count; ++i)
        {
            const FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION* metricDescription = type.LoadMetrics->Items + i;
            testOutput.Write(" (Name = {0}, PrimaryDefaultLoad = '{1}', SecondaryDefaultLoad = '{2}', Weight = '{3}') ",
                metricDescription->Name,
                metricDescription->PrimaryDefaultLoad,
                metricDescription->SecondaryDefaultLoad,
                metricDescription->Weight);
        }
        testOutput.WriteLine("]");
    }

    if (type.Extensions->Count > 0)
    {
        VERIFY_IS_FALSE(type.Extensions->Items == NULL);

        testOutput.Write("Extensions: [");
        for (ULONG i = 0; i < type.Extensions->Count; ++i)
        {
            const FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION* descriptionExtension = type.Extensions->Items + i;
            testOutput.Write(" (Name = {0}, Value = {1}) ", descriptionExtension->Name, descriptionExtension->Value);
        }
        testOutput.WriteLine("]");
    }
}

void CodePackageActivationContextTest::PrintStatelessServiceType(FABRIC_STATELESS_SERVICE_TYPE_DESCRIPTION & type, StringWriter & testOutput)
{
    testOutput.WriteLine(
        "StatelessTypeDescription: ServiceTypeName = {0}, PlacementConstraints = '{1}'",
        type.ServiceTypeName,
        type.PlacementConstraints);

    if (type.LoadMetrics->Count > 0)
    {
        VERIFY_IS_FALSE(type.LoadMetrics->Items == NULL);

        testOutput.Write("LoadMetrics: [");
        for (ULONG i = 0; i < type.LoadMetrics->Count; ++i)
        {
            const FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION* metricDescription = type.LoadMetrics->Items + i;
            testOutput.Write(" (Name = {0}, PrimaryDefaultLoad = '{1}', SecondaryDefaultLoad = '{2}', Weight = '{3}') ",
                metricDescription->Name,
                metricDescription->PrimaryDefaultLoad,
                metricDescription->SecondaryDefaultLoad,
                metricDescription->Weight);
        }
        testOutput.WriteLine("]");
    }

    if (type.Extensions->Count > 0)
    {
        VERIFY_IS_FALSE(type.Extensions->Items == NULL);

        testOutput.Write("Extensions: [");
        for (ULONG i = 0; i < type.Extensions->Count; ++i)
        {
            const FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION* descriptionExtension = type.Extensions->Items + i;
            wstring descriptionExtensionValue = wstring(descriptionExtension->Value);
            descriptionExtensionValue.erase(descriptionExtensionValue.begin());
            testOutput.Write(" (Name = {0}, Value = {1}) ", descriptionExtension->Name, descriptionExtension->Value);
        }
        testOutput.WriteLine("]");
    }
}

void CodePackageActivationContextTest::VerifyComSecurityCredentials(SecuritySettings credentials)
{
    std::shared_ptr<Common::X509FindValue> findValue;
    X509FindValue::Create(X509FindType::FindByThumbprint, CodePackageActivationContextTest::ComSecurityCertIssuerThumbprint1, findValue);
    CertContextUPtr cert;
    auto error = CryptoUtility::GetCertificate(
        X509StoreLocation::LocalMachine,
        L"Root",
        findValue,
        cert);
    VERIFY_IS_TRUE(error.IsSuccess());
    X509PubKey::SPtr certIssuerPubKey = make_shared<X509PubKey>(cert.get());
    Thumbprint::SPtr certIssuerThumbprint;
    error = Thumbprint::Create(cert.get(), certIssuerThumbprint);
    VERIFY_IS_TRUE(error.IsSuccess());

    std::shared_ptr<Common::X509FindValue> findValue2;
    X509FindValue::Create(X509FindType::FindByThumbprint, CodePackageActivationContextTest::ComSecurityCertIssuerThumbprint2, findValue2);
    CertContextUPtr cert2;
    error = CryptoUtility::GetCertificate(
        X509StoreLocation::LocalMachine,
        L"Root",
        findValue,
        cert2);
    VERIFY_IS_TRUE(error.IsSuccess());
    X509PubKey::SPtr certIssuerPubKey2 = make_shared<X509PubKey>(cert2.get());
    Thumbprint::SPtr certIssuerThumbprint2;
    error = Thumbprint::Create(cert2.get(), certIssuerThumbprint2);
    VERIFY_IS_TRUE(error.IsSuccess());

    wstring certCName = L"WinFabric-Test-SAN1-Alice";
    SecurityConfig::X509NameMapBase::const_iterator match;
    bool matched = credentials.RemoteX509Names().Match(
        certCName,
        certIssuerPubKey,
        certIssuerThumbprint,
        match);
    VERIFY_IS_TRUE(matched);
    VERIFY_IS_TRUE(match->second.Size() == 2);

    matched = credentials.RemoteX509Names().Match(
        certCName,
        certIssuerPubKey,
        match);
    VERIFY_IS_TRUE(matched);
    VERIFY_IS_TRUE(match->second.Size() == 2);

    matched = credentials.RemoteX509Names().Match(
        certCName,
        certIssuerThumbprint,
        match);
    VERIFY_IS_FALSE(matched);

    matched = credentials.RemoteX509Names().Match(
    certCName,
    certIssuerPubKey2,
    certIssuerThumbprint2,
    match);

    VERIFY_IS_TRUE(matched);
    VERIFY_IS_TRUE(match->second.Size() == 2);

    matched = credentials.RemoteX509Names().Match(
    certCName,
    certIssuerPubKey2,
    match);
    VERIFY_IS_TRUE(matched);
    VERIFY_IS_TRUE(match->second.Size() == 2);

    matched = credentials.RemoteX509Names().Match(
    certCName,
    certIssuerThumbprint2,
    match);
    VERIFY_IS_FALSE(matched);
}
