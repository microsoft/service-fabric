// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestImageBuilder.h"
#include "TestServiceModelServiceNameArray.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#define VERIFY( condition, fmt, ...) \
    { \
    wstring tmp; \
    StringWriter writer(tmp); \
    writer.Write(fmt, __VA_ARGS__); \
    VERIFY_IS_TRUE(condition, tmp.c_str()); \
    } \

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ServiceModel;
using namespace Naming;

using namespace Management::ImageModel;
using namespace Management::ClusterManager;

const StringLiteral TestSource = "CM.SingleNode.Test";

class ClusterManagerTest
{

protected:
    ClusterManagerTest() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~ClusterManagerTest() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    void SetupImageBuilderProxyTestHelper(
        NamingUri const & appName,
        __out ServiceModelApplicationId &,
        __out ServiceModelVersion & expectedAppTypeVersion,
        __out vector<StoreDataServicePackage> &,
        __out vector<StoreDataServiceTemplate> &,
        __out vector<PartitionedServiceDescriptor> &);

    void ImageBuilderProxyTestHelper(
        NamingUri const & appName,
        ServiceModelApplicationId const &,
        wstring const & baseDirectory,
        wstring const & srcAppManifest,
        wstring const & srcServiceManifest,
        ServiceModelVersion const & expectedAppTypeVersion,
        vector<StoreDataServicePackage> const &,
        vector<StoreDataServiceTemplate> const &,
        vector<PartitionedServiceDescriptor> const &);

    Common::ErrorCode ImageBuilderProxyTestHelper(
        NamingUri const & appName,
        ServiceModelApplicationId const &,
        wstring const & baseDirectory,
        wstring const & srcAppManifest,
        wstring const & srcServiceManifest,
        map<wstring, wstring> const & appParams,
        bool expectError,
        ServiceModelVersion const & expectedAppTypeVersion,
        vector<StoreDataServicePackage> const &,
        vector<StoreDataServiceTemplate> const &,
        vector<PartitionedServiceDescriptor> const &);

    void ImageBuilderProxyForSfpkgTestHelper(
        NamingUri const & appName,
        ServiceModelApplicationId const &,
        ServiceModelVersion const & expectedAppTypeVersion,
        vector<StoreDataServicePackage> const &,
        vector<StoreDataServiceTemplate> const &,
        vector<PartitionedServiceDescriptor> const &);

    void CheckManifests(
        vector<ServiceModelServiceManifestDescription> const & manifests,
        vector<StoreDataServicePackage> const & expectedPackages);

    void CheckDigestedApplication(
        DigestedApplicationDescription const & appDescription,
        vector<StoreDataServicePackage> const & expectedPackages,
        vector<StoreDataServiceTemplate> const & expectedTemplates,
        vector<PartitionedServiceDescriptor> const & expectedDefaultServices);

    void DeleteDirectory(wstring const & directory);
    void ResetDirectory(wstring const & directory);
    void CopyFile(wstring const & src, wstring const & dest);

    void ApplicationUpgradeDescriptionUnitTestHelper(bool validHealthPolicy);
    void ApplicationUpgradeDescriptionWrapperUnitTestHelper(bool validHealthPolicy);

    void FabricUpgradeDescriptionUnitTestHelper(bool validHealthPolicy);
    void FabricUpgradeDescriptionWrapperUnitTestHelper(bool validHealthPolicy);

    template <class TPublicDescription, class TInternalDescription>
    void UpgradeWrapperMaxTimeoutTestHelper();

    template <class TUpdateDescription>
    void VerifyJsonSerialization(
        shared_ptr<TUpdateDescription> const &);

    static void VerifyClusterHealthPolicy(FABRIC_CLUSTER_HEALTH_POLICY const & publicPolicy, ClusterHealthPolicy const & internalPolicy);
    static void VerifyApplicationHealthPoliciesEqual(ApplicationHealthPolicyMapSPtr const & l, ApplicationHealthPolicyMapSPtr const & r);

    template <class TPublicUpdateDescription, class TPublicHealthPolicy, class TInternalUpdateDescription>
    shared_ptr<TInternalUpdateDescription> CreateUpdateDescriptionFromPublic(
        LPCWSTR name,
        DWORD flags,
        __in Random &,
        __inout map<DWORD, int> & expectedValues,
        ErrorCodeValue::Enum const expectedError = ErrorCodeValue::Success);

    template <class TUpdateDescription>
    void VerifyUpdateDescription(
        wstring const & name,
        DWORD flags,
        shared_ptr<TUpdateDescription> const &,
        map<DWORD, int> const & expectedValues);
};
bool ModuleSetup()
{
    // load tracing configuration
    Config cfg;
    Common::TraceProvider::LoadConfiguration(cfg);

    return true;
}

bool ModuleCleanup()
{
    return true;
}
struct GlobalFixture
{
    GlobalFixture() { ModuleSetup(); }

    ~GlobalFixture() { ModuleCleanup(); }
};
// Test manifest and package reference counting during unprovision
//
BOOST_GLOBAL_FIXTURE(GlobalFixture);

BOOST_FIXTURE_TEST_SUITE(ClusterManagerTestSuite, ClusterManagerTest)

// Test ImageBuilderProxy when some passed in parameters are invalid. Expected image builder error.
//
BOOST_AUTO_TEST_CASE(ImageBuilderProxyWithNewLineArguments)
{
    Trace.WriteInfo(TestSource, "*** ImageBuilderProxyWithNewLineArguments");
    NamingUri appName(L"proxy_test_invalid_arg");

    wstring ApplicationManifestSourceFile(L"ClusterManager.SingleNode.Test.ApplicationManifest1.xml");
    wstring ServiceManifestSourceFile(L"ClusterManager.SingleNode.Test.ServiceManifest.xml");

    ServiceModelApplicationId appId;
    ServiceModelVersion expectedAppTypeVersion;
    vector<StoreDataServicePackage> expectedPackages;
    vector<StoreDataServiceTemplate> expectedTemplates;
    vector<PartitionedServiceDescriptor> expectedDefaultServices;

    SetupImageBuilderProxyTestHelper(
        appName,
        appId,
        expectedAppTypeVersion,
        expectedPackages,
        expectedTemplates,
        expectedDefaultServices);

    map<wstring, wstring> appParams;
    appParams[L"Key1"] = L"value1";
    appParams[L"Key2"] = L"value2\r\nwith\r\nwhitespaces\r\nwhich\r\nis\r\ninvalid";

    auto error = ImageBuilderProxyTestHelper(
        appName,
        appId,
        Directory::GetCurrentDirectory(),
        ApplicationManifestSourceFile,
        ServiceManifestSourceFile,
        appParams,
        true /*expectedResult*/,
        expectedAppTypeVersion,
        expectedPackages,
        expectedTemplates,
        expectedDefaultServices);

    Trace.WriteInfo(TestSource, "*** ImageBuilderProxyWithNewLineArguments: ImageBuilderProxy ran with \\r\\n characters in the app parameters with error code {0} and message {1}", error, error.Message);

    VERIFY_IS_TRUE(ErrorCodeValue::ImageBuilderValidationError == error.ReadValue());

    // Look for LF only characters which are how Unix environments handle newlines
    appParams[L"Key2"] = L"value2\nwith\nwhitespaces\nwhich\nis\ninvalid";

    error = ImageBuilderProxyTestHelper(
        appName,
        appId,
        Directory::GetCurrentDirectory(),
        ApplicationManifestSourceFile,
        ServiceManifestSourceFile,
        appParams,
        true /*expectedResult*/,
        expectedAppTypeVersion,
        expectedPackages,
        expectedTemplates,
        expectedDefaultServices);

    Trace.WriteInfo(TestSource, "*** ImageBuilderProxyWithNewLineArguments: ImageBuilderProxy ran with \\n characters in the app parameters with error code {0} and message {1}", error, error.Message);

    VERIFY_IS_TRUE(ErrorCodeValue::ImageBuilderValidationError == error.ReadValue());
}

BOOST_AUTO_TEST_CASE(ImageBuilderProxyWithSfpkgs)
{
    NamingUri appName(L"ImageBuilderProxyWithSfpkgs");

    ServiceModelApplicationId appId;
    ServiceModelVersion expectedAppTypeVersion;
    vector<StoreDataServicePackage> expectedPackages;
    vector<StoreDataServiceTemplate> expectedTemplates;
    vector<PartitionedServiceDescriptor> expectedDefaultServices;

    SetupImageBuilderProxyTestHelper(
        appName,
        appId,
        expectedAppTypeVersion,
        expectedPackages,
        expectedTemplates,
        expectedDefaultServices);

    ImageBuilderProxyForSfpkgTestHelper(
        appName,
        appId,
        expectedAppTypeVersion,
        expectedPackages,
        expectedTemplates,
        expectedDefaultServices);
}

