// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace ServiceModelTests
{
    using namespace std;
    using namespace Common;
    using namespace ServiceModel;
    using namespace Reliability;

    StringLiteral const TestSource("ServiceModelParserTest");

    class ServiceModelParserTest
    {
    protected:
        static wstring GetTestFilePath(wstring const & testFileName);
        static void ParseConfigSettings(wstring const & testFileName);
        static void ParseAndValidateTargetInformationFile(wstring const & fileName, bool expectValidCurrentInstallation, bool expectValidTargetInstallation);
        void ApplicationHealthPolicyTestInternal(wstring const & testFileName, ApplicationHealthPolicy const & expectedPolicy);
        void ApplicationPoliciesTestInternal(wstring const & testFileName, ApplicationPoliciesDescription const & appPoliciesDescription);
    };

    BOOST_FIXTURE_TEST_SUITE(ServiceModelParserTestSuite,ServiceModelParserTest)

    BOOST_AUTO_TEST_CASE(ApplicationHealthPolicyTest)
    {
        {
            // Test 1: ApplicationHealthPolicy is not specified, parse default
            ApplicationHealthPolicy expectedPolicy;
            this->ApplicationHealthPolicyTestInternal(
                GetTestFilePath(L"ApplicationPackage-HealthPolicy-1.1.0.xml"),
                expectedPolicy);
        }

        {
            // Test 2: Empty health policy element, should parse default
            ApplicationHealthPolicy expectedPolicy;
            this->ApplicationHealthPolicyTestInternal(
                GetTestFilePath(L"ApplicationPackage-HealthPolicy-2.1.0.xml"),
                expectedPolicy);
        }

        {
            // Test 3: ApplicationHealthPolicy specified, no ServiceTypeHealthPolicies
            ApplicationHealthPolicy expectedPolicy;
            expectedPolicy.ConsiderWarningAsError = true;
            expectedPolicy.MaxPercentUnhealthyDeployedApplications = 20;
            this->ApplicationHealthPolicyTestInternal(
                GetTestFilePath(L"ApplicationPackage-HealthPolicy-3.1.0.xml"),
                expectedPolicy);
        }

        {
            // Test 4: ApplicationHealthPolicy specified, non empty element
            ApplicationHealthPolicy expectedPolicy;
            expectedPolicy.ConsiderWarningAsError = true;
            expectedPolicy.MaxPercentUnhealthyDeployedApplications = 20;
            this->ApplicationHealthPolicyTestInternal(
                GetTestFilePath(L"ApplicationPackage-HealthPolicy-4.1.0.xml"),
                expectedPolicy);
        }

        {
            // Test 5: Default DefaultServiceTypeHealthPolicy, default ServiceTypeHealthPolicy
            ApplicationHealthPolicy expectedPolicy;
            expectedPolicy.ConsiderWarningAsError = true;
            expectedPolicy.MaxPercentUnhealthyDeployedApplications = 20;
            expectedPolicy.ServiceTypeHealthPolicies.insert(
                make_pair(L"HealthPolicyServiceTypeName", ServiceTypeHealthPolicy()));

            this->ApplicationHealthPolicyTestInternal(
                GetTestFilePath(L"ApplicationPackage-HealthPolicy-5.1.0.xml"),
                expectedPolicy);
        }

        {
            // Test 6: all policies specified
            ApplicationHealthPolicy expectedPolicy;
            expectedPolicy.ConsiderWarningAsError = true;
            expectedPolicy.MaxPercentUnhealthyDeployedApplications = 20;

            expectedPolicy.DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyServices = 30;
            expectedPolicy.DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyPartitionsPerService = 100;
            expectedPolicy.DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyReplicasPerPartition = 0;

            expectedPolicy.ServiceTypeHealthPolicies.insert(
                make_pair(L"HealthPolicyServiceTypeName1", ServiceTypeHealthPolicy(30, 40, 50)));
            expectedPolicy.ServiceTypeHealthPolicies.insert(
                make_pair(L"HealthPolicyServiceTypeName2", ServiceTypeHealthPolicy(0, 10, 20)));

            this->ApplicationHealthPolicyTestInternal(
                GetTestFilePath(L"ApplicationPackage-HealthPolicy-6.1.0.xml"),
                expectedPolicy);
        }
    }


    BOOST_AUTO_TEST_CASE(ApplicationPoliciesTest)
    {
        {
            // Test 1: Only log collection policies specified
            ApplicationPoliciesDescription expectedPolicy;
            expectedPolicy.LogCollectionEntries.push_back(LogCollectionPolicyDescription());
            LogCollectionPolicyDescription policyDes1, policyDes2;
            policyDes1.Path = L"innerFolder1";
            expectedPolicy.LogCollectionEntries.push_back(policyDes1);
            policyDes2.Path = L"innerFolder2";
            expectedPolicy.LogCollectionEntries.push_back(policyDes2);
            expectedPolicy.DefaultRunAs.UserRef = L"MyUser";
            this->ApplicationPoliciesTestInternal(
                GetTestFilePath(L"ApplicationPackage-Policies-1.1.0.xml"),
                expectedPolicy);
        }

        {
            // Test 2: LogCollectionPolicies, DefaultRunAs and Health
            ApplicationPoliciesDescription expectedPolicy;
            expectedPolicy.LogCollectionEntries.push_back(LogCollectionPolicyDescription());
            LogCollectionPolicyDescription policyDes1, policyDes2;
            policyDes1.Path = L"innerFolder1";
            expectedPolicy.LogCollectionEntries.push_back(policyDes1);
            policyDes2.Path = L"innerFolder2";
            expectedPolicy.LogCollectionEntries.push_back(policyDes2);
            expectedPolicy.DefaultRunAs.UserRef = L"MyUser";
            expectedPolicy.HealthPolicy.ConsiderWarningAsError = true;
            expectedPolicy.HealthPolicy.MaxPercentUnhealthyDeployedApplications = 20;
            this->ApplicationPoliciesTestInternal(
                GetTestFilePath(L"ApplicationPackage-Policies-2.1.0.xml"),
                expectedPolicy);
        }

        {
            // Test 3: Health and DefaultRunAs
            ApplicationPoliciesDescription expectedPolicy;
            expectedPolicy.DefaultRunAs.UserRef = L"MyUser";
            expectedPolicy.HealthPolicy.ConsiderWarningAsError = true;
            expectedPolicy.HealthPolicy.MaxPercentUnhealthyDeployedApplications = 20;
            this->ApplicationPoliciesTestInternal(
                GetTestFilePath(L"ApplicationPackage-Policies-3.1.0.xml"),
                expectedPolicy);
        }
    }


    BOOST_AUTO_TEST_CASE(ServicePackageTest)
    {
        ServicePackageDescription servicePackage;

        auto error = Parser::ParseServicePackage(
            GetTestFilePath(L"CalculatorSample.Package.Bootstrap.xml"),
            servicePackage);

        Trace.WriteNoise(TestSource, "ParseServicePackage={0}", error);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ParseServicePackage.IsSuccess");
    }

    BOOST_AUTO_TEST_CASE(ServicePackageWithDiagnosticsTest)
    {
        ServicePackageDescription servicePackage;

        auto error = Parser::ParseServicePackage(
            GetTestFilePath(L"CalculatorSample.Package.WithDiagnostics.xml"),
            servicePackage);

        Trace.WriteNoise(TestSource, "ParseServicePackage={0}", error);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ParseServicePackage.IsSuccess");
    }

    BOOST_AUTO_TEST_CASE(ServiceManifestTest)
    {
        ServiceManifestDescription serviceManifest;

        auto error = Parser::ParseServiceManifest(
            GetTestFilePath(L"CalculatorSample.Manifest.1.0.xml"),
            serviceManifest);

        Trace.WriteNoise(TestSource, "ParseServiceManifest={0}", error);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ParseServiceManifest.IsSuccess");

        wstring fileName(L"CalculatorSample.Manifest.Endpoints");
        EndpointDescription::WriteToFile(fileName, serviceManifest.Resources.Endpoints);

        vector<EndpointDescription> endpointDescriptions;
        EndpointDescription::ReadFromFile(fileName, endpointDescriptions);
        VERIFY_IS_TRUE(endpointDescriptions.size() == serviceManifest.Resources.Endpoints.size());
        for(size_t i = 0; i < endpointDescriptions.size(); ++i)
        {
            VERIFY_IS_TRUE(endpointDescriptions[i] == serviceManifest.Resources.Endpoints[i]);
        }
    }

    BOOST_AUTO_TEST_CASE(ServiceManifestWithServiceGroupTest)
    {
        ServiceManifestDescription serviceManifest;

        auto error = Parser::ParseServiceManifest(
            GetTestFilePath(L"ServiceGroupSample.Manifest.1.0.xml"),
            serviceManifest);

        Trace.WriteNoise(TestSource, "ParseServiceManifest={0}", error);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ParseServiceManifest.IsSuccess");
    }

    BOOST_AUTO_TEST_CASE(ServiceManifestWithRunFrequencyAndConsoleRedirection)
    {
        ServiceManifestDescription serviceManifest;

        auto error = Parser::ParseServiceManifest(
            GetTestFilePath(L"CalculatorSample.Manifest.2.0.xml"),
            serviceManifest);

        Trace.WriteNoise(TestSource, "ParseServiceManifest={0}", error);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ParseServiceManifest.IsSuccess");
    }

    BOOST_AUTO_TEST_CASE(ServiceManifestNegtiveTest)
    {
        ServiceManifestDescription serviceManifest;

        auto error = Parser::ParseServiceManifest(
            GetTestFilePath(L"CalculatorSample.Manifest.3.0.xml"),
            serviceManifest);

        Trace.WriteNoise(TestSource, "ParseServiceManifest={0}", error);
        VERIFY_IS_FALSE(error.IsSuccess(), L"ParseServiceManifest.IsError");

        wstring fileName(L"CalculatorSample.Manifest.Endpoints");
        EndpointDescription::WriteToFile(fileName, serviceManifest.Resources.Endpoints);

        vector<EndpointDescription> endpointDescriptions;
        EndpointDescription::ReadFromFile(fileName, endpointDescriptions);
        VERIFY_IS_TRUE(endpointDescriptions.size() == serviceManifest.Resources.Endpoints.size());
        for (size_t i = 0; i < endpointDescriptions.size(); ++i)
        {
            VERIFY_IS_TRUE(endpointDescriptions[i] == serviceManifest.Resources.Endpoints[i]);
        }
    }

    BOOST_AUTO_TEST_CASE(ApplicationManifestTest)
    {
        ApplicationManifestDescription applicationManifest;

        auto error = Parser::ParseApplicationManifest(
            GetTestFilePath(L"ApplicationManifest.1.0.xml"),
            applicationManifest);

        Trace.WriteNoise(TestSource, "ParseApplicationManifest={0}", error);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ParseApplicationManifest.IsSuccess");

        Trace.WriteNoise(TestSource, "{0}", applicationManifest);
        VERIFY_IS_TRUE(applicationManifest.Name == L"CalculatorApp");
        VERIFY_IS_TRUE(applicationManifest.Version == L"1.0");

        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports.size() == 2);

		VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ServiceManifestRef.Name == L"CalculatorServicePackage");
		VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[1].ServiceManifestRef.Name == L"CalculatorGatewayPackage");

		VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ServiceManifestRef.Version == L"2.0");
		VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[1].ServiceManifestRef.Version == L"3.0");

        VERIFY_IS_TRUE(applicationManifest.ApplicationParameters.size() == 1);
        VERIFY_IS_TRUE(applicationManifest.ApplicationParameters.count(L"ServiceInstanceCount") == 1);
        VERIFY_IS_TRUE(applicationManifest.ApplicationParameters[L"ServiceInstanceCount"] == L"3");

        VERIFY_IS_TRUE(applicationManifest.DefaultServices.size() == 3, L"Default service count.");

        VERIFY_IS_TRUE(applicationManifest.DefaultServices[0].Name == L"RequiredCalcService", L"Default service Name.");
        VERIFY_IS_TRUE(applicationManifest.DefaultServices[0].PackageActivationMode == L"SharedProcess", L"Default service PackageActivationMode");
		VERIFY_IS_TRUE(applicationManifest.DefaultServices[0].ServiceDnsName == L"", L"Default service ServiceDnsName");

        VERIFY_IS_TRUE(applicationManifest.DefaultServices[1].Name == L"RequiredCalcServiceShared", L"Default service Name.");
        VERIFY_IS_TRUE(applicationManifest.DefaultServices[1].PackageActivationMode == L"SharedProcess", L"Default service PackageActivationMode");
		VERIFY_IS_TRUE(applicationManifest.DefaultServices[1].ServiceDnsName == L"", L"Default service ServiceDnsName");

        VERIFY_IS_TRUE(applicationManifest.DefaultServices[2].Name == L"RequiredCalcServiceExclusive", L"Default service Name.");
        VERIFY_IS_TRUE(applicationManifest.DefaultServices[2].PackageActivationMode == L"ExclusiveProcess", L"Default service PackageActivationMode");
		VERIFY_IS_TRUE(applicationManifest.DefaultServices[2].ServiceDnsName == L"Required.CalcService.Exclusive", L"Default service ServiceDnsName");

        error = Parser::ParseApplicationManifest(
            GetTestFilePath(L"ApplicationManifest.2.0.xml"),
            applicationManifest);

        Trace.WriteNoise(TestSource, "ParseApplicationManifestWithEmptyPolicies={0}", error);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ParseApplicationManifestWithEmptyPolicies.IsSuccess");

        VERIFY_IS_TRUE(applicationManifest.DefaultServices.size() == 4, L"default service count");

        VERIFY_IS_FALSE(applicationManifest.DefaultServices[0].DefaultService.IsServiceGroup, L"non-servicegroup service");
        VERIFY_IS_TRUE(applicationManifest.DefaultServices[1].DefaultService.IsServiceGroup, L"servicegroup service");
        VERIFY_IS_TRUE(applicationManifest.DefaultServices[2].DefaultService.IsServiceGroup, L"servicegroup service");
        VERIFY_IS_TRUE(applicationManifest.DefaultServices[3].DefaultService.IsServiceGroup, L"servicegroup service");

        VERIFY_IS_TRUE(applicationManifest.DefaultServices[0].Name == L"Service1", L"service name");
        VERIFY_IS_TRUE(applicationManifest.DefaultServices[0].PackageActivationMode == L"SharedProcess", L"service PackageActivationMode");

        VERIFY_IS_TRUE(applicationManifest.DefaultServices[1].Name == L"ServiceGroup1", L"servicegroup name");
        VERIFY_IS_TRUE(applicationManifest.DefaultServices[1].PackageActivationMode == L"SharedProcess", L"service group PackageActivationMode");

        VERIFY_IS_TRUE(applicationManifest.DefaultServices[2].Name == L"ServiceGroup2", L"servicegroup name");
        VERIFY_IS_TRUE(applicationManifest.DefaultServices[2].PackageActivationMode == L"SharedProcess", L"service group PackageActivationMode");

        VERIFY_IS_TRUE(applicationManifest.DefaultServices[3].Name == L"ServiceGroup3", L"servicegroup name");
        VERIFY_IS_TRUE(applicationManifest.DefaultServices[3].PackageActivationMode == L"ExclusiveProcess", L"service group PackageActivationMode");

        VERIFY_IS_TRUE(applicationManifest.DefaultServices[1].DefaultService.IsStateful, L"stateful servicegroup");
        VERIFY_IS_FALSE(applicationManifest.DefaultServices[2].DefaultService.IsStateful, L"stateless servicegroup");
        VERIFY_IS_FALSE(applicationManifest.DefaultServices[3].DefaultService.IsStateful, L"stateless servicegroup");

        VERIFY_IS_TRUE(applicationManifest.DefaultServices[1].DefaultService.ServiceGroupMembers.size() == 2, L"service group member count");
        VERIFY_IS_TRUE(applicationManifest.DefaultServices[2].DefaultService.ServiceGroupMembers.size() == 2, L"service group member count");
        VERIFY_IS_TRUE(applicationManifest.DefaultServices[3].DefaultService.ServiceGroupMembers.size() == 2, L"service group member count");

        VERIFY_IS_TRUE(applicationManifest.DefaultServices[1].DefaultService.ServiceGroupMembers[0].LoadMetrics.size() == 0, L"no load metrics");
        VERIFY_IS_TRUE(applicationManifest.DefaultServices[1].DefaultService.ServiceGroupMembers[1].LoadMetrics.size() == 0, L"no load metrics");
        VERIFY_IS_TRUE(applicationManifest.DefaultServices[2].DefaultService.ServiceGroupMembers[0].LoadMetrics.size() == 0, L"no load metrics");
        VERIFY_IS_TRUE(applicationManifest.DefaultServices[2].DefaultService.ServiceGroupMembers[1].LoadMetrics.size() == 1, L"load metrics");
        VERIFY_IS_TRUE(applicationManifest.DefaultServices[3].DefaultService.ServiceGroupMembers[0].LoadMetrics.size() == 0, L"no load metrics");
        VERIFY_IS_TRUE(applicationManifest.DefaultServices[3].DefaultService.ServiceGroupMembers[1].LoadMetrics.size() == 1, L"load metrics");
    }

    BOOST_AUTO_TEST_CASE(ApplicationManifestTestWithContainerHostPolicies)
    {
        ApplicationManifestDescription applicationManifest;

        auto error = Parser::ParseApplicationManifest(
            GetTestFilePath(L"ApplicationManifest.3.0.xml"),
            applicationManifest);

        Trace.WriteNoise(TestSource, "ParseApplicationManifest={0}", error);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ParseApplicationManifest.IsSuccess");

        Trace.WriteNoise(TestSource, "{0}", applicationManifest);
        VERIFY_IS_TRUE(applicationManifest.Name == L"ContainerApplicationType");
        VERIFY_IS_TRUE(applicationManifest.Version == L"2.0.0");

        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports.size() == 1);

        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ServiceManifestRef.Name == L"ContainerServicePkg");

        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ServiceManifestRef.Version == L"2.0.0");

        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies.size() == 1);

        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].CodePackageRef == L"Code");
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].RepositoryCredentials.AccountName == L"sftestcontainerreg");
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].RepositoryCredentials.Password == L"[ContainerRepositoryPassword]");

        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].HealthConfig.IncludeDockerHealthStatusInSystemHealthReport == false);
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].HealthConfig.RestartContainerOnUnhealthyDockerHealthStatus == true);

        VERIFY_IS_TRUE(applicationManifest.ApplicationParameters.size() == 1);
        VERIFY_IS_TRUE(applicationManifest.ApplicationParameters.count(L"ContainerRepositoryPassword") == 1);
        VERIFY_IS_TRUE(applicationManifest.ApplicationParameters[L"ContainerRepositoryPassword"] == L"");

        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].PortBindings.size() == 1);
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].PortBindings[0].ContainerPort == 15243);
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].PortBindings[0].EndpointResourceRef == L"Endpoint");

        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].CertificateRef.size() == 3);
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].CertificateRef[0].X509StoreName == L"My");
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].CertificateRef[0].X509FindValue == L"ad fc 91 97 13 16 8d 9f a8 ee 71 2b a2 f4 37 62 00 03 49 0d");
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].CertificateRef[0].Name == L"MyCert1");

        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].CertificateRef[1].DataPackageRef == L"Data");
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].CertificateRef[1].DataPackageVersion == L"1.0");
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].CertificateRef[1].RelativePath == L"Certificate.pfx");
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].CertificateRef[1].Password == L"password");
        VERIFY_IS_FALSE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].CertificateRef[1].IsPasswordEncrypted);
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].CertificateRef[1].Name == L"MyCert2");

        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].CertificateRef[2].DataPackageRef == L"Data");
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].CertificateRef[2].DataPackageVersion == L"2.0");
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].CertificateRef[2].RelativePath == L"Certificate.pfx");
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].CertificateRef[2].Password == L"password");
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].CertificateRef[2].IsPasswordEncrypted);
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].CertificateRef[2].Name == L"MyCert3");

        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].LogConfig.Driver == L"etwlogs");
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].LogConfig.DriverOpts.size() == 1);
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].LogConfig.DriverOpts[0].Name == L"test");
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].LogConfig.DriverOpts[0].Value == L"value");

        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].Volumes.size() == 3);
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].Volumes[0].Source == L"c:\\workspace");
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].Volumes[0].Destination == L"c:\\testmountlocation1");
        VERIFY_IS_FALSE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].Volumes[0].IsReadOnly);
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].Volumes[1].Source == L"d:\\myfolder");
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].Volumes[1].Destination == L"c:\\testmountlocation2");
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].Volumes[1].IsReadOnly);
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].Volumes[2].Source == L"myexternalvolume");
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].Volumes[2].Destination == L"c:\\testmountlocation3");
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].Volumes[2].IsReadOnly);
        VERIFY_IS_TRUE(applicationManifest.ServiceManifestImports[0].ContainerHostPolicies[0].Volumes[2].Driver == L"sf");
    }

    BOOST_AUTO_TEST_CASE(ApplicationManifestWithEmptyPolicyTest)
    {
        ApplicationManifestDescription applicationManifest;
        auto error = Parser::ParseApplicationManifest(
            GetTestFilePath(L"ApplicationManifest.2.0.xml"),
            applicationManifest);

        Trace.WriteNoise(TestSource, "ParseApplicationManifestWithEmptyPolicies={0}", error);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ParseApplicationManifestWithEmptyPolicies.IsSuccess");
    }

    BOOST_AUTO_TEST_CASE(ApplicationInstanceTest)
    {
        ApplicationInstanceDescription applicationInstance;

        auto error = Parser::ParseApplicationInstance(
            GetTestFilePath(L"ApplicationInstance.1.xml"),
            applicationInstance);

        Trace.WriteNoise(TestSource, "ParseApplicationInstance={0}", error);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ParseApplicationInstance.IsSuccess");

        VERIFY_IS_TRUE(applicationInstance.DefaultServices.size() == 4, L"Default service count");

        VERIFY_IS_TRUE(applicationInstance.DefaultServices[0].Name == L"Service1", L"Default service Name.");
        VERIFY_IS_TRUE(applicationInstance.DefaultServices[0].PackageActivationMode == ServicePackageActivationMode::SharedProcess, L"Default service PackageActivationMode");
		VERIFY_IS_TRUE(applicationInstance.DefaultServices[0].ServiceDnsName == L"", L"Default service ServiceDnsName");

        VERIFY_IS_TRUE(applicationInstance.DefaultServices[1].Name == L"Service2", L"Default service Name.");
        VERIFY_IS_TRUE(applicationInstance.DefaultServices[1].PackageActivationMode == ServicePackageActivationMode::SharedProcess, L"Default service PackageActivationMode");
		VERIFY_IS_TRUE(applicationInstance.DefaultServices[1].ServiceDnsName == L"", L"Default service ServiceDnsName");
        VERIFY_IS_TRUE(applicationInstance.DefaultServices[1].DefaultService.ScalingPolicies.size() == 0, L"ScalingPolicies");

        VERIFY_IS_TRUE(applicationInstance.DefaultServices[2].Name == L"Service3", L"Default service Name.");
        VERIFY_IS_TRUE(applicationInstance.DefaultServices[2].PackageActivationMode == ServicePackageActivationMode::ExclusiveProcess, L"Default service PackageActivationMode");
		VERIFY_IS_TRUE(applicationInstance.DefaultServices[2].ServiceDnsName == L"Service3.Dns.Name", L"Default service ServiceDnsName");

        VERIFY_IS_TRUE(applicationInstance.DefaultServices[3].DefaultService.ScalingPolicies.size() == 1, L"ScalingPolicy");
        VERIFY_IS_TRUE(applicationInstance.DefaultServices[3].DefaultService.ScalingPolicies[0].Mechanism->Kind == ScalingMechanismKind::Enum::PartitionInstanceCount, L"ScalingPolicyMechanismKind");
        VERIFY_IS_TRUE(applicationInstance.DefaultServices[3].DefaultService.ScalingPolicies[0].Trigger->Kind == ScalingTriggerKind::Enum::AverageServiceLoad, L"ScalingPolicyTriggerKind");

        auto averageLoadPolicy1 = (AveragePartitionLoadScalingTrigger const &)(*applicationInstance.DefaultServices[3].DefaultService.ScalingPolicies[0].Trigger);
        VERIFY_IS_TRUE(averageLoadPolicy1.MetricName == L"MetricA", L"ScalingPolicyMetricName");

        auto instanceCount = (InstanceCountScalingMechanism const &)(*applicationInstance.DefaultServices[3].DefaultService.ScalingPolicies[0].Mechanism);
        VERIFY_IS_TRUE(instanceCount.ScaleIncrement == 1, L"ScalingPolicyIncrement");

        VERIFY_IS_TRUE(applicationInstance.ServiceTemplates.size() == 6, L"Service template count");
        VERIFY_IS_TRUE(applicationInstance.ServiceTemplates[1].ScalingPolicies.size() == 0, L"ScalingPolicy");
        VERIFY_IS_TRUE(applicationInstance.ServiceTemplates[2].ScalingPolicies.size() == 1, L"ScalingPolicy");
        VERIFY_IS_TRUE(applicationInstance.ServiceTemplates[2].ScalingPolicies[0].Trigger->Kind == ScalingTriggerKind::Enum::AveragePartitionLoad, L"ScalingPolicyTriggerKind");

        auto averageLoadPolicy2 = (AverageServiceLoadScalingTrigger const &)(*applicationInstance.ServiceTemplates[2].ScalingPolicies[0].Trigger);
        VERIFY_IS_TRUE(averageLoadPolicy2.MetricName == L"MetricB", L"ScalingPolicyMetricName");
        VERIFY_IS_TRUE(averageLoadPolicy2.ScaleIntervalInSeconds == 100, L"ScalingPolicyInterval");
    }

    BOOST_AUTO_TEST_CASE(ApplicationInstanceWithDefaultServiceGroupTest)
    {
        ApplicationInstanceDescription applicationInstance;

        auto error = Parser::ParseApplicationInstance(
            GetTestFilePath(L"ApplicationInstanceWithDefaultServiceGroup.1.xml"),
            applicationInstance);

        Trace.WriteNoise(TestSource, "ParseApplicationInstance={0}", error);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ParseApplicationInstance.IsSuccess");

        VERIFY_IS_TRUE(applicationInstance.DefaultServices.size() == 4, L"default service count");

        VERIFY_IS_FALSE(applicationInstance.DefaultServices[0].DefaultService.IsServiceGroup, L"non-servicegroup service");
        VERIFY_IS_TRUE(applicationInstance.DefaultServices[1].DefaultService.IsServiceGroup, L"servicegroup service");
        VERIFY_IS_TRUE(applicationInstance.DefaultServices[2].DefaultService.IsServiceGroup, L"servicegroup service");
        VERIFY_IS_TRUE(applicationInstance.DefaultServices[3].DefaultService.IsServiceGroup, L"servicegroup service");

        VERIFY_IS_TRUE(applicationInstance.DefaultServices[0].Name == L"Service1", L"service name");
        VERIFY_IS_TRUE(applicationInstance.DefaultServices[0].PackageActivationMode == ServicePackageActivationMode::SharedProcess, L"service PackageActivationMode");

        VERIFY_IS_TRUE(applicationInstance.DefaultServices[1].Name == L"ServiceGroup1", L"servicegroup name");
        VERIFY_IS_TRUE(applicationInstance.DefaultServices[1].PackageActivationMode == ServicePackageActivationMode::SharedProcess, L"service group PackageActivationMode");

        VERIFY_IS_TRUE(applicationInstance.DefaultServices[2].Name == L"ServiceGroup2", L"servicegroup name");
        VERIFY_IS_TRUE(applicationInstance.DefaultServices[2].PackageActivationMode == ServicePackageActivationMode::SharedProcess, L"service group PackageActivationMode");

        VERIFY_IS_TRUE(applicationInstance.DefaultServices[3].Name == L"ServiceGroup3", L"servicegroup name");
        VERIFY_IS_TRUE(applicationInstance.DefaultServices[3].PackageActivationMode == ServicePackageActivationMode::ExclusiveProcess, L"service group PackageActivationMode");

        VERIFY_IS_TRUE(applicationInstance.DefaultServices[1].DefaultService.IsStateful, L"stateful servicegroup");
        VERIFY_IS_FALSE(applicationInstance.DefaultServices[2].DefaultService.IsStateful, L"stateless servicegroup");
        VERIFY_IS_FALSE(applicationInstance.DefaultServices[3].DefaultService.IsStateful, L"stateless servicegroup");

        VERIFY_IS_TRUE(applicationInstance.DefaultServices[1].DefaultService.ServiceGroupMembers.size() == 2, L"service group member count");
        VERIFY_IS_TRUE(applicationInstance.DefaultServices[2].DefaultService.ServiceGroupMembers.size() == 2, L"service group member count");
        VERIFY_IS_TRUE(applicationInstance.DefaultServices[3].DefaultService.ServiceGroupMembers.size() == 2, L"service group member count");

        VERIFY_IS_TRUE(applicationInstance.DefaultServices[1].DefaultService.ServiceGroupMembers[0].LoadMetrics.size() == 0, L"no load metrics");
        VERIFY_IS_TRUE(applicationInstance.DefaultServices[1].DefaultService.ServiceGroupMembers[1].LoadMetrics.size() == 0, L"no load metrics");
        VERIFY_IS_TRUE(applicationInstance.DefaultServices[2].DefaultService.ServiceGroupMembers[0].LoadMetrics.size() == 0, L"no load metrics");
        VERIFY_IS_TRUE(applicationInstance.DefaultServices[2].DefaultService.ServiceGroupMembers[1].LoadMetrics.size() == 1, L"load metrics");
        VERIFY_IS_TRUE(applicationInstance.DefaultServices[3].DefaultService.ServiceGroupMembers[0].LoadMetrics.size() == 0, L"no load metrics");
        VERIFY_IS_TRUE(applicationInstance.DefaultServices[3].DefaultService.ServiceGroupMembers[1].LoadMetrics.size() == 1, L"load metrics");
    }

    BOOST_AUTO_TEST_CASE(ApplicationPackageTest)
    {
        ApplicationPackageDescription applicationPackage;

        auto error = Parser::ParseApplicationPackage(
            GetTestFilePath(L"ApplicationPackage.1.0.xml"),
            applicationPackage);

        Trace.WriteNoise(TestSource, "ParseApplicationPackage={0}", error);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ParseApplicationPackage.IsSuccess");
    }

    BOOST_AUTO_TEST_CASE(ApplicationPackageWithDiagnosticsTest)
    {
        ApplicationPackageDescription applicationPackage;

        auto error = Parser::ParseApplicationPackage(
            GetTestFilePath(L"ApplicationPackageWithDiagnostics.1.xml"),
            applicationPackage);

        Trace.WriteNoise(TestSource, "ParseApplicationPackage={0}", error);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ParseApplicationPackage.IsSuccess");
    }


    BOOST_AUTO_TEST_CASE(ConfigSettingsBasicTest)
    {
        ParseConfigSettings(GetTestFilePath(L"ConfigSettings-BasicTest-1.xml"));
        ParseConfigSettings(GetTestFilePath(L"ConfigSettings-BasicTest-2.xml")); 
        ParseConfigSettings(GetTestFilePath(L"ConfigSettings-BasicTest-3.xml")); 
        ParseConfigSettings(GetTestFilePath(L"ConfigSettings-BasicTest-4.xml")); 
    }


    BOOST_AUTO_TEST_CASE(TargetInformationFileTest)
    {
        ParseAndValidateTargetInformationFile(GetTestFilePath(L"TargetInformation.Valid.xml"), true, true);
        ParseAndValidateTargetInformationFile(GetTestFilePath(L"TargetInformation.NoCurrentInstallation.xml"), false, true);
        ParseAndValidateTargetInformationFile(GetTestFilePath(L"TargetInformation.NoTargetInstallation.xml"), true, false);
        ParseAndValidateTargetInformationFile(GetTestFilePath(L"TargetInformation.Empty.xml"), false, false);
    }

    BOOST_AUTO_TEST_SUITE_END()

    void ServiceModelParserTest::ApplicationHealthPolicyTestInternal(wstring const & testFileName, ApplicationHealthPolicy const & expectedPolicy)
    {
        Trace.WriteNoise(TestSource, "ApplicationHealthPolicyTest(TestFileName={0}, ExpectedHPolicy={1}", testFileName, expectedPolicy);

        ApplicationPackageDescription appPackage;
        auto error = Parser::ParseApplicationPackage(testFileName, appPackage);
        Trace.WriteNoise(TestSource, "ParseApplicationPackage={0}", error);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ParseApplicationPackage.IsSuccess");

        Trace.WriteNoise(TestSource, "ParsedPolicy={0}", appPackage.DigestedEnvironment.Policies.HealthPolicy);
        Trace.WriteNoise(TestSource, "ExpectedPolicy={0}", expectedPolicy);
        VERIFY_IS_TRUE(expectedPolicy == appPackage.DigestedEnvironment.Policies.HealthPolicy, L"Parsed Policy == Expected Policy");
    }

    void ServiceModelParserTest::ApplicationPoliciesTestInternal(wstring const & testFileName, ApplicationPoliciesDescription const & expectedPolicy)
    {
        Trace.WriteNoise(TestSource, "ApplicationPoliciesTest(TestFileName={0}, ExpectedPoliciesDescription={1}", testFileName, expectedPolicy);

        ApplicationPackageDescription appPackage;
        auto error = Parser::ParseApplicationPackage(testFileName, appPackage);
        Trace.WriteNoise(TestSource, "ParseApplicationPackage={0}", error);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ParseApplicationPackage.IsSuccess");

        Trace.WriteNoise(TestSource, "ParsedPolicy={0}", appPackage.DigestedEnvironment.Policies);
        Trace.WriteNoise(TestSource, "ExpectedPolicy={0}", expectedPolicy);
        VERIFY_IS_TRUE(expectedPolicy == appPackage.DigestedEnvironment.Policies, L"Parsed Policy == Expected Policy");
    }

    wstring ServiceModelParserTest::GetTestFilePath(wstring const & testFileName)
    {
        wstring folder = Directory::GetCurrentDirectoryW();
        wstring filePath = Path::Combine(folder, testFileName);

        if (!File::Exists(filePath))
        {
            wstring unitTestsFolder;
#if !defined(PLATFORM_UNIX)
            VERIFY_IS_TRUE(
                Environment::ExpandEnvironmentStringsW(L"%_NTTREE%\\FabricUnitTests", unitTestsFolder),
                L"ExpandEnvironmentStringsW");
#else
            VERIFY_IS_TRUE(
                Environment::ExpandEnvironmentStringsW(L"%_NTTREE%/FabricUnitTests", unitTestsFolder),
                L"ExpandEnvironmentStringsW");
#endif

            return Path::Combine(unitTestsFolder, testFileName);
        }
        else
        {
            return filePath;
        }
    }

    void ServiceModelParserTest::ParseConfigSettings(wstring const & testFileName)
    {
        Trace.WriteInfo(TestSource, "Parsing Config Settings from {0}", testFileName);

        // parse the config settings
        ConfigSettings parsedConfigSettings;
        auto error = Parser::ParseConfigSettings(testFileName, parsedConfigSettings);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ParseConfigSettings");
    }


    void ServiceModelParserTest::ParseAndValidateTargetInformationFile(wstring const & fileName, bool expectValidCurrentInstallation, bool expectValidTargetInstallation)
    {
        Trace.WriteInfo(TestSource, "Parsing target information file {0}", fileName);

        // parse the config settings
        TargetInformationFileDescription desc;
        auto error = desc.FromXml(fileName);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ParseAndValidateTargetInformationFile");
        VERIFY_ARE_EQUAL(expectValidCurrentInstallation, desc.CurrentInstallation.IsValid);
        if (expectValidCurrentInstallation)
        {
            VERIFY_ARE_EQUAL(L"130575528337412373", desc.CurrentInstallation.InstanceId);
            VERIFY_ARE_EQUAL(L"3.1.204.9488", desc.CurrentInstallation.TargetVersion);
            VERIFY_ARE_EQUAL(L"C:\\ProgramData\\Windows Fabric\\Node3\\Fabric\\work\\FabricUpgrade\\WindowsFabric.3.1.204.9488", desc.CurrentInstallation.MSILocation);
            VERIFY_ARE_EQUAL(L"C:\\ProgramData\\Windows Fabric\\Node3\\Fabric\\work\\FabricUpgrade\\ClusterManifest.1.0.xml", desc.CurrentInstallation.ClusterManifestLocation);
            VERIFY_ARE_EQUAL(L"Node3", desc.CurrentInstallation.NodeName);
            VERIFY_IS_TRUE(desc.CurrentInstallation.InfrastructureManifestLocation.empty());
            VERIFY_ARE_EQUAL(L"FabricSetup.exe", desc.CurrentInstallation.UpgradeEntryPointExe);
            VERIFY_ARE_EQUAL(L"/operation:upgrade", desc.CurrentInstallation.UpgradeEntryPointExeParameters);
            VERIFY_ARE_EQUAL(L"FabricSetup.exe", desc.CurrentInstallation.UndoUpgradeEntryPointExe);
            VERIFY_ARE_EQUAL(L"/operation:undoupgrade", desc.CurrentInstallation.UndoUpgradeEntryPointExeParameters);
        }

        VERIFY_ARE_EQUAL(expectValidTargetInstallation, desc.TargetInstallation.IsValid);
        if (expectValidTargetInstallation)
        {
            VERIFY_ARE_EQUAL(L"130575528337412370", desc.TargetInstallation.InstanceId);
            VERIFY_ARE_EQUAL(L"3.1.205.9488", desc.TargetInstallation.TargetVersion);
            VERIFY_ARE_EQUAL(L"C:\\ProgramData\\Windows Fabric\\Node3\\Fabric\\work\\FabricUpgrade\\WindowsFabric.3.1.205.9488.msi", desc.TargetInstallation.MSILocation);
            VERIFY_ARE_EQUAL(L"C:\\ProgramData\\Windows Fabric\\Node3\\Fabric\\work\\FabricUpgrade\\ClusterManifest.1.0.xml", desc.TargetInstallation.ClusterManifestLocation);
            VERIFY_ARE_EQUAL(L"Node3", desc.TargetInstallation.NodeName);
            VERIFY_ARE_EQUAL(L"TestIM", desc.TargetInstallation.InfrastructureManifestLocation);
            VERIFY_IS_TRUE(desc.TargetInstallation.UpgradeEntryPointExe.empty());
            VERIFY_IS_TRUE(desc.TargetInstallation.UpgradeEntryPointExeParameters.empty());
            VERIFY_IS_TRUE(desc.TargetInstallation.UndoUpgradeEntryPointExe.empty());
            VERIFY_IS_TRUE(desc.TargetInstallation.UndoUpgradeEntryPointExeParameters.empty());
        }
    }
}