BOOST_AUTO_TEST_CASE(ServiceModelServiceNameExCompatibilityUnitTest)
{
    Trace.WriteInfo(TestSource, "*** ServiceModelServiceNameExCompatibilityUnitTest");

    // ServiceModelServiceNameEx basic functionality
    {
        std::wstring serviceName(L"fabric:/myservice");
        Common::NamingUri namingUri;
        bool success = NamingUri::TryParse(serviceName, namingUri);
        VERIFY(success, "Could not parse {0} into a NamingUri", serviceName);

        ServiceModelServiceNameEx nameEx(namingUri, L"my.dns.name");
        ServiceModelServiceNameEx nameEx1(nameEx);
        VERIFY(nameEx1.Name == serviceName, "Failed to verify ServiceModelServiceNameEx copy");
        ServiceModelServiceNameEx nameEx2(move(nameEx));
        VERIFY(nameEx.Name.empty() && nameEx.DnsName.empty(), "Failed to verify ServiceModelServiceNameEx move");
    }
    // Serialize ServiceModelServiceName, deserialize ServiceModelServiceNameEx
    {
        std::vector<byte> serializedName;
        ServiceModelServiceName name(L"fabric:/myservice");

        ErrorCode error = Common::FabricSerializer::Serialize(&name, serializedName);
        VERIFY(error.IsSuccess(), "Serialization of ServiceModelServiceName failed with {0}", error);

        ServiceModelServiceNameEx nameEx;
        error = Common::FabricSerializer::Deserialize(nameEx, serializedName);
        VERIFY(error.IsSuccess(), "Deserialization of ServiceModelServiceNameEx failed with {0}", error);
        VERIFY(nameEx.Name == name.Value, "Deserialization of ServiceModelServiceNameEx failed, actual value {0}, expected {1}", nameEx.Name, name.Value);
    }

    // Serialize ServiceModelServiceNameEx, deserialize ServiceModelServiceName
    {
        std::wstring serviceName(L"fabric:/myservice");
        Common::NamingUri namingUri;
        bool success = NamingUri::TryParse(serviceName, namingUri);
        VERIFY(success, "Could not parse {0} into a NamingUri", serviceName);

        ServiceModelServiceNameEx nameEx(namingUri, L"my.dns.name");

        std::vector<byte> serializedNameEx;
        ErrorCode error = Common::FabricSerializer::Serialize(&nameEx, serializedNameEx);
        VERIFY(error.IsSuccess(), "Serialization of ServiceModelServiceNameEx failed with {0}", error);

        ServiceModelServiceName name;
        error = Common::FabricSerializer::Deserialize(name, serializedNameEx);
        VERIFY(error.IsSuccess(), "Deserialization of ServiceModelServiceName failed with {0}", error);
        VERIFY(nameEx.Name == name.Value, "Deserialization of ServiceModelServiceName failed, actual value {0}, expected {1}", name.Value, nameEx.Name);
    }

    // Verify ServiceModelServiceNameEx behaves properly in std::vector
    {
        std::vector<ServiceModelServiceNameEx> vectorNamesEx;

        Common::NamingUri namingUri1;
        {
            std::wstring serviceName(L"fabric:/myservice1");
            bool success = NamingUri::TryParse(serviceName, namingUri1);
            VERIFY(success, "Could not parse {0} into a NamingUri", serviceName);
        }

        Common::NamingUri namingUri2;
        {
            std::wstring serviceName(L"fabric:/myservice2");
            bool success = NamingUri::TryParse(serviceName, namingUri1);
            VERIFY(success, "Could not parse {0} into a NamingUri", serviceName);
        }

        {
            ServiceModelServiceNameEx nameEx1(namingUri1, L"my.dns.name");
            auto findIter = find(vectorNamesEx.begin(), vectorNamesEx.end(), nameEx1);
            if (findIter == vectorNamesEx.end())
            {
                vectorNamesEx.push_back(move(nameEx1));
            }
            else
            {
                VERIFY(false, "ServiceModelServiceNameEx unexpectedly found in the vector");
            }
        }
        {
            ServiceModelServiceNameEx nameEx2(namingUri2, L"my.dns.name");
            auto findIter = find(vectorNamesEx.begin(), vectorNamesEx.end(), nameEx2);
            if (findIter == vectorNamesEx.end())
            {
                vectorNamesEx.push_back(move(nameEx2));
            }
            else
            {
                VERIFY(false, "ServiceModelServiceNameEx unexpectedly found in the vector");
            }
        }
        {
            ServiceModelServiceNameEx nameEx2(namingUri2, L"my.dns.name");
            auto findIter = find(vectorNamesEx.begin(), vectorNamesEx.end(), nameEx2);
            if (findIter == vectorNamesEx.end())
            {
                VERIFY(false, "Faield to find ServiceModelServiceNameEx in the vector");
            }
        }
    }
    // Serialize vector ServiceModelServiceName, deserialize vector ServiceModelServiceNameEx
    {
        TestServiceModelServiceNames names;
        std::vector<byte> serializedName;
        {
            ServiceModelServiceName name1(L"fabric:/myservice1");
            ServiceModelServiceName name2(L"fabric:/myservice2");

            names.ServiceNames.push_back(name1);
            names.ServiceNames.push_back(name2);

            ErrorCode error = Common::FabricSerializer::Serialize(&names, serializedName);
            VERIFY(error.IsSuccess(), "Serialization of TestServiceModelServiceNames failed with {0}", error);
        }

        {
            TestServiceModelServiceNamesEx namesEx;
            ErrorCode error = Common::FabricSerializer::Deserialize(namesEx, serializedName);

            VERIFY(error.IsSuccess(), "Deserialization of TestServiceModelServiceNamesEx failed with {0}", error);
            VERIFY(names.ServiceNames.size() == namesEx.ServiceNames.size(), "Deserialization of TestServiceModelServiceNamesEx failed, actual size {0}, expected size{1}", namesEx.ServiceNames.size(), names.ServiceNames.size());
            for (int i = 0; i < names.ServiceNames.size(); i++)
            {
                VERIFY(names.ServiceNames[i].Value == namesEx.ServiceNames[i].Name, "Deserialization of TestServiceModelServiceNamesEx failed, item doesn't match, actual {0}, expected {1}", namesEx.ServiceNames[i].Name, names.ServiceNames[i].Value);
            }
        }
    }
    // Serialize vector ServiceModelServiceNameEx, deserialize vector ServiceModelServiceName
    {
        TestServiceModelServiceNamesEx namesEx;
        std::vector<byte> serializedNameEx;
        {
            Common::NamingUri namingUri1;
            {
                std::wstring serviceName(L"fabric:/myservice1");
                bool success = NamingUri::TryParse(serviceName, namingUri1);
                VERIFY(success, "Could not parse {0} into a NamingUri", serviceName);
            }

            Common::NamingUri namingUri2;
            {
                std::wstring serviceName(L"fabric:/myservice2");
                bool success = NamingUri::TryParse(serviceName, namingUri1);
                VERIFY(success, "Could not parse {0} into a NamingUri", serviceName);
            }

            ServiceModelServiceNameEx name1(namingUri1, L"my.dns1");
            ServiceModelServiceNameEx name2(namingUri2, L"my.dns2");

            namesEx.ServiceNames.push_back(name1);
            namesEx.ServiceNames.push_back(name2);

            ErrorCode error = Common::FabricSerializer::Serialize(&namesEx, serializedNameEx);
            VERIFY(error.IsSuccess(), "Serialization of TestServiceModelServiceNamesEx failed with {0}", error);
        }

        {
            TestServiceModelServiceNames names;
            ErrorCode error = Common::FabricSerializer::Deserialize(names, serializedNameEx);

            VERIFY(error.IsSuccess(), "Deserialization of TestServiceModelServiceNames failed with {0}", error);
            VERIFY(names.ServiceNames.size() == namesEx.ServiceNames.size(), "Deserialization of TestServiceModelServiceNames failed, actual size {0}, expected size{1}", names.ServiceNames.size(), namesEx.ServiceNames.size());
            for (int i = 0; i < names.ServiceNames.size(); i++)
            {
                VERIFY(names.ServiceNames[i].Value == namesEx.ServiceNames[i].Name, "Deserialization of TestServiceModelServiceNames failed, item doesn't match, actual {0}, expected {1}", names.ServiceNames[i].Value, namesEx.ServiceNames[i].Name);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(UnprovisionCleanup)
{
    std::wstring TestBuildPath(L".\\TestBuildPath");
    ServiceModelTypeName TestAppTypeName(L"TestAppTypeName");
    ServiceModelVersion TestAppTypeVersion(L"TestAppTypeVersion");

    std::wstring TestBuildPathX(L".\\TestBuildPathX");
    ServiceModelTypeName TestAppTypeNameX(L"TestAppTypeName");
    ServiceModelVersion TestAppTypeVersionX(L"TestAppTypeVersionX");

    std::wstring TestBuildPath2(L".\\TestBuildPath2");
    ServiceModelTypeName TestAppTypeName2(L"TestAppTypeName");
    ServiceModelVersion TestAppTypeVersion2(L"TestAppTypeVersion2");

    std::wstring TestBuildPath3(L".\\TestBuildPath3");
    ServiceModelTypeName TestAppTypeName3(L"TestAppTypeName");
    ServiceModelVersion TestAppTypeVersion3(L"TestAppTypeVersion3");

    std::wstring TestBuildPath4(L".\\TestBuildPath4");
    ServiceModelTypeName TestAppTypeName4(L"TestAppTypeName");
    ServiceModelVersion TestAppTypeVersion4(L"TestAppTypeVersion4");

    std::wstring TestBuildPath5(L".\\TestBuildPath5");
    ServiceModelTypeName TestAppTypeName5(L"TestAppTypeName5");
    ServiceModelVersion TestAppTypeVersion5(L"TestAppTypeVersion");

    std::wstring TestBuildPath6(L".\\TestBuildPath6");
    ServiceModelTypeName TestAppTypeName6(L"TestAppTypeName");
    ServiceModelVersion TestAppTypeVersion6(L"TestAppTypeVersion6");

    std::wstring TestBuildPath7(L".\\TestBuildPath7");
    ServiceModelTypeName TestAppTypeName7(L"TestAppTypeName");
    ServiceModelVersion TestAppTypeVersion7(L"TestAppTypeVersion7");

    auto imageBuilder = make_unique<TestImageBuilder>();

    vector<ServiceModelServiceManifestDescription> manifests1;
    vector<ServiceModelServiceManifestDescription> manifests2;
    vector<ServiceModelServiceManifestDescription> manifests3;
    vector<ServiceModelServiceManifestDescription> manifests4;
    vector<ServiceModelServiceManifestDescription> manifests5;
    vector<ServiceModelServiceManifestDescription> manifests6;
    vector<ServiceModelServiceManifestDescription> manifests7;

    // un-shared service manifest and packages
    ServiceModelServiceManifestDescription manifest(L"manifestA", L"versionA");
    manifest.AddCodePackage(L"codeA", L"versionA");
    manifest.AddConfigPackage(L"configA", L"versionA");
    manifest.AddDataPackage(L"dataA", L"versionA");
    manifests1.push_back(manifest);

    // shared service manifest
    manifest = ServiceModelServiceManifestDescription(L"manifestB", L"versionB");
    manifest.AddCodePackage(L"codeB", L"versionB");
    manifest.AddConfigPackage(L"configB", L"versionB");
    manifest.AddDataPackage(L"dataB", L"versionB");
    manifests1.push_back(manifest);
    manifests2.push_back(manifest);

    // shared code package only
    manifest = ServiceModelServiceManifestDescription(L"manifestC", L"versionC");
    manifest.AddCodePackage(L"codeC", L"versionC");
    manifest.AddConfigPackage(L"configC", L"versionC");
    manifest.AddDataPackage(L"dataC", L"versionC");
    manifests2.push_back(manifest);

    manifest = ServiceModelServiceManifestDescription(L"manifestC", L"versionC2");
    manifest.AddCodePackage(L"codeC", L"versionC");
    manifest.AddConfigPackage(L"configC", L"versionC2");
    manifest.AddDataPackage(L"dataC", L"versionC2");
    manifests3.push_back(manifest);

    // shared config and data packages
    manifest = ServiceModelServiceManifestDescription(L"manifestD", L"versionD");
    manifest.AddCodePackage(L"codeD", L"versionD");
    manifest.AddConfigPackage(L"configD", L"versionD");
    manifest.AddDataPackage(L"dataD", L"versionD");
    manifests3.push_back(manifest);

    manifest = ServiceModelServiceManifestDescription(L"manifestD", L"versionD2");
    manifest.AddCodePackage(L"codeD", L"versionD2");
    manifest.AddConfigPackage(L"configD", L"versionD");
    manifest.AddDataPackage(L"dataD", L"versionD");
    manifests4.push_back(manifest);

    // un-shared service manifest (due to being in a different application type)
    manifest = ServiceModelServiceManifestDescription(L"manifestE", L"versionE");
    manifest.AddCodePackage(L"codeE", L"versionE");
    manifest.AddConfigPackage(L"configE", L"versionE");
    manifest.AddDataPackage(L"dataE", L"versionE");
    manifests4.push_back(manifest);

    manifest = ServiceModelServiceManifestDescription(L"manifestE", L"versionE");
    manifest.AddCodePackage(L"codeE", L"versionE");
    manifest.AddConfigPackage(L"configE", L"versionE");
    manifest.AddDataPackage(L"dataE", L"versionE");
    manifests5.push_back(manifest);

    // un-shared service manifest (due to being in a different service manifest)
    manifest = ServiceModelServiceManifestDescription(L"manifestF", L"versionF");
    manifest.AddCodePackage(L"codeF", L"versionF");
    manifest.AddConfigPackage(L"configF", L"versionF");
    manifest.AddDataPackage(L"dataF", L"versionF");
    manifests5.push_back(manifest);

    manifest = ServiceModelServiceManifestDescription(L"manifestF2", L"versionF");
    manifest.AddCodePackage(L"codeF", L"versionF");
    manifest.AddCodePackage(L"codeF2", L"versionF2");
    manifest.AddConfigPackage(L"configF", L"versionF");
    manifest.AddConfigPackage(L"configF2", L"versionF2");
    manifest.AddDataPackage(L"dataF", L"versionF");
    manifest.AddDataPackage(L"dataF2", L"versionF2");
    manifests6.push_back(manifest);

    manifest = ServiceModelServiceManifestDescription(L"manifestF2b", L"versionFb");
    manifest.AddCodePackage(L"codeFb", L"versionFb");
    manifest.AddCodePackage(L"codeF2b", L"versionF2b");
    manifest.AddConfigPackage(L"configFb", L"versionFb");
    manifest.AddConfigPackage(L"configF2b", L"versionF2b");
    manifest.AddDataPackage(L"dataFb", L"versionFb");
    manifest.AddDataPackage(L"dataF2b", L"versionF2b");
    manifests6.push_back(manifest);

    // multiple shared manifests and multiple shared packages
    manifest = ServiceModelServiceManifestDescription(L"manifestF2", L"versionF2");
    manifest.AddCodePackage(L"codeF", L"versionF");
    manifest.AddCodePackage(L"codeF2", L"versionF2");
    manifest.AddConfigPackage(L"configF", L"versionF");
    manifest.AddConfigPackage(L"configF2", L"versionF2");
    manifest.AddDataPackage(L"dataF", L"versionF");
    manifest.AddDataPackage(L"dataF2", L"versionF2");
    manifests7.push_back(manifest);

    manifest = ServiceModelServiceManifestDescription(L"manifestF2b", L"versionF2b");
    manifest.AddCodePackage(L"codeFb", L"versionFb");
    manifest.AddCodePackage(L"codeF2b", L"versionF2b");
    manifest.AddConfigPackage(L"configFb", L"versionFb");
    manifest.AddConfigPackage(L"configF2b", L"versionF2b");
    manifest.AddDataPackage(L"dataFb", L"versionFb");
    manifest.AddDataPackage(L"dataF2b", L"versionF2b");
    manifests7.push_back(manifest);

    // mock image store
    imageBuilder->SetServiceManifests(TestAppTypeName, TestAppTypeVersion, move(manifests1));
    imageBuilder->SetServiceManifests(TestAppTypeName2, TestAppTypeVersion2, move(manifests2));
    imageBuilder->SetServiceManifests(TestAppTypeName3, TestAppTypeVersion3, move(manifests3));
    imageBuilder->SetServiceManifests(TestAppTypeName4, TestAppTypeVersion4, move(manifests4));
    imageBuilder->SetServiceManifests(TestAppTypeName5, TestAppTypeVersion5, move(manifests5));
    imageBuilder->SetServiceManifests(TestAppTypeName6, TestAppTypeVersion6, move(manifests6));
    imageBuilder->SetServiceManifests(TestAppTypeName7, TestAppTypeVersion7, move(manifests7));

    // mock provision
    imageBuilder->MockProvision(TestBuildPath, TestAppTypeName, TestAppTypeVersion);
    imageBuilder->MockProvision(TestBuildPath2, TestAppTypeName2, TestAppTypeVersion2);
    imageBuilder->MockProvision(TestBuildPath3, TestAppTypeName3, TestAppTypeVersion3);
    imageBuilder->MockProvision(TestBuildPath4, TestAppTypeName4, TestAppTypeVersion4);
    imageBuilder->MockProvision(TestBuildPath5, TestAppTypeName5, TestAppTypeVersion5);
    imageBuilder->MockProvision(TestBuildPath6, TestAppTypeName6, TestAppTypeVersion6);
    imageBuilder->MockProvision(TestBuildPath7, TestAppTypeName7, TestAppTypeVersion7);
    imageBuilder->MockProvision(TestBuildPathX, TestAppTypeNameX, TestAppTypeVersionX);

    auto sharedState = imageBuilder->GetSharedState();

    ServiceModelServiceManifestDescription manifestToVerify;
    bool manifestExists = sharedState.TryGetServiceManifest(TestAppTypeName, TestAppTypeVersion, L"manifestA", L"versionA", manifestToVerify);
    VERIFY(manifestExists, "manifestA:versionA does not exist");

    imageBuilder->MockUnprovisionApplicationType(TestAppTypeName, TestAppTypeVersion);

    manifestExists = sharedState.TryGetServiceManifest(TestAppTypeName, TestAppTypeVersion, L"manifestA", L"versionA", manifestToVerify);
    VERIFY(!manifestExists, "manifestA:versionA still exists");

    manifestExists = sharedState.TryGetServiceManifest(TestAppTypeName, TestAppTypeVersion, L"manifestB", L"versionB", manifestToVerify);
    VERIFY(manifestExists, "manifestB:versionB does not exist");
    VERIFY(manifestToVerify.HasCodePackage(L"codeB", L"versionB"), "manifestB:versionB missing code package");
    VERIFY(manifestToVerify.HasConfigPackage(L"configB", L"versionB"), "manifestB:versionB missing config package");
    VERIFY(manifestToVerify.HasDataPackage(L"dataB", L"versionB"), "manifestB:versionB missing data package");

    imageBuilder->MockUnprovisionApplicationType(TestAppTypeName2, TestAppTypeVersion2);

    manifestExists = sharedState.TryGetServiceManifest(TestAppTypeName2, TestAppTypeVersion2, L"manifestB", L"versionB", manifestToVerify);
    VERIFY(!manifestExists, "manifestB:versionB still exists");

    manifestExists = sharedState.TryGetServiceManifest(TestAppTypeName2, TestAppTypeVersion2, L"manifestC", L"versionC", manifestToVerify);
    VERIFY(manifestExists, "manifestC:versionC does not exist");
    VERIFY(manifestToVerify.HasCodePackage(L"codeC", L"versionC"), "manifestC:versionC missing code package");
    VERIFY(!manifestToVerify.HasConfigPackage(L"configC", L"versionC"), "manifestC:versionC still has config package");
    VERIFY(!manifestToVerify.HasDataPackage(L"dataC", L"versionC"), "manifestC:versionC still has data package");

    imageBuilder->MockUnprovisionApplicationType(TestAppTypeName3, TestAppTypeVersion3);

    manifestExists = sharedState.TryGetServiceManifest(TestAppTypeName3, TestAppTypeVersion3, L"manifestC", L"versionC2", manifestToVerify);
    VERIFY(!manifestExists, "manifestC:versionC2 still exists");

    manifestExists = sharedState.TryGetServiceManifest(TestAppTypeName3, TestAppTypeVersion3, L"manifestD", L"versionD", manifestToVerify);
    VERIFY(manifestExists, "manifestD:versionD does not exist");
    VERIFY(!manifestToVerify.HasCodePackage(L"codeD", L"versionD"), "manifestD:versionD still has code package");
    VERIFY(manifestToVerify.HasConfigPackage(L"configD", L"versionD"), "manifestD:versionD missing config package");
    VERIFY(manifestToVerify.HasDataPackage(L"dataD", L"versionD"), "manifestD:versionD missing data package");

    imageBuilder->MockUnprovisionApplicationType(TestAppTypeName4, TestAppTypeVersion4);

    manifestExists = sharedState.TryGetServiceManifest(TestAppTypeName4, TestAppTypeVersion4, L"manifestD", L"versionD2", manifestToVerify);
    VERIFY(!manifestExists, "manifestD:versionD2 still exists");

    manifestExists = sharedState.TryGetServiceManifest(TestAppTypeName4, TestAppTypeVersion4, L"manifestE", L"versionE", manifestToVerify);
    VERIFY(!manifestExists, "manifestE:versionE still exists");

    imageBuilder->MockUnprovisionApplicationType(TestAppTypeName4, TestAppTypeVersion4);

    manifestExists = sharedState.TryGetServiceManifest(TestAppTypeName4, TestAppTypeVersion4, L"manifestD", L"versionD2", manifestToVerify);
    VERIFY(!manifestExists, "manifestD:versionD2 still exists");

    manifestExists = sharedState.TryGetServiceManifest(TestAppTypeName4, TestAppTypeVersion4, L"manifestE", L"versionE", manifestToVerify);
    VERIFY(!manifestExists, "manifestE:versionE still exists");

    imageBuilder->MockUnprovisionApplicationType(TestAppTypeName5, TestAppTypeVersion5);

    manifestExists = sharedState.TryGetServiceManifest(TestAppTypeName5, TestAppTypeVersion5, L"manifestF", L"versionF", manifestToVerify);
    VERIFY(!manifestExists, "manifestF:versionF still exists");

    manifestExists = sharedState.TryGetServiceManifest(TestAppTypeName6, TestAppTypeVersion6, L"manifestF2", L"versionF", manifestToVerify);
    VERIFY(manifestExists, "manifestF2:versionF does not exist");
    VERIFY(manifestToVerify.HasCodePackage(L"codeF", L"versionF"), "manifestF2:versionF missing code package");
    VERIFY(manifestToVerify.HasConfigPackage(L"configF", L"versionF"), "manifestF2:versionF missing config package");
    VERIFY(manifestToVerify.HasDataPackage(L"dataF", L"versionF"), "manifestF2:versionF missing data package");

    imageBuilder->MockUnprovisionApplicationType(TestAppTypeName6, TestAppTypeVersion6);

    manifestExists = sharedState.TryGetServiceManifest(TestAppTypeName6, TestAppTypeVersion6, L"manifestF2", L"versionF", manifestToVerify);
    VERIFY(manifestExists, "manifestF2:versionF does not exist");
    VERIFY(manifestToVerify.HasCodePackage(L"codeF", L"versionF"), "manifestF2:versionF missing code package");
    VERIFY(manifestToVerify.HasConfigPackage(L"configF", L"versionF"), "manifestF2:versionF missing config package");
    VERIFY(manifestToVerify.HasDataPackage(L"dataF", L"versionF"), "manifestF2:versionF missing data package");
    VERIFY(manifestToVerify.HasCodePackage(L"codeF2", L"versionF2"), "manifestF2:versionF missing code package 2");
    VERIFY(manifestToVerify.HasConfigPackage(L"configF2", L"versionF2"), "manifestF2:versionF missing config package 2");
    VERIFY(manifestToVerify.HasDataPackage(L"dataF2", L"versionF2"), "manifestF2:versionF missing data package 2");

    manifestExists = sharedState.TryGetServiceManifest(TestAppTypeName6, TestAppTypeVersion6, L"manifestF2b", L"versionFb", manifestToVerify);
    VERIFY(manifestExists, "manifestF2b:versionFb does not exist");
    VERIFY(manifestToVerify.HasCodePackage(L"codeFb", L"versionFb"), "manifestF2b:versionFb missing code package");
    VERIFY(manifestToVerify.HasConfigPackage(L"configFb", L"versionFb"), "manifestF2b:versionFb missing config package");
    VERIFY(manifestToVerify.HasDataPackage(L"dataFb", L"versionFb"), "manifestF2b:versionFb missing data package");
    VERIFY(manifestToVerify.HasCodePackage(L"codeF2b", L"versionF2b"), "manifestF2b:versionFb missing code package 2");
    VERIFY(manifestToVerify.HasConfigPackage(L"configF2b", L"versionF2b"), "manifestF2b:versionFb missing config package 2");
    VERIFY(manifestToVerify.HasDataPackage(L"dataF2b", L"versionF2b"), "manifestF2b:versionFb missing data package 2");

    imageBuilder->MockUnprovisionApplicationType(TestAppTypeName7, TestAppTypeVersion7);

    manifestExists = sharedState.TryGetServiceManifest(TestAppTypeName7, TestAppTypeVersion7, L"manifestF2", L"versionF2", manifestToVerify);
    VERIFY(!manifestExists, "manifestF2:versionF still exists");
    VERIFY(!manifestToVerify.HasCodePackage(L"codeF", L"versionF"), "manifestF2:versionF still has code package");
    VERIFY(!manifestToVerify.HasConfigPackage(L"configF", L"versionF"), "manifestF2:versionF still has config package");
    VERIFY(!manifestToVerify.HasDataPackage(L"dataF", L"versionF"), "manifestF2:versionF still has data package");
    VERIFY(!manifestToVerify.HasCodePackage(L"codeF2", L"versionF2"), "manifestF2:versionF still has code package 2");
    VERIFY(!manifestToVerify.HasConfigPackage(L"configF2", L"versionF2"), "manifestF2:versionF still has config package 2");
    VERIFY(!manifestToVerify.HasDataPackage(L"dataF2", L"versionF2"), "manifestF2:versionF still has data package 2");

    manifestExists = sharedState.TryGetServiceManifest(TestAppTypeName7, TestAppTypeVersion7, L"manifestF2b", L"versionF2b", manifestToVerify);
    VERIFY(!manifestExists, "manifestF2b:versionFb still exists");
    VERIFY(!manifestToVerify.HasCodePackage(L"codeFb", L"versionFb"), "manifestF2b:versionFb still has code package");
    VERIFY(!manifestToVerify.HasConfigPackage(L"configFb", L"versionFb"), "manifestF2b:versionFb still has config package");
    VERIFY(!manifestToVerify.HasDataPackage(L"dataFb", L"versionFb"), "manifestF2b:versionFb still has data package");
    VERIFY(!manifestToVerify.HasCodePackage(L"codeF2b", L"versionF2b"), "manifestF2b:versionFb still has code package 2");
    VERIFY(!manifestToVerify.HasConfigPackage(L"configF2b", L"versionF2b"), "manifestF2b:versionFb still has config package 2");
    VERIFY(!manifestToVerify.HasDataPackage(L"dataF2b", L"versionF2b"), "manifestF2b:versionFb still has data package 2");
}

// Test the wrapper class around ImageBuilder, which performs
// parsing of ServiceModel objects from XML. Covers the following:
//
// - Launching ImageBuilder EXE
// - Application manifest containing both required services and templates
//
BOOST_AUTO_TEST_CASE(ImageBuilderProxy1)
{
    NamingUri appName(L"proxy_test_1");

    wstring ApplicationManifestSourceFile(L"ClusterManager.SingleNode.Test.ApplicationManifest1.xml");
    wstring ServiceManifestSourceFile(L"ClusterManager.SingleNode.Test.ServiceManifest.xml");

    ServiceModelApplicationId appId;
    ServiceModelVersion expectedAppTypeVersion;
    vector<StoreDataServicePackage> expectedPackages;
    vector<StoreDataServiceTemplate> expectedTemplates;
    vector<PartitionedServiceDescriptor> expectedDefaultServices;

    SetupImageBuilderProxyTestHelper(
        appName,
        appId,
        expectedAppTypeVersion,
        expectedPackages,
        expectedTemplates,
        expectedDefaultServices);

    ImageBuilderProxyTestHelper(
        appName,
        appId,
        Directory::GetCurrentDirectory(),
        ApplicationManifestSourceFile,
        ServiceManifestSourceFile,
        expectedAppTypeVersion,
        expectedPackages,
        expectedTemplates,
        expectedDefaultServices);
}

// The same as ImageBuilderProxy1 but tests a manifest with no
// templates or required services
//
BOOST_AUTO_TEST_CASE(ImageBuilderProxy2)
{
    NamingUri appName(L"proxy_test_2");
    ServiceModelApplicationId appId(L"TestApplicationType_App0");

    wstring ApplicationManifestSourceFile(L"ClusterManager.SingleNode.Test.ApplicationManifest2.xml");
    wstring ServiceManifestSourceFile(L"ClusterManager.SingleNode.Test.ServiceManifest.xml");

    ServiceModelVersion expectedAppTypeVersion(L"InitialAppTypeVersion");

    // expected values are found in the source XML files

    vector<StoreDataServicePackage> expectedPackages;
    vector<StoreDataServiceTemplate> expectedTemplates;
    vector<PartitionedServiceDescriptor> expectedDefaultServices;

    expectedPackages.push_back(StoreDataServicePackage(
        appId,
        appName,
        ServiceModelTypeName(L"TestServiceType1"),
        ServiceModelPackageName(L"TestServices"),
        ServiceModelVersion(ServicePackageVersion().ToString()),
        ServiceModel::ServiceTypeDescription()));

    expectedPackages.push_back(StoreDataServicePackage(
        appId,
        appName,
        ServiceModelTypeName(L"TestServiceType2"),
        ServiceModelPackageName(L"TestServices"),
        ServiceModelVersion(ServicePackageVersion().ToString()),
        ServiceModel::ServiceTypeDescription()));

    expectedPackages.push_back(StoreDataServicePackage(
        appId,
        appName,
        ServiceModelTypeName(L"TestServiceType3"),
        ServiceModelPackageName(L"TestServices"),
        ServiceModelVersion(ServicePackageVersion().ToString()),
        ServiceModel::ServiceTypeDescription()));

    ImageBuilderProxyTestHelper(
        appName,
        appId,
        Directory::GetCurrentDirectory(),
        ApplicationManifestSourceFile,
        ServiceManifestSourceFile,
        expectedAppTypeVersion,
        expectedPackages,
        expectedTemplates,
        expectedDefaultServices);
}

// Tests ImageBuilderProxy on paths with spaces
//
BOOST_AUTO_TEST_CASE(ImageBuilderProxy3)
{
    NamingUri appName(L"proxy_test_3");
    ServiceModelApplicationId appId(L"TestApplicationType_App0");

    wstring ApplicationManifestSourceFile(L"ClusterManager.SingleNode.Test.ApplicationManifest2.xml");
    wstring ServiceManifestSourceFile(L"ClusterManager.SingleNode.Test.ServiceManifest.xml");

    ServiceModelVersion expectedAppTypeVersion(L"InitialAppTypeVersion");

    // expected values are found in the source XML files

    vector<StoreDataServicePackage> expectedPackages;
    vector<StoreDataServiceTemplate> expectedTemplates;
    vector<PartitionedServiceDescriptor> expectedDefaultServices;

    expectedPackages.push_back(StoreDataServicePackage(
        appId,
        appName,
        ServiceModelTypeName(L"TestServiceType1"),
        ServiceModelPackageName(L"TestServices"),
        ServiceModelVersion(ServicePackageVersion().ToString()),
        ServiceModel::ServiceTypeDescription()));

    expectedPackages.push_back(StoreDataServicePackage(
        appId,
        appName,
        ServiceModelTypeName(L"TestServiceType2"),
        ServiceModelPackageName(L"TestServices"),
        ServiceModelVersion(ServicePackageVersion().ToString()),
        ServiceModel::ServiceTypeDescription()));

    expectedPackages.push_back(StoreDataServicePackage(
        appId,
        appName,
        ServiceModelTypeName(L"TestServiceType3"),
        ServiceModelPackageName(L"TestServices"),
        ServiceModelVersion(ServicePackageVersion().ToString()),
        ServiceModel::ServiceTypeDescription()));

    wstring srcDirectory = Directory::GetCurrentDirectory();
	wstring bins;
    if (Environment::GetEnvironmentVariableW(L"_NTTREE", bins, Common::NOTHROW()))
    {
        srcDirectory = Path::Combine(bins, L"FabricUnitTests");
    }
	// RunCITs will end up setting current directory to FabricUnitTests\log
	else if (StringUtility::EndsWith<wstring>(srcDirectory, L"log"))
    {
        srcDirectory = Path::Combine(srcDirectory, L"..\\");
    }

    wstring baseDirectory = Path::Combine(Directory::GetCurrentDirectory(), L"Base Directory With Spaces");
    ResetDirectory(baseDirectory);

    CopyFile(
        Path::Combine(srcDirectory, L"ImageBuilder.exe"),
        Path::Combine(baseDirectory, L"ImageBuilder.exe"));

    CopyFile(
        Path::Combine(srcDirectory, L"ImageStoreClient.exe"),
        Path::Combine(baseDirectory, L"ImageStoreClient.exe"));

    CopyFile(
        Path::Combine(srcDirectory, L"System.Fabric.Management.dll"),
        Path::Combine(baseDirectory, L"System.Fabric.Management.dll"));

    CopyFile(
        Path::Combine(srcDirectory, L"System.Fabric.Management.ServiceModel.dll"),
        Path::Combine(baseDirectory, L"System.Fabric.Management.ServiceModel.dll"));

    CopyFile(
        Path::Combine(srcDirectory, L"System.Fabric.Management.ServiceModel.XmlSerializers.dll"),
        Path::Combine(baseDirectory, L"System.Fabric.Management.ServiceModel.XmlSerializers.dll"));

    CopyFile(
        Path::Combine(srcDirectory, L"System.Fabric.dll"),
        Path::Combine(baseDirectory, L"System.Fabric.dll"));

    CopyFile(
        Path::Combine(srcDirectory, L"System.Fabric.Strings.dll"),
        Path::Combine(baseDirectory, L"System.Fabric.Strings.dll"));

    CopyFile(
        Path::Combine(srcDirectory, L"FabricCommon.dll"),
        Path::Combine(baseDirectory, L"FabricCommon.dll"));

    CopyFile(
        Path::Combine(srcDirectory, L"ServiceFabricServiceModel.xsd"),
        Path::Combine(baseDirectory, L"ServiceFabricServiceModel.xsd"));

    ImageBuilderProxyTestHelper(
        appName,
        appId,
        baseDirectory,
        ApplicationManifestSourceFile,
        ServiceManifestSourceFile,
        expectedAppTypeVersion,
        expectedPackages,
        expectedTemplates,
        expectedDefaultServices);

    DeleteDirectory(baseDirectory);
}

BOOST_AUTO_TEST_CASE(ImageBuilderProxy4)
{
    NamingUri appName(L"proxy_test_4");
    ServiceModelApplicationId appId(L"TestApplicationType_App0");

    wstring ApplicationManifestSourceFile(L"ClusterManager.SingleNode.Test.ApplicationManifest3.xml");
    wstring ServiceManifestSourceFile(L"ClusterManager.SingleNode.Test.ServiceManifestWithServiceGroup.xml");

    ServiceModelVersion expectedAppTypeVersion(L"1.0");

    // expected values are found in the source XML files

    vector<StoreDataServicePackage> expectedPackages;
    vector<StoreDataServiceTemplate> expectedTemplates;
    vector<PartitionedServiceDescriptor> expectedDefaultServices;

    expectedPackages.push_back(StoreDataServicePackage(
        appId,
        appName,
        ServiceModelTypeName(L"TestServiceType1"),
        ServiceModelPackageName(L"TestServices"),
        ServiceModelVersion(ServicePackageVersion().ToString()),
        ServiceModel::ServiceTypeDescription()));

    expectedPackages.push_back(StoreDataServicePackage(
        appId,
        appName,
        ServiceModelTypeName(L"TestServiceType2"),
        ServiceModelPackageName(L"TestServices"),
        ServiceModelVersion(ServicePackageVersion().ToString()),
        ServiceModel::ServiceTypeDescription()));

    wstring serviceGroupTypeName1(L"TestServiceGroupType1");
    expectedPackages.push_back(StoreDataServicePackage(
        appId,
        appName,
        ServiceModelTypeName(serviceGroupTypeName1),
        ServiceModelPackageName(L"TestServices"),
        ServiceModelVersion(ServicePackageVersion().ToString()),
        ServiceModel::ServiceTypeDescription()));

    wstring serviceGroupTypeName2(L"TestServiceGroupType2");
    expectedPackages.push_back(StoreDataServicePackage(
        appId,
        appName,
        ServiceModelTypeName(serviceGroupTypeName2),
        ServiceModelPackageName(L"TestServices"),
        ServiceModelVersion(ServicePackageVersion().ToString()),
        ServiceModel::ServiceTypeDescription()));

    ServiceModelTypeName serviceGroupTypeName1Template(L"TestServiceGroupType1");
    ServiceDescription serviceDescription = ServiceDescription(
        L"",
        0,  // instance
        0,  // update version
        1,  // partition count
        8,  // replica set size
        3,  // min writeable
        true,  // is stateful
        false,  // has persisted
        TimeSpan::MinValue, // replica restart wait duration
        TimeSpan::MinValue, // quorum loss wait duration
        TimeSpan::MinValue, // stand by replica keep duration
        ServiceTypeIdentifier(ServicePackageIdentifier(), serviceGroupTypeName1Template.Value),
        vector<Reliability::ServiceCorrelationDescription>(),
        L"", // placement
        0,
        vector<Reliability::ServiceLoadMetricDescription>(),
        0,
        vector<byte>(),
        appName.ToString());
    serviceDescription.IsServiceGroup = true;
    CServiceGroupDescription cServiceGroupDescription;
    int serviceGroupMemberCount = 2;
    cServiceGroupDescription.ServiceGroupMemberData.resize(serviceGroupMemberCount);
    cServiceGroupDescription.ServiceGroupMemberData[0].ServiceName = L"";
    cServiceGroupDescription.ServiceGroupMemberData[0].ServiceType = L"MemberType1";
    cServiceGroupDescription.ServiceGroupMemberData[1].ServiceName = L"";
    cServiceGroupDescription.ServiceGroupMemberData[1].ServiceName = L"MemberType2";
    vector<byte> serializedInitializationData;
    Common::FabricSerializer::Serialize(&cServiceGroupDescription, serializedInitializationData);
    serviceDescription.put_InitializationData(std::move(serializedInitializationData));
    {
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            serviceDescription,
            0,
            0,
            psd).IsSuccess());

        expectedTemplates.push_back(StoreDataServiceTemplate(
            appId,
            appName,
            serviceGroupTypeName1Template,
            move(psd)));
    }

    wstring serviceName1(L"DefaultServiceGroup1");

    vector<Reliability::ServiceLoadMetricDescription> metrics1;
    metrics1.push_back(
        Reliability::ServiceLoadMetricDescription(
        L"M1",
        FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH,
        3 + 1, 30 + 10));
    metrics1.push_back(
        Reliability::ServiceLoadMetricDescription(
        L"M2",
        FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM,
        17, 170));

    PartitionedServiceDescriptor defaultServiceGroup1;
    VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
        ServiceDescription(
        NamingUri::Combine(appName, serviceName1).ToString(),
        0,  // instance
        0,  // update version
        1,  // partition count
        11,  // replica set size
        6,  // min writeable
        true,  // is stateful
        false,  // has persisted
        TimeSpan::MinValue, // replica restart wait duration
        TimeSpan::MinValue, // quorum loss wait duration
        TimeSpan::MinValue, // stand by replica keep duration
        ServiceTypeIdentifier(ServicePackageIdentifier(), serviceGroupTypeName1),
        vector<Reliability::ServiceCorrelationDescription>(),
        L"", // placement
        0, // scaleout count
        metrics1,
        400 + 2, // default move cost (sum of member move cost)
        vector<byte>(),
        appName.ToString()),
        defaultServiceGroup1).IsSuccess());
    defaultServiceGroup1.IsServiceGroup = true;
    expectedDefaultServices.push_back(move(defaultServiceGroup1));

    wstring serviceName2(L"DefaultServiceGroup2");

    vector<Reliability::ServiceLoadMetricDescription> metrics2;
    metrics2.push_back(
        Reliability::ServiceLoadMetricDescription(
        L"M1",
        FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM,
        55, 550));

    PartitionedServiceDescriptor defaultServiceGroup2;
    VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
        ServiceDescription(
        NamingUri::Combine(appName, serviceName2).ToString(),
        0,  // instance
        0,  // update version
        1,  // partition count
        4,  // replica set size
        2,  // min writeable
        true,  // is stateful
        false,  // has persisted
        TimeSpan::MinValue, // replica restart wait duration
        TimeSpan::MinValue, // quorum loss wait duration
        TimeSpan::MinValue, // stand by replica keep duration
        ServiceTypeIdentifier(ServicePackageIdentifier(), serviceGroupTypeName2),
        vector<Reliability::ServiceCorrelationDescription>(),
        L"", // placement
        0, // scale out count
        metrics2,
        11, // default move cost (specified on service)
        vector<byte>(),
        appName.ToString()),
        defaultServiceGroup2).IsSuccess());
    defaultServiceGroup2.IsServiceGroup = true;
    expectedDefaultServices.push_back(move(defaultServiceGroup2));

    ImageBuilderProxyTestHelper(
        appName,
        appId,
        Directory::GetCurrentDirectory(),
        ApplicationManifestSourceFile,
        ServiceManifestSourceFile,
        expectedAppTypeVersion,
        expectedPackages,
        expectedTemplates,
        expectedDefaultServices);
}

// The same as ImageBuilderProxy1 but with unicode application type name and type versions
//
BOOST_AUTO_TEST_CASE(ImageBuilderProxy5)
{
    NamingUri appName(L"proxy_test_5");
    ServiceModelApplicationId appId(L"úestApplicatiònTypé_App0");

    wstring ApplicationManifestSourceFile(L"ClusterManager.SingleNode.Test.ApplicationManifestUnicode.xml");
    wstring ServiceManifestSourceFile(L"ClusterManager.SingleNode.Test.ServiceManifest.xml");

    ServiceModelVersion expectedAppTypeVersion(L"ï.õ");

    // expected values are found in the source XML files

    vector<StoreDataServicePackage> expectedPackages;
    vector<StoreDataServiceTemplate> expectedTemplates;
    vector<PartitionedServiceDescriptor> expectedDefaultServices;

    expectedPackages.push_back(StoreDataServicePackage(
        appId,
        appName,
        ServiceModelTypeName(L"TestServiceType1"),
        ServiceModelPackageName(L"TestServices"),
        ServiceModelVersion(ServicePackageVersion().ToString()),
        ServiceModel::ServiceTypeDescription()));

    expectedPackages.push_back(StoreDataServicePackage(
        appId,
        appName,
        ServiceModelTypeName(L"TestServiceType2"),
        ServiceModelPackageName(L"TestServices"),
        ServiceModelVersion(ServicePackageVersion().ToString()),
        ServiceModel::ServiceTypeDescription()));

    expectedPackages.push_back(StoreDataServicePackage(
        appId,
        appName,
        ServiceModelTypeName(L"TestServiceType3"),
        ServiceModelPackageName(L"TestServices"),
        ServiceModelVersion(ServicePackageVersion().ToString()),
        ServiceModel::ServiceTypeDescription()));

    ServiceModelTypeName serviceTypeName1(L"TestServiceType1");
    {
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            ServiceDescription(
                L"",
                0,  // instance
                0,  // update version
                1,  // partition count
                3,  // replica set size
                1,  // min writeable
                false,  // is stateful
                false,  // has persisted
                TimeSpan::MinValue, // replica restart wait duration
                TimeSpan::MinValue, // quorum loss wait duration
                TimeSpan::MinValue, // stand by replica keep duration
                ServiceTypeIdentifier(ServicePackageIdentifier(), serviceTypeName1.Value),
                vector<Reliability::ServiceCorrelationDescription>(),
                L"", // placement
                0,
                vector<Reliability::ServiceLoadMetricDescription>(),
                0,
                vector<byte>(),
                appName.ToString()),
            0,
            0,
            psd).IsSuccess());
        
        expectedTemplates.push_back(StoreDataServiceTemplate(
            appId,
            appName,
            serviceTypeName1,
            move(psd)));
    }

    ServiceModelTypeName serviceTypeName2(L"TestServiceType2");
    {
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            ServiceDescription(
                L"",
                0,  // instance
                0,  // update version
                7,  // partition count
                5,  // replica set size
                3,  // min writeable
                true,  // is stateful
                false,  // has persisted
                TimeSpan::MinValue, // replica restart wait duration
                TimeSpan::MinValue, // quorum loss wait duration
                TimeSpan::MinValue, // stand by replica keep duration
                ServiceTypeIdentifier(ServicePackageIdentifier(), serviceTypeName2.Value),
                vector<Reliability::ServiceCorrelationDescription>(),
                L"", // placement
                0,
                vector<Reliability::ServiceLoadMetricDescription>(),
                0,
                vector<byte>(),
                appName.ToString()),
            13,
            20,
            psd).IsSuccess());

        expectedTemplates.push_back(StoreDataServiceTemplate(
            appId,
            appName,
            serviceTypeName2,
            move(psd)));
    }

    ServiceModelTypeName serviceTypeName3(L"TestServiceType3");
    vector<wstring> partitionNames;
    partitionNames.push_back(L"Partition1");
    partitionNames.push_back(L"Partition2");
    {
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            ServiceDescription(
                L"",
                0,  // instance
                0,  // update version
                2,  // partition count
                6,  // replica set size
                4,  // min writeable
                true,  // is stateful
                false,  // has persisted
                TimeSpan::MinValue, // replica restart wait duration
                TimeSpan::MinValue, // quorum loss wait duration
                TimeSpan::MinValue, // stand by replica keep duration
                ServiceTypeIdentifier(ServicePackageIdentifier(), serviceTypeName3.Value),
                vector<Reliability::ServiceCorrelationDescription>(),
                L"", // placement
                0,
                vector<Reliability::ServiceLoadMetricDescription>(),
                0,
                vector<byte>(),
                appName.ToString()),
            partitionNames,
            psd).IsSuccess());

        expectedTemplates.push_back(StoreDataServiceTemplate(
            appId,
            appName,
            serviceTypeName3,
            move(psd)));
    }

    wstring serviceName1(L"DefaultService1");
    {
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            ServiceDescription(
                NamingUri::Combine(appName, serviceName1).ToString(),
                0,  // instance
                0,  // update version
                1,  // partition count
                23,  // replica set size
                1,  // min writeable
                false,  // is stateful
                false,  // has persisted
                TimeSpan::MinValue, // replica restart wait duration
                TimeSpan::MinValue, // quorum loss wait duration
                TimeSpan::MinValue, // stand by replica keep duration
                ServiceTypeIdentifier(ServicePackageIdentifier(), serviceTypeName1.Value),
                vector<Reliability::ServiceCorrelationDescription>(),
                L"", // placement
                0,
                vector<Reliability::ServiceLoadMetricDescription>(),
                0,
                vector<byte>(),
                appName.ToString()),
            0,
            0,
            psd).IsSuccess());
        expectedDefaultServices.push_back(move(psd));
    }

    wstring serviceName2(L"DefaultService2");
    {
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            ServiceDescription(
                NamingUri::Combine(appName, serviceName2).ToString(),
                0,  // instance
                0,  // update version
                13,  // partition count
                11,  // replica set size
                6,  // min writeable
                true,  // is stateful
                false,  // has persisted
                TimeSpan::MinValue, // replica restart wait duration
                TimeSpan::MinValue, // quorum loss wait duration
                TimeSpan::MinValue, // stand by replica keep duration
                ServiceTypeIdentifier(ServicePackageIdentifier(), serviceTypeName2.Value),
                vector<Reliability::ServiceCorrelationDescription>(),
                L"", // placement
                0,
                vector<Reliability::ServiceLoadMetricDescription>(),
                0,
                vector<byte>(),
                appName.ToString()),
            17,
            30,
            psd).IsSuccess());
        expectedDefaultServices.push_back(move(psd));
    }

    wstring serviceName3(L"DefaultService3");
    vector<wstring> expectedPartitionNames;
    expectedPartitionNames.push_back(L"Partition1");
    expectedPartitionNames.push_back(L"Partition2");
    expectedPartitionNames.push_back(L"Partition3");
    {
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            ServiceDescription(
                NamingUri::Combine(appName, serviceName3).ToString(),
                0,  // instance
                0,  // update version
                3,  // partition count
                5,  // replica set size
                3,  // min writeable
                true,  // is stateful
                false,  // has persisted
                TimeSpan::MinValue, // replica restart wait duration
                TimeSpan::MinValue, // quorum loss wait duration
                TimeSpan::MinValue, // stand by replica keep duration
                ServiceTypeIdentifier(ServicePackageIdentifier(), serviceTypeName3.Value),
                vector<Reliability::ServiceCorrelationDescription>(),
                L"", // placement
                0,
                vector<Reliability::ServiceLoadMetricDescription>(),
                0,
                vector<byte>(),
                appName.ToString()),
            expectedPartitionNames,
            psd).IsSuccess());
        expectedDefaultServices.push_back(move(psd));
    }

    ImageBuilderProxyTestHelper(
        appName,
        appId,
        Directory::GetCurrentDirectory(),
        ApplicationManifestSourceFile,
        ServiceManifestSourceFile,
        expectedAppTypeVersion,
        expectedPackages,
        expectedTemplates,
        expectedDefaultServices);
}

BOOST_AUTO_TEST_CASE(ImageBuilderProxyTempDirectoryTest)
{
    int targetDirCount = 1000;

    wstring baseDirectory = Directory::GetCurrentDirectory();
    wstring ImageStoreRoot(Path::Combine(baseDirectory, L"CM.Test.ImageStoreRoot"));
    wstring OutputPath(Path::Combine(baseDirectory, L"CM.Test.ImageBuilderProxy"));

    Management::ManagementConfig::GetConfig().ImageStoreConnectionString = SecureString(ImageStoreRoot);

    auto imageBuilder = ImageBuilderProxy::Create(
        baseDirectory,
        OutputPath,
        L"TempDirectoryTest",
        Federation::NodeInstance());

    set<wstring> dirNames;
    ManualResetEvent event(false);
    atomic_long pendingCount(targetDirCount);

    for (auto ix = 0; ix<targetDirCount; ++ix)
    {
        auto tempDir = imageBuilder->Test_GetImageBuilderTempWorkingDirectory();
        auto error = Directory::Create2(tempDir);
        VERIFY(error.IsSuccess(), "Create {0}: error={1}", tempDir, error);

        dirNames.insert(tempDir);

        if (--pendingCount == 0)
        {
            event.Set();
        }
    }

    auto result = event.WaitOne(TimeSpan::FromSeconds(60));
    VERIFY(result, "WaitOne result={0}", result);

    VERIFY(dirNames.size() == targetDirCount, "Unique directory name count={0}", dirNames.size());

    // Should not archive
    //
    Management::ManagementConfig::GetConfig().DisableImageBuilderDirectoryArchives = false;
    auto dirIter = dirNames.begin();
    auto srcDir = *dirIter;
    auto destDir = Path::Combine(
        OutputPath,
        Path::Combine(
        ImageBuilderProxy::ArchiveDirectoryPrefix,
        Path::GetFileName(srcDir)));

    VERIFY(Directory::Exists(srcDir), "Target {0} should exist", srcDir);
    VERIFY(!Directory::Exists(destDir), "Destination {0} should not exist", destDir);

    imageBuilder->Test_DeleteOrArchiveDirectory(srcDir, ErrorCodeValue::Timeout);

    VERIFY(!Directory::Exists(srcDir), "Target {0} should not exist", srcDir);
    VERIFY(!Directory::Exists(destDir), "Destination {0} should not exist", destDir);

    // Should archive
    //
    Management::ManagementConfig::GetConfig().DisableImageBuilderDirectoryArchives = false;
    ++dirIter;
    srcDir = *dirIter;
    destDir = Path::Combine(
        OutputPath,
        Path::Combine(
        ImageBuilderProxy::ArchiveDirectoryPrefix,
        Path::GetFileName(srcDir)));

    VERIFY(Directory::Exists(srcDir), "Target {0} should exist", srcDir);
    VERIFY(!Directory::Exists(destDir), "Destination {0} should not exist", destDir);

    imageBuilder->Test_DeleteOrArchiveDirectory(srcDir, ErrorCodeValue::ImageBuilderValidationError);

    VERIFY(!Directory::Exists(srcDir), "Target {0} should not exist", srcDir);
    VERIFY(Directory::Exists(destDir), "Destination {0} should exist", destDir);

    // Disable archiving
    //
    Management::ManagementConfig::GetConfig().DisableImageBuilderDirectoryArchives = true;
    ++dirIter;
    srcDir = *dirIter;
    destDir = Path::Combine(
        OutputPath,
        Path::Combine(
        ImageBuilderProxy::ArchiveDirectoryPrefix,
        Path::GetFileName(srcDir)));

    VERIFY(Directory::Exists(srcDir), "Target {0} should exist", srcDir);
    VERIFY(!Directory::Exists(destDir), "Destination {0} should not exist", destDir);

    imageBuilder->Test_DeleteOrArchiveDirectory(srcDir, ErrorCodeValue::ImageBuilderValidationError);

    VERIFY(!Directory::Exists(srcDir), "Target {0} should not exist", srcDir);
    VERIFY(!Directory::Exists(destDir), "Destination {0} should not exist", destDir);


    DeleteDirectory(ImageStoreRoot);
}


BOOST_AUTO_TEST_CASE(ConfigClusterHealthPolicyTest)
{
    // Read the config cluster manifest and compare against expected value
    //
    // [HealthManager / ClusterHealthPolicy]
    // ApplicationTypeMaxPercentUnhealthyApplications - AppTypeA = 10
    // ApplicationTypeMaxPercentUnhealthyApplications - AppTypeB = 5
    // ApplicationTypeMaxPercentUnhealthyApplications - AppTypeC = 30
    // MaxPercentUnhealthyApplications = 10
    // MaxPercentUnhealthyNodes = 5

    ClusterHealthPolicy expected(false, 5, 10);
    VERIFY(expected.AddApplicationTypeHealthPolicy(L"AppTypeA", 10).IsSuccess(), "Add AppTypeA to app type health policy map");
    VERIFY(expected.AddApplicationTypeHealthPolicy(L"AppTypeB", 5).IsSuccess(), "Add AppTypeB to app type health policy map");
    VERIFY(expected.AddApplicationTypeHealthPolicy(L"AppTypeC", 30).IsSuccess(), "Add AppTypeC to app type health policy map");

    ClusterHealthPolicy configPolicy = Management::ManagementConfig::GetConfig().GetClusterHealthPolicy();
    VERIFY(expected == configPolicy, "Verify expected policy {0} against config policy {1}", expected.ToString(), configPolicy.ToString());

    // Check policy with conversion from/to native
    ScopedHeap heap;
    auto publicApiConfigPolicy = heap.AddItem<FABRIC_CLUSTER_HEALTH_POLICY>();
    configPolicy.ToPublicApi(heap, *publicApiConfigPolicy);
    VerifyClusterHealthPolicy(*publicApiConfigPolicy, expected);
}

BOOST_AUTO_TEST_CASE(ApplicationDescriptionWrapperUnitTest)
{
    Trace.WriteInfo(TestSource, "*** ApplicationDescriptionWrapperUnitTest");

    wstring appName = L"fabric:/ApplicationDescriptionWrapperUnitTest";
    wstring appType = L"MyType";
    wstring appVersion = L"MyVersion";

    wstring maxLengthValue;
    maxLengthValue.resize(ServiceModelConfig::GetConfig().MaxApplicationParameterLength - 1, 'x');

    map<wstring, wstring> parameters;
    parameters[L"Param1"] = L"Value1";
    parameters[L"Param2"] = L"Value2";
    parameters[L"Param3"] = L"Value3";
    parameters[L"Param4"] = maxLengthValue;

    ApplicationDescriptionWrapper wrapper(
        appName,
        appType,
        appVersion,
        parameters);

    ScopedHeap heap;
    auto * publicDescription = heap.AddItem<FABRIC_APPLICATION_DESCRIPTION>().GetRawPointer();
    publicDescription->ApplicationName = appName.c_str();
    publicDescription->ApplicationTypeName = appType.c_str();
    publicDescription->ApplicationTypeVersion = appVersion.c_str();
    publicDescription->ApplicationParameters = heap.AddItem<FABRIC_APPLICATION_PARAMETER_LIST>().GetRawPointer();

    auto * publicParamList = const_cast<FABRIC_APPLICATION_PARAMETER_LIST *>(publicDescription->ApplicationParameters);
    publicParamList->Count = static_cast<ULONG>(parameters.size());
    publicParamList->Items = heap.AddArray<FABRIC_APPLICATION_PARAMETER>(parameters.size()).GetRawArray();

    size_t index = 0;
    for (auto const & param : parameters)
    {
        publicParamList->Items[index].Name = param.first.c_str();
        publicParamList->Items[index].Value = param.second.c_str();
        ++index;
    }

    ApplicationDescriptionWrapper fromPublicWrapper;

    auto error = fromPublicWrapper.FromPublicApi(*publicDescription);
    VERIFY_IS_TRUE(error.IsSuccess());

    VERIFY_IS_TRUE(wrapper.ApplicationName == fromPublicWrapper.ApplicationName);
    VERIFY_IS_TRUE(wrapper.ApplicationTypeName == fromPublicWrapper.ApplicationTypeName);
    VERIFY_IS_TRUE(wrapper.ApplicationTypeVersion == fromPublicWrapper.ApplicationTypeVersion);
    VERIFY_IS_TRUE(wrapper.Parameters.size() == fromPublicWrapper.Parameters.size());

    auto wrapperIter = wrapper.Parameters.begin();
    auto fromPublicIter = fromPublicWrapper.Parameters.begin();
    while (wrapperIter != wrapper.Parameters.end() && fromPublicIter != fromPublicWrapper.Parameters.end())
    {
        VERIFY_IS_TRUE(wrapperIter->first == fromPublicIter->first);
        VERIFY_IS_TRUE(wrapperIter->second == fromPublicIter->second);

        ++wrapperIter;
        ++fromPublicIter;
    }
}

// Unit test To/FromPublicApi() functions on ApplicationUpgradeDescription class
//
BOOST_AUTO_TEST_CASE(ApplicationUpgradeDescriptionUnitTest)
{
    Trace.WriteInfo(TestSource, "*** ApplicationUpgradeDescriptionUnitTest: with health policy");
    ApplicationUpgradeDescriptionUnitTestHelper(true);

    Trace.WriteInfo(TestSource, "*** ApplicationUpgradeDescriptionUnitTest: no health policy");
    ApplicationUpgradeDescriptionUnitTestHelper(false);
}


// Unit test To/FromWrapper() functions on ApplicationUpgradeDescription class
//
BOOST_AUTO_TEST_CASE(ApplicationUpgradeDescriptionWrapperUnitTest)
{
    Trace.WriteInfo(TestSource, "*** ApplicationUpgradeDescriptionWrapperUnitTest: with health policy");
    ApplicationUpgradeDescriptionWrapperUnitTestHelper(true);

    Trace.WriteInfo(TestSource, "*** ApplicationUpgradeDescriptionWrapperUnitTest: no health policy");
    ApplicationUpgradeDescriptionWrapperUnitTestHelper(false);
}


// Unit test To/FromPublicApi() functions on FabricUpgradeDescription class
//
BOOST_AUTO_TEST_CASE(FabricUpgradeDescriptionUnitTest)
{
    Trace.WriteInfo(TestSource, "*** FabricUpgradeDescriptionUnitTest: with health policy");
    FabricUpgradeDescriptionUnitTestHelper(true);

    Trace.WriteInfo(TestSource, "*** FabricUpgradeDescriptionUnitTest: no health policy");
    FabricUpgradeDescriptionUnitTestHelper(false);
}


// Unit test To/FromWrapper() functions on FabricUpgradeDescription class
//
BOOST_AUTO_TEST_CASE(FabricUpgradeDescriptionWrapperUnitTest)
{
    Trace.WriteInfo(TestSource, "*** FabricUpgradeDescriptionWrapperUnitTest: with health policy");
    FabricUpgradeDescriptionWrapperUnitTestHelper(true);

    Trace.WriteInfo(TestSource, "*** FabricUpgradeDescriptionWrapperUnitTest: no health policy");
    FabricUpgradeDescriptionWrapperUnitTestHelper(false);
}


// Regression test to ensure that max DWORD does not cause overflow 
// during FromPublicAPI conversion of timeout values
//
BOOST_AUTO_TEST_CASE(UpgradeWrapperMaxTimeoutTest)
{
    Trace.WriteInfo(TestSource, "*** UpgradeWrapperMaxTimeoutTest: Application");
    UpgradeWrapperMaxTimeoutTestHelper<FABRIC_APPLICATION_UPGRADE_DESCRIPTION, ApplicationUpgradeDescription>();

    Trace.WriteInfo(TestSource, "*** UpgradeWrapperMaxTimeoutTest: Fabric");
    UpgradeWrapperMaxTimeoutTestHelper<FABRIC_UPGRADE_DESCRIPTION, FabricUpgradeDescription>();
}

BOOST_AUTO_TEST_CASE(ApplicationUpgradeUpdateDescriptionUnitTest)
{
    Trace.WriteInfo(TestSource, "*** ApplicationUpgradeUpdateDescriptionUnitTest");

    {
        FABRIC_APPLICATION_UPGRADE_UPDATE_DESCRIPTION publicDescription;
        memset(&publicDescription, 0, sizeof(publicDescription));

        wstring invalidName(L"invalidName");
        wstring validName(L"fabric:/validname");

        Trace.WriteWarning(TestSource, "Testing invalid name");
        {
            publicDescription.ApplicationName = invalidName.c_str();
            auto internalDescription = make_shared<ApplicationUpgradeUpdateDescription>();
            auto error = internalDescription->FromPublicApi(publicDescription);
            VERIFY(error.IsError(ErrorCodeValue::InvalidNameUri), "FromPublicAPI: error={0}", error);
        }

        Trace.WriteWarning(TestSource, "Testing invalid kind");
        {
            publicDescription.ApplicationName = validName.c_str();
            auto internalDescription = make_shared<ApplicationUpgradeUpdateDescription>();
            auto error = internalDescription->FromPublicApi(publicDescription);
            VERIFY(error.IsError(ErrorCodeValue::InvalidArgument), "FromPublicAPI: error={0}", error);
            VERIFY(
                StringUtility::StartsWith(error.Message, GET_RC(Invalid_Upgrade_Kind)),
                "FromPublicAPI: message={0}", error.Message);
        }

        Trace.WriteWarning(TestSource, "Testing invalid flags");
        {
            publicDescription.UpgradeKind = FABRIC_APPLICATION_UPGRADE_KIND_ROLLING;
            auto internalDescription = make_shared<ApplicationUpgradeUpdateDescription>();
            auto error = internalDescription->FromPublicApi(publicDescription);
            VERIFY(error.IsError(ErrorCodeValue::InvalidArgument), "FromPublicAPI: error={0}", error);
            VERIFY(
                StringUtility::StartsWith(error.Message, GET_RC(Invalid_Flags)),
                "FromPublicAPI: message={0}", error.Message);
        }

        Trace.WriteWarning(TestSource, "Testing invalid description");
        {
            publicDescription.UpdateFlags = FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_MODE;
            auto internalDescription = make_shared<ApplicationUpgradeUpdateDescription>();
            auto error = internalDescription->FromPublicApi(publicDescription);
            VERIFY(error.IsError(ErrorCodeValue::ArgumentNull), "FromPublicAPI: error={0}", error);
            VERIFY(
                StringUtility::StartsWith(error.Message, GET_RC(Invalid_Null_Pointer)),
                "FromPublicAPI: message={0}", error.Message);
        }
    }

    wstring name(L"fabric:/applicationupgradeupdatedescription/unittest");
    Random rand;
    map<DWORD, int> expectedValues;

    // Run through all possible non-zero flag combinations.
    // The last valid flag value is taken from IDL and must be updated
    // when new flags are added.
    //
    for (DWORD flags = 1;
        flags < (static_cast<DWORD>(FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_POLICY) << 1);
        ++flags)
    {
        Trace.WriteWarning(TestSource, "Testing Flags={0:X}", flags);

        auto updateDescription = CreateUpdateDescriptionFromPublic<FABRIC_APPLICATION_UPGRADE_UPDATE_DESCRIPTION, FABRIC_APPLICATION_HEALTH_POLICY, ApplicationUpgradeUpdateDescription>(name.c_str(), flags, rand, expectedValues);
        VerifyUpdateDescription(name, flags, updateDescription, expectedValues);

        VerifyJsonSerialization(updateDescription);
    }
}

BOOST_AUTO_TEST_CASE(FabricUpgradeUpdateDescriptionUnitTest)
{
    Trace.WriteInfo(TestSource, "*** FabricUpgradeUpdateDescriptionUnitTest");

    {
        FABRIC_UPGRADE_UPDATE_DESCRIPTION publicDescription;
        memset(&publicDescription, 0, sizeof(publicDescription));

        Trace.WriteWarning(TestSource, "Testing invalid kind");
        {
            auto internalDescription = make_shared<FabricUpgradeUpdateDescription>();
            auto error = internalDescription->FromPublicApi(publicDescription);
            VERIFY(error.IsError(ErrorCodeValue::InvalidArgument), "FromPublicAPI: error={0}", error);
            VERIFY(
                StringUtility::StartsWith(error.Message, GET_RC(Invalid_Upgrade_Kind)),
                "FromPublicAPI: message={0}", error.Message);
        }

        Trace.WriteWarning(TestSource, "Testing invalid flags");
        {
            publicDescription.UpgradeKind = FABRIC_UPGRADE_KIND_ROLLING;
            auto internalDescription = make_shared<FabricUpgradeUpdateDescription>();
            auto error = internalDescription->FromPublicApi(publicDescription);
            VERIFY(error.IsError(ErrorCodeValue::InvalidArgument), "FromPublicAPI: error={0}", error);
            VERIFY(
                StringUtility::StartsWith(error.Message, GET_RC(Invalid_Flags)),
                "FromPublicAPI: message={0}", error.Message);
        }

        Trace.WriteWarning(TestSource, "Testing invalid description");
        {
            publicDescription.UpdateFlags = FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_MODE;
            auto internalDescription = make_shared<FabricUpgradeUpdateDescription>();
            auto error = internalDescription->FromPublicApi(publicDescription);
            VERIFY(error.IsError(ErrorCodeValue::ArgumentNull), "FromPublicAPI: error={0}", error);
            VERIFY(
                StringUtility::StartsWith(error.Message, GET_RC(Invalid_Null_Pointer)),
                "FromPublicAPI: message={0}", error.Message);
        }
    }

    wstring name(L"fabric:/clusterupgradeupdatedescription/unittest");
    Random rand;
    map<DWORD, int> expectedValues;

    // Run through all possible non-zero flag combinations.
    // The last valid flag value is taken from IDL and must be updated
    // when new flags are added.
    //
    for (DWORD flags = 1;
        flags < (static_cast<DWORD>(FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_APPLICATION_HEALTH_POLICY_MAP) << 1);
        ++flags)
    {
        Trace.WriteWarning(TestSource, "Testing Flags={0:X}", flags);

        wstring unusedName;
        auto updateDescription = CreateUpdateDescriptionFromPublic<FABRIC_UPGRADE_UPDATE_DESCRIPTION, FABRIC_CLUSTER_HEALTH_POLICY, FabricUpgradeUpdateDescription>(unusedName.c_str(), flags, rand, expectedValues);
        VerifyUpdateDescription(unusedName, flags, updateDescription, expectedValues);

        VerifyJsonSerialization(updateDescription);
    }
}

BOOST_AUTO_TEST_SUITE_END()

void ClusterManagerTest::SetupImageBuilderProxyTestHelper(
    NamingUri const & appName,
    __out ServiceModelApplicationId & appId,
    __out ServiceModelVersion & expectedAppTypeVersion,
    __out vector<StoreDataServicePackage> & expectedPackages,
    __out vector<StoreDataServiceTemplate> & expectedTemplates,
    __out vector<PartitionedServiceDescriptor> & expectedDefaultServices)
{
    appId = ServiceModelApplicationId(L"TestApplicationType_App0");

    expectedAppTypeVersion = ServiceModelVersion(L"1.0");

    // expected values are found in the source XML files
    expectedPackages.push_back(StoreDataServicePackage(
        appId,
        appName,
        ServiceModelTypeName(L"TestServiceType1"),
        ServiceModelPackageName(L"TestServices"),
        ServiceModelVersion(ServicePackageVersion().ToString()),
        ServiceModel::ServiceTypeDescription()));

    expectedPackages.push_back(StoreDataServicePackage(
        appId,
        appName,
        ServiceModelTypeName(L"TestServiceType2"),
        ServiceModelPackageName(L"TestServices"),
        ServiceModelVersion(ServicePackageVersion().ToString()),
        ServiceModel::ServiceTypeDescription()));

    expectedPackages.push_back(StoreDataServicePackage(
        appId,
        appName,
        ServiceModelTypeName(L"TestServiceType3"),
        ServiceModelPackageName(L"TestServices"),
        ServiceModelVersion(ServicePackageVersion().ToString()),
        ServiceModel::ServiceTypeDescription()));

    ServiceModelTypeName serviceTypeName1(L"TestServiceType1");
    {
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            ServiceDescription(
                L"",
                0,  // instance
                0,  // update version
                1,  // partition count
                3,  // replica set size
                1,  // min writeable
                false,  // is stateful
                false,  // has persisted
                TimeSpan::MinValue, // replica restart wait duration
                TimeSpan::MinValue, // quorum loss wait duration
                TimeSpan::MinValue, // stand by replica keep duration
                ServiceTypeIdentifier(ServicePackageIdentifier(), serviceTypeName1.Value),
                vector<Reliability::ServiceCorrelationDescription>(),
                L"", // placement
                0,
                vector<Reliability::ServiceLoadMetricDescription>(),
                0,
                vector<byte>(),
                appName.ToString()),
            0,
            0,
            psd).IsSuccess());

        expectedTemplates.push_back(StoreDataServiceTemplate(
            appId,
            appName,
            serviceTypeName1,
            move(psd)));
    }

    ServiceModelTypeName serviceTypeName2(L"TestServiceType2");
    {
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            ServiceDescription(
                L"",
                0,  // instance
                0,  // update version
                7,  // partition count
                5,  // replica set size
                3,  // min writeable
                true,  // is stateful
                false,  // has persisted
                TimeSpan::MinValue, // replica restart wait duration
                TimeSpan::MinValue, // quorum loss wait duration
                TimeSpan::MinValue, // stand by replica keep duration
                ServiceTypeIdentifier(ServicePackageIdentifier(), serviceTypeName2.Value),
                vector<Reliability::ServiceCorrelationDescription>(),
                L"", // placement
                0,
                vector<Reliability::ServiceLoadMetricDescription>(),
                0,
                vector<byte>(),
                appName.ToString()),
            13,
            20,
            psd).IsSuccess());

        expectedTemplates.push_back(StoreDataServiceTemplate(
            appId,
            appName,
            serviceTypeName2,
            move(psd)));
    }

    ServiceModelTypeName serviceTypeName3(L"TestServiceType3");
    vector<wstring> partitionNames;
    partitionNames.push_back(L"Partition1");
    partitionNames.push_back(L"Partition2");
    {
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            ServiceDescription(
                L"",
                0,  // instance
                0,  // update version
                2,  // partition count
                6,  // replica set size
                4,  // min writeable
                true,  // is stateful
                false,  // has persisted
                TimeSpan::MinValue, // replica restart wait duration
                TimeSpan::MinValue, // quorum loss wait duration
                TimeSpan::MinValue, // stand by replica keep duration
                ServiceTypeIdentifier(ServicePackageIdentifier(), serviceTypeName3.Value),
                vector<Reliability::ServiceCorrelationDescription>(),
                L"", // placement
                0,
                vector<Reliability::ServiceLoadMetricDescription>(),
                0,
                vector<byte>(),
                appName.ToString()),
            partitionNames,
            psd).IsSuccess());

        expectedTemplates.push_back(StoreDataServiceTemplate(
            appId,
            appName,
            serviceTypeName3,
            move(psd)));
    }

    wstring serviceName1(L"DefaultService1");
    {
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            ServiceDescription(
                NamingUri::Combine(appName, serviceName1).ToString(),
                0,  // instance
                0,  // update version
                1,  // partition count
                23,  // replica set size
                1,  // min writeable
                false,  // is stateful
                false,  // has persisted
                TimeSpan::MinValue, // replica restart wait duration
                TimeSpan::MinValue, // quorum loss wait duration
                TimeSpan::MinValue, // stand by replica keep duration
                ServiceTypeIdentifier(ServicePackageIdentifier(), serviceTypeName1.Value),
                vector<Reliability::ServiceCorrelationDescription>(),
                L"", // placement
                0,
                vector<Reliability::ServiceLoadMetricDescription>(),
                0,
                vector<byte>(),
                appName.ToString()),
            0,
            0,
            psd).IsSuccess());

        expectedDefaultServices.push_back(psd);
    }

    wstring serviceName2(L"DefaultService2");
    {
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            ServiceDescription(
                NamingUri::Combine(appName, serviceName2).ToString(),
                0,  // instance
                0,  // update version
                13,  // partition count
                11,  // replica set size
                6,  // min writeable
                true,  // is stateful
                false,  // has persisted
                TimeSpan::MinValue, // replica restart wait duration
                TimeSpan::MinValue, // quorum loss wait duration
                TimeSpan::MinValue, // stand by replica keep duration
                ServiceTypeIdentifier(ServicePackageIdentifier(), serviceTypeName2.Value),
                vector<Reliability::ServiceCorrelationDescription>(),
                L"", // placement
                0,
                vector<Reliability::ServiceLoadMetricDescription>(),
                0,
                vector<byte>(),
                appName.ToString()),
            17,
            30,
            psd).IsSuccess());

        expectedDefaultServices.push_back(psd);
    }

    wstring serviceName3(L"DefaultService3");
    vector<wstring> expectedPartitionNames;
    expectedPartitionNames.push_back(L"Partition1");
    expectedPartitionNames.push_back(L"Partition2");
    expectedPartitionNames.push_back(L"Partition3");
    {
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(
            ServiceDescription(
                NamingUri::Combine(appName, serviceName3).ToString(),
                0,  // instance
                0,  // update version
                3,  // partition count
                5,  // replica set size
                3,  // min writeable
                true,  // is stateful
                false,  // has persisted
                TimeSpan::MinValue, // replica restart wait duration
                TimeSpan::MinValue, // quorum loss wait duration
                TimeSpan::MinValue, // stand by replica keep duration
                ServiceTypeIdentifier(ServicePackageIdentifier(), serviceTypeName3.Value),
                vector<Reliability::ServiceCorrelationDescription>(),
                L"", // placement
                0,
                vector<Reliability::ServiceLoadMetricDescription>(),
                0,
                vector<byte>(),
                appName.ToString()),
            expectedPartitionNames,
            psd).IsSuccess());

        expectedDefaultServices.push_back(psd);
    }
}

void ClusterManagerTest::ImageBuilderProxyForSfpkgTestHelper(
    NamingUri const & appName,
    ServiceModelApplicationId const & appId,
    ServiceModelVersion const & expectedAppTypeVersion,
    vector<StoreDataServicePackage> const & expectedPackages,
    vector<StoreDataServiceTemplate> const & expectedTemplates,
    vector<PartitionedServiceDescriptor> const & expectedDefaultServices)
{
    // Download the package from XStore.
    // The package was uploaded out of band at 
    // https://sftestresources.blob.core.windows.net:443/clustermanagertest/TestApp.sfpkg
    // NOTE: if you are changing the test package, make sure to re-upload it to xstore.

    wstring downloadPath = L"https://sftestresources.blob.core.windows.net:443/clustermanagertest/TestApp.sfpkg";
    
    Trace.WriteInfo(TestSource, "*** ImageBuilderProxyForSfpkgTestHelper");

    wstring srcDirectory = Directory::GetCurrentDirectory();
    wstring destDirectory = Directory::GetCurrentDirectory();
    
    wstring bins;
    if (Environment::GetEnvironmentVariableW(L"_NTTREE", bins, Common::NOTHROW()))
    {
        srcDirectory = Path::Combine(bins, L"FabricUnitTests");
    }
    // RunCITs will end up setting current directory to FabricUnitTests\log
    else if (StringUtility::EndsWith<wstring>(srcDirectory, L"log"))
    {
        srcDirectory = Path::Combine(srcDirectory, L"..\\");
    }

    wstring ImageStoreRoot(Path::Combine(destDirectory, L"CM.Test.ImageStoreRoot"));
    ResetDirectory(ImageStoreRoot);
    Management::ManagementConfig::GetConfig().ImageStoreConnectionString = SecureString(wformatString("file:{0}", ImageStoreRoot));

    wstring OutputPath(Path::Combine(destDirectory, L"CM.Test.ImageBuilderProxy"));
    DeleteDirectory(OutputPath);

    vector<wstring> tokens;
    StringUtility::Split<wstring>(appId.Value, tokens, L"_");
    ServiceModelTypeName expectedAppTypeName(tokens[0]);
    
    auto imageBuilder = ImageBuilderProxy::Create(
        srcDirectory,
        OutputPath,
        L"TestNodeName",
        Federation::NodeInstance());

    imageBuilder->Enable();

    int const MaxAttempts = 3;

    // Used to create a unique folder name for the package, containing the GUID
    ActivityId activityId;
    ErrorCode error;

    int retries = MaxAttempts;
    auto smallTimeout = TimeSpan::FromMilliseconds(100);
    auto imageBuilderTimeout = TimeSpan::FromSeconds(60);
        
   //
    // Build application type with Download
    // Use small timeout first, to test timeout error. This will clear the output directory.
    // Use large timeout for next tries, so the operation succeeds. Because the directory was deleted, the package must be downloaded again.
    //
    vector<ServiceModelServiceManifestDescription> manifests;
    retries = MaxAttempts;
    do
    {
        auto timeout = (retries == MaxAttempts) ? smallTimeout : imageBuilderTimeout;
        wstring applicationManifest;
        ApplicationHealthPolicy healthPolicy;
        map<wstring, wstring> defaultParamList;
        error = imageBuilder->Test_BuildApplicationType(
            activityId,
            wstring(),
            downloadPath,
            expectedAppTypeName,
            expectedAppTypeVersion,
            timeout,
            manifests,
            applicationManifest,
            healthPolicy,
            defaultParamList);
        Trace.WriteInfo(TestSource, "*** Test_BuildApplicationType with timeout {0}: {1}", timeout, error);
    } while (!error.IsSuccess() && --retries > 0);

    VERIFY(error.IsSuccess(), "Test_BuildApplicationType() failed with {0}", error);

    // verify resulting service manifests parsed from application manifest
    this->CheckManifests(manifests, expectedPackages);
   
    DigestedApplicationDescription appDescription;

    retries = MaxAttempts;
    do
    {
        Trace.WriteInfo(TestSource, "*** Test_BuildApplication");
        error = imageBuilder->Test_BuildApplication(
            appName,
            appId,
            expectedAppTypeName,
            expectedAppTypeVersion,
            map<wstring, wstring>(),
            imageBuilderTimeout,
            /*out*/ appDescription);
    } while (!error.IsSuccess() && --retries > 0);

    VERIFY(error.IsSuccess(), "BuildApplication() failed with {0}", error);

    // Verify resulting packages
    // Verify resulting packages
    this->CheckDigestedApplication(appDescription, expectedPackages, expectedTemplates, expectedDefaultServices);

    // clean-up

    error = imageBuilder->Test_CleanupApplication(expectedAppTypeName, appId, imageBuilderTimeout);
    VERIFY(error.IsSuccess(), "Failed to cleanup application with {0}", error);

    error = imageBuilder->CleanupApplicationType(expectedAppTypeName, expectedAppTypeVersion, manifests, true, imageBuilderTimeout);
    VERIFY(error.IsSuccess(), "Failed to cleanup application type with {0}", error);

    StoreLayoutSpecification storeLayout(ImageStoreRoot);
    wstring appTypeDirectory = storeLayout.GetApplicationTypeFolder(expectedAppTypeName.Value);
    VERIFY(!Directory::Exists(appTypeDirectory), "Application type directory '{0}' still exists", appTypeDirectory);

    wstring appInstanceDirectory = storeLayout.GetApplicationInstanceFolder(expectedAppTypeName.Value, appId.Value);
    VERIFY(!Directory::Exists(appInstanceDirectory), "Application instance directory '{0}' still exists", appInstanceDirectory);

    imageBuilder->Disable();

    DeleteDirectory(ImageStoreRoot);
}

void ClusterManagerTest::CheckManifests(
    vector<ServiceModelServiceManifestDescription> const & manifests,
    vector<StoreDataServicePackage> const & expectedPackages)
{
    vector<wstring> manifestNames;

    for (auto itExpected = expectedPackages.begin(); itExpected != expectedPackages.end(); ++itExpected)
    {
        vector<wstring> tmpNames;

        bool matched = false;
        for (auto itManifest = manifests.begin(); itManifest != manifests.end(); ++itManifest)
        {
            if (itManifest->Name == itExpected->PackageName.Value)
            {
                matched = true;
                break;
            }

            if (manifestNames.empty())
            {
                tmpNames.push_back(itManifest->Name);
            }
        }

        if (manifestNames.empty())
        {
            manifestNames.swap(tmpNames);
        }

        VERIFY(
            matched,
            "Expected package not found: {0} in {1}",
            itExpected->PackageName,
            manifestNames);
    }
}

void ClusterManagerTest::CheckDigestedApplication(
    DigestedApplicationDescription const & appDescription,
    vector<StoreDataServicePackage> const & expectedPackages,
    vector<StoreDataServiceTemplate> const & expectedTemplates,
    vector<PartitionedServiceDescriptor> const & expectedDefaultServices)
{
    vector<StoreDataServicePackage> const & packages = appDescription.ServicePackages;

    VERIFY(packages.size() == expectedPackages.size(), "Service packages mismatch {0} != {1}", packages.size(), expectedPackages.size());
    for (auto iter = expectedPackages.begin(); iter != expectedPackages.end(); ++iter)
    {
        bool packageFound = false;
        for (auto iter2 = packages.begin(); iter2 != packages.end(); ++iter2)
        {
            if (iter->PackageName == iter2->PackageName &&
                iter->PackageVersion == iter2->PackageVersion &&
                iter->TypeName == iter2->TypeName &&
                iter->ApplicationId == iter2->ApplicationId &&
                iter->ApplicationName == iter2->ApplicationName)
            {
                packageFound = true;
                break;
            }
        }

        VERIFY(
            packageFound,
            "Expected package not found: {0} packages = [{1}]",
            *iter,
            packages);
    }

    // Verify resulting templates

    vector<StoreDataServiceTemplate> const & templates = appDescription.ServiceTemplates;

    Trace.WriteNoise(TestSource, "Resulting templates: {0}", templates);
    for (auto iter = expectedTemplates.begin(); iter != expectedTemplates.end(); ++iter)
    {
        bool success = false;

        for (auto iter2 = templates.begin(); iter2 != templates.end(); ++iter2)
        {
            if (iter2->ApplicationId == iter->ApplicationId &&
                iter2->ApplicationName == iter->ApplicationName &&
                iter2->TypeName == iter->TypeName)
            {
                PartitionedServiceDescriptor const & psd1 = iter->PartitionedServiceDescriptor;
                PartitionedServiceDescriptor const & psd2 = iter2->PartitionedServiceDescriptor;

                success =
                    (psd1.Service.ApplicationName == psd2.Service.ApplicationName) &&
                    (psd1.Service.Type.ServiceTypeName == psd2.Service.Type.ServiceTypeName) &&
                    (psd1.Service.PartitionCount == psd2.Service.PartitionCount) &&
                    (psd1.Service.TargetReplicaSetSize == psd2.Service.TargetReplicaSetSize) &&
                    (psd1.Service.MinReplicaSetSize == psd2.Service.MinReplicaSetSize) &&
                    (psd1.Service.IsStateful == psd2.Service.IsStateful) &&
                    (psd1.Service.HasPersistedState == psd2.Service.HasPersistedState) &&
                    (psd1.Service.ReplicaRestartWaitDuration == psd2.Service.ReplicaRestartWaitDuration) &&
                    (psd1.Service.QuorumLossWaitDuration == psd2.Service.QuorumLossWaitDuration) &&
                    (psd1.Service.StandByReplicaKeepDuration == psd2.Service.StandByReplicaKeepDuration) &&
                    (psd1.Service.PlacementConstraints == psd2.Service.PlacementConstraints) &&
                    (psd1.Service.ScaleoutCount == psd2.Service.ScaleoutCount) &&
                    (psd1.LowRange == psd2.LowRange) &&
                    (psd1.HighRange == psd2.HighRange);
            }
        }

        VERIFY(success, "Expected template not found: {0}", *iter);
    }

    // Verify resulting required services

    vector<Naming::PartitionedServiceDescriptor> const & defaultServices = appDescription.DefaultServices;

    Trace.WriteNoise(TestSource, "Resulting required services: {0}", defaultServices);
    for (auto iter = expectedDefaultServices.begin(); iter != expectedDefaultServices.end(); ++iter)
    {
        bool success = false;

        for (auto iter2 = defaultServices.begin(); iter2 != defaultServices.end(); ++iter2)
        {
            if (iter->Service.Type.ServiceTypeName == iter2->Service.Type.ServiceTypeName)
            {
                VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(iter->Service.ApplicationName, iter2->Service.ApplicationName));
                VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(iter->Service.Name, iter2->Service.Name));
                VERIFY_IS_TRUE(iter->Service.PartitionCount == iter2->Service.PartitionCount);
                VERIFY_IS_TRUE(iter->Service.TargetReplicaSetSize == iter2->Service.TargetReplicaSetSize);
                VERIFY_IS_TRUE(iter->Service.MinReplicaSetSize == iter2->Service.MinReplicaSetSize);
                VERIFY_IS_TRUE(iter->Service.IsStateful == iter2->Service.IsStateful);
                VERIFY_IS_TRUE(iter->Service.HasPersistedState == iter2->Service.HasPersistedState);
                VERIFY_IS_TRUE(iter->Service.ReplicaRestartWaitDuration == iter2->Service.ReplicaRestartWaitDuration);
                VERIFY_IS_TRUE(iter->Service.QuorumLossWaitDuration == iter2->Service.QuorumLossWaitDuration);
                VERIFY_IS_TRUE(iter->Service.StandByReplicaKeepDuration == iter2->Service.StandByReplicaKeepDuration);
                VERIFY_IS_TRUE(iter->Service.PlacementConstraints == iter2->Service.PlacementConstraints);
                VERIFY_IS_TRUE(iter->Service.ScaleoutCount == iter2->Service.ScaleoutCount);
                VERIFY_IS_TRUE(iter->LowRange == iter2->LowRange);
                VERIFY_IS_TRUE(iter->HighRange == iter2->HighRange);
                VERIFY_IS_TRUE(iter->IsServiceGroup == iter2->IsServiceGroup);

                if (iter->IsServiceGroup)
                {
                    VERIFY_IS_TRUE(iter->Service.Metrics.size() == iter2->Service.Metrics.size());

                    for (auto expectedMetric = begin(iter->Service.Metrics); expectedMetric != end(iter->Service.Metrics); ++expectedMetric)
                    {
                        bool hasMetric = false;
                        for (auto actualMetric = begin(iter2->Service.Metrics); actualMetric != end(iter2->Service.Metrics); ++actualMetric)
                        {
                            if (actualMetric->Name == expectedMetric->Name)
                            {
                                VERIFY_IS_TRUE(expectedMetric->PrimaryDefaultLoad == actualMetric->PrimaryDefaultLoad);
                                VERIFY_IS_TRUE(expectedMetric->SecondaryDefaultLoad == actualMetric->SecondaryDefaultLoad);
                                VERIFY_IS_TRUE(expectedMetric->Weight == actualMetric->Weight);

                                hasMetric = true;
                            }
                        }

                        VERIFY(hasMetric, "Expected metric {0} not found", *expectedMetric);
                    }
                }

                success = true;
            }
        }

        VERIFY(success, "Expected required service not found: {0}", *iter);
    }
}

void ClusterManagerTest::ImageBuilderProxyTestHelper(
    NamingUri const & appName,
    ServiceModelApplicationId const & appId,
    wstring const & baseDirectory,
    wstring const & srcAppManifest,
    wstring const & srcServiceManifest,
    ServiceModelVersion const & expectedAppTypeVersion,
    vector<StoreDataServicePackage> const & expectedPackages,
    vector<StoreDataServiceTemplate> const & expectedTemplates,
    vector<PartitionedServiceDescriptor> const & expectedDefaultServices)
{
    auto error = ImageBuilderProxyTestHelper(
        appName,
        appId,
        baseDirectory,
        srcAppManifest,
        srcServiceManifest,
        map<wstring, wstring>(),
        false /*expectError*/,
        expectedAppTypeVersion,
        expectedPackages,
        expectedTemplates,
        expectedDefaultServices);
    VERIFY(error.IsSuccess(), "ImageBuilderProxyTestHelper() failed with {0}", error);
}

ErrorCode ClusterManagerTest::ImageBuilderProxyTestHelper(
    NamingUri const & appName,
    ServiceModelApplicationId const & appId,
    wstring const & baseDirectory,
    wstring const & srcAppManifest,
    wstring const & srcServiceManifest,
    map<wstring, wstring> const & appParams,
    bool expectError,
    ServiceModelVersion const & expectedAppTypeVersion,
    vector<StoreDataServicePackage> const & expectedPackages,
    vector<StoreDataServiceTemplate> const & expectedTemplates,
    vector<PartitionedServiceDescriptor> const & expectedDefaultServices)
{
    TimeSpan imageBuilderTimeout = TimeSpan::FromSeconds(30);
    int const MaxAttempts = 3;

    wstring TraceId(L"ImageBuilderProxy.CIT");

    wstring srcDirectory = Directory::GetCurrentDirectory();
    wstring destDirectory = baseDirectory;
	
	wstring bins;
    if (Environment::GetEnvironmentVariableW(L"_NTTREE", bins, Common::NOTHROW()))
    {
        srcDirectory = Path::Combine(bins, L"FabricUnitTests");
    }
	// RunCITs will end up setting current directory to FabricUnitTests\log
	else if (StringUtility::EndsWith<wstring>(srcDirectory, L"log"))
    {
        srcDirectory = Path::Combine(srcDirectory, L"..\\");
    }

    wstring ImageStoreRoot(Path::Combine(destDirectory, L"CM.Test.ImageStoreRoot"));
    wstring OutputPath(Path::Combine(destDirectory, L"CM.Test.ImageBuilderProxy"));

    wstring BuildPath(L"CM.Test.BuildPath");
    wstring BuildDirectory(Path::Combine(ImageStoreRoot, BuildPath));

    wstring ServiceManifestName(L"TestServices");

    vector<wstring> tokens;
    StringUtility::Split<wstring>(appId.Value, tokens, L"_");
    ServiceModelTypeName expectedAppTypeName(tokens[0]);

    DeleteDirectory(OutputPath);
    ResetDirectory(ImageStoreRoot);

    BuildLayoutSpecification buildLayout(BuildDirectory);

    CopyFile(
        Path::Combine(srcDirectory, srcAppManifest),
        buildLayout.GetApplicationManifestFile());

    CopyFile(
        Path::Combine(srcDirectory, srcServiceManifest),
        buildLayout.GetServiceManifestFile(ServiceManifestName));

    Directory::CreateDirectory(buildLayout.GetCodePackageFolder(ServiceManifestName, L"TestService.Code"));
    File::Copy(Path::Combine(srcDirectory, srcServiceManifest), Path::Combine(buildLayout.GetCodePackageFolder(ServiceManifestName, L"TestService.Code"), L"MyEntryPoint.exe"));

    Directory::CreateDirectory(buildLayout.GetConfigPackageFolder(ServiceManifestName, L"TestService.Config"));
    Directory::CreateDirectory(buildLayout.GetDataPackageFolder(ServiceManifestName, L"TestService.Data"));

    Management::ManagementConfig::GetConfig().ImageStoreConnectionString = SecureString(wformatString("file:{0}", ImageStoreRoot));

    auto imageBuilder = ImageBuilderProxy::Create(
        srcDirectory,
        OutputPath,
        L"TestNodeName",
        Federation::NodeInstance());

    imageBuilder->Enable();

    ErrorCode error;
    int retries = MaxAttempts;

    ServiceModelTypeName appTypeName;
    ServiceModelVersion appTypeVersion;

    do
    {
        error = imageBuilder->GetApplicationTypeInfo(BuildPath, imageBuilderTimeout, appTypeName, appTypeVersion);
    } while (!error.IsSuccess() && --retries > 0);

    // expected values come from the source XML files

    VERIFY(error.IsSuccess(), "GetApplicationTypeInfo() failed with {0}", error);
    VERIFY(appTypeName == expectedAppTypeName, "Application type name mismatch: {0} != {1}", appTypeName, expectedAppTypeName);
    VERIFY(appTypeVersion == expectedAppTypeVersion, "Application type version mismatch: {0} != {1}", appTypeVersion, expectedAppTypeVersion);

    vector<ServiceModelServiceManifestDescription> manifests;

    retries = MaxAttempts;
    do
    {
        wstring applicationManifest;
        ApplicationHealthPolicy healthPolicy;
        map<wstring, wstring> defaultParamList;
        error = imageBuilder->Test_BuildApplicationType(
            ActivityId(),
            BuildPath,
            wstring(),
            appTypeName,
            appTypeVersion,
            imageBuilderTimeout,
            manifests,
            applicationManifest,
            healthPolicy,
            defaultParamList);

        if (expectError && !error.IsSuccess())
        {
            // Since we are expecting an error at some point we should return that error code immediately 
            // instead of doing a retry and potentially losing the error
            return error;
        }

    } while (!error.IsSuccess() && --retries > 0);

    VERIFY(error.IsSuccess(), "Test_BuildApplicationType() failed with {0}", error);

    // verify resulting service manifests parsed from application manifest
    this->CheckManifests(manifests, expectedPackages);
    
    DigestedApplicationDescription appDescription;

    retries = MaxAttempts;
    do
    {
        error = imageBuilder->Test_BuildApplication(
            appName,
            appId,
            appTypeName,
            appTypeVersion,
            appParams,
            imageBuilderTimeout,
            /*out*/ appDescription);

        if (expectError && !error.IsSuccess())
        {
            // Since we are expecting an error at some point we should return that error code immediately 
            // instead of doing a retry and potentially losing the error
            return error;
        }

    } while (!error.IsSuccess() && --retries > 0);

    VERIFY(error.IsSuccess(), "BuildApplication() failed with {0}", error);

    // Verify resulting packages
    this->CheckDigestedApplication(appDescription, expectedPackages, expectedTemplates, expectedDefaultServices);

    // clean-up

    error = imageBuilder->Test_CleanupApplication(appTypeName, appId, imageBuilderTimeout);
    VERIFY(error.IsSuccess(), "Failed to cleanup application with {0}", error);

    error = imageBuilder->CleanupApplicationType(appTypeName, appTypeVersion, manifests, true, imageBuilderTimeout);
    VERIFY(error.IsSuccess(), "Failed to cleanup application type with {0}", error);

    StoreLayoutSpecification storeLayout(ImageStoreRoot);
    wstring appTypeDirectory = storeLayout.GetApplicationTypeFolder(appTypeName.Value);
    VERIFY(!Directory::Exists(appTypeDirectory), "Application type directory '{0}' still exists", appTypeDirectory);

    wstring appInstanceDirectory = storeLayout.GetApplicationInstanceFolder(appTypeName.Value, appId.Value);
    VERIFY(!Directory::Exists(appInstanceDirectory), "Application instance directory '{0}' still exists", appInstanceDirectory);

    imageBuilder->Disable();

    DeleteDirectory(ImageStoreRoot);

    return ErrorCode::Success();
}

void ClusterManagerTest::ApplicationUpgradeDescriptionUnitTestHelper(bool validHealthPolicy)
{
    wstring maxLengthValue;
    maxLengthValue.resize(ServiceModelConfig::GetConfig().MaxApplicationParameterLength - 1, 'x');

    map<wstring, wstring> parameters;
    parameters[L"Param1"] = L"Value1";
    parameters[L"Param2"] = L"Value2";
    parameters[L"Param3"] = L"Value3";
    parameters[L"Param4"] = maxLengthValue;

    UpgradeType::Enum expectedUpgradeType(UpgradeType::Rolling_ForceRestart);
    RollingUpgradeMode::Enum expectedUpgradeMode(RollingUpgradeMode::Monitored);
    TimeSpan expectedReplicaSetCheckTimeout(TimeSpan::FromSeconds(37));

    ServiceTypeHealthPolicyMap typePolicyMap;
    typePolicyMap[L"Type1"] = ServiceTypeHealthPolicy(1, 2, 3);
    typePolicyMap[L"Type2"] = ServiceTypeHealthPolicy(4, 5, 6);
    typePolicyMap[L"Type3"] = ServiceTypeHealthPolicy(7, 8, 9);

    ApplicationUpgradeDescription internalDescription(
        NamingUri(L"app1"),
        L"TargetVersion1",
        parameters,
        expectedUpgradeType,
        expectedUpgradeMode,
        expectedReplicaSetCheckTimeout,
        RollingUpgradeMonitoringPolicy(
        MonitoredUpgradeFailureAction::Rollback,
        TimeSpan::FromSeconds(42),
        TimeSpan::FromSeconds(24),
        TimeSpan::FromSeconds(732),
        TimeSpan::FromSeconds(237),
        TimeSpan::FromSeconds(1408)),
        ApplicationHealthPolicy(
        true,
        13,
        ServiceTypeHealthPolicy(14, 15, 16),
        typePolicyMap),
        validHealthPolicy);

    ScopedHeap heap;
    FABRIC_APPLICATION_UPGRADE_DESCRIPTION publicDescription = { 0 };
    internalDescription.ToPublicApi(heap, publicDescription);

    VERIFY(
        wstring(publicDescription.ApplicationName) == internalDescription.ApplicationName.ToString(),
        "Application name mismatch {0} != {1}",
        wstring(publicDescription.ApplicationName),
        internalDescription.ApplicationName);

    VERIFY(
        wstring(publicDescription.TargetApplicationTypeVersion) == internalDescription.TargetApplicationTypeVersion,
        "Target type version mismatch {0} != {1}",
        wstring(publicDescription.TargetApplicationTypeVersion),
        internalDescription.TargetApplicationTypeVersion);

    VERIFY(
        publicDescription.ApplicationParameters->Count == static_cast<ULONG>(internalDescription.ApplicationParameters.size()),
        "Application parameters mismatch {0} != {1}",
        publicDescription.ApplicationParameters->Count,
        internalDescription.ApplicationParameters.size());

    for (ULONG ix = 0; ix < publicDescription.ApplicationParameters->Count; ++ix)
    {
        wstring publicName(publicDescription.ApplicationParameters->Items[ix].Name);
        wstring publicValue(publicDescription.ApplicationParameters->Items[ix].Value);

        auto iter = internalDescription.ApplicationParameters.find(publicName);
        if (iter != internalDescription.ApplicationParameters.end())
        {
            wstring internalName = iter->first;
            wstring internalValue = iter->second;

            VERIFY(
                publicName == internalName && publicValue == internalValue,
                "Parameter mismatch [{0}, {1}] != [{2}, {3}]",
                publicName,
                publicValue,
                internalName,
                internalValue);
        }
    }

    VERIFY(
        publicDescription.UpgradeKind == FABRIC_APPLICATION_UPGRADE_KIND_ROLLING,
        "UpgradeType mismatch {0} != {1}",
        static_cast<int>(publicDescription.UpgradeKind),
        expectedUpgradeType);

    VERIFY(
        publicDescription.UpgradePolicyDescription != NULL,
        "UpgradePolicyDescription is NULL");

    auto policyDescription = reinterpret_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION *>(publicDescription.UpgradePolicyDescription);

    VERIFY(
        policyDescription->RollingUpgradeMode == FABRIC_ROLLING_UPGRADE_MODE_MONITORED,
        "RollingUpgradeMode mismatch {0} != {1}",
        static_cast<int>(policyDescription->RollingUpgradeMode),
        expectedUpgradeMode);

    VERIFY(
        policyDescription->ForceRestart == TRUE,
        "ForceRestart mismatch {0} != {1}",
        policyDescription->ForceRestart,
        true);

    VERIFY(
        policyDescription->UpgradeReplicaSetCheckTimeoutInSeconds ==
        static_cast<DWORD>(expectedReplicaSetCheckTimeout.TotalSeconds()),
        "ReplicaSetCheckTimeout mismatch {0} != {1}",
        policyDescription->UpgradeReplicaSetCheckTimeoutInSeconds,
        expectedReplicaSetCheckTimeout.TotalSeconds());

    auto policyDescriptionEx = reinterpret_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1 *>(policyDescription->Reserved);

    VERIFY(
        policyDescriptionEx->MonitoringPolicy != NULL,
        "Monitoring Policy missing");

    auto monitoringPolicy = policyDescriptionEx->MonitoringPolicy;
    auto monitoringPolicyEx1 = static_cast<FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_EX1*>(monitoringPolicy->Reserved);

    VERIFY(
        MonitoredUpgradeFailureAction::FromPublicApi(monitoringPolicy->FailureAction) == internalDescription.MonitoringPolicy.FailureAction,
        "Failure Action mismatch {0} != {1}",
        MonitoredUpgradeFailureAction::FromPublicApi(monitoringPolicy->FailureAction),
        internalDescription.MonitoringPolicy.FailureAction);

    VERIFY(
        monitoringPolicy->HealthCheckWaitDurationInSeconds == internalDescription.MonitoringPolicy.HealthCheckWaitDuration.TotalSeconds(),
        "Health Check Wait mismatch {0} != {1}",
        monitoringPolicy->HealthCheckWaitDurationInSeconds,
        internalDescription.MonitoringPolicy.HealthCheckWaitDuration);

    VERIFY(
        monitoringPolicyEx1->HealthCheckStableDurationInSeconds == internalDescription.MonitoringPolicy.HealthCheckStableDuration.TotalSeconds(),
        "Health Check Stable mismatch {0} != {1}",
        monitoringPolicyEx1->HealthCheckStableDurationInSeconds,
        internalDescription.MonitoringPolicy.HealthCheckStableDuration);

    VERIFY(
        monitoringPolicy->HealthCheckRetryTimeoutInSeconds == internalDescription.MonitoringPolicy.HealthCheckRetryTimeout.TotalSeconds(),
        "Health Check Retry mismatch {0} != {1}",
        monitoringPolicy->HealthCheckRetryTimeoutInSeconds,
        internalDescription.MonitoringPolicy.HealthCheckRetryTimeout);

    VERIFY(
        monitoringPolicy->UpgradeTimeoutInSeconds == internalDescription.MonitoringPolicy.UpgradeTimeout.TotalSeconds(),
        "Upgrade Timeout mismatch {0} != {1}",
        monitoringPolicy->UpgradeTimeoutInSeconds,
        internalDescription.MonitoringPolicy.UpgradeTimeout);

    VERIFY(
        monitoringPolicy->UpgradeDomainTimeoutInSeconds == internalDescription.MonitoringPolicy.UpgradeDomainTimeout.TotalSeconds(),
        "Upgrade Domain Timeout mismatch {0} != {1}",
        monitoringPolicy->UpgradeDomainTimeoutInSeconds,
        internalDescription.MonitoringPolicy.UpgradeDomainTimeout);

    if (validHealthPolicy)
    {
        VERIFY(
            policyDescriptionEx->HealthPolicy != NULL,
            "Health Policy missing");

        auto healthPolicy = reinterpret_cast<FABRIC_APPLICATION_HEALTH_POLICY *>(policyDescriptionEx->HealthPolicy);

        VERIFY(
            healthPolicy->ConsiderWarningAsError == (internalDescription.HealthPolicy.ConsiderWarningAsError ? TRUE : FALSE),
            "Health Policy ConsiderWarningAsError mismatch {0} != {1}",
            healthPolicy->ConsiderWarningAsError,
            internalDescription.HealthPolicy.ConsiderWarningAsError);

        VERIFY(
            healthPolicy->MaxPercentUnhealthyDeployedApplications == internalDescription.HealthPolicy.MaxPercentUnhealthyDeployedApplications,
            "Health Policy MaxPercentUnhealthyDeployedApplications mismatch {0} != {1}",
            healthPolicy->MaxPercentUnhealthyDeployedApplications,
            internalDescription.HealthPolicy.MaxPercentUnhealthyDeployedApplications);

        VERIFY(
            healthPolicy->DefaultServiceTypeHealthPolicy != NULL,
            "Service Type Health Policy missing");

        auto defaultServiceTypePolicy = reinterpret_cast<FABRIC_SERVICE_TYPE_HEALTH_POLICY const *>(healthPolicy->DefaultServiceTypeHealthPolicy);

        VERIFY(
            defaultServiceTypePolicy->MaxPercentUnhealthyServices == internalDescription.HealthPolicy.GetServiceTypeHealthPolicy(L"").MaxPercentUnhealthyServices,
            "Service Type Health Policy MaxPercentUnhealthyServices mismatch {0} != {1}",
            defaultServiceTypePolicy->MaxPercentUnhealthyServices,
            internalDescription.HealthPolicy.GetServiceTypeHealthPolicy(L"").MaxPercentUnhealthyServices);

        VERIFY(
            defaultServiceTypePolicy->MaxPercentUnhealthyPartitionsPerService == internalDescription.HealthPolicy.GetServiceTypeHealthPolicy(L"").MaxPercentUnhealthyPartitionsPerService,
            "Service Type Health Policy MaxPercentUnhealthyPartitionsPerService mismatch {0} != {1}",
            defaultServiceTypePolicy->MaxPercentUnhealthyPartitionsPerService,
            internalDescription.HealthPolicy.GetServiceTypeHealthPolicy(L"").MaxPercentUnhealthyPartitionsPerService);

        VERIFY(
            defaultServiceTypePolicy->MaxPercentUnhealthyReplicasPerPartition == internalDescription.HealthPolicy.GetServiceTypeHealthPolicy(L"").MaxPercentUnhealthyReplicasPerPartition,
            "Service Type Health Policy MaxPercentUnhealthyReplicasPerPartition mismatch {0} != {1}",
            defaultServiceTypePolicy->MaxPercentUnhealthyReplicasPerPartition,
            internalDescription.HealthPolicy.GetServiceTypeHealthPolicy(L"").MaxPercentUnhealthyReplicasPerPartition);

        VERIFY(
            healthPolicy->ServiceTypeHealthPolicyMap != NULL,
            "Service Type Health Policy Map missing");

        auto serviceTypePolicyMap = reinterpret_cast<FABRIC_SERVICE_TYPE_HEALTH_POLICY_MAP const *>(healthPolicy->ServiceTypeHealthPolicyMap);

        VERIFY(
            serviceTypePolicyMap->Count == static_cast<ULONG>(typePolicyMap.size()),
            "Service Type Health Policy Map mismatch {0} != {1}",
            serviceTypePolicyMap->Count,
            typePolicyMap.size());

        for (ULONG ix = 0; ix<serviceTypePolicyMap->Count; ++ix)
        {
            FABRIC_SERVICE_TYPE_HEALTH_POLICY_MAP_ITEM const & it = serviceTypePolicyMap->Items[ix];
            FABRIC_SERVICE_TYPE_HEALTH_POLICY const * serviceTypePolicy = it.ServiceTypeHealthPolicy;

            auto it2 = internalDescription.HealthPolicy.GetServiceTypeHealthPolicy(wstring(it.ServiceTypeName));

            VERIFY(
                serviceTypePolicy->MaxPercentUnhealthyServices == it2.MaxPercentUnhealthyServices,
                "Service Type Health Policy MaxPercentUnhealthyServices mismatch {0} != {1}",
                serviceTypePolicy->MaxPercentUnhealthyServices,
                it2.MaxPercentUnhealthyServices);

            VERIFY(
                serviceTypePolicy->MaxPercentUnhealthyPartitionsPerService == it2.MaxPercentUnhealthyPartitionsPerService,
                "Service Type Health Policy MaxPercentUnhealthyPartitionsPerService mismatch {0} != {1}",
                serviceTypePolicy->MaxPercentUnhealthyPartitionsPerService,
                it2.MaxPercentUnhealthyPartitionsPerService);

            VERIFY(
                serviceTypePolicy->MaxPercentUnhealthyReplicasPerPartition == it2.MaxPercentUnhealthyReplicasPerPartition,
                "Service Type Health Policy MaxPercentUnhealthyReplicasPerPartition mismatch {0} != {1}",
                serviceTypePolicy->MaxPercentUnhealthyReplicasPerPartition,
                it2.MaxPercentUnhealthyReplicasPerPartition);
        }
    }
    else
    {
        VERIFY(
            policyDescriptionEx->HealthPolicy == NULL,
            "Health Policy unexpected");
    }

    ApplicationUpgradeDescription rebuiltDescription;
    auto error = rebuiltDescription.FromPublicApi(publicDescription);
    VERIFY(error.IsSuccess(), "FromPublicApi() failed with {0}", error);

    VERIFY(
        internalDescription.MonitoringPolicy == rebuiltDescription.MonitoringPolicy,
        "Rebuilt MonitoringPolicy mismatch {0} != {1}",
        internalDescription.MonitoringPolicy,
        rebuiltDescription.MonitoringPolicy);

    VERIFY(
        rebuiltDescription.IsHealthPolicyValid == validHealthPolicy,
        "Rebuilt HealthPolicy mismatch {0} != {1}",
        rebuiltDescription.IsHealthPolicyValid,
        validHealthPolicy);

    VERIFY(
        internalDescription.IsHealthPolicyValid == rebuiltDescription.IsHealthPolicyValid,
        "Rebuilt IsHealthPolicyValid mismatch {0} != {1}",
        internalDescription.IsHealthPolicyValid,
        rebuiltDescription.IsHealthPolicyValid);

    if (validHealthPolicy)
    {
        VERIFY(
            internalDescription == rebuiltDescription,
            "Rebuilt description mismatch {0} != {1}",
            internalDescription,
            rebuiltDescription);
    }
    else
    {
        VERIFY(
            internalDescription.EqualsIgnoreHealthPolicies(rebuiltDescription),
            "Rebuilt description mismatch {0} != {1}",
            internalDescription,
            rebuiltDescription);
    }

    bool expectedIsMonitored(true);
    VERIFY(
        rebuiltDescription.IsInternalMonitored == expectedIsMonitored,
        "Rebuilt description IsInternalMonitored mismatch {0} != {1}",
        rebuiltDescription.IsInternalMonitored,
        expectedIsMonitored);

    bool expectedUnmonitoredManual(false);
    VERIFY(
        rebuiltDescription.IsUnmonitoredManual == expectedUnmonitoredManual,
        "Rebuilt description IsUnmonitoredManual mismatch {0} != {1}",
        rebuiltDescription.IsUnmonitoredManual,
        expectedUnmonitoredManual);

    bool expectedHealthMonitored(true);
    VERIFY(
        rebuiltDescription.IsHealthMonitored == expectedHealthMonitored,
        "Rebuilt description IsHealthMonitored mismatch {0} != {1}",
        rebuiltDescription.IsHealthMonitored,
        expectedHealthMonitored);
}

void ClusterManagerTest::ApplicationUpgradeDescriptionWrapperUnitTestHelper(bool validHealthPolicy)
{
    wstring maxLengthValue;
    maxLengthValue.resize(ServiceModelConfig::GetConfig().MaxApplicationParameterLength, 'x');

    map<wstring, wstring> parameters;
    parameters[L"Param1"] = L"Value1";
    parameters[L"Param2"] = L"Value2";
    parameters[L"Param3"] = L"Value3";
    parameters[L"Param4"] = maxLengthValue;

    UpgradeType::Enum expectedUpgradeType(UpgradeType::Rolling_ForceRestart);
    RollingUpgradeMode::Enum expectedUpgradeMode(RollingUpgradeMode::Monitored);
    TimeSpan expectedReplicaSetCheckTimeout(TimeSpan::FromSeconds(37));

    ServiceTypeHealthPolicyMap typePolicyMap;
    typePolicyMap[L"Type1"] = ServiceTypeHealthPolicy(1, 2, 3);
    typePolicyMap[L"Type2"] = ServiceTypeHealthPolicy(4, 5, 6);
    typePolicyMap[L"Type3"] = ServiceTypeHealthPolicy(7, 8, 9);

    ApplicationUpgradeDescription internalDescription(
        NamingUri(L"app1"),
        L"TargetVersion1",
        parameters,
        expectedUpgradeType,
        expectedUpgradeMode,
        expectedReplicaSetCheckTimeout,
        RollingUpgradeMonitoringPolicy(
        MonitoredUpgradeFailureAction::Rollback,
        TimeSpan::FromSeconds(42),
        TimeSpan::FromSeconds(24),
        TimeSpan::FromSeconds(732),
        TimeSpan::FromSeconds(237),
        TimeSpan::FromSeconds(1408)),
        ApplicationHealthPolicy(
        true,
        13,
        ServiceTypeHealthPolicy(14, 15, 16),
        typePolicyMap),
        validHealthPolicy);

    ServiceModel::ApplicationUpgradeDescriptionWrapper wrapper;
    internalDescription.ToWrapper(wrapper);

    VERIFY(
        wrapper.ApplicationName == internalDescription.ApplicationName.ToString(),
        "Application name mismatch {0} != {1}",
        wrapper.ApplicationName,
        internalDescription.ApplicationName);

    VERIFY(
        wrapper.TargetApplicationTypeVersion == internalDescription.TargetApplicationTypeVersion,
        "Target type version mismatch {0} != {1}",
        wrapper.TargetApplicationTypeVersion,
        internalDescription.TargetApplicationTypeVersion);

    VERIFY(
        wrapper.Parameters.size() == internalDescription.ApplicationParameters.size(),
        "Application parameters size mismatch {0} != {1}",
        wrapper.Parameters.size(),
        internalDescription.ApplicationParameters.size());

    VERIFY(
        wrapper.Parameters == internalDescription.ApplicationParameters,
        "Application parameters mismatch");

    VERIFY(
        wrapper.UpgradeKind == ServiceModel::UpgradeType::Rolling,
        "UpgradeType mismatch {0} != {1}",
        static_cast<int>(wrapper.UpgradeKind),
        ServiceModel::UpgradeType::Rolling);

    VERIFY(
        wrapper.MonitoringPolicy == internalDescription.MonitoringPolicy,
        "Monitoring Policy mismatch {0} != {1}",
        wrapper.MonitoringPolicy,
        internalDescription.MonitoringPolicy);

    VERIFY(
        wrapper.UpgradeMode == internalDescription.RollingUpgradeMode,
        "RollingUpgradeMode mismatch {0} != {1}",
        wrapper.UpgradeMode,
        internalDescription.RollingUpgradeMode);

    VERIFY(
        wrapper.ForceRestart == true,
        "ForceRestart mismatch {0} != {1}",
        wrapper.ForceRestart,
        true);

    VERIFY(
        wrapper.ReplicaSetTimeoutInSec == internalDescription.ReplicaSetCheckTimeout.TotalSeconds(),
        "ReplicaSetCheckTimeout mismatch {0} != {1}",
        wrapper.ReplicaSetTimeoutInSec,
        internalDescription.ReplicaSetCheckTimeout);

    if (validHealthPolicy)
    {
        VERIFY(
            wrapper.HealthPolicy != nullptr,
            "HealthPolicy missing");

        VERIFY(
            *wrapper.HealthPolicy == internalDescription.HealthPolicy,
            "Health Policy mismatch {0} != {1}",
            *wrapper.HealthPolicy,
            internalDescription.HealthPolicy);
    }
    else
    {
        VERIFY(
            wrapper.HealthPolicy == nullptr,
            "HealthPolicy unexpected");
    }

    wstring json;
    JsonHelper::Serialize(wrapper, json);

    Trace.WriteInfo(TestSource, "Serialized JSON = '{0}'", json);

    if (validHealthPolicy)
    {
        VERIFY(StringUtility::Contains<wstring>(json, wstring(ServiceModel::Constants::ApplicationHealthPolicy.begin())), "Serialized JSON = '{0}'", json);
    }
    else
    {
        VERIFY(!StringUtility::Contains<wstring>(json, wstring(ServiceModel::Constants::ApplicationHealthPolicy.begin())), "Serialized JSON = '{0}'", json);
    }

    ApplicationUpgradeDescriptionWrapper rebuiltWrapper;
    JsonHelper::Deserialize(rebuiltWrapper, json);

    if (validHealthPolicy)
    {
        VERIFY(
            wrapper.HealthPolicy != nullptr,
            "HealthPolicy missing");

        VERIFY(
            *wrapper.HealthPolicy == internalDescription.HealthPolicy,
            "Health Policy mismatch {0} != {1}",
            *wrapper.HealthPolicy,
            internalDescription.HealthPolicy);
    }
    else
    {
        VERIFY(
            wrapper.HealthPolicy == nullptr,
            "HealthPolicy unexpected");
    }

    ApplicationUpgradeDescription rebuiltDescription;
    auto error = rebuiltDescription.FromWrapper(wrapper);
    VERIFY(error.IsSuccess(), "FromWrapper() failed with {0}", error);

    VERIFY(
        rebuiltDescription.IsHealthPolicyValid == validHealthPolicy,
        "Rebuilt IsHealthPolicyValid mismatch {0} != {1}",
        rebuiltDescription.IsHealthPolicyValid,
        validHealthPolicy);

    if (validHealthPolicy)
    {
        VERIFY(
            internalDescription == rebuiltDescription,
            "Rebuilt description mismatch {0} != {1}",
            internalDescription,
            rebuiltDescription);
    }
    else
    {
        VERIFY(
            internalDescription.EqualsIgnoreHealthPolicies(rebuiltDescription),
            "Rebuilt description mismatch {0} != {1}",
            internalDescription,
            rebuiltDescription);

        VERIFY(
            internalDescription.IsHealthPolicyValid == rebuiltDescription.IsHealthPolicyValid,
            "Rebuilt IsHealthPolicyValid mismatch {0} != {1}",
            internalDescription.IsHealthPolicyValid,
            rebuiltDescription.IsHealthPolicyValid);
    }
}

void ClusterManagerTest::FabricUpgradeDescriptionUnitTestHelper(bool validHealthPolicy)
{
    FabricConfigVersion configVersion;
    auto error = configVersion.FromString(L"config");
    VERIFY(error.IsSuccess(), "Invalid FabricConfigVersion");

    FabricVersion expectedVersion(FabricCodeVersion(1, 2, 3, 0), configVersion);
    UpgradeType::Enum expectedUpgradeType(UpgradeType::Rolling_ForceRestart);
    RollingUpgradeMode::Enum expectedUpgradeMode(RollingUpgradeMode::Monitored);
    TimeSpan expectedReplicaSetCheckTimeout(TimeSpan::FromSeconds(37));

    auto applicationHealthPolicies = make_shared<ApplicationHealthPolicyMap>();
    applicationHealthPolicies->Insert(L"fabric:/TestApp", ApplicationHealthPolicy(false, 10, ServiceTypeHealthPolicy(10, 10, 10), ServiceTypeHealthPolicyMap()));
    applicationHealthPolicies->Insert(L"fabric:/TestApp2", ApplicationHealthPolicy(true, 0, ServiceTypeHealthPolicy(), ServiceTypeHealthPolicyMap()));

    FabricUpgradeDescription internalDescription(
        expectedVersion,
        expectedUpgradeType,
        expectedUpgradeMode,
        expectedReplicaSetCheckTimeout,
        RollingUpgradeMonitoringPolicy(
        MonitoredUpgradeFailureAction::Rollback,
        TimeSpan::FromSeconds(42),
        TimeSpan::FromSeconds(24),
        TimeSpan::FromSeconds(732),
        TimeSpan::FromSeconds(237),
        TimeSpan::FromSeconds(1408)),
        ClusterHealthPolicy(true, 12, 23),
        validHealthPolicy,
        true, // enable delta health evaluation
        ClusterUpgradeHealthPolicy(13, 23),
        validHealthPolicy,
        applicationHealthPolicies);

    ScopedHeap heap;
    FABRIC_UPGRADE_DESCRIPTION publicDescription = { 0 };
    internalDescription.ToPublicApi(heap, publicDescription);

    VERIFY(
        wstring(publicDescription.CodeVersion) == internalDescription.Version.CodeVersion.ToString(),
        "Code Version mismatch {0} != {1}",
        wstring(publicDescription.CodeVersion),
        internalDescription.Version.CodeVersion);

    VERIFY(
        wstring(publicDescription.ConfigVersion) == internalDescription.Version.ConfigVersion.ToString(),
        "Config Version mismatch {0} != {1}",
        wstring(publicDescription.ConfigVersion),
        internalDescription.Version.ConfigVersion);

    VERIFY(
        publicDescription.UpgradeKind == FABRIC_APPLICATION_UPGRADE_KIND_ROLLING,
        "UpgradeType mismatch {0} != {1}",
        static_cast<int>(publicDescription.UpgradeKind),
        expectedUpgradeType);

    VERIFY(
        publicDescription.UpgradePolicyDescription != NULL,
        "UpgradePolicyDescription is NULL");

    auto policyDescription = reinterpret_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION *>(publicDescription.UpgradePolicyDescription);

    VERIFY(
        policyDescription->RollingUpgradeMode == FABRIC_ROLLING_UPGRADE_MODE_MONITORED,
        "RollingUpgradeMode mismatch {0} != {1}",
        static_cast<int>(policyDescription->RollingUpgradeMode),
        expectedUpgradeMode);

    VERIFY(
        policyDescription->ForceRestart == TRUE,
        "ForceRestart mismatch {0} != {1}",
        policyDescription->ForceRestart,
        true);

    VERIFY(
        policyDescription->UpgradeReplicaSetCheckTimeoutInSeconds ==
        static_cast<DWORD>(expectedReplicaSetCheckTimeout.TotalSeconds()),
        "ReplicaSetCheckTimeout mismatch {0} != {1}",
        policyDescription->UpgradeReplicaSetCheckTimeoutInSeconds,
        expectedReplicaSetCheckTimeout.TotalSeconds());

    auto policyDescriptionEx = reinterpret_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1 *>(policyDescription->Reserved);

    VERIFY(
        policyDescriptionEx->MonitoringPolicy != NULL,
        "Monitoring Policy missing");

    auto monitoringPolicy = policyDescriptionEx->MonitoringPolicy;
    auto monitoringPolicyEx1 = static_cast<FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_EX1*>(monitoringPolicy->Reserved);

    VERIFY(
        MonitoredUpgradeFailureAction::FromPublicApi(monitoringPolicy->FailureAction) == internalDescription.MonitoringPolicy.FailureAction,
        "Failure Action mismatch {0} != {1}",
        MonitoredUpgradeFailureAction::FromPublicApi(monitoringPolicy->FailureAction),
        internalDescription.MonitoringPolicy.FailureAction);

    VERIFY(
        monitoringPolicy->HealthCheckWaitDurationInSeconds == internalDescription.MonitoringPolicy.HealthCheckWaitDuration.TotalSeconds(),
        "Health Check Wait mismatch {0} != {1}",
        monitoringPolicy->HealthCheckWaitDurationInSeconds,
        internalDescription.MonitoringPolicy.HealthCheckWaitDuration);

    VERIFY(
        monitoringPolicyEx1->HealthCheckStableDurationInSeconds == internalDescription.MonitoringPolicy.HealthCheckStableDuration.TotalSeconds(),
        "Health Check Stable mismatch {0} != {1}",
        monitoringPolicyEx1->HealthCheckStableDurationInSeconds,
        internalDescription.MonitoringPolicy.HealthCheckStableDuration);


    VERIFY(
        monitoringPolicy->HealthCheckRetryTimeoutInSeconds == internalDescription.MonitoringPolicy.HealthCheckRetryTimeout.TotalSeconds(),
        "Health Check Retry mismatch {0} != {1}",
        monitoringPolicy->HealthCheckRetryTimeoutInSeconds,
        internalDescription.MonitoringPolicy.HealthCheckRetryTimeout);

    VERIFY(
        monitoringPolicy->UpgradeTimeoutInSeconds == internalDescription.MonitoringPolicy.UpgradeTimeout.TotalSeconds(),
        "Upgrade Timeout mismatch {0} != {1}",
        monitoringPolicy->UpgradeTimeoutInSeconds,
        internalDescription.MonitoringPolicy.UpgradeTimeout);

    VERIFY(
        monitoringPolicy->UpgradeDomainTimeoutInSeconds == internalDescription.MonitoringPolicy.UpgradeDomainTimeout.TotalSeconds(),
        "Upgrade Domain Timeout mismatch {0} != {1}",
        monitoringPolicy->UpgradeDomainTimeoutInSeconds,
        internalDescription.MonitoringPolicy.UpgradeDomainTimeout);

    auto policyDescriptionEx2 = reinterpret_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX2 *>(policyDescriptionEx->Reserved);

    VERIFY(
        policyDescriptionEx2->EnableDeltaHealthEvaluation == (internalDescription.EnableDeltaHealthEvaluation ? TRUE : FALSE),
        "EnableDeltaHealthEvaluation mismatch {0} != {1}",
        policyDescriptionEx2->EnableDeltaHealthEvaluation,
        internalDescription.EnableDeltaHealthEvaluation);

    if (validHealthPolicy)
    {
        VERIFY(
            policyDescriptionEx->HealthPolicy != NULL,
            "Health Policy missing");

        auto healthPolicy = reinterpret_cast<FABRIC_CLUSTER_HEALTH_POLICY *>(policyDescriptionEx->HealthPolicy);

        VerifyClusterHealthPolicy(*healthPolicy, internalDescription.HealthPolicy);
        
        auto upgradeHealthPolicy = reinterpret_cast<FABRIC_CLUSTER_UPGRADE_HEALTH_POLICY *>(policyDescriptionEx2->UpgradeHealthPolicy);

        VERIFY(
            upgradeHealthPolicy->MaxPercentDeltaUnhealthyNodes == internalDescription.UpgradeHealthPolicy.MaxPercentDeltaUnhealthyNodes,
            "Health Policy MaxPercentDeltaUnhealthyNodes mismatch {0} != {1}",
            upgradeHealthPolicy->MaxPercentDeltaUnhealthyNodes,
            internalDescription.UpgradeHealthPolicy.MaxPercentDeltaUnhealthyNodes);

        VERIFY(
            upgradeHealthPolicy->MaxPercentUpgradeDomainDeltaUnhealthyNodes == internalDescription.UpgradeHealthPolicy.MaxPercentUpgradeDomainDeltaUnhealthyNodes,
            "Health Policy MaxPercentUpgradeDomainDeltaUnhealthyNodes mismatch {0} != {1}",
            upgradeHealthPolicy->MaxPercentUpgradeDomainDeltaUnhealthyNodes,
            internalDescription.UpgradeHealthPolicy.MaxPercentDeltaUnhealthyNodes);
    }
    else
    {
        VERIFY(
            policyDescriptionEx->HealthPolicy == NULL,
            "Health Policy unexpected");

        VERIFY(
            policyDescriptionEx2->UpgradeHealthPolicy == NULL,
            "Upgrade Health Policy unexpected");
    }

    auto policyDescriptionEx3 = reinterpret_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX3 *>(policyDescriptionEx2->Reserved);
    auto publicApplicationHealthPolicyMap = policyDescriptionEx3->ApplicationHealthPolicyMap;
    VERIFY(
        publicApplicationHealthPolicyMap != NULL,
        "ApplicationHealthPolicyMap missing");

    VERIFY(
        publicApplicationHealthPolicyMap->Count == static_cast<ULONG>(applicationHealthPolicies->size()),
        "publicApplicationHealthPolicyMap item mismatch, expected {0}, actual {1}",
        applicationHealthPolicies->size(),
        publicApplicationHealthPolicyMap->Count);

    FabricUpgradeDescription rebuiltDescription;
    error = rebuiltDescription.FromPublicApi(publicDescription);
    VERIFY(error.IsSuccess(), "FromPublicApi() failed with {0}", error);

    VERIFY(
        internalDescription.MonitoringPolicy == rebuiltDescription.MonitoringPolicy,
        "Rebuilt MonitoringPolicy mismatch {0} != {1}",
        internalDescription.MonitoringPolicy,
        rebuiltDescription.MonitoringPolicy);

    VERIFY(
        rebuiltDescription.IsHealthPolicyValid == validHealthPolicy,
        "Rebuilt HealthPolicy mismatch {0} != {1}",
        rebuiltDescription.IsHealthPolicyValid,
        validHealthPolicy);

    VERIFY(
        internalDescription.IsHealthPolicyValid == rebuiltDescription.IsHealthPolicyValid,
        "Rebuilt IsHealthPolicyValid mismatch {0} != {1}",
        internalDescription.IsHealthPolicyValid,
        rebuiltDescription.IsHealthPolicyValid);

    if (validHealthPolicy)
    {
        VERIFY(
            internalDescription == rebuiltDescription,
            "Rebuilt description mismatch {0} != {1}",
            internalDescription,
            rebuiltDescription);
    }
    else
    {
        VERIFY(
            internalDescription.EqualsIgnoreHealthPolicies(rebuiltDescription),
            "Rebuilt description mismatch {0} != {1}",
            internalDescription,
            rebuiltDescription);
    }

    bool expectedIsMonitored(true);
    VERIFY(
        rebuiltDescription.IsInternalMonitored == expectedIsMonitored,
        "Rebuilt description IsInternalMonitored mismatch {0} != {1}",
        rebuiltDescription.IsInternalMonitored,
        expectedIsMonitored);

    bool expectedUnmonitoredManual(false);
    VERIFY(
        rebuiltDescription.IsUnmonitoredManual == expectedUnmonitoredManual,
        "Rebuilt description IsUnmonitoredManual mismatch {0} != {1}",
        rebuiltDescription.IsUnmonitoredManual,
        expectedUnmonitoredManual);

    bool expectedHealthMonitored(true);
    VERIFY(
        rebuiltDescription.IsHealthMonitored == expectedHealthMonitored,
        "Rebuilt description IsHealthMonitored mismatch {0} != {1}",
        rebuiltDescription.IsHealthMonitored,
        expectedHealthMonitored);
}

void ClusterManagerTest::FabricUpgradeDescriptionWrapperUnitTestHelper(bool validHealthPolicy)
{
    FabricConfigVersion configVersion;
    auto error = configVersion.FromString(L"config");
    VERIFY(error.IsSuccess(), "Invalid FabricConfigVersion");

    FabricVersion expectedVersion(FabricCodeVersion(1, 2, 3, 0), configVersion);
    UpgradeType::Enum expectedUpgradeType(UpgradeType::Rolling_ForceRestart);
    RollingUpgradeMode::Enum expectedUpgradeMode(RollingUpgradeMode::Monitored);
    TimeSpan expectedReplicaSetCheckTimeout(TimeSpan::FromSeconds(37));

    ServiceTypeHealthPolicyMap typePolicyMap;
    typePolicyMap[L"Type1"] = ServiceTypeHealthPolicy(1, 2, 3);
    typePolicyMap[L"Type2"] = ServiceTypeHealthPolicy(4, 5, 6);
    typePolicyMap[L"Type3"] = ServiceTypeHealthPolicy(7, 8, 9);

    auto applicationHealthPolicies = make_shared<ApplicationHealthPolicyMap>();
    applicationHealthPolicies->Insert(L"fabric:/TestApp", ApplicationHealthPolicy(false, 10, ServiceTypeHealthPolicy(10, 10, 10), typePolicyMap));

    FabricUpgradeDescription internalDescription(
        expectedVersion,
        expectedUpgradeType,
        expectedUpgradeMode,
        expectedReplicaSetCheckTimeout,
        RollingUpgradeMonitoringPolicy(
        MonitoredUpgradeFailureAction::Rollback,
        TimeSpan::FromSeconds(42),
        TimeSpan::FromSeconds(24),
        TimeSpan::FromSeconds(732),
        TimeSpan::FromSeconds(237),
        TimeSpan::FromSeconds(1408)),
        ClusterHealthPolicy(true, 12, 23),
        validHealthPolicy,
        true, // enable delta health evaluation
        ClusterUpgradeHealthPolicy(13, 16),
        validHealthPolicy,
        applicationHealthPolicies);

    ServiceModel::FabricUpgradeDescriptionWrapper wrapper;
    internalDescription.ToWrapper(wrapper);

    VERIFY(
        wrapper.CodeVersion == internalDescription.Version.CodeVersion.ToString(),
        "Code Version mismatch {0} != {1}",
        wrapper.CodeVersion,
        internalDescription.Version.CodeVersion.ToString());

    VERIFY(
        wrapper.ConfigVersion == internalDescription.Version.ConfigVersion.ToString(),
        "Config Version mismatch {0} != {1}",
        wrapper.ConfigVersion,
        internalDescription.Version.ConfigVersion.ToString());

    VERIFY(
        wrapper.UpgradeKind == ServiceModel::UpgradeType::Rolling,
        "UpgradeType mismatch {0} != {1}",
        static_cast<int>(wrapper.UpgradeKind),
        ServiceModel::UpgradeType::Rolling);

    VERIFY(
        wrapper.MonitoringPolicy == internalDescription.MonitoringPolicy,
        "Monitoring Policy mismatch {0} != {1}",
        wrapper.MonitoringPolicy,
        internalDescription.MonitoringPolicy);

    VERIFY(
        wrapper.UpgradeMode == FABRIC_ROLLING_UPGRADE_MODE_MONITORED,
        "RollingUpgradeMode mismatch {0} != {1}",
        static_cast<int>(wrapper.UpgradeMode),
        expectedUpgradeMode);

    VERIFY(
        wrapper.ForceRestart == true,
        "ForceRestart mismatch {0} != {1}",
        wrapper.ForceRestart,
        true);

    VERIFY(
        wrapper.ReplicaSetTimeoutInSec == internalDescription.ReplicaSetCheckTimeout.TotalSeconds(),
        "ReplicaSetCheckTimeout mismatch {0} != {1}",
        wrapper.ReplicaSetTimeoutInSec,
        internalDescription.ReplicaSetCheckTimeout);

    VERIFY(
        wrapper.EnableDeltaHealthEvaluation == internalDescription.EnableDeltaHealthEvaluation,
        "EnableDeltaHealthEvaluation mismatch {0} != {1}",
        wrapper.EnableDeltaHealthEvaluation,
        internalDescription.EnableDeltaHealthEvaluation);

    VerifyApplicationHealthPoliciesEqual(wrapper.ApplicationHealthPolicies, internalDescription.ApplicationHealthPolicies);

    if (validHealthPolicy)
    {
        VERIFY(
            wrapper.HealthPolicy != nullptr,
            "HealthPolicy missing");

        VERIFY(
            *wrapper.HealthPolicy == internalDescription.HealthPolicy,
            "Health Policy mismatch {0} != {1}",
            *wrapper.HealthPolicy,
            internalDescription.HealthPolicy);

        VERIFY(
            wrapper.UpgradeHealthPolicy != nullptr,
            "UpgradeHealthPolicy missing");

        VERIFY(
            *wrapper.UpgradeHealthPolicy == internalDescription.UpgradeHealthPolicy,
            "Upgrade Health Policy mismatch {0} != {1}",
            *wrapper.UpgradeHealthPolicy,
            internalDescription.UpgradeHealthPolicy);
    }
    else
    {
        VERIFY(
            wrapper.HealthPolicy == nullptr,
            "HealthPolicy unexpected");

        VERIFY(
            wrapper.UpgradeHealthPolicy == nullptr,
            "UpgradeHealthPolicy unexpected");
    }

    wstring json;
    JsonHelper::Serialize(wrapper, json);

    Trace.WriteInfo(TestSource, "Serialized JSON = '{0}'", json);

    if (validHealthPolicy)
    {
        VERIFY(StringUtility::Contains<wstring>(json, wstring(ServiceModel::Constants::ClusterHealthPolicy.begin())), "Serialized JSON = '{0}'", json);
        VERIFY(StringUtility::Contains<wstring>(json, wstring(ServiceModel::Constants::ClusterUpgradeHealthPolicy.begin())), "Serialized JSON = '{0}'", json);
    }
    else
    {
        VERIFY(!StringUtility::Contains<wstring>(json, wstring(ServiceModel::Constants::ClusterHealthPolicy.begin())), "Serialized JSON = '{0}'", json);
        VERIFY(!StringUtility::Contains<wstring>(json, wstring(ServiceModel::Constants::ClusterUpgradeHealthPolicy.begin())), "Serialized JSON = '{0}'", json);
    }

    FabricUpgradeDescriptionWrapper rebuiltWrapper;
    JsonHelper::Deserialize(rebuiltWrapper, json);

    if (validHealthPolicy)
    {
        VERIFY(
            wrapper.HealthPolicy != nullptr,
            "HealthPolicy missing");

        VERIFY(
            *wrapper.HealthPolicy == internalDescription.HealthPolicy,
            "Health Policy mismatch {0} != {1}",
            *wrapper.HealthPolicy,
            internalDescription.HealthPolicy);

        VERIFY(
            wrapper.UpgradeHealthPolicy != nullptr,
            "UpgradeHealthPolicy missing");

        VERIFY(
            *wrapper.UpgradeHealthPolicy == internalDescription.UpgradeHealthPolicy,
            "Upgrade Health Policy mismatch {0} != {1}",
            *wrapper.UpgradeHealthPolicy,
            internalDescription.UpgradeHealthPolicy);
    }
    else
    {
        VERIFY(
            wrapper.HealthPolicy == nullptr,
            "HealthPolicy unexpected");

        VERIFY(
            wrapper.UpgradeHealthPolicy == nullptr,
            "UpgradeHealthPolicy unexpected");
    }

    FabricUpgradeDescription rebuiltDescription;
    error = rebuiltDescription.FromWrapper(wrapper);
    VERIFY(error.IsSuccess(), "FromWrapper() failed with {0}", error);

    VERIFY(
        rebuiltDescription.IsHealthPolicyValid == validHealthPolicy,
        "Rebuilt IsHealthPolicyValid mismatch {0} != {1}",
        rebuiltDescription.IsHealthPolicyValid,
        validHealthPolicy);

    VERIFY(
        rebuiltDescription.IsUpgradeHealthPolicyValid == validHealthPolicy,
        "Rebuilt IsUpgradeHealthPolicyValid mismatch {0} != {1}",
        rebuiltDescription.IsUpgradeHealthPolicyValid,
        validHealthPolicy);

    if (validHealthPolicy)
    {
        VERIFY(
            internalDescription == rebuiltDescription,
            "Rebuilt description mismatch {0} != {1}",
            internalDescription,
            rebuiltDescription);
    }
    else
    {
        VERIFY(
            internalDescription.EqualsIgnoreHealthPolicies(rebuiltDescription),
            "Rebuilt description mismatch {0} != {1}",
            internalDescription,
            rebuiltDescription);

        VERIFY(
            internalDescription.IsHealthPolicyValid == rebuiltDescription.IsHealthPolicyValid,
            "Rebuilt IsHealthPolicyValid mismatch {0} != {1}",
            internalDescription.IsHealthPolicyValid,
            rebuiltDescription.IsHealthPolicyValid);

        VERIFY(
            internalDescription.IsUpgradeHealthPolicyValid == rebuiltDescription.IsUpgradeHealthPolicyValid,
            "Rebuilt IsUpgradeHealthPolicyValid mismatch {0} != {1}",
            internalDescription.IsUpgradeHealthPolicyValid,
            rebuiltDescription.IsUpgradeHealthPolicyValid);
    }

    VerifyApplicationHealthPoliciesEqual(internalDescription.ApplicationHealthPolicies, rebuiltDescription.ApplicationHealthPolicies);
}

template <class TPublicDescription, class TInternalDescription>
void ClusterManagerTest::UpgradeWrapperMaxTimeoutTestHelper()
{
    TPublicDescription publicDescription = { 0 };
    FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION policyDescription;
    FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1 policyDescriptionEx1 = { 0 };
    FABRIC_ROLLING_UPGRADE_MONITORING_POLICY monitoringPolicy;
    FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_EX1 monitoringPolicyEx1 = { 0 };

    memset(&policyDescription, 0, sizeof(policyDescription));
    memset(&monitoringPolicy, 0, sizeof(monitoringPolicy));

    __if_exists (TPublicDescription::ApplicationName)
    {
        wstring appName = L"fabric:/testapp";
        wstring targetVersion = L"TargetVersion";
        publicDescription.ApplicationName = appName.c_str();
        publicDescription.TargetApplicationTypeVersion = targetVersion.c_str();
        publicDescription.UpgradeKind = FABRIC_APPLICATION_UPGRADE_KIND_ROLLING;
    }
    __if_not_exists (TPublicDescription::ApplicationName)
    {
        wstring codeVersion = L"1.2.3.4";
        wstring configVersion = L"cfg";
        publicDescription.CodeVersion = codeVersion.c_str();
        publicDescription.ConfigVersion = configVersion.c_str();
        publicDescription.UpgradeKind = FABRIC_UPGRADE_KIND_ROLLING;
    }

    publicDescription.UpgradePolicyDescription = &policyDescription;

    policyDescription.RollingUpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_MONITORED;
    policyDescription.UpgradeReplicaSetCheckTimeoutInSeconds = numeric_limits<DWORD>::max();
    policyDescription.Reserved = &policyDescriptionEx1;

    policyDescriptionEx1.MonitoringPolicy = &monitoringPolicy;
    monitoringPolicy.FailureAction = FABRIC_MONITORED_UPGRADE_FAILURE_ACTION_ROLLBACK;
    monitoringPolicy.HealthCheckWaitDurationInSeconds = numeric_limits<DWORD>::max();
    monitoringPolicy.HealthCheckRetryTimeoutInSeconds = numeric_limits<DWORD>::max();
    monitoringPolicy.UpgradeTimeoutInSeconds = numeric_limits<DWORD>::max();
    monitoringPolicy.UpgradeDomainTimeoutInSeconds = numeric_limits<DWORD>::max();
    monitoringPolicy.Reserved = &monitoringPolicyEx1;

    monitoringPolicyEx1.HealthCheckStableDurationInSeconds = numeric_limits<DWORD>::max();

    TInternalDescription internalDescription;
    auto error = internalDescription.FromPublicApi(publicDescription);
    VERIFY(error.IsSuccess(), "FromPublicApi={0}", error);

    {
        // Internal conversion of max DWORD to TimeSpan::MaxValue
        auto expected = TimeSpan::MaxValue;
        auto actual = internalDescription.ReplicaSetCheckTimeout;
        VERIFY(expected == actual, "UpgradeReplicaSetCheckTimeout: expected={0} actual={1}", expected, actual);
    }

    {
        auto expected = TimeSpan::MaxValue;
        auto actual = internalDescription.MonitoringPolicy.HealthCheckWaitDuration;
        VERIFY(expected == actual, "HealthCheckWaitDuration: expected={0} actual={1}", expected, actual);
    }

    {
        auto expected = TimeSpan::MaxValue;
        auto actual = internalDescription.MonitoringPolicy.HealthCheckRetryTimeout;
        VERIFY(expected == actual, "HealthCheckRetryTimeout: expected={0} actual={1}", expected, actual);
    }

    {
        auto expected = TimeSpan::MaxValue;
        auto actual = internalDescription.MonitoringPolicy.UpgradeTimeout;
        VERIFY(expected == actual, "UpgradeTimeout: expected={0} actual={1}", expected, actual);
    }

    {
        auto expected = TimeSpan::MaxValue;
        auto actual = internalDescription.MonitoringPolicy.UpgradeDomainTimeout;
        VERIFY(expected == actual, "UpgradeDomainTimeout: expected={0} actual={1}", expected, actual);
    }

    {
        auto expected = TimeSpan::MaxValue;
        auto actual = internalDescription.MonitoringPolicy.HealthCheckStableDuration;
        VERIFY(expected == actual, "HealthCheckStableDuration: expected={0} actual={1}", expected, actual);
    }
}


// ************ 
// Test Helpers
// ************ 

template <class TUpdateDescription>
void ClusterManagerTest::VerifyJsonSerialization(
    shared_ptr<TUpdateDescription> const & updateDescription)
{
    Trace.WriteWarning(TestSource, "Testing JSON serialization");

    wstring jsonString;
    auto error = JsonHelper::Serialize(*updateDescription, jsonString);
    VERIFY(error.IsSuccess(), "Serialize: error={0}", error);

    TUpdateDescription jsonDescription;
    error = JsonHelper::Deserialize(jsonDescription, jsonString);
    VERIFY(error.IsSuccess(), "Deserialize: '{0}' error={1}", jsonString, error);

    VERIFY_IS_TRUE(*updateDescription == jsonDescription);
}

#define SET_UPDATE_DESCRIPTION_VALUE( bit, publicDescription, property ) \
    SET_UPDATE_DESCRIPTION_VALUE_RANGE(int, bit, publicDescription, property, 1, 9999) \

#define SET_UPDATE_DESCRIPTION_VALUE_RANGE( type, bit, publicDescription, property, min, max ) \
if (flags & bit) \
{ \
    auto value = rand.Next(static_cast<int>(min), static_cast<int>(max)); \
    publicDescription.property = static_cast<type>(value); \
    expectedValues[bit] = value; \
} \

template <class TPublicUpdateDescription, class TPublicHealthPolicy, class TInternalUpdateDescription>
shared_ptr<TInternalUpdateDescription> ClusterManagerTest::CreateUpdateDescriptionFromPublic(
    LPCWSTR name,
    DWORD flags,
    __in Random & rand,
    __inout map<DWORD, int> & expectedValues,
    ErrorCodeValue::Enum expectedError)
{
    TPublicUpdateDescription publicDescription;
    FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION policyDescription;
    FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1 policyDescriptionEx1;
    FABRIC_ROLLING_UPGRADE_MONITORING_POLICY monitoringPolicy;
    FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_EX1 monitoringPolicyEx1;
    TPublicHealthPolicy healthPolicy;

    memset(&publicDescription, 0, sizeof(publicDescription));
    memset(&policyDescription, 0, sizeof(policyDescription));
    memset(&policyDescriptionEx1, 0, sizeof(policyDescriptionEx1));
    memset(&monitoringPolicy, 0, sizeof(monitoringPolicy));
    memset(&monitoringPolicyEx1, 0, sizeof(monitoringPolicyEx1));
    memset(&healthPolicy, 0, sizeof(healthPolicy));

    __if_exists (TPublicUpdateDescription::ApplicationName)
    {
        publicDescription.ApplicationName = name;
        publicDescription.UpgradeKind = FABRIC_APPLICATION_UPGRADE_KIND_ROLLING;
    }
    __if_not_exists (TPublicUpdateDescription::ApplicationName)
    {
        UNREFERENCED_PARAMETER(name);
        publicDescription.UpgradeKind = FABRIC_UPGRADE_KIND_ROLLING;

        FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX2 policyDescriptionEx2 = { 0 };
        FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX3 policyDescriptionEx3 = { 0 };
        FABRIC_CLUSTER_UPGRADE_HEALTH_POLICY upgradeHealthPolicy = { 0 };
        FABRIC_APPLICATION_HEALTH_POLICY_MAP applicationHealthPolicyMap = { 0 };

        policyDescriptionEx2.UpgradeHealthPolicy = &upgradeHealthPolicy;
        policyDescriptionEx1.Reserved = &policyDescriptionEx2;

        SET_UPDATE_DESCRIPTION_VALUE_RANGE(
            BYTE,
            FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_HEALTH_POLICY,
            upgradeHealthPolicy,
            MaxPercentDeltaUnhealthyNodes,
            0, 101)


        SET_UPDATE_DESCRIPTION_VALUE_RANGE(
            BYTE,
            FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_HEALTH_POLICY,
            upgradeHealthPolicy,
            MaxPercentUpgradeDomainDeltaUnhealthyNodes,
            0, 101)

        policyDescriptionEx3.ApplicationHealthPolicyMap = &applicationHealthPolicyMap;
        policyDescriptionEx2.Reserved = &policyDescriptionEx3;
    }

    publicDescription.UpdateFlags = flags;

    publicDescription.UpgradePolicyDescription = &policyDescription;
    policyDescription.Reserved = &policyDescriptionEx1;
    policyDescriptionEx1.MonitoringPolicy = &monitoringPolicy;
    policyDescriptionEx1.HealthPolicy = &healthPolicy;
    monitoringPolicy.Reserved = &monitoringPolicyEx1;

    expectedValues.clear();

    SET_UPDATE_DESCRIPTION_VALUE_RANGE(
        FABRIC_ROLLING_UPGRADE_MODE,
        FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_MODE,
        policyDescription,
        RollingUpgradeMode,
        FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO,
        static_cast<int>(FABRIC_ROLLING_UPGRADE_MODE_MONITORED)+1)

    SET_UPDATE_DESCRIPTION_VALUE_RANGE(
        BOOLEAN,
        FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_FORCE_RESTART,
        policyDescription,
        ForceRestart,
        FALSE,
        static_cast<int>(TRUE)+1)

    SET_UPDATE_DESCRIPTION_VALUE(
        FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_REPLICA_SET_CHECK_TIMEOUT,
        policyDescription,
        UpgradeReplicaSetCheckTimeoutInSeconds)

    SET_UPDATE_DESCRIPTION_VALUE_RANGE(
        FABRIC_MONITORED_UPGRADE_FAILURE_ACTION,
        FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_FAILURE_ACTION,
        monitoringPolicy,
        FailureAction,
        FABRIC_MONITORED_UPGRADE_FAILURE_ACTION_ROLLBACK,
        static_cast<int>(FABRIC_MONITORED_UPGRADE_FAILURE_ACTION_MANUAL)+1)

    SET_UPDATE_DESCRIPTION_VALUE(
        FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_CHECK_WAIT,
        monitoringPolicy,
        HealthCheckWaitDurationInSeconds)

    SET_UPDATE_DESCRIPTION_VALUE(
        FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_CHECK_STABLE,
        monitoringPolicyEx1,
        HealthCheckStableDurationInSeconds)

    SET_UPDATE_DESCRIPTION_VALUE(
        FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_CHECK_RETRY,
        monitoringPolicy,
        HealthCheckRetryTimeoutInSeconds)

    SET_UPDATE_DESCRIPTION_VALUE(
        FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_TIMEOUT,
        monitoringPolicy,
        UpgradeTimeoutInSeconds)

    SET_UPDATE_DESCRIPTION_VALUE(
        FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_DOMAIN_TIMEOUT,
        monitoringPolicy,
        UpgradeDomainTimeoutInSeconds)

    __if_exists (TPublicHealthPolicy::MaxPercentUnhealthyDeployedApplications)
    {
        SET_UPDATE_DESCRIPTION_VALUE_RANGE(
            BYTE,
            FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_POLICY,
            healthPolicy,
            MaxPercentUnhealthyDeployedApplications,
            0, 101)
    }
    __if_exists (TPublicHealthPolicy::MaxPercentUnhealthyNodes)
    {
        SET_UPDATE_DESCRIPTION_VALUE_RANGE(
            BYTE,
            FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_POLICY,
            healthPolicy,
            MaxPercentUnhealthyNodes,
            0, 101)
    }

    auto internalDescription = make_shared<TInternalUpdateDescription>();
    auto error = internalDescription->FromPublicApi(publicDescription);
    VERIFY(
        error.IsError(expectedError),
        "FromPublicAPI: flags={0:X} error={1} expected={2}",
        flags,
        error,
        expectedError);
    return internalDescription;
}

#define VERIFY_UPDATE_DESCRIPTION_VALUE_BEGIN( bit, description, property ) \
{ \
if (flags & bit) \
{ \
    VERIFY(description, "{0} = {1}", #description, description ? "valid" : "null"); \
    VERIFY(description->property, "{0} = {1}", #property, description->property ? "valid" : "null"); \
    auto findIt = expectedValues.find(bit); \
    VERIFY(findIt != expectedValues.end(), "find expectedValue for: {0:X}", static_cast<DWORD>(bit)); \
    auto expected = findIt->second; \

#define VERIFY_UPDATE_DESCRIPTION_VALUE_END( bit, description, property ) \
    VERIFY(expected == actual, "expectedValue={0} actual={1}", expected, actual); \
} \
else \
{ \
    bool isNull = !description || !description->property; \
    VERIFY(isNull, "desc={0} property={1}", description.get(), #property); \
} \
} \

#define VERIFY_UPDATE_DESCRIPTION_VALUE( bit, description, property ) \
    VERIFY_UPDATE_DESCRIPTION_VALUE_BEGIN(bit, description, property) \
    auto actual = static_cast<int>(*(description->property)); \
    VERIFY_UPDATE_DESCRIPTION_VALUE_END(bit, description, property) \

#define VERIFY_UPDATE_DESCRIPTION_BOOL( bit, description, property ) \
    VERIFY_UPDATE_DESCRIPTION_VALUE_BEGIN(bit, description, property) \
    auto actual = (*(description->property) ? 1 : 0); \
    VERIFY_UPDATE_DESCRIPTION_VALUE_END(bit, description, property) \

#define VERIFY_UPDATE_DESCRIPTION_TIMESPAN( bit, description, property ) \
    VERIFY_UPDATE_DESCRIPTION_VALUE_BEGIN(bit, description, property) \
    auto actual = static_cast<int>(description->property->TotalSeconds()); \
    VERIFY_UPDATE_DESCRIPTION_VALUE_END(bit, description, property) \

#define VERIFY_UPDATE_DESCRIPTION_HEALTH( bit, description, property, healthProperty ) \
    VERIFY_UPDATE_DESCRIPTION_VALUE_BEGIN(bit, description, property) \
    auto actual = static_cast<int>(description->property->healthProperty); \
    VERIFY_UPDATE_DESCRIPTION_VALUE_END(bit, description, property) \

template <class TUpdateDescription>
void ClusterManagerTest::VerifyUpdateDescription(
    wstring const & name,
    DWORD flags,
    shared_ptr<TUpdateDescription> const & description,
    map<DWORD, int> const & expectedValues)
{
    __if_exists (TUpdateDescription::ApplicationName)
    {
        VERIFY(name == description->ApplicationName.ToString(), "expected={0} actual={1}", name, description->ApplicationName);
    }
    __if_not_exists (TUpdateDescription::ApplicationName)
    {
        UNREFERENCED_PARAMETER(name);
    }

    VERIFY_UPDATE_DESCRIPTION_VALUE(
        FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_MODE,
        description->UpdateDescription,
        RollingUpgradeMode)

        VERIFY_UPDATE_DESCRIPTION_BOOL(
        FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_FORCE_RESTART,
        description->UpdateDescription,
        ForceRestart)

        VERIFY_UPDATE_DESCRIPTION_TIMESPAN(
        FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_REPLICA_SET_CHECK_TIMEOUT,
        description->UpdateDescription,
        ReplicaSetCheckTimeout)

        VERIFY_UPDATE_DESCRIPTION_VALUE(
        FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_FAILURE_ACTION,
        description->UpdateDescription,
        FailureAction)

        VERIFY_UPDATE_DESCRIPTION_TIMESPAN(
        FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_CHECK_WAIT,
        description->UpdateDescription,
        HealthCheckWaitDuration)

        VERIFY_UPDATE_DESCRIPTION_TIMESPAN(
        FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_CHECK_STABLE,
        description->UpdateDescription,
        HealthCheckStableDuration)

        VERIFY_UPDATE_DESCRIPTION_TIMESPAN(
        FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_CHECK_RETRY,
        description->UpdateDescription,
        HealthCheckRetryTimeout)

        VERIFY_UPDATE_DESCRIPTION_TIMESPAN(
        FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_TIMEOUT,
        description->UpdateDescription,
        UpgradeTimeout)

        VERIFY_UPDATE_DESCRIPTION_TIMESPAN(
        FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_DOMAIN_TIMEOUT,
        description->UpdateDescription,
        UpgradeDomainTimeout)

        __if_exists (TUpdateDescription::ApplicationName)
    {
        VERIFY_UPDATE_DESCRIPTION_HEALTH(
            FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_POLICY,
            description,
            HealthPolicy,
            MaxPercentUnhealthyDeployedApplications)
    }
    __if_not_exists (TUpdateDescription::ApplicationName)
    {
        VERIFY_UPDATE_DESCRIPTION_HEALTH(
            FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_POLICY,
            description,
            HealthPolicy,
            MaxPercentUnhealthyNodes)
    }
}

void ClusterManagerTest::VerifyApplicationHealthPoliciesEqual(ApplicationHealthPolicyMapSPtr const & l, ApplicationHealthPolicyMapSPtr const & r)
{
    if (l)
    {
        if (!r)
        {
            VERIFY(false, "ApplicationHealthPolicies mismatch. l.size={0}, r is null", l->size());
        }

        VERIFY(*l == *r, "ApplicationHealthPolicies mismatch. l.size={0}, r.size={1}", l->size(), r->size());
    }
    else if (r)
    {
        VERIFY(false, "ApplicationHealthPolicies mismatch. l is null, r.size()={0}", r->size());
    }
}

void ClusterManagerTest::VerifyClusterHealthPolicy(FABRIC_CLUSTER_HEALTH_POLICY const & publicPolicy, ClusterHealthPolicy const & internalPolicy)
{
    VERIFY(
        publicPolicy.ConsiderWarningAsError == (internalPolicy.ConsiderWarningAsError ? TRUE : FALSE),
        "Health Policy ConsiderWarningAsError mismatch {0} != {1}",
        publicPolicy.ConsiderWarningAsError,
        internalPolicy.ConsiderWarningAsError);

    VERIFY(
        publicPolicy.MaxPercentUnhealthyNodes == internalPolicy.MaxPercentUnhealthyNodes,
        "Health Policy MaxPercentUnhealthyNodes mismatch {0} != {1}",
        publicPolicy.MaxPercentUnhealthyNodes,
        internalPolicy.MaxPercentUnhealthyNodes);

    VERIFY(
        publicPolicy.MaxPercentUnhealthyApplications == internalPolicy.MaxPercentUnhealthyApplications,
        "Health Policy MaxPercentUnhealthyApplications mismatch {0} != {1}",
        publicPolicy.MaxPercentUnhealthyApplications,
        internalPolicy.MaxPercentUnhealthyApplications);

    ClusterHealthPolicy publicPolicyFromApi;
    auto error = publicPolicyFromApi.FromPublicApi(publicPolicy);
    VERIFY(error.IsSuccess(), "Convert public policy");
    VERIFY(publicPolicyFromApi == internalPolicy, "Check FromPublicApi cluster health policy");
}

void ClusterManagerTest::ResetDirectory(wstring const & directory)
{
    DeleteDirectory(directory);

    ErrorCode error = Directory::Create2(directory);
    VERIFY(error.IsSuccess(), "Could not create directory '{0}' due to {1}", directory, error);
}

void ClusterManagerTest::CopyFile(wstring const & src, wstring const & dest)
{
    ErrorCode error = File::SafeCopy(src, dest);
		  Trace.WriteInfo(TestSource, "Error Code {0}", error);
    VERIFY(error.IsSuccess(), "Could not copy '{0}' to '{1}' due to {2}", src, dest, error);
}

void ClusterManagerTest::DeleteDirectory(wstring const & directory)
{
    ErrorCode error(ErrorCodeValue::Success);
    int attempts = 5;

    do
    {
        if (Directory::Exists(directory))
        {
            error = Directory::Delete(directory, true);

            if (!error.IsSuccess())
            {
                Sleep(2000);
            }
        }
    } while (!error.IsSuccess() && --attempts > 0);

    VERIFY(error.IsSuccess(), "Could not delete directory '{0}' due to {1}", directory, error);
}

bool ClusterManagerTest::TestSetup()
{
    Management::ManagementConfig::GetConfig().Test_Reset();

    return true;
}

bool ClusterManagerTest::TestCleanup()
{
    Trace.WriteNoise(TestSource, "++ Test Complete");

    Management::ManagementConfig::GetConfig().Test_Reset();

    return true;
}
