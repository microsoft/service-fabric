// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"


//Roundtrip Tests for serialization and parsing of service model classes

namespace ServiceModelTests
{
    using namespace std;
    using namespace Common;
    using namespace ServiceModel;

    StringLiteral const TestSource("ServiceModelSerializerTest");

    class ServiceModelSerializerTest
    {
    protected:
        void generateBaseServiceManifest(ServiceManifestDescription & manifest);
        void validateBaseServiceManifest(ServiceManifestDescription & manifest);

        void generateAllServiceType(ServiceManifestDescription & manifest);
        void generateBaseStatefulServiceType(ServiceTypeDescription & service);
        void generateFullStatelessServiceType(ServiceTypeDescription & service);
        void generateFullStatefulServiceGroupType(ServiceGroupTypeDescription & service);
        void generateBaseStatelessServiceGroupType(ServiceGroupTypeDescription & service);
        
        void generateManyCodeConfigData(ServiceManifestDescription & manifest);
        void validateManyCodeConfigData(ServiceManifestDescription & manifest);

        void validateAllServiceType(ServiceManifestDescription & manifest);
        void validateBaseStatefulServiceType(ServiceTypeDescription & service);
        void validateFullStatelessServiceType(ServiceTypeDescription & service);
        void validateFullStatefulServiceGroupType(ServiceGroupTypeDescription & service);
        void validateBaseStatelessServiceGroupType(ServiceGroupTypeDescription & service);

        void generateServiceDiagnostics(ServiceManifestDescription & manifest);
        void generateServiceResources(ServiceManifestDescription & manifest);

        void validateServiceDiagnostics(ServiceManifestDescription & manifest);
        void validateServiceResources(ServiceManifestDescription & manifest);
        
        void generateBaseApplicationManifest(ApplicationManifestDescription & manifest);
        void validateBaseApplicationManifest(ApplicationManifestDescription & manifest);

        void generateApplicationServices(ApplicationManifestDescription & manifest);
        void validateApplicationServices(ApplicationManifestDescription & manifest);

        void generateStatelessApplicationService(ApplicationServiceTemplateDescription & service);
        void generateStatefulGroupApplicationService(ApplicationServiceTemplateDescription & service);

        void validateStatelessApplicationService(ApplicationServiceTemplateDescription & service);
        void validateStatefulGroupApplicationService(ApplicationServiceTemplateDescription & service);

        void generateParametersApplication(ApplicationManifestDescription & manifest);
        void validateParametersApplication(ApplicationManifestDescription & manifest);

        void generatePoliciesApplication(ApplicationManifestDescription & manifest);
        void validatePoliciesApplication(ApplicationManifestDescription & manifest);

        void generateDiagnosticsApplication(ApplicationManifestDescription & manifest);
        void validateDiagnosticsApplication(ApplicationManifestDescription & manifest);

        void generateCertificatesApplication(ApplicationManifestDescription & manifest);
        void validateCertificatesApplication(ApplicationManifestDescription & manifest);

        void generatePrincipalsApplication(ApplicationManifestDescription & manifest);
        void validatePrincipalsApplication(ApplicationManifestDescription & manifest);

        void generateBaseServicePackage(ServicePackageDescription & package);
        void validateBaseServicePackage(ServicePackageDescription & package);

        void generateConfigDataServicePackage(ServicePackageDescription & package);
        void validateConfigDataServicePackage(ServicePackageDescription & package);

        void generateApplicationPackage(ApplicationPackageDescription & package);
        void validateApplicationPackage(ApplicationPackageDescription & package);

        void generateApplicationInstance(ApplicationInstanceDescription & instance);
        void validateApplicationInstance(ApplicationInstanceDescription & instance);
    };
    
    void ServiceModelSerializerTest::generateBaseServiceManifest(ServiceManifestDescription & manifest)
    {
        manifest.clear();
        manifest.Name = L"ServiceManifestDescription";
        manifest.Version = L"ServiceManifestVersion1";


        //Generate a ServiceTypeDescription
        ServiceTypeDescription serviceType;
        generateBaseStatefulServiceType(serviceType);
        //add service type to manifest
        manifest.ServiceTypeNames.push_back(serviceType);

        //Make ExeHost for code package
        ExeEntryPointDescription SetupEntryPoint;
        SetupEntryPoint.Program = L"ExeProgram1";
        SetupEntryPoint.Arguments = L"Args1";
        SetupEntryPoint.WorkingFolder = ExeHostWorkingFolder::Enum::CodeBase;
        SetupEntryPoint.ConsoleRedirectionEnabled = true;
        SetupEntryPoint.ConsoleRedirectionFileRetentionCount = 4;
        SetupEntryPoint.ConsoleRedirectionFileMaxSizeInKb = 44;
        SetupEntryPoint.PeriodicIntervalInSeconds = 15;

        CodePackageDescription cpd;
        cpd.Name = L"cpdName";
        cpd.EntryPoint.EntryPointType = EntryPointType::Enum::Exe;
        cpd.EntryPoint.ExeEntryPoint = SetupEntryPoint;

        //add code package to manifest
        manifest.CodePackages.push_back(cpd);
    }

    void ServiceModelSerializerTest::validateBaseServiceManifest(ServiceManifestDescription & manifest){
        VERIFY_ARE_EQUAL(manifest.Name, L"ServiceManifestDescription");
        VERIFY_ARE_EQUAL(manifest.Version, L"ServiceManifestVersion1");

        ServiceTypeDescription serviceType = manifest.ServiceTypeNames[0];
        validateBaseStatefulServiceType(serviceType);

        CodePackageDescription cpd = manifest.CodePackages[0];
        VERIFY_ARE_EQUAL(cpd.Name, L"cpdName");
        //Make ExeHost for code package
        ExeEntryPointDescription SetupEntryPoint = cpd.EntryPoint.ExeEntryPoint;
        VERIFY_ARE_EQUAL(SetupEntryPoint.Program, L"ExeProgram1");
        VERIFY_ARE_EQUAL(SetupEntryPoint.Arguments, L"Args1");
        VERIFY_ARE_EQUAL(SetupEntryPoint.WorkingFolder, ExeHostWorkingFolder::Enum::CodeBase);
        VERIFY_ARE_EQUAL(SetupEntryPoint.ConsoleRedirectionEnabled, true);
        VERIFY_ARE_EQUAL(SetupEntryPoint.ConsoleRedirectionFileRetentionCount, 4);
        VERIFY_ARE_EQUAL(SetupEntryPoint.ConsoleRedirectionFileMaxSizeInKb, 44);
        VERIFY_ARE_EQUAL(SetupEntryPoint.PeriodicIntervalInSeconds, 15);


        VERIFY_ARE_EQUAL(cpd.EntryPoint.EntryPointType, EntryPointType::Enum::Exe);
    }

    void ServiceModelSerializerTest::generateAllServiceType(ServiceManifestDescription & manifestDescription)
    {
        generateBaseServiceManifest(manifestDescription);
        ServiceTypeDescription fullStateless;
        generateFullStatelessServiceType(fullStateless);
        ServiceGroupTypeDescription fullGroupStateful;
        generateFullStatefulServiceGroupType(fullGroupStateful);
        ServiceGroupTypeDescription baseGroupStateless;
        generateBaseStatelessServiceGroupType(baseGroupStateless);
        
        manifestDescription.ServiceTypeNames.push_back(fullStateless);
        manifestDescription.ServiceGroupTypes.push_back(fullGroupStateful);
        manifestDescription.ServiceGroupTypes.push_back(baseGroupStateless);
    }

    void ServiceModelSerializerTest::generateBaseStatefulServiceType(ServiceTypeDescription & serviceType)
    {
        serviceType.clear();
        serviceType.ServiceTypeName = L"ServiceTypeName";
        serviceType.HasPersistedState = true;
        serviceType.IsStateful = true;
        serviceType.PlacementConstraints = L"PlacementConstraints";
    }
    void ServiceModelSerializerTest::validateBaseStatefulServiceType(ServiceTypeDescription & serviceType)
    {
        VERIFY_ARE_EQUAL(serviceType.ServiceTypeName, L"ServiceTypeName");
        VERIFY_IS_TRUE(serviceType.HasPersistedState);
        VERIFY_IS_TRUE(serviceType.IsStateful);
        VERIFY_ARE_EQUAL(serviceType.PlacementConstraints, L"PlacementConstraints");
    }
    void ServiceModelSerializerTest::generateFullStatelessServiceType(ServiceTypeDescription & serviceType)
    {
        //Create stateless ServiceType
        serviceType.clear();
        serviceType.IsStateful = false;
        serviceType.ServiceTypeName = L"StatelessType";
        serviceType.UseImplicitHost = false;
        serviceType.PlacementConstraints = L"PlacementConstraint";

        ServiceLoadMetricDescription firstLoadMetric;
        firstLoadMetric.Name = L"FirstLoadMetric";
        firstLoadMetric.PrimaryDefaultLoad = 11;
        firstLoadMetric.SecondaryDefaultLoad = 111;
        firstLoadMetric.Weight = WeightType::High;

        ServiceLoadMetricDescription secondLoadMetric;
        secondLoadMetric.Name = L"SecondLoadMetric";
        secondLoadMetric.PrimaryDefaultLoad = 22;
        secondLoadMetric.SecondaryDefaultLoad = 222;
        secondLoadMetric.Weight = WeightType::Low;

        serviceType.LoadMetrics.push_back(firstLoadMetric);
        serviceType.LoadMetrics.push_back(secondLoadMetric);

        ServicePlacementPolicyDescription firstPolicyDescription;
        firstPolicyDescription.domainName_ = L"FirstPolicyType";
        firstPolicyDescription.type_ = FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN;

        ServicePlacementPolicyDescription secondPolicyDescription;
        secondPolicyDescription.domainName_ = L"SecondPolicyType";
        secondPolicyDescription.type_ = FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN;

        serviceType.ServicePlacementPolicies.push_back(firstPolicyDescription);
        serviceType.ServicePlacementPolicies.push_back(secondPolicyDescription);
    }
    void ServiceModelSerializerTest::validateFullStatelessServiceType(ServiceTypeDescription & serviceType)
    {
        VERIFY_IS_FALSE(serviceType.IsStateful);
        VERIFY_ARE_EQUAL(serviceType.ServiceTypeName, L"StatelessType");
        VERIFY_IS_FALSE(serviceType.UseImplicitHost);
        VERIFY_ARE_EQUAL(serviceType.PlacementConstraints, L"PlacementConstraint");

        VERIFY_ARE_EQUAL(2, serviceType.LoadMetrics.size());
        ServiceLoadMetricDescription firstLoadMetric = serviceType.LoadMetrics[0];
        VERIFY_ARE_EQUAL(firstLoadMetric.Name, L"FirstLoadMetric");
        VERIFY_ARE_EQUAL(firstLoadMetric.PrimaryDefaultLoad, 11);
        VERIFY_ARE_EQUAL(firstLoadMetric.SecondaryDefaultLoad, 111);
        VERIFY_ARE_EQUAL(firstLoadMetric.Weight, WeightType::High);

        ServiceLoadMetricDescription secondLoadMetric= serviceType.LoadMetrics[1];
        VERIFY_ARE_EQUAL(secondLoadMetric.Name, L"SecondLoadMetric");
        VERIFY_ARE_EQUAL(secondLoadMetric.PrimaryDefaultLoad, 22);
        VERIFY_ARE_EQUAL(secondLoadMetric.SecondaryDefaultLoad, 222);
        VERIFY_ARE_EQUAL(secondLoadMetric.Weight, WeightType::Low);

    
        VERIFY_ARE_EQUAL(2, serviceType.ServicePlacementPolicies.size());
        ServicePlacementPolicyDescription firstPolicyDescription = serviceType.ServicePlacementPolicies[0];
        VERIFY_ARE_EQUAL(firstPolicyDescription.domainName_, L"FirstPolicyType");
        VERIFY_ARE_EQUAL(firstPolicyDescription.type_, FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN);

        ServicePlacementPolicyDescription secondPolicyDescription = serviceType.ServicePlacementPolicies[1];
        VERIFY_ARE_EQUAL(secondPolicyDescription.domainName_, L"SecondPolicyType");
        VERIFY_ARE_EQUAL(secondPolicyDescription.type_, FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN);

    }
    void ServiceModelSerializerTest::generateFullStatefulServiceGroupType(ServiceGroupTypeDescription & service)
    {
        service.clear();
        service.Description.ServiceTypeName = L"FullStatefulServiceGroupType";
        service.Description.IsStateful = true;
        service.UseImplicitFactory = true;
        service.Description.HasPersistedState = true;

        service.Description.PlacementConstraints = L"ServiceGroupTypePlacementConstraint";
        
        ServiceLoadMetricDescription loadMetric;
        loadMetric.Name = L"LoadMetric";
        loadMetric.PrimaryDefaultLoad = 44;
        loadMetric.SecondaryDefaultLoad = 404;
        loadMetric.Weight = WeightType::High;


        service.Description.LoadMetrics.push_back(loadMetric);
        
        
        DescriptionExtension firstExtension;
        firstExtension.Name = L"Name1";

        DescriptionExtension secondExtension;
        secondExtension.Name = L"Name2";

        service.Description.Extensions.push_back(firstExtension);
        service.Description.Extensions.push_back(secondExtension);

        
        ServiceGroupTypeMemberDescription firstGroupType;
        firstGroupType.ServiceTypeName = L"ServiceGroupTypeName";
        
        ServiceGroupTypeMemberDescription secondGroupType;
        secondGroupType.ServiceTypeName = L"2ServiceGroupTypeName";

        ServiceLoadMetricDescription memberLoadMetric;
        memberLoadMetric.Name = L"LoadMetricMember";

        ServiceLoadMetricDescription memberLoadMetric2;
        memberLoadMetric2.Name = L"LoadMetricMember2";

        secondGroupType.LoadMetrics.push_back(memberLoadMetric);
        secondGroupType.LoadMetrics.push_back(memberLoadMetric2);


        service.Members.push_back(firstGroupType);
        service.Members.push_back(secondGroupType);
    }

    void ServiceModelSerializerTest::validateFullStatefulServiceGroupType(ServiceGroupTypeDescription & service)
    {
        VERIFY_ARE_EQUAL(service.Description.ServiceTypeName, L"FullStatefulServiceGroupType");
        VERIFY_IS_TRUE(service.Description.IsStateful);
        VERIFY_IS_TRUE(service.UseImplicitFactory);
        VERIFY_IS_TRUE(service.Description.HasPersistedState);

        VERIFY_ARE_EQUAL(service.Description.PlacementConstraints, L"ServiceGroupTypePlacementConstraint");

        ServiceLoadMetricDescription loadMetric = service.Description.LoadMetrics[0];
        VERIFY_ARE_EQUAL(loadMetric.Name, L"LoadMetric");
        VERIFY_ARE_EQUAL(loadMetric.PrimaryDefaultLoad, 44);
        VERIFY_ARE_EQUAL(loadMetric.SecondaryDefaultLoad, 404);
        VERIFY_ARE_EQUAL(loadMetric.Weight, WeightType::High);

        VERIFY_ARE_EQUAL(2, service.Description.Extensions.size());
        DescriptionExtension firstExtension = service.Description.Extensions[0];
        VERIFY_ARE_EQUAL(firstExtension.Name, L"Name1");
        DescriptionExtension secondExtension = service.Description.Extensions[1];
        VERIFY_ARE_EQUAL(secondExtension.Name, L"Name2");

        VERIFY_ARE_EQUAL(2, service.Members.size());
        ServiceGroupTypeMemberDescription firstGroupType = service.Members[0];
        VERIFY_ARE_EQUAL(firstGroupType.ServiceTypeName, L"ServiceGroupTypeName");
        ServiceGroupTypeMemberDescription secondGroupType = service.Members[1];
        VERIFY_ARE_EQUAL(secondGroupType.ServiceTypeName, L"2ServiceGroupTypeName");

        VERIFY_ARE_EQUAL(2, secondGroupType.LoadMetrics.size());
        ServiceLoadMetricDescription memberLoadMetric = secondGroupType.LoadMetrics[0];
        VERIFY_ARE_EQUAL(memberLoadMetric.Name, L"LoadMetricMember");
        ServiceLoadMetricDescription memberLoadMetric2 = secondGroupType.LoadMetrics[1];
        VERIFY_ARE_EQUAL(memberLoadMetric2.Name, L"LoadMetricMember2");
    }

    void ServiceModelSerializerTest::generateBaseStatelessServiceGroupType(ServiceGroupTypeDescription & statelessGroupType)
    {
        statelessGroupType.clear();
        statelessGroupType.UseImplicitFactory = true;
        statelessGroupType.Description.ServiceTypeName = L"FullStatefulServiceGroupType";
    }

    void ServiceModelSerializerTest::validateBaseStatelessServiceGroupType(ServiceGroupTypeDescription & service)
    {
        VERIFY_IS_TRUE(service.UseImplicitFactory);
        VERIFY_ARE_EQUAL(service.Description.ServiceTypeName, L"FullStatefulServiceGroupType");
    }

    void ServiceModelSerializerTest::validateAllServiceType(ServiceManifestDescription & manifest)
    {
        VERIFY_ARE_EQUAL(manifest.ServiceGroupTypes.size(), 2);
        VERIFY_ARE_EQUAL(manifest.ServiceTypeNames.size(), 2);
        this->validateBaseServiceManifest(manifest);
        this->validateFullStatelessServiceType(manifest.ServiceTypeNames[1]);
        this->validateFullStatefulServiceGroupType(manifest.ServiceGroupTypes[0]);
        this->validateBaseStatelessServiceGroupType(manifest.ServiceGroupTypes[1]);
    }

    void ServiceModelSerializerTest::generateManyCodeConfigData(ServiceManifestDescription & manifest)
    {
        generateBaseServiceManifest(manifest);

        CodePackageDescription code;
        code.HasSetupEntryPoint = true;
        
        code.Name = L"CodePackage2";
        code.Version = L"DllEntryCodePackage";
        code.IsShared = false;

        code.SetupEntryPoint.Program = L"SetupExeProgram";
        code.SetupEntryPoint.Arguments = L"Setup exe arguments";
        code.SetupEntryPoint.WorkingFolder = ExeHostWorkingFolder::CodeBase;
        code.SetupEntryPoint.ConsoleRedirectionEnabled = true;
        code.SetupEntryPoint.ConsoleRedirectionFileMaxSizeInKb = 44;
        code.SetupEntryPoint.ConsoleRedirectionFileRetentionCount = 4;

        DllHostEntryPointDescription dllHost;
        code.EntryPoint.EntryPointType = EntryPointType::DllHost;

        dllHost.IsolationPolicyType = CodePackageIsolationPolicyType::DedicatedDomain;

        DllHostHostedUnmanagedDllDescription Unmanaged;
        Unmanaged.DllName = L"UnmanagedDLLName";
        DllHostHostedManagedDllDescription Managed;
        Managed.AssemblyName = L"ManagedDllName";

        DllHostHostedDllDescription unmanagedDescription;
        unmanagedDescription.Unmanaged = Unmanaged;
        unmanagedDescription.Kind = DllHostHostedDllKind::Unmanaged;
        DllHostHostedDllDescription managedDescription;
        managedDescription.Managed = Managed;
        managedDescription.Kind = DllHostHostedDllKind::Managed;

        dllHost.HostedDlls.push_back(unmanagedDescription);
        dllHost.HostedDlls.push_back(managedDescription);

        code.EntryPoint.DllHostEntryPoint = dllHost;

        ConfigPackageDescription config1;
        config1.Name = L"ConfigName1";
        config1.Version = L"ConfigVersion1";

        ConfigPackageDescription config2;
        config2.Name = L"ConfigName2";
        config2.Version = L"ConfigVersion2";

        manifest.ConfigPackages.push_back(config1);
        manifest.ConfigPackages.push_back(config2);

        manifest.CodePackages.push_back(code);

        DataPackageDescription data1;
        data1.Name = L"DataName1";
        data1.Version = L"DataVersion1";

        DataPackageDescription data2;
        data2.Name = L"DataName2";
        data2.Version = L"DataVersion2";

        manifest.DataPackages.push_back(data1);
        manifest.DataPackages.push_back(data2);
    }

    void ServiceModelSerializerTest::validateManyCodeConfigData(ServiceManifestDescription & manifest)
    {
        validateBaseServiceManifest(manifest);

        VERIFY_IS_TRUE(manifest.CodePackages.size() > 1);
        CodePackageDescription code = manifest.CodePackages[1];
        VERIFY_ARE_EQUAL(code.Name, L"CodePackage2");
        VERIFY_ARE_EQUAL(code.Version, L"DllEntryCodePackage");
        VERIFY_ARE_EQUAL(code.IsShared, false);

        VERIFY_ARE_EQUAL(code.SetupEntryPoint.Program, L"SetupExeProgram");
        VERIFY_ARE_EQUAL(code.SetupEntryPoint.Arguments, L"Setup exe arguments");
        VERIFY_ARE_EQUAL(code.SetupEntryPoint.WorkingFolder, ExeHostWorkingFolder::CodeBase);
        VERIFY_ARE_EQUAL(code.SetupEntryPoint.ConsoleRedirectionEnabled, true);
        VERIFY_ARE_EQUAL(code.SetupEntryPoint.ConsoleRedirectionFileMaxSizeInKb, 44);
        VERIFY_ARE_EQUAL(code.SetupEntryPoint.ConsoleRedirectionFileRetentionCount, 4);

        DllHostEntryPointDescription dllHost = code.EntryPoint.DllHostEntryPoint;
        VERIFY_ARE_EQUAL(code.EntryPoint.EntryPointType, EntryPointType::DllHost);
        VERIFY_ARE_EQUAL(dllHost.IsolationPolicyType, CodePackageIsolationPolicyType::DedicatedDomain);

        VERIFY_IS_TRUE(dllHost.HostedDlls.size() > 1);
        DllHostHostedDllDescription unmanagedDescription = dllHost.HostedDlls[0];
        VERIFY_ARE_EQUAL(unmanagedDescription.Kind, DllHostHostedDllKind::Unmanaged);
        DllHostHostedDllDescription managedDescription = dllHost.HostedDlls[1];
        VERIFY_ARE_EQUAL(managedDescription.Kind, DllHostHostedDllKind::Managed);

        DllHostHostedUnmanagedDllDescription Unmanaged = unmanagedDescription.Unmanaged;
        VERIFY_ARE_EQUAL(Unmanaged.DllName, L"UnmanagedDLLName");
        DllHostHostedManagedDllDescription Managed = managedDescription.Managed;
        VERIFY_ARE_EQUAL(Managed.AssemblyName, L"ManagedDllName");

        VERIFY_ARE_EQUAL(manifest.ConfigPackages.size(), 2);
        ConfigPackageDescription config1 = manifest.ConfigPackages[0];
        VERIFY_ARE_EQUAL(config1.Name, L"ConfigName1");
        VERIFY_ARE_EQUAL(config1.Version, L"ConfigVersion1");

        ConfigPackageDescription config2 = manifest.ConfigPackages[1];
        VERIFY_ARE_EQUAL(config2.Name, L"ConfigName2");
        VERIFY_ARE_EQUAL(config2.Version, L"ConfigVersion2");

        VERIFY_ARE_EQUAL(manifest.DataPackages.size(), 2);

        DataPackageDescription data1 = manifest.DataPackages[0];
        VERIFY_ARE_EQUAL(data1.Name, L"DataName1");
        VERIFY_ARE_EQUAL(data1.Version, L"DataVersion1");

        DataPackageDescription data2 = manifest.DataPackages[1];
        VERIFY_ARE_EQUAL(data2.Name, L"DataName2");
        VERIFY_ARE_EQUAL(data2.Version, L"DataVersion2");
    }

    void ServiceModelSerializerTest::generateServiceDiagnostics(ServiceManifestDescription & manifest)
    {
        generateBaseServiceManifest(manifest);
        manifest.Diagnostics.EtwDescription.ProviderGuids.push_back(L"FirstGuid");
        manifest.Diagnostics.EtwDescription.ProviderGuids.push_back(L"SecondGuid");

        DataPackageDescription data1, data2;
        data1.Name = L"FirstDiagnosticDataName";
        data2.Name = L"SecondDiagnosticDataName";
        data1.Version = L"Version1";
        data2.Version = L"Version2";

        manifest.Diagnostics.EtwDescription.ManifestDataPackages.push_back(data1);
        manifest.Diagnostics.EtwDescription.ManifestDataPackages.push_back(data2);
    }
    
    
    void ServiceModelSerializerTest::validateServiceDiagnostics(ServiceManifestDescription & manifest)
    {
        validateBaseServiceManifest(manifest);
        VERIFY_ARE_EQUAL(manifest.Diagnostics.EtwDescription.ProviderGuids.size(), 2);
        VERIFY_ARE_EQUAL(manifest.Diagnostics.EtwDescription.ProviderGuids[0], (L"FirstGuid"));
        VERIFY_ARE_EQUAL(manifest.Diagnostics.EtwDescription.ProviderGuids[1], (L"SecondGuid"));

        VERIFY_ARE_EQUAL(manifest.Diagnostics.EtwDescription.ManifestDataPackages.size(), 2);
        DataPackageDescription data1 = manifest.Diagnostics.EtwDescription.ManifestDataPackages[0];
        DataPackageDescription data2 = manifest.Diagnostics.EtwDescription.ManifestDataPackages[1];
        VERIFY_ARE_EQUAL(data1.Name, L"FirstDiagnosticDataName");
        VERIFY_ARE_EQUAL(data2.Name, L"SecondDiagnosticDataName");
        VERIFY_ARE_EQUAL(data1.Version, L"Version1");
        VERIFY_ARE_EQUAL(data2.Version, L"Version2");
    }
    
    void ServiceModelSerializerTest::generateServiceResources(ServiceManifestDescription & manifest)
    {
        generateBaseServiceManifest(manifest);
        EndpointDescription end1, end2, end3;
        end1.Name = L"EndPointName1";
        end2.Name = L"EndPointName2";
        end3.Name = L"EndPointName3";
        end1.Protocol = ProtocolType::Https;
        end2.Protocol = ProtocolType::Tcp;
        end3.Protocol = ProtocolType::Udp;
        end1.Type = EndpointType::Input;
        end2.Type = EndpointType::Internal;
    end1.CodePackageRef = L"CodePackageRef1";
    end2.CodePackageRef = L"";
        end1.CertificateRef = L"CertificateRef1";
        end2.CertificateRef = L"CertificateRef2";
        end3.CertificateRef = L"CertificateRef3";
        end1.Port = 4444;
        end2.Port = 9001;
        end1.ExplicitPortSpecified = true;
        end2.ExplicitPortSpecified = true;
        end3.ExplicitPortSpecified = false;
        manifest.Resources.Endpoints.push_back(end1);
        manifest.Resources.Endpoints.push_back(end2);
        manifest.Resources.Endpoints.push_back(end3);
    }


    void ServiceModelSerializerTest::validateServiceResources(ServiceManifestDescription & manifest)
    {
        validateBaseServiceManifest(manifest);
        EndpointDescription end1, end2, end3;
        VERIFY_ARE_EQUAL(manifest.Resources.Endpoints.size(), 3);
        end1 = manifest.Resources.Endpoints[0];
        end2 = manifest.Resources.Endpoints[1];
        end3 = manifest.Resources.Endpoints[2];
        VERIFY_ARE_EQUAL(end1.Name, L"EndPointName1");
        VERIFY_ARE_EQUAL(end2.Name, L"EndPointName2");
        VERIFY_ARE_EQUAL(end3.Name, L"EndPointName3");
        VERIFY_ARE_EQUAL(end1.Protocol, ProtocolType::Https);
        VERIFY_ARE_EQUAL(end2.Protocol, ProtocolType::Tcp);
        VERIFY_ARE_EQUAL(end3.Protocol, ProtocolType::Udp);
        VERIFY_ARE_EQUAL(end1.Type, EndpointType::Input);
        VERIFY_ARE_EQUAL(end2.Type, EndpointType::Internal);
    VERIFY_ARE_EQUAL(end1.CodePackageRef, L"CodePackageRef1");
    VERIFY_ARE_EQUAL(end2.CodePackageRef, L"");
        VERIFY_ARE_EQUAL(end1.CertificateRef, L"CertificateRef1");
        VERIFY_ARE_EQUAL(end2.CertificateRef, L"CertificateRef2");
        VERIFY_ARE_EQUAL(end3.CertificateRef, L"CertificateRef3");
        VERIFY_ARE_EQUAL(end1.Port, 4444);
        VERIFY_ARE_EQUAL(end2.Port, 9001);
    }

    void ServiceModelSerializerTest::generateBaseApplicationManifest(ApplicationManifestDescription & manifest)
    {
        manifest.Name = L"ApplicationManifestName";
        manifest.Version = L"ApplicationManifestVersion";
        manifest.Description = L"ApplicaitonManifestDescription";
        ServiceManifestImportDescription import1;
        import1.ServiceManifestRef.Name = L"Import1";
        import1.ServiceManifestRef.Version = L"ImportVersion1";
        ConfigOverrideDescription configOverride1;
        configOverride1.ConfigPackageName = L"ConfigPackageName1";
        
        //has all sections
        ConfigSettingsOverride configSettingsOverride;
        //has parameters
        ConfigSectionOverride configSectionOverride;
        configSectionOverride.Name = L"SectionOverride";
        ConfigParameterOverride param1;
        param1.Name = L"Param1";
        param1.Value = L"Value1";
        param1.IsEncrypted = true;
        ConfigParameterOverride param2;
        param2.Name = L"Param2";
        param2.Value = L"Value2";
        param2.IsEncrypted = false;

        configSectionOverride.Parameters[L"Param1"] = param1;
        configSectionOverride.Parameters[L"Param2"] = param2;
        configSettingsOverride.Sections[L"SectionOverride"] = configSectionOverride;
        configOverride1.SettingsOverride = configSettingsOverride;

        import1.ConfigOverrides.push_back(configOverride1);

        ConfigOverrideDescription configOverride2;
        configOverride2.ConfigPackageName = L"ConfigPackageName2";

        import1.ConfigOverrides.push_back(configOverride2);

        manifest.ServiceManifestImports.push_back(import1);

        ServiceManifestImportDescription import2;
        import2.ServiceManifestRef.Name = L"Name2";
        import2.ServiceManifestRef.Version = L"Version2";

        RunAsPolicyDescription runPolicy;
        runPolicy.CodePackageRef = L"CodePackage";
        runPolicy.UserRef = L"UserReference";
        runPolicy.EntryPointType = RunAsPolicyTypeEntryPointType::Main;

        PackageSharingPolicyDescription packagePolicy;
        packagePolicy.PackageRef = L"PackagePolicy-PackageRef";
        packagePolicy.Scope = PackageSharingPolicyTypeScope::Code;

        SecurityAccessPolicyDescription securityPolicy;
        securityPolicy.ResourceRef = L"SecurityResoruceRef";
        securityPolicy.PrincipalRef = L"SecurityPrincipalRef";
        securityPolicy.Rights = GrantAccessType::Full;
        securityPolicy.ResourceType = SecurityAccessPolicyTypeResourceType::Certificate;

        import2.RunAsPolicies.push_back(runPolicy);
        import2.PackageSharingPolicies.push_back(packagePolicy);
        import2.SecurityAccessPolicies.push_back(securityPolicy);

        manifest.ServiceManifestImports.push_back(import2);

    }
    void ServiceModelSerializerTest::validateBaseApplicationManifest(ApplicationManifestDescription & manifest)
    {
        VERIFY_ARE_EQUAL(manifest.Name, L"ApplicationManifestName");
        VERIFY_ARE_EQUAL(manifest.Version, L"ApplicationManifestVersion");
        VERIFY_ARE_EQUAL(manifest.Description, L"ApplicaitonManifestDescription");
        VERIFY_ARE_EQUAL(manifest.ServiceManifestImports.size(), 2);
        ServiceManifestImportDescription import1 = manifest.ServiceManifestImports[0];
        VERIFY_ARE_EQUAL(import1.ServiceManifestRef.Name, L"Import1");
        VERIFY_ARE_EQUAL(import1.ServiceManifestRef.Version, L"ImportVersion1");
        
        VERIFY_ARE_EQUAL(import1.ConfigOverrides.size(), 2);
        ConfigOverrideDescription configOverride1 = import1.ConfigOverrides[0];
        VERIFY_ARE_EQUAL(configOverride1.ConfigPackageName, L"ConfigPackageName1");
        ConfigOverrideDescription configOverride2 = import1.ConfigOverrides[1];
        VERIFY_ARE_EQUAL(configOverride2.ConfigPackageName, L"ConfigPackageName2");
        //has all sections
        ConfigSettingsOverride configSettingsOverride = configOverride1.SettingsOverride;
        //has parameters
        ConfigSectionOverride configSectionOverride = configSettingsOverride.Sections[L"SectionOverride"];
        VERIFY_ARE_EQUAL(configSectionOverride.Name, L"SectionOverride");
        ConfigParameterOverride param1 = configSectionOverride.Parameters[L"Param1"];
        VERIFY_ARE_EQUAL(param1.Name, L"Param1");
        VERIFY_ARE_EQUAL(param1.Value, L"Value1");
        VERIFY_ARE_EQUAL(param1.IsEncrypted, true);
        ConfigParameterOverride param2 = configSectionOverride.Parameters[L"Param2"];
        VERIFY_ARE_EQUAL(param2.Name, L"Param2");
        VERIFY_ARE_EQUAL(param2.Value, L"Value2");
        VERIFY_ARE_EQUAL(param2.IsEncrypted, false);
        
        ServiceManifestImportDescription import2 = manifest.ServiceManifestImports[1];
        VERIFY_ARE_EQUAL(import2.ServiceManifestRef.Name, L"Name2");
        VERIFY_ARE_EQUAL(import2.ServiceManifestRef.Version, L"Version2");

        RunAsPolicyDescription runPolicy = import2.RunAsPolicies[0];
        VERIFY_ARE_EQUAL(runPolicy.CodePackageRef, L"CodePackage");
        VERIFY_ARE_EQUAL(runPolicy.UserRef, L"UserReference");
        VERIFY_ARE_EQUAL(runPolicy.EntryPointType, RunAsPolicyTypeEntryPointType::Main);
        
        PackageSharingPolicyDescription packagePolicy = import2.PackageSharingPolicies[0];
        VERIFY_ARE_EQUAL(packagePolicy.PackageRef, L"PackagePolicy-PackageRef");
        VERIFY_ARE_EQUAL(packagePolicy.Scope, PackageSharingPolicyTypeScope::Code);
        
        SecurityAccessPolicyDescription securityPolicy = import2.SecurityAccessPolicies[0];
        VERIFY_ARE_EQUAL(securityPolicy.ResourceRef, L"SecurityResoruceRef");
        VERIFY_ARE_EQUAL(securityPolicy.PrincipalRef, L"SecurityPrincipalRef");
        VERIFY_ARE_EQUAL(securityPolicy.Rights, GrantAccessType::Full);
        VERIFY_ARE_EQUAL(securityPolicy.ResourceType, SecurityAccessPolicyTypeResourceType::Certificate);
        
    }

    void ServiceModelSerializerTest::generateApplicationServices(ApplicationManifestDescription & manifest)
    {
        generateBaseApplicationManifest(manifest);
        ApplicationServiceTemplateDescription service1;
        generateStatelessApplicationService(service1);
        ApplicationServiceTemplateDescription service2;
        generateStatefulGroupApplicationService(service2);

        ApplicationDefaultServiceDescription def1;
        ApplicationDefaultServiceDescription def2;
        ApplicationDefaultServiceDescription def3;

        def1.Name = L"Default1";

        def2.Name = L"Default2";
        def2.PackageActivationMode = L"SharedProcess";

        def3.Name = L"Default3";
        def3.ServiceDnsName = L"Default.3";
        def3.PackageActivationMode = L"ExclusiveProcess";

        ApplicationServiceTemplateDescription service3;
        generateStatelessApplicationService(service3);
        ApplicationServiceTemplateDescription service4;
        generateStatefulGroupApplicationService(service4);
        ApplicationServiceTemplateDescription service5;
        generateStatelessApplicationService(service5);
        
        def1.DefaultService = service3;
        def2.DefaultService = service4;
        def3.DefaultService = service5;
        
        manifest.ServiceTemplates.push_back(service1);
        manifest.ServiceTemplates.push_back(service2);

        manifest.DefaultServices.push_back(def1);
        manifest.DefaultServices.push_back(def2);
        manifest.DefaultServices.push_back(def3);
    }

    void ServiceModelSerializerTest::validateApplicationServices(ApplicationManifestDescription & manifest)
    {
        validateBaseApplicationManifest(manifest);
        validateStatelessApplicationService(manifest.ServiceTemplates[0]);
        validateStatefulGroupApplicationService(manifest.ServiceTemplates[1]);

        ApplicationDefaultServiceDescription def1 = manifest.DefaultServices[0];
        ApplicationDefaultServiceDescription def2 = manifest.DefaultServices[1];
        ApplicationDefaultServiceDescription def3 = manifest.DefaultServices[2];

        VERIFY_ARE_EQUAL(def1.Name, L"Default1");
        VERIFY_ARE_EQUAL(def1.PackageActivationMode, L"SharedProcess");

        VERIFY_ARE_EQUAL(def2.Name, L"Default2");
        VERIFY_ARE_EQUAL(def1.PackageActivationMode, L"SharedProcess");

        VERIFY_ARE_EQUAL(def3.Name, L"Default3");
        VERIFY_ARE_EQUAL(def3.ServiceDnsName, L"Default.3");
        VERIFY_ARE_EQUAL(def3.PackageActivationMode, L"ExclusiveProcess");

        ApplicationServiceTemplateDescription service3 = def1.DefaultService;
        validateStatelessApplicationService(service3);
        ApplicationServiceTemplateDescription service4 = def2.DefaultService;
        validateStatefulGroupApplicationService(service4);
        ApplicationServiceTemplateDescription service5 = def3.DefaultService;
        validateStatelessApplicationService(service5);
    }

    void ServiceModelSerializerTest::generateStatelessApplicationService(ApplicationServiceTemplateDescription & service)
    {
        service.clear();
        service.ServiceTypeName = L"StatelessApplicationService";
        service.DefaultMoveCost = FABRIC_MOVE_COST_LOW;
        service.InstanceCount = L"1";
        service.IsStateful = false;
        service.IsServiceGroup = false;
        service.Partition.Scheme = PartitionSchemeDescription::Named;
        service.Partition.PartitionNames.push_back(L"NamedPartition1");
        service.Partition.PartitionNames.push_back(L"NamedPartition2");

        ServiceLoadMetricDescription loadMetric;
        loadMetric.Name = L"LoadMetricName";
        loadMetric.Weight = WeightType::Medium;
        loadMetric.PrimaryDefaultLoad = 1;
        loadMetric.SecondaryDefaultLoad = 2;

        service.LoadMetrics.push_back(loadMetric);
        service.PlacementConstraints = L"StatelessApplicationServicePlacementConstraints";
        
        ServiceCorrelationDescription correlation1;
        correlation1.ServiceName = L"Correlation1";
        correlation1.Scheme = ServiceCorrelationScheme::AlignedAffinity;

        ServiceCorrelationDescription correlation2;
        correlation2.ServiceName = L"Correlation2";
        correlation2.Scheme = ServiceCorrelationScheme::Affinity;

        service.ServiceCorrelations.push_back(correlation1);
        service.ServiceCorrelations.push_back(correlation2);

        ServicePlacementPolicyDescription placement1;
        placement1.domainName_ = L"Placement1";
        placement1.type_ = FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN;

        ServicePlacementPolicyDescription placement2;
        placement2.domainName_ = L"Placement2";
        placement2.type_ = FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN;

        service.ServicePlacementPolicies.push_back(placement1);
        service.ServicePlacementPolicies.push_back(placement2);
    }

    void ServiceModelSerializerTest::generateStatefulGroupApplicationService(ApplicationServiceTemplateDescription & service)
    {
        service.IsStateful = true;
        service.IsServiceGroup = true;
        service.ServiceTypeName = L"StatefulGroupApplication";
        service.DefaultMoveCost = FABRIC_MOVE_COST_MEDIUM;
        service.TargetReplicaSetSize = L"4";
        service.MinReplicaSetSize = L"1";
        service.ReplicaRestartWaitDuration = L"1234";
        service.QuorumLossWaitDuration = L"5678";
        service.StandByReplicaKeepDuration = L"9012";

        service.Partition.Scheme = PartitionSchemeDescription::UniformInt64Range;
        service.Partition.PartitionCount = L"5";
        service.Partition.LowKey = L"444";
        service.Partition.HighKey = L"4444";

        ServiceGroupMemberDescription member;
        member.ServiceTypeName = L"MemberServiceTypeName";
        member.MemberName = L"MemberServiceName";

        ServiceLoadMetricDescription loadMetric;
        loadMetric.Name = L"LoadMetricName";
        loadMetric.PrimaryDefaultLoad = 5;
        loadMetric.SecondaryDefaultLoad = 6;
        loadMetric.Weight = WeightType::Medium;

        member.LoadMetrics.push_back(loadMetric);

        service.ServiceGroupMembers.push_back(member);

    }

    void ServiceModelSerializerTest::validateStatelessApplicationService(ApplicationServiceTemplateDescription & service)
    {
        VERIFY_ARE_EQUAL(service.ServiceTypeName, L"StatelessApplicationService");
        VERIFY_ARE_EQUAL(service.DefaultMoveCost, FABRIC_MOVE_COST_LOW);
        VERIFY_ARE_EQUAL(service.InstanceCount, L"1");
        VERIFY_ARE_EQUAL(service.IsStateful, false);
        VERIFY_ARE_EQUAL(service.IsServiceGroup, false);
        VERIFY_ARE_EQUAL(service.Partition.Scheme, PartitionSchemeDescription::Named);
        VERIFY_ARE_EQUAL(service.Partition.PartitionNames[0], (L"NamedPartition1"));
        VERIFY_ARE_EQUAL(service.Partition.PartitionNames[1], (L"NamedPartition2"));

        ServiceLoadMetricDescription loadMetric = service.LoadMetrics[0];
        VERIFY_ARE_EQUAL(loadMetric.Name, L"LoadMetricName");
        VERIFY_ARE_EQUAL(loadMetric.Weight, WeightType::Medium);
        VERIFY_ARE_EQUAL(loadMetric.PrimaryDefaultLoad, 1);
        VERIFY_ARE_EQUAL(loadMetric.SecondaryDefaultLoad, 2);

        VERIFY_ARE_EQUAL(service.PlacementConstraints, L"StatelessApplicationServicePlacementConstraints");

        ServiceCorrelationDescription correlation1 = service.ServiceCorrelations[0];
        ServiceCorrelationDescription correlation2 = service.ServiceCorrelations[1];
        
        VERIFY_ARE_EQUAL(correlation1.ServiceName, L"Correlation1");
        VERIFY_ARE_EQUAL(correlation1.Scheme, ServiceCorrelationScheme::AlignedAffinity);
        VERIFY_ARE_EQUAL(correlation2.ServiceName, L"Correlation2");
        VERIFY_ARE_EQUAL(correlation2.Scheme, ServiceCorrelationScheme::Affinity);

        ServicePlacementPolicyDescription placement1 = service.ServicePlacementPolicies[0];
        ServicePlacementPolicyDescription placement2 = service.ServicePlacementPolicies[1];
        VERIFY_ARE_EQUAL(placement1.domainName_, L"Placement1");
        VERIFY_ARE_EQUAL(placement1.Type, FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN);
        VERIFY_ARE_EQUAL(placement2.domainName_, L"Placement2");
        VERIFY_ARE_EQUAL(placement2.Type, FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN);

    }

    void ServiceModelSerializerTest::validateStatefulGroupApplicationService(ApplicationServiceTemplateDescription & service)
    {
        VERIFY_ARE_EQUAL(service.IsStateful, true);
        VERIFY_ARE_EQUAL(service.IsServiceGroup, true);
        VERIFY_ARE_EQUAL(service.ServiceTypeName, L"StatefulGroupApplication");
        VERIFY_ARE_EQUAL(service.DefaultMoveCost, FABRIC_MOVE_COST_MEDIUM);
        VERIFY_ARE_EQUAL(service.TargetReplicaSetSize, L"4");
        VERIFY_ARE_EQUAL(service.MinReplicaSetSize, L"1");
        VERIFY_ARE_EQUAL(service.ReplicaRestartWaitDuration, L"1234");
        VERIFY_ARE_EQUAL(service.QuorumLossWaitDuration, L"5678");
        VERIFY_ARE_EQUAL(service.StandByReplicaKeepDuration, L"9012");

        VERIFY_ARE_EQUAL(service.Partition.Scheme, PartitionSchemeDescription::UniformInt64Range);
        VERIFY_ARE_EQUAL(service.Partition.PartitionCount, L"5");
        VERIFY_ARE_EQUAL(service.Partition.LowKey, L"444");
        VERIFY_ARE_EQUAL(service.Partition.HighKey, L"4444");

        ServiceGroupMemberDescription member = service.ServiceGroupMembers[0];
        ServiceLoadMetricDescription loadMetric = member.LoadMetrics[0];
        VERIFY_ARE_EQUAL(member.ServiceTypeName, L"MemberServiceTypeName");
        VERIFY_ARE_EQUAL(member.MemberName, L"MemberServiceName");

        VERIFY_ARE_EQUAL(loadMetric.Name, L"LoadMetricName");
        VERIFY_ARE_EQUAL(loadMetric.PrimaryDefaultLoad, 5);
        VERIFY_ARE_EQUAL(loadMetric.SecondaryDefaultLoad, 6);
        VERIFY_ARE_EQUAL(loadMetric.Weight, WeightType::Medium);
    }

    void ServiceModelSerializerTest::generateParametersApplication(ApplicationManifestDescription & manifest)
    {
        generateBaseApplicationManifest(manifest);
        manifest.ApplicationParameters[L"firstParam"] = L"firstValue";
        manifest.ApplicationParameters[L"secondParam"] = L"secondParam";
    }

    void ServiceModelSerializerTest::validateParametersApplication(ApplicationManifestDescription & manifest)
    {
        validateBaseApplicationManifest(manifest);
        VERIFY_ARE_EQUAL(manifest.ApplicationParameters[L"firstParam"], L"firstValue");
        VERIFY_ARE_EQUAL(manifest.ApplicationParameters[L"secondParam"], L"secondParam");
    }

    void ServiceModelSerializerTest::generatePoliciesApplication(ApplicationManifestDescription & manifest)
    {
        generateBaseApplicationManifest(manifest);
        DefaultRunAsPolicyDescription runPolicy;
        runPolicy.UserRef = L"RunAsPolicyUserRef";

        LogCollectionPolicyDescription logPolicy;
        logPolicy.Path = L"LogPath";

        SecurityAccessPolicyDescription securityPolicy;
        securityPolicy.PrincipalRef = L"PRef";
        securityPolicy.ResourceRef = L"URef";
        securityPolicy.ResourceType = SecurityAccessPolicyTypeResourceType::Endpoint;
        securityPolicy.Rights = GrantAccessType::Read;

        ApplicationHealthPolicy healthPolicy;
        ServiceTypeHealthPolicy defaultHealthPolicy;
        defaultHealthPolicy.MaxPercentUnhealthyPartitionsPerService = 5;
        defaultHealthPolicy.MaxPercentUnhealthyReplicasPerPartition = 4;
        defaultHealthPolicy.MaxPercentUnhealthyServices = 44;

        ServiceTypeHealthPolicy otherHealthPolicy;
        otherHealthPolicy.MaxPercentUnhealthyPartitionsPerService =7;
        otherHealthPolicy.MaxPercentUnhealthyReplicasPerPartition = 8;
        otherHealthPolicy.MaxPercentUnhealthyServices = 49;
        healthPolicy.ServiceTypeHealthPolicies[L"OtherName"] = otherHealthPolicy;
        healthPolicy.DefaultServiceTypeHealthPolicy = defaultHealthPolicy;
        healthPolicy.ConsiderWarningAsError = true;

        manifest.Policies.DefaultRunAs = runPolicy;
        manifest.Policies.SecurityAccessPolicies.push_back(securityPolicy);
        manifest.Policies.LogCollectionEntries.push_back(logPolicy);
        manifest.Policies.HealthPolicy = healthPolicy;
    }

    void ServiceModelSerializerTest::validatePoliciesApplication(ApplicationManifestDescription & manifest)
    {
        validateBaseApplicationManifest(manifest);

        DefaultRunAsPolicyDescription runPolicy = manifest.Policies.DefaultRunAs;
        VERIFY_ARE_EQUAL(runPolicy.UserRef, L"RunAsPolicyUserRef");

        LogCollectionPolicyDescription logPolicy = manifest.Policies.LogCollectionEntries[0];
        VERIFY_ARE_EQUAL(logPolicy.Path, L"LogPath");

        SecurityAccessPolicyDescription securityPolicy = manifest.Policies.SecurityAccessPolicies[0];
        VERIFY_ARE_EQUAL(securityPolicy.PrincipalRef, L"PRef");
        VERIFY_ARE_EQUAL(securityPolicy.ResourceRef, L"URef");
        VERIFY_ARE_EQUAL(securityPolicy.ResourceType, SecurityAccessPolicyTypeResourceType::Endpoint);
        VERIFY_ARE_EQUAL(securityPolicy.Rights, GrantAccessType::Read);

        ApplicationHealthPolicy healthPolicy = manifest.Policies.HealthPolicy;
        ServiceTypeHealthPolicy defaultHealthPolicy = healthPolicy.DefaultServiceTypeHealthPolicy;
        VERIFY_ARE_EQUAL(defaultHealthPolicy.MaxPercentUnhealthyPartitionsPerService, 5);
        VERIFY_ARE_EQUAL(defaultHealthPolicy.MaxPercentUnhealthyReplicasPerPartition, 4);
        VERIFY_ARE_EQUAL(defaultHealthPolicy.MaxPercentUnhealthyServices, 44);
        VERIFY_ARE_EQUAL(healthPolicy.ConsiderWarningAsError, true);
        ServiceTypeHealthPolicy otherHealthPolicy = healthPolicy.ServiceTypeHealthPolicies[L"OtherName"];
        VERIFY_ARE_EQUAL(otherHealthPolicy.MaxPercentUnhealthyPartitionsPerService, 7);
        VERIFY_ARE_EQUAL(otherHealthPolicy.MaxPercentUnhealthyReplicasPerPartition, 8);
        VERIFY_ARE_EQUAL(otherHealthPolicy.MaxPercentUnhealthyServices, 49);
    }

    void ServiceModelSerializerTest::generateDiagnosticsApplication(ApplicationManifestDescription & manifest)
    {
        generateBaseApplicationManifest(manifest);

        CrashDumpSourceDescription crashDump;
        crashDump.IsEnabled = L"true";

        ParameterDescription cparam1;
        cparam1.IsEncrypted = L"true";
        cparam1.Name = L"CParamName";
        cparam1.Value = L"CParamValue";
        crashDump.Parameters.Parameter.push_back(cparam1);

        LocalStoreDescription cLocalStore;
        cLocalStore.RelativeFolderPath = L"CrashDumpLocalStorePath";
        cLocalStore.DataDeletionAgeInDays = L"Days";
        
        ParameterDescription cLParam1;
        cLParam1.IsEncrypted = L"true";
        cLParam1.Name = L"CLParamName";
        cLParam1.Value = L"CLParamValue";
        cLocalStore.Parameters.Parameter.push_back(cLParam1);

        FileStoreDescription fileStore;
        fileStore.IsEnabled = L"true";
        fileStore.Path = L"FPath";
        fileStore.UploadIntervalInMinutes = L"5";
        fileStore.DataDeletionAgeInDays = L"6";
        fileStore.AccountType = L"FileStoreAccountType";
        fileStore.AccountName = L"FSAccountName";
        fileStore.Password = L"PlainTextPassword";
        fileStore.PasswordEncrypted = L"false";

        AzureBlobDescription cAzureBlob;
        cAzureBlob.IsEnabled = L"true";
        cAzureBlob.ConnectionString = L"PlainText";
        cAzureBlob.ConnectionStringIsEncrypted = L"no";

        crashDump.Destinations.LocalStore.push_back(cLocalStore);
        crashDump.Destinations.FileStore.push_back(fileStore);
        crashDump.Destinations.AzureBlob.push_back(cAzureBlob);

        manifest.Diagnostics.CrashDumpSource = crashDump;

        ETWSourceDescription etw;
        
        LocalStoreETWDescription etwLocal;
        etwLocal.LevelFilter = L"ETWLevelFilter";
        etwLocal.RelativeFolderPath = L"ETWRelativePath";
        etwLocal.DataDeletionAgeInDays = L"5";

        FileStoreETWDescription etwFile;
        etwFile.DataDeletionAgeInDays = L"4";
        etwFile.LevelFilter = L"6";
        etwFile.Path = L"path";

        AzureBlobETWDescription etwAzure;
        etwAzure.ConnectionString = L"plaintext";
        etwAzure.ConnectionStringIsEncrypted = L"false";
        etwAzure.LevelFilter = L"AzureLevelFilter";
        
        etw.Destinations.LocalStore.push_back(etwLocal);
        etw.Destinations.FileStore.push_back(etwFile);
        etw.Destinations.AzureBlob.push_back(etwAzure);
        
        manifest.Diagnostics.ETWSource = etw;

        FolderSourceDescription folder;
        LocalStoreDescription fLocal;
        folder.RelativeFolderPath = L"RelFolPath";
        folder.IsEnabled = L"no";
        folder.DataDeletionAgeInDays = L"4";
        fLocal.RelativeFolderPath = L"relativePath";
        folder.Destinations.LocalStore.push_back(fLocal);
        manifest.Diagnostics.FolderSource.push_back(folder);
    }

    void ServiceModelSerializerTest::validateDiagnosticsApplication(ApplicationManifestDescription & manifest)
    {
        validateBaseApplicationManifest(manifest);
        
        CrashDumpSourceDescription crashDump = manifest.Diagnostics.CrashDumpSource;
        VERIFY_ARE_EQUAL(crashDump.IsEnabled, L"true");

        ParameterDescription cparam1 = crashDump.Parameters.Parameter[0];
        VERIFY_ARE_EQUAL(cparam1.IsEncrypted, L"true");
        VERIFY_ARE_EQUAL(cparam1.Name, L"CParamName");
        VERIFY_ARE_EQUAL(cparam1.Value, L"CParamValue");

        LocalStoreDescription cLocalStore = crashDump.Destinations.LocalStore[0];
        FileStoreDescription fileStore = crashDump.Destinations.FileStore[0];
        ParameterDescription cLParam1 = cLocalStore.Parameters.Parameter[0];
        AzureBlobDescription cAzureBlob = crashDump.Destinations.AzureBlob[0];
        
        VERIFY_ARE_EQUAL(cLocalStore.RelativeFolderPath, L"CrashDumpLocalStorePath");
        VERIFY_ARE_EQUAL(cLocalStore.DataDeletionAgeInDays, L"Days");

        VERIFY_ARE_EQUAL(cLParam1.IsEncrypted, L"true");
        VERIFY_ARE_EQUAL(cLParam1.Name, L"CLParamName");
        VERIFY_ARE_EQUAL(cLParam1.Value, L"CLParamValue");
        
        VERIFY_ARE_EQUAL(fileStore.IsEnabled, L"true");
        VERIFY_ARE_EQUAL(fileStore.Path, L"FPath");
        VERIFY_ARE_EQUAL(fileStore.UploadIntervalInMinutes, L"5");
        VERIFY_ARE_EQUAL(fileStore.DataDeletionAgeInDays, L"6");
        VERIFY_ARE_EQUAL(fileStore.AccountType, L"FileStoreAccountType");
        VERIFY_ARE_EQUAL(fileStore.AccountName, L"FSAccountName");
        VERIFY_ARE_EQUAL(fileStore.Password, L"PlainTextPassword");
        VERIFY_ARE_EQUAL(fileStore.PasswordEncrypted, L"false");
        
        VERIFY_ARE_EQUAL(cAzureBlob.IsEnabled, L"true");
        VERIFY_ARE_EQUAL(cAzureBlob.ConnectionString, L"PlainText");
        VERIFY_ARE_EQUAL(cAzureBlob.ConnectionStringIsEncrypted, L"no");

        ETWSourceDescription etw = manifest.Diagnostics.ETWSource;
        LocalStoreETWDescription etwLocal = etw.Destinations.LocalStore[0];
        FileStoreETWDescription etwFile = etw.Destinations.FileStore[0];
        AzureBlobETWDescription etwAzure = etw.Destinations.AzureBlob[0];

        VERIFY_ARE_EQUAL(etwLocal.LevelFilter, L"ETWLevelFilter");
        VERIFY_ARE_EQUAL(etwLocal.RelativeFolderPath, L"ETWRelativePath");
        VERIFY_ARE_EQUAL(etwLocal.DataDeletionAgeInDays, L"5");

        VERIFY_ARE_EQUAL(etwFile.DataDeletionAgeInDays, L"4");
        VERIFY_ARE_EQUAL(etwFile.LevelFilter, L"6");
        VERIFY_ARE_EQUAL(etwFile.Path, L"path");

        VERIFY_ARE_EQUAL(etwAzure.ConnectionString, L"plaintext");
        VERIFY_ARE_EQUAL(etwAzure.ConnectionStringIsEncrypted, L"false");
        VERIFY_ARE_EQUAL(etwAzure.LevelFilter, L"AzureLevelFilter");

        FolderSourceDescription folder = manifest.Diagnostics.FolderSource[0];
        LocalStoreDescription fLocal = folder.Destinations.LocalStore[0];
        VERIFY_ARE_EQUAL(folder.RelativeFolderPath, L"RelFolPath");
        VERIFY_ARE_EQUAL(folder.IsEnabled, L"no");
        VERIFY_ARE_EQUAL(folder.DataDeletionAgeInDays, L"4");
        VERIFY_ARE_EQUAL(fLocal.RelativeFolderPath, L"relativePath");
    }

    void ServiceModelSerializerTest::generateCertificatesApplication(ApplicationManifestDescription & manifest)
    {
        generateBaseApplicationManifest(manifest);
        SecretsCertificateDescription certif1;
        certif1.Name = L"certif1Name";
        certif1.X509StoreName = L"certif1StoreName";
        certif1.X509FindType = X509FindType::FindByThumbprint;
        certif1.X509FindValue = L"certif1Value";
        certif1.X509FindValueSecondary = L"certif1ValueSecondary";

        SecretsCertificateDescription certif2;
        certif2.Name = L"certif2Name";
        certif2.X509StoreName = L"certif2StoreName";
        certif2.X509FindType = X509FindType::FindBySubjectName;
        certif2.X509FindValue = L"certif2Value";
        certif2.X509FindValueSecondary = L"certif2ValueSecondary";

        manifest.Certificates.push_back(certif1);
        manifest.Certificates.push_back(certif2);
    }

    void ServiceModelSerializerTest::validateCertificatesApplication(ApplicationManifestDescription & manifest)
    {
        validateBaseApplicationManifest(manifest);
        SecretsCertificateDescription certif1 = manifest.Certificates[0];
        SecretsCertificateDescription certif2 = manifest.Certificates[1];
        
        VERIFY_ARE_EQUAL(certif1.Name, L"certif1Name");
        VERIFY_ARE_EQUAL(certif1.X509StoreName, L"certif1StoreName");
        VERIFY_ARE_EQUAL(certif1.X509FindType, X509FindType::FindByThumbprint);
        VERIFY_ARE_EQUAL(certif1.X509FindValue, L"certif1Value");
        VERIFY_ARE_EQUAL(certif1.X509FindValueSecondary, L"certif1ValueSecondary");
        VERIFY_ARE_EQUAL(certif2.Name, L"certif2Name");
        VERIFY_ARE_EQUAL(certif2.X509StoreName, L"certif2StoreName");
        VERIFY_ARE_EQUAL(certif2.X509FindType, X509FindType::FindBySubjectName);
        VERIFY_ARE_EQUAL(certif2.X509FindValue, L"certif2Value");
        VERIFY_ARE_EQUAL(certif2.X509FindValueSecondary, L"certif2ValueSecondary");
    }

    void ServiceModelSerializerTest::generatePrincipalsApplication(ApplicationManifestDescription & manifest)
    {
        generateBaseApplicationManifest(manifest);
        
        SecurityGroupDescription securityGroup;
        securityGroup.Name = L"SecurityGroupName";
        securityGroup.NTLMAuthenticationPolicy.IsEnabled = true;
        securityGroup.SystemGroupMembers.push_back(L"SystemGroupMember0");
        securityGroup.DomainGroupMembers.push_back(L"DomainGroupMember0");
        securityGroup.DomainGroupMembers.push_back(L"DomainGroupMember1");
        securityGroup.DomainUserMembers.push_back(L"DomainUserMember0");

        manifest.Principals.Groups.push_back(securityGroup);
        SecurityUserDescription securityUser;
        securityUser.Name = L"SecurityUser";
        securityUser.AccountType = SecurityPrincipalAccountType::NetworkService;
        securityUser.LoadProfile = false;
        securityUser.PerformInteractiveLogon = true;
        securityUser.AccountName = L"SecurityUserAccountName";
        securityUser.Password = L"PlainTextPassword";
        securityUser.IsPasswordEncrypted = false;

        securityUser.NTLMAuthenticationPolicy.IsEnabled = true;
        securityUser.NTLMAuthenticationPolicy.PasswordSecret = L"NotPlainText";
        securityUser.NTLMAuthenticationPolicy.IsPasswordSecretEncrypted = true;

        securityUser.ParentSystemGroups.push_back(L"ParentSystemGroup");
        securityUser.ParentApplicationGroups.push_back(L"ApplicationGroup");
        manifest.Principals.Users.push_back(securityUser);
    }

    void ServiceModelSerializerTest::validatePrincipalsApplication(ApplicationManifestDescription & manifest)
    {
        validateBaseApplicationManifest(manifest);

        SecurityGroupDescription securityGroup = manifest.Principals.Groups[0];
        SecurityUserDescription securityUser = manifest.Principals.Users[0];

        VERIFY_ARE_EQUAL(securityGroup.Name, L"SecurityGroupName");
        VERIFY_ARE_EQUAL(securityGroup.NTLMAuthenticationPolicy.IsEnabled, true);
        VERIFY_ARE_EQUAL(securityGroup.SystemGroupMembers[0], (L"SystemGroupMember0"));
        VERIFY_ARE_EQUAL(securityGroup.DomainGroupMembers[0], (L"DomainGroupMember0"));
        VERIFY_ARE_EQUAL(securityGroup.DomainGroupMembers[1], (L"DomainGroupMember1"));
        VERIFY_ARE_EQUAL(securityGroup.DomainUserMembers[0], (L"DomainUserMember0"));

        VERIFY_ARE_EQUAL(securityUser.Name, L"SecurityUser");
        VERIFY_ARE_EQUAL(securityUser.AccountType, SecurityPrincipalAccountType::NetworkService);
        VERIFY_ARE_EQUAL(securityUser.LoadProfile, false);
        VERIFY_ARE_EQUAL(securityUser.PerformInteractiveLogon, true);
        VERIFY_ARE_EQUAL(securityUser.AccountName, L"SecurityUserAccountName");
        VERIFY_ARE_EQUAL(securityUser.Password, L"PlainTextPassword");
        VERIFY_ARE_EQUAL(securityUser.IsPasswordEncrypted, false);

        VERIFY_ARE_EQUAL(securityUser.NTLMAuthenticationPolicy.IsEnabled, true);
        VERIFY_ARE_EQUAL(securityUser.NTLMAuthenticationPolicy.PasswordSecret, L"NotPlainText");
        VERIFY_ARE_EQUAL(securityUser.NTLMAuthenticationPolicy.IsPasswordSecretEncrypted, true);

        VERIFY_ARE_EQUAL(securityUser.ParentSystemGroups[0], (L"ParentSystemGroup"));
        VERIFY_ARE_EQUAL(securityUser.ParentApplicationGroups[0], (L"ApplicationGroup"));
    }

    void ServiceModelSerializerTest::generateBaseServicePackage(ServicePackageDescription & package)
    {
        package.DigestedServiceTypes.RolloutVersionValue = RolloutVersion(123, 456);
        ServiceTypeDescription service;
        generateFullStatelessServiceType(service);
        package.DigestedServiceTypes.ServiceTypes.push_back(service);

        DigestedCodePackageDescription dCPD;
        dCPD.RolloutVersionValue = RolloutVersion(1, 2);
        
        CodePackageDescription code;
        code.HasSetupEntryPoint = true;

        code.Name = L"CodePackage2";
        code.Version = L"DllEntryCodePackage";
        code.IsShared = false;

        code.SetupEntryPoint.Program = L"SetupExeProgram";
        code.SetupEntryPoint.Arguments = L"Setup exe arguments";
        code.SetupEntryPoint.WorkingFolder = ExeHostWorkingFolder::CodeBase;
        code.SetupEntryPoint.ConsoleRedirectionEnabled = true;
        code.SetupEntryPoint.ConsoleRedirectionFileMaxSizeInKb = 44;
        code.SetupEntryPoint.ConsoleRedirectionFileRetentionCount = 4;

        DllHostEntryPointDescription dllHost;
        code.EntryPoint.EntryPointType = EntryPointType::DllHost;

        dllHost.IsolationPolicyType = CodePackageIsolationPolicyType::DedicatedDomain;

        DllHostHostedUnmanagedDllDescription Unmanaged;
        Unmanaged.DllName = L"UnmanagedDLLName";
        DllHostHostedManagedDllDescription Managed;
        Managed.AssemblyName = L"ManagedDllName";

        DllHostHostedDllDescription unmanagedDescription;
        unmanagedDescription.Unmanaged = Unmanaged;
        unmanagedDescription.Kind = DllHostHostedDllKind::Unmanaged;
        DllHostHostedDllDescription managedDescription;
        managedDescription.Managed = Managed;
        managedDescription.Kind = DllHostHostedDllKind::Managed;

        dllHost.HostedDlls.push_back(unmanagedDescription);
        dllHost.HostedDlls.push_back(managedDescription);

        code.EntryPoint.DllHostEntryPoint = dllHost;

        dCPD.CodePackage = code;

        DebugParametersDescription debugParameters;
        debugParameters.ExePath = L"ThisPath";
        debugParameters.Arguments = L"TheseArguments";
        debugParameters.EnvironmentBlock = L"TheEnvironmentBlock";
        debugParameters.EntryPointType = RunAsPolicyTypeEntryPointType::Main;

        dCPD.DebugParameters = debugParameters;

        package.DigestedCodePackages.push_back(dCPD);

        DigestedResourcesDescription resources;
        resources.RolloutVersionValue = RolloutVersion(4, 44);
        DigestedEndpointDescription endpoint;
        SecurityAccessPolicyDescription securityPolicy;
        securityPolicy.ResourceRef = L"SecurityResoruceRef";
        securityPolicy.PrincipalRef = L"SecurityPrincipalRef";
        securityPolicy.Rights = GrantAccessType::Full;
        securityPolicy.ResourceType = SecurityAccessPolicyTypeResourceType::Certificate;

        EndpointDescription end1;
        end1.Name = L"EndPointName1";
        end1.Protocol = ProtocolType::Https;
        end1.Type = EndpointType::Input;
        end1.CertificateRef = L"CertificateRef1";
        end1.Port = 4444;
        end1.ExplicitPortSpecified = true;

        endpoint.SecurityAccessPolicy = securityPolicy;
        endpoint.Endpoint = end1;

        resources.DigestedEndpoints[L"EndPointName1"] = endpoint;

        package.DigestedResources = resources;

        package.Diagnostics.EtwDescription.ProviderGuids.push_back(L"FirstGuid");
        package.Diagnostics.EtwDescription.ProviderGuids.push_back(L"SecondGuid");

        DataPackageDescription data1, data2;
        data1.Name = L"FirstDiagnosticDataName";
        data2.Name = L"SecondDiagnosticDataName";
        data1.Version = L"Version1";
        data2.Version = L"Version2";

        package.Diagnostics.EtwDescription.ManifestDataPackages.push_back(data1);
        package.Diagnostics.EtwDescription.ManifestDataPackages.push_back(data2);
    }

    void ServiceModelSerializerTest::validateBaseServicePackage(ServicePackageDescription & package)
    {
        VERIFY_ARE_EQUAL(package.DigestedServiceTypes.RolloutVersionValue, RolloutVersion(123, 456));
        ServiceTypeDescription service = package.DigestedServiceTypes.ServiceTypes[0];
        validateFullStatelessServiceType(service);

        DigestedCodePackageDescription dCPD = package.DigestedCodePackages[0];
        CodePackageDescription code = dCPD.CodePackage;
        DllHostEntryPointDescription dllHost = code.EntryPoint.DllHostEntryPoint;
        DllHostHostedDllDescription unmanagedDescription = dllHost.HostedDlls[0];
        DllHostHostedDllDescription managedDescription = dllHost.HostedDlls[1];
        DllHostHostedUnmanagedDllDescription Unmanaged = unmanagedDescription.Unmanaged;
        DllHostHostedManagedDllDescription Managed = managedDescription.Managed;

        VERIFY_ARE_EQUAL(dCPD.RolloutVersionValue, RolloutVersion(1, 2));

        VERIFY_ARE_EQUAL(code.HasSetupEntryPoint, true);

        VERIFY_ARE_EQUAL(code.Name, L"CodePackage2");
        VERIFY_ARE_EQUAL(code.Version, L"DllEntryCodePackage");
        VERIFY_ARE_EQUAL(code.IsShared, false);

        VERIFY_ARE_EQUAL(code.SetupEntryPoint.Program, L"SetupExeProgram");
        VERIFY_ARE_EQUAL(code.SetupEntryPoint.Arguments, L"Setup exe arguments");
        VERIFY_ARE_EQUAL(code.SetupEntryPoint.WorkingFolder, ExeHostWorkingFolder::CodeBase);
        VERIFY_ARE_EQUAL(code.SetupEntryPoint.ConsoleRedirectionEnabled, true);
        VERIFY_ARE_EQUAL(code.SetupEntryPoint.ConsoleRedirectionFileMaxSizeInKb, 44);
        VERIFY_ARE_EQUAL(code.SetupEntryPoint.ConsoleRedirectionFileRetentionCount, 4);
        
        VERIFY_ARE_EQUAL(code.EntryPoint.EntryPointType, EntryPointType::DllHost);
        VERIFY_ARE_EQUAL(dllHost.IsolationPolicyType, CodePackageIsolationPolicyType::DedicatedDomain);

        VERIFY_ARE_EQUAL(Unmanaged.DllName, L"UnmanagedDLLName");
        VERIFY_ARE_EQUAL(Managed.AssemblyName, L"ManagedDllName");
        VERIFY_ARE_EQUAL(unmanagedDescription.Unmanaged, Unmanaged);
        VERIFY_ARE_EQUAL(unmanagedDescription.Kind, DllHostHostedDllKind::Unmanaged);
        VERIFY_ARE_EQUAL(managedDescription.Managed, Managed);
        VERIFY_ARE_EQUAL(managedDescription.Kind, DllHostHostedDllKind::Managed);

        VERIFY_ARE_EQUAL(Unmanaged.DllName, L"UnmanagedDLLName");
        VERIFY_ARE_EQUAL(Managed.AssemblyName, L"ManagedDllName");
        VERIFY_ARE_EQUAL(unmanagedDescription.Unmanaged, Unmanaged);
        VERIFY_ARE_EQUAL(unmanagedDescription.Kind, DllHostHostedDllKind::Unmanaged);
        VERIFY_ARE_EQUAL(managedDescription.Managed, Managed);
        VERIFY_ARE_EQUAL(managedDescription.Kind, DllHostHostedDllKind::Managed);

        DigestedResourcesDescription resources = package.DigestedResources;
        DigestedEndpointDescription endpoint = resources.DigestedEndpoints[L"EndPointName1"];
        SecurityAccessPolicyDescription securityPolicy = endpoint.SecurityAccessPolicy;
        EndpointDescription end1 = endpoint.Endpoint;
        
        VERIFY_ARE_EQUAL(resources.RolloutVersionValue, RolloutVersion(4, 44));
        VERIFY_ARE_EQUAL(securityPolicy.ResourceRef, L"SecurityResoruceRef");
        VERIFY_ARE_EQUAL(securityPolicy.PrincipalRef, L"SecurityPrincipalRef");
        VERIFY_ARE_EQUAL(securityPolicy.Rights, GrantAccessType::Full);
        VERIFY_ARE_EQUAL(securityPolicy.ResourceType, SecurityAccessPolicyTypeResourceType::Certificate);
        VERIFY_ARE_EQUAL(end1.Name, L"EndPointName1");
        VERIFY_ARE_EQUAL(end1.Protocol, ProtocolType::Https);
        VERIFY_ARE_EQUAL(end1.Type, EndpointType::Input);
        VERIFY_ARE_EQUAL(end1.CertificateRef, L"CertificateRef1");
        VERIFY_ARE_EQUAL(end1.Port, 4444);
        VERIFY_ARE_EQUAL(end1.ExplicitPortSpecified, true);

        VERIFY_ARE_EQUAL(package.Diagnostics.EtwDescription.ProviderGuids[0], (L"FirstGuid"));
        VERIFY_ARE_EQUAL(package.Diagnostics.EtwDescription.ProviderGuids[1], (L"SecondGuid"));

        DataPackageDescription data1 = package.Diagnostics.EtwDescription.ManifestDataPackages[0];
        DataPackageDescription data2 = package.Diagnostics.EtwDescription.ManifestDataPackages[1];
        VERIFY_ARE_EQUAL(data1.Name, L"FirstDiagnosticDataName");
        VERIFY_ARE_EQUAL(data2.Name, L"SecondDiagnosticDataName");
        VERIFY_ARE_EQUAL(data1.Version, L"Version1");
        VERIFY_ARE_EQUAL(data2.Version, L"Version2");
    }

    void ServiceModelSerializerTest::generateConfigDataServicePackage(ServicePackageDescription & package)
    {
        generateBaseServicePackage(package);

        DigestedDataPackageDescription digestedData1;
        DigestedDataPackageDescription digestedData2;
        digestedData1.RolloutVersionValue = RolloutVersion(4444, 4);
        digestedData2.RolloutVersionValue = RolloutVersion(1338, 8999);
        digestedData1.IsShared = true;
        digestedData2.IsShared = false;
        digestedData1.ContentChecksum = L"PlainChecksum";
        digestedData2.ContentChecksum = L"Not-PlainChecksum";
        digestedData1.DataPackage.Name = L"DigestedDataName1";
        digestedData2.DataPackage.Name = L"DigestedDataName2";
        digestedData1.DataPackage.Version = L"DigestedDataVersion1";
        digestedData2.DataPackage.Version = L"DigestedDataVersion2";

        DigestedConfigPackageDescription configPackage;
        configPackage.RolloutVersionValue = RolloutVersion(1, 11);
        configPackage.ConfigPackage.Name = L"ConfigPackageName";
        configPackage.ConfigPackage.Version = L"ConfigPackageVersion";
        configPackage.IsShared = true;
        configPackage.ContentChecksum = L"SumCheck";

        ConfigOverrideDescription configOverride1;
        configOverride1.ConfigPackageName = L"ConfigPackageName1";

        //has all sections
        ConfigSettingsOverride configSettingsOverride;
        ConfigSectionOverride configSectionOverride;
        ConfigParameterOverride param1;
        ConfigParameterOverride param2;
        configSectionOverride.Name = L"SectionOverride";
        param1.Name = L"Param1";
        param1.Value = L"Value1";
        param1.IsEncrypted = true;
        param2.Name = L"Param2";
        param2.Value = L"Value2";
        param2.IsEncrypted = false;
        
        configSectionOverride.Parameters[L"Param1"] = param1;
        configSectionOverride.Parameters[L"Param2"] = param2;
        configSettingsOverride.Sections[L"SectionOverride"] = configSectionOverride;
        configOverride1.SettingsOverride = configSettingsOverride;

        configPackage.ConfigOverrides = configOverride1;

        package.DigestedDataPackages.push_back(digestedData1);
        package.DigestedDataPackages.push_back(digestedData2);
        package.DigestedConfigPackages.push_back(configPackage);
    }

    void ServiceModelSerializerTest::validateConfigDataServicePackage(ServicePackageDescription & package)
    {
        validateBaseServicePackage(package);

        DigestedDataPackageDescription digestedData1 = package.DigestedDataPackages[0];
        DigestedDataPackageDescription digestedData2 = package.DigestedDataPackages[1];
        VERIFY_ARE_EQUAL(digestedData1.RolloutVersionValue, RolloutVersion(4444, 4));
        VERIFY_ARE_EQUAL(digestedData2.RolloutVersionValue, RolloutVersion(1338, 8999));
        VERIFY_ARE_EQUAL(digestedData1.IsShared, true);
        VERIFY_ARE_EQUAL(digestedData2.IsShared, false);
        VERIFY_ARE_EQUAL(digestedData1.ContentChecksum, L"PlainChecksum");
        VERIFY_ARE_EQUAL(digestedData2.ContentChecksum, L"Not-PlainChecksum");
        VERIFY_ARE_EQUAL(digestedData1.DataPackage.Name, L"DigestedDataName1");
        VERIFY_ARE_EQUAL(digestedData2.DataPackage.Name, L"DigestedDataName2");
        VERIFY_ARE_EQUAL(digestedData1.DataPackage.Version, L"DigestedDataVersion1");
        VERIFY_ARE_EQUAL(digestedData2.DataPackage.Version, L"DigestedDataVersion2");

        DigestedConfigPackageDescription configPackage = package.DigestedConfigPackages[0];
        ConfigOverrideDescription configOverride1 = configPackage.ConfigOverrides;
        ConfigSettingsOverride configSettingsOverride = configOverride1.SettingsOverride;
        ConfigSectionOverride configSectionOverride = configSettingsOverride.Sections[L"SectionOverride"];
        ConfigParameterOverride param1 = configSectionOverride.Parameters[L"Param1"];
        ConfigParameterOverride param2 = configSectionOverride.Parameters[L"Param2"]; 
        
        VERIFY_ARE_EQUAL(configPackage.RolloutVersionValue, RolloutVersion(1, 11));
        VERIFY_ARE_EQUAL(configPackage.ConfigPackage.Name, L"ConfigPackageName");
        VERIFY_ARE_EQUAL(configPackage.ConfigPackage.Version, L"ConfigPackageVersion");
        VERIFY_ARE_EQUAL(configPackage.IsShared, true);
        VERIFY_ARE_EQUAL(configPackage.ContentChecksum, L"SumCheck");
        VERIFY_ARE_EQUAL(configOverride1.ConfigPackageName, L"ConfigPackageName1");
        VERIFY_ARE_EQUAL(configSectionOverride.Name, L"SectionOverride");
        VERIFY_ARE_EQUAL(param1.Name, L"Param1");
        VERIFY_ARE_EQUAL(param1.Value, L"Value1");
        VERIFY_ARE_EQUAL(param1.IsEncrypted, true);
        VERIFY_ARE_EQUAL(param2.Name, L"Param2");
        VERIFY_ARE_EQUAL(param2.Value, L"Value2");
        VERIFY_ARE_EQUAL(param2.IsEncrypted, false);
    }

    void ServiceModelSerializerTest::generateApplicationPackage(ApplicationPackageDescription & package)
    {
        DigestedCertificatesDescription digestedCertificates;
        SecretsCertificateDescription certif1;
        SecretsCertificateDescription certif2;

        digestedCertificates.RolloutVersionValue = RolloutVersion(17, 14);

        certif1.Name = L"certif1Name";
        certif1.X509StoreName = L"certif1StoreName";
        certif1.X509FindType = X509FindType::FindByThumbprint;
        certif1.X509FindValue = L"certif1Value";
        certif1.X509FindValueSecondary = L"certif1ValueSecondary";

        certif2.Name = L"certif2Name";
        certif2.X509StoreName = L"certif2StoreName";
        certif2.X509FindType = X509FindType::FindBySubjectName;
        certif2.X509FindValue = L"certif2Value";
        certif2.X509FindValueSecondary = L"certif2ValueSecondary";

        digestedCertificates.SecretsCertificate.push_back(certif1);
        digestedCertificates.SecretsCertificate.push_back(certif2);

        package.DigestedCertificates = digestedCertificates;

        DigestedEnvironmentDescription digestedEnvironment;

        DefaultRunAsPolicyDescription runPolicy;
        runPolicy.UserRef = L"RunAsPolicyUserRef";

        LogCollectionPolicyDescription logPolicy;
        logPolicy.Path = L"LogPath";

        SecurityAccessPolicyDescription securityPolicy;
        securityPolicy.PrincipalRef = L"PRef";
        securityPolicy.ResourceRef = L"URef";
        securityPolicy.ResourceType = SecurityAccessPolicyTypeResourceType::Endpoint;
        securityPolicy.Rights = GrantAccessType::Read;

        ApplicationHealthPolicy healthPolicy;
        ServiceTypeHealthPolicy defaultHealthPolicy;
        defaultHealthPolicy.MaxPercentUnhealthyPartitionsPerService = 5;
        defaultHealthPolicy.MaxPercentUnhealthyReplicasPerPartition = 4;
        defaultHealthPolicy.MaxPercentUnhealthyServices = 44;

        ServiceTypeHealthPolicy otherHealthPolicy;
        otherHealthPolicy.MaxPercentUnhealthyPartitionsPerService = 7;
        otherHealthPolicy.MaxPercentUnhealthyReplicasPerPartition = 8;
        otherHealthPolicy.MaxPercentUnhealthyServices = 49;
        healthPolicy.ServiceTypeHealthPolicies[L"OtherName"] = otherHealthPolicy;
        healthPolicy.DefaultServiceTypeHealthPolicy = defaultHealthPolicy;
        healthPolicy.ConsiderWarningAsError = true;
        digestedEnvironment.Policies.DefaultRunAs = runPolicy;
        digestedEnvironment.Policies.SecurityAccessPolicies.push_back(securityPolicy);
        digestedEnvironment.Policies.LogCollectionEntries.push_back(logPolicy);
        digestedEnvironment.Policies.HealthPolicy = healthPolicy;

        SecurityGroupDescription securityGroup;
        securityGroup.Name = L"SecurityGroupName";
        securityGroup.NTLMAuthenticationPolicy.IsEnabled = true;
        securityGroup.SystemGroupMembers.push_back(L"SystemGroupMember0");
        securityGroup.DomainGroupMembers.push_back(L"DomainGroupMember0");
        securityGroup.DomainGroupMembers.push_back(L"DomainGroupMember1");
        securityGroup.DomainUserMembers.push_back(L"DomainUserMember0");

        digestedEnvironment.Principals.Groups.push_back(securityGroup);
        SecurityUserDescription securityUser;
        securityUser.Name = L"SecurityUser";
        securityUser.AccountType = SecurityPrincipalAccountType::NetworkService;
        securityUser.LoadProfile = false;
        securityUser.PerformInteractiveLogon = true;
        securityUser.AccountName = L"SecurityUserAccountName";
        securityUser.Password = L"PlainTextPassword";
        securityUser.IsPasswordEncrypted = false;

        securityUser.NTLMAuthenticationPolicy.IsEnabled = true;
        securityUser.NTLMAuthenticationPolicy.PasswordSecret = L"NotPlainText";
        securityUser.NTLMAuthenticationPolicy.IsPasswordSecretEncrypted = true;

        securityUser.ParentSystemGroups.push_back(L"ParentSystemGroup");
        securityUser.ParentApplicationGroups.push_back(L"ApplicationGroup");
        digestedEnvironment.Principals.Users.push_back(securityUser);

        CrashDumpSourceDescription crashDump;
        crashDump.IsEnabled = L"true";

        ParameterDescription cparam1;
        cparam1.IsEncrypted = L"true";
        cparam1.Name = L"CParamName";
        cparam1.Value = L"CParamValue";
        crashDump.Parameters.Parameter.push_back(cparam1);

        LocalStoreDescription cLocalStore;
        cLocalStore.RelativeFolderPath = L"CrashDumpLocalStorePath";
        cLocalStore.DataDeletionAgeInDays = L"Days";

        ParameterDescription cLParam1;
        cLParam1.IsEncrypted = L"true";
        cLParam1.Name = L"CLParamName";
        cLParam1.Value = L"CLParamValue";
        cLocalStore.Parameters.Parameter.push_back(cLParam1);

        FileStoreDescription fileStore;
        fileStore.IsEnabled = L"true";
        fileStore.Path = L"FPath";
        fileStore.UploadIntervalInMinutes = L"5";
        fileStore.DataDeletionAgeInDays = L"6";
        fileStore.AccountType = L"FileStoreAccountType";
        fileStore.AccountName = L"FSAccountName";
        fileStore.Password = L"PlainTextPassword";
        fileStore.PasswordEncrypted = L"false";

        AzureBlobDescription cAzureBlob;
        cAzureBlob.IsEnabled = L"true";
        cAzureBlob.ConnectionString = L"PlainText";
        cAzureBlob.ConnectionStringIsEncrypted = L"no";

        crashDump.Destinations.LocalStore.push_back(cLocalStore);
        crashDump.Destinations.FileStore.push_back(fileStore);
        crashDump.Destinations.AzureBlob.push_back(cAzureBlob);

        digestedEnvironment.Diagnostics.CrashDumpSource = crashDump;

        ETWSourceDescription etw;

        LocalStoreETWDescription etwLocal;
        etwLocal.LevelFilter = L"ETWLevelFilter";
        etwLocal.RelativeFolderPath = L"ETWRelativePath";
        etwLocal.DataDeletionAgeInDays = L"5";

        FileStoreETWDescription etwFile;
        etwFile.DataDeletionAgeInDays = L"4";
        etwFile.LevelFilter = L"6";
        etwFile.Path = L"path";

        AzureBlobETWDescription etwAzure;
        etwAzure.ConnectionString = L"plaintext";
        etwAzure.ConnectionStringIsEncrypted = L"false";
        etwAzure.LevelFilter = L"AzureLevelFilter";

        etw.Destinations.LocalStore.push_back(etwLocal);
        etw.Destinations.FileStore.push_back(etwFile);
        etw.Destinations.AzureBlob.push_back(etwAzure);

        digestedEnvironment.Diagnostics.ETWSource = etw;

        FolderSourceDescription folder;
        LocalStoreDescription fLocal;
        folder.RelativeFolderPath = L"RelFolPath";
        folder.IsEnabled = L"no";
        folder.DataDeletionAgeInDays = L"4";
        fLocal.RelativeFolderPath = L"relativePath";
        folder.Destinations.LocalStore.push_back(fLocal);
        digestedEnvironment.Diagnostics.FolderSource.push_back(folder);

        package.DigestedEnvironment = digestedEnvironment;
    }

    void ServiceModelSerializerTest::validateApplicationPackage(ApplicationPackageDescription & package)
    {
        DigestedCertificatesDescription digestedCertificates = package.DigestedCertificates;
        SecretsCertificateDescription certif1 = digestedCertificates.SecretsCertificate[0];
        SecretsCertificateDescription certif2 = digestedCertificates.SecretsCertificate[1];

        digestedCertificates.RolloutVersionValue = RolloutVersion(17, 14);

        VERIFY_ARE_EQUAL(certif1.Name, L"certif1Name");
        VERIFY_ARE_EQUAL(certif1.X509StoreName, L"certif1StoreName");
        VERIFY_ARE_EQUAL(certif1.X509FindType, X509FindType::FindByThumbprint);
        VERIFY_ARE_EQUAL(certif1.X509FindValue, L"certif1Value");
        VERIFY_ARE_EQUAL(certif1.X509FindValueSecondary, L"certif1ValueSecondary");

        VERIFY_ARE_EQUAL(certif2.Name, L"certif2Name");
        VERIFY_ARE_EQUAL(certif2.X509StoreName, L"certif2StoreName");
        VERIFY_ARE_EQUAL(certif2.X509FindType, X509FindType::FindBySubjectName);
        VERIFY_ARE_EQUAL(certif2.X509FindValue, L"certif2Value");
        VERIFY_ARE_EQUAL(certif2.X509FindValueSecondary, L"certif2ValueSecondary");
        
        DigestedEnvironmentDescription digestedEnvironment = package.DigestedEnvironment;
        DefaultRunAsPolicyDescription runPolicy = digestedEnvironment.Policies.DefaultRunAs;
        LogCollectionPolicyDescription logPolicy = digestedEnvironment.Policies.LogCollectionEntries[0];
        SecurityAccessPolicyDescription securityPolicy = digestedEnvironment.Policies.SecurityAccessPolicies[0];
        ApplicationHealthPolicy healthPolicy = digestedEnvironment.Policies.HealthPolicy;
        ServiceTypeHealthPolicy defaultHealthPolicy = healthPolicy.DefaultServiceTypeHealthPolicy;
        ServiceTypeHealthPolicy otherHealthPolicy = healthPolicy.ServiceTypeHealthPolicies[L"OtherName"];
        VERIFY_ARE_EQUAL(runPolicy.UserRef, L"RunAsPolicyUserRef");
        VERIFY_ARE_EQUAL(logPolicy.Path, L"LogPath");
        VERIFY_ARE_EQUAL(securityPolicy.PrincipalRef, L"PRef");
        VERIFY_ARE_EQUAL(securityPolicy.ResourceRef, L"URef");
        VERIFY_ARE_EQUAL(securityPolicy.ResourceType, SecurityAccessPolicyTypeResourceType::Endpoint);
        VERIFY_ARE_EQUAL(securityPolicy.Rights, GrantAccessType::Read);
        VERIFY_ARE_EQUAL(defaultHealthPolicy.MaxPercentUnhealthyPartitionsPerService, 5);
        VERIFY_ARE_EQUAL(defaultHealthPolicy.MaxPercentUnhealthyReplicasPerPartition, 4);
        VERIFY_ARE_EQUAL(defaultHealthPolicy.MaxPercentUnhealthyServices, 44);
        VERIFY_ARE_EQUAL(otherHealthPolicy.MaxPercentUnhealthyPartitionsPerService, 7);
        VERIFY_ARE_EQUAL(otherHealthPolicy.MaxPercentUnhealthyReplicasPerPartition, 8);
        VERIFY_ARE_EQUAL(otherHealthPolicy.MaxPercentUnhealthyServices, 49);
        VERIFY_ARE_EQUAL(healthPolicy.ConsiderWarningAsError, true);

        SecurityGroupDescription securityGroup = digestedEnvironment.Principals.Groups[0];
        SecurityUserDescription securityUser = digestedEnvironment.Principals.Users[0];
        VERIFY_ARE_EQUAL(securityGroup.Name, L"SecurityGroupName");
        VERIFY_ARE_EQUAL(securityGroup.NTLMAuthenticationPolicy.IsEnabled, true);
        VERIFY_ARE_EQUAL(securityGroup.SystemGroupMembers[0], (L"SystemGroupMember0"));
        VERIFY_ARE_EQUAL(securityGroup.DomainGroupMembers[0], (L"DomainGroupMember0"));
        VERIFY_ARE_EQUAL(securityGroup.DomainGroupMembers[1], (L"DomainGroupMember1"));
        VERIFY_ARE_EQUAL(securityGroup.DomainUserMembers[0], (L"DomainUserMember0"));

        VERIFY_ARE_EQUAL(securityUser.Name, L"SecurityUser");
        VERIFY_ARE_EQUAL(securityUser.AccountType, SecurityPrincipalAccountType::NetworkService);
        VERIFY_ARE_EQUAL(securityUser.LoadProfile, false);
        VERIFY_ARE_EQUAL(securityUser.PerformInteractiveLogon, true);
        VERIFY_ARE_EQUAL(securityUser.AccountName, L"SecurityUserAccountName");
        VERIFY_ARE_EQUAL(securityUser.Password, L"PlainTextPassword");
        VERIFY_ARE_EQUAL(securityUser.IsPasswordEncrypted, false);

        VERIFY_ARE_EQUAL(securityUser.NTLMAuthenticationPolicy.IsEnabled, true);
        VERIFY_ARE_EQUAL(securityUser.NTLMAuthenticationPolicy.PasswordSecret, L"NotPlainText");
        VERIFY_ARE_EQUAL(securityUser.NTLMAuthenticationPolicy.IsPasswordSecretEncrypted, true);

        VERIFY_ARE_EQUAL(securityUser.ParentSystemGroups[0], (L"ParentSystemGroup"));
        VERIFY_ARE_EQUAL(securityUser.ParentApplicationGroups[0], (L"ApplicationGroup"));

        CrashDumpSourceDescription crashDump = digestedEnvironment.Diagnostics.CrashDumpSource;
        ParameterDescription cparam1 = crashDump.Parameters.Parameter[0] ;
        LocalStoreDescription cLocalStore = crashDump.Destinations.LocalStore[0];
        ParameterDescription cLParam1 = cLocalStore.Parameters.Parameter[0];
        FileStoreDescription fileStore = crashDump.Destinations.FileStore[0];
        AzureBlobDescription cAzureBlob = crashDump.Destinations.AzureBlob[0];
        ETWSourceDescription etw = digestedEnvironment.Diagnostics.ETWSource;
        LocalStoreETWDescription etwLocal = etw.Destinations.LocalStore[0];
        FileStoreETWDescription etwFile = etw.Destinations.FileStore[0];
        AzureBlobETWDescription etwAzure = etw.Destinations.AzureBlob[0];
        FolderSourceDescription folder = digestedEnvironment.Diagnostics.FolderSource[0];
        LocalStoreDescription fLocal = folder.Destinations.LocalStore[0];
        
        VERIFY_ARE_EQUAL(crashDump.IsEnabled, L"true");
        VERIFY_ARE_EQUAL(cparam1.IsEncrypted, L"true");
        VERIFY_ARE_EQUAL(cparam1.Name, L"CParamName");
        VERIFY_ARE_EQUAL(cparam1.Value, L"CParamValue");
        VERIFY_ARE_EQUAL(cLocalStore.RelativeFolderPath, L"CrashDumpLocalStorePath");
        VERIFY_ARE_EQUAL(cLocalStore.DataDeletionAgeInDays, L"Days");
        VERIFY_ARE_EQUAL(cLParam1.IsEncrypted, L"true");
        VERIFY_ARE_EQUAL(cLParam1.Name, L"CLParamName");
        VERIFY_ARE_EQUAL(cLParam1.Value, L"CLParamValue");
        VERIFY_ARE_EQUAL(fileStore.IsEnabled, L"true");
        VERIFY_ARE_EQUAL(fileStore.Path, L"FPath");
        VERIFY_ARE_EQUAL(fileStore.UploadIntervalInMinutes, L"5");
        VERIFY_ARE_EQUAL(fileStore.DataDeletionAgeInDays, L"6");
        VERIFY_ARE_EQUAL(fileStore.AccountType, L"FileStoreAccountType");
        VERIFY_ARE_EQUAL(fileStore.AccountName, L"FSAccountName");
        VERIFY_ARE_EQUAL(fileStore.Password, L"PlainTextPassword");
        VERIFY_ARE_EQUAL(fileStore.PasswordEncrypted, L"false");
        VERIFY_ARE_EQUAL(cAzureBlob.IsEnabled, L"true");
        VERIFY_ARE_EQUAL(cAzureBlob.ConnectionString, L"PlainText");
        VERIFY_ARE_EQUAL(cAzureBlob.ConnectionStringIsEncrypted, L"no");
        VERIFY_ARE_EQUAL(etwLocal.LevelFilter, L"ETWLevelFilter");
        VERIFY_ARE_EQUAL(etwLocal.RelativeFolderPath, L"ETWRelativePath");
        VERIFY_ARE_EQUAL(etwLocal.DataDeletionAgeInDays, L"5");
        VERIFY_ARE_EQUAL(etwFile.DataDeletionAgeInDays, L"4");
        VERIFY_ARE_EQUAL(etwFile.LevelFilter, L"6");
        VERIFY_ARE_EQUAL(etwFile.Path, L"path");
        VERIFY_ARE_EQUAL(etwAzure.ConnectionString, L"plaintext");
        VERIFY_ARE_EQUAL(etwAzure.ConnectionStringIsEncrypted, L"false");
        VERIFY_ARE_EQUAL(etwAzure.LevelFilter, L"AzureLevelFilter");
        VERIFY_ARE_EQUAL(folder.RelativeFolderPath, L"RelFolPath");
        VERIFY_ARE_EQUAL(folder.IsEnabled, L"no");
        VERIFY_ARE_EQUAL(folder.DataDeletionAgeInDays, L"4");
        VERIFY_ARE_EQUAL(fLocal.RelativeFolderPath, L"relativePath");
    }

    void ServiceModelSerializerTest::generateApplicationInstance(ApplicationInstanceDescription & instance)
    {
        ApplicationPackageReference appReference;
        ServicePackageReference servicePack1, servicePack2;
        instance.Version = 4;
        instance.NameUri = L"NewNameui";
        instance.ApplicationId = L"AppInstanceId";
        instance.ApplicationTypeName = L"AppTypeName";
        instance.ApplicationTypeVersion = L"AppTypeVer";
        appReference.RolloutVersionValue = RolloutVersion(44, 44);
        servicePack1.Name = L"ServicePack1";
        servicePack2.Name = L"ServicePack2";
        servicePack1.RolloutVersionValue = RolloutVersion(1, 0);
        servicePack2.RolloutVersionValue = RolloutVersion(5, 6);

        instance.ApplicationPackageReference = appReference;
        instance.ServicePackageReferences.push_back(servicePack1);
        instance.ServicePackageReferences.push_back(servicePack2);

        ApplicationServiceDescription statelessService;
        ServiceLoadMetricDescription loadMetric;
        ServiceCorrelationDescription correlation;
        
        statelessService.IsServiceGroup = false;
        statelessService.IsStateful = false;
        statelessService.ServiceTypeName = L"StatelessServiceName";
        statelessService.DefaultMoveCost = FABRIC_MOVE_COST_HIGH;
        statelessService.InstanceCount = 4;
        statelessService.Partition.Scheme = PartitionSchemeDescription::UniformInt64Range;
        statelessService.Partition.PartitionCount = 7;
        statelessService.Partition.LowKey = 11;
        statelessService.Partition.HighKey = 111;
        loadMetric.Name = L"SecondLoadMetric";
        loadMetric.PrimaryDefaultLoad = 5;
        loadMetric.SecondaryDefaultLoad = 6;
        loadMetric.Weight = WeightType::High;
        statelessService.PlacementConstraints = L"PlacementConstraintsForStatelessService";
        correlation.ServiceName = L"CorrelatedToXService";
        correlation.Scheme = ServiceCorrelationScheme::AlignedAffinity;
        statelessService.LoadMetrics.push_back(loadMetric);
        statelessService.ServiceCorrelations.push_back(correlation);
        
        ApplicationServiceDescription statefulGroupService;
        ServicePlacementPolicyDescription servicePlacement;
        ServiceGroupMemberDescription serviceMember;
        statefulGroupService.IsServiceGroup = true;
        statefulGroupService.IsStateful = true;
        statefulGroupService.ServiceTypeName = L"StatefulServiceGroupName";
        statefulGroupService.DefaultMoveCost = FABRIC_MOVE_COST_MEDIUM;
        statefulGroupService.TargetReplicaSetSize = 4;
        statefulGroupService.MinReplicaSetSize = 3;
        statefulGroupService.ReplicaRestartWaitDuration = TimeSpan::FromSeconds(1);
        statefulGroupService.QuorumLossWaitDuration = TimeSpan::FromSeconds(2);
        statefulGroupService.StandByReplicaKeepDuration = TimeSpan::FromSeconds(3);
        statefulGroupService.Partition.Scheme = PartitionSchemeDescription::Named;
        statefulGroupService.Partition.PartitionNames.push_back(L"PartitionName");
        servicePlacement.domainName_ = L"MyDomainName";
        servicePlacement.type_ = FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN;
        statefulGroupService.ServicePlacementPolicies.push_back(servicePlacement);
        serviceMember.ServiceTypeName = L"TypeName";
        serviceMember.MemberName = L"memberName";
        statefulGroupService.ServiceGroupMembers.push_back(serviceMember);

        instance.ServiceTemplates.push_back(statelessService);
        instance.ServiceTemplates.push_back(statefulGroupService);

        DefaultServiceDescription defaultService1;
        ApplicationServiceDescription app1;
        DefaultServiceDescription defaultService2;
        ApplicationServiceDescription app2;
        ServiceGroupMemberDescription defaultMember;
        DefaultServiceDescription defaultService3;
        ApplicationServiceDescription app3;

        app1.ServiceTypeName = L"StatefulService";
        app1.IsStateful = true;
        app1.IsServiceGroup = false;
        app1.Partition.Scheme = PartitionSchemeDescription::Singleton;
        defaultService1.Name = L"DefaultServiceName1";
        defaultService1.DefaultService = app1;

        app2.IsStateful = false;
        app2.IsServiceGroup = true;
        app2.ServiceTypeName = L"ServiceName";
        app2.InstanceCount = 4;
        app2.Partition.Scheme = PartitionSchemeDescription::Singleton;
        defaultMember.MemberName = L"DefaultMemberName";
        defaultMember.ServiceTypeName = L"DefaultServiceName";
        defaultService2.Name = L"DefaultServiceName2";
        defaultService2.PackageActivationMode = ServicePackageActivationMode::SharedProcess;
        app2.ServiceGroupMembers.push_back(defaultMember);
        defaultService2.DefaultService = app2;
        
        app3.ServiceTypeName = L"StatefulService";
        app3.IsStateful = true;
        app3.IsServiceGroup = false;
        app3.Partition.Scheme = PartitionSchemeDescription::Singleton;
        defaultService3.Name = L"DefaultServiceName3";
        defaultService3.ServiceDnsName = L"Default.Service.Name3";
        defaultService3.PackageActivationMode = ServicePackageActivationMode::ExclusiveProcess;
        defaultService3.DefaultService = app3;

        instance.DefaultServices.push_back(defaultService1);
        instance.DefaultServices.push_back(defaultService2);
        instance.DefaultServices.push_back(defaultService3);
    }

    void ServiceModelSerializerTest::validateApplicationInstance(ApplicationInstanceDescription & instance)
    {
        ApplicationPackageReference appReference = instance.ApplicationPackageReference;
        ServicePackageReference servicePack1 = instance.ServicePackageReferences[0];
        ServicePackageReference servicePack2 = instance.ServicePackageReferences[1];
        VERIFY_ARE_EQUAL(instance.Version, 4);
        VERIFY_ARE_EQUAL(instance.NameUri, L"NewNameui");
        VERIFY_ARE_EQUAL(instance.ApplicationId, L"AppInstanceId");
        VERIFY_ARE_EQUAL(instance.ApplicationTypeName, L"AppTypeName");
        VERIFY_ARE_EQUAL(instance.ApplicationTypeVersion, L"AppTypeVer");
        VERIFY_ARE_EQUAL(appReference.RolloutVersionValue, RolloutVersion(44, 44));
        VERIFY_ARE_EQUAL(servicePack1.Name, L"ServicePack1");
        VERIFY_ARE_EQUAL(servicePack2.Name, L"ServicePack2");
        VERIFY_ARE_EQUAL(servicePack1.RolloutVersionValue, RolloutVersion(1, 0));
        VERIFY_ARE_EQUAL(servicePack2.RolloutVersionValue, RolloutVersion(5, 6));

        ApplicationServiceDescription statelessService = instance.ServiceTemplates[0];
        ServiceLoadMetricDescription loadMetric = statelessService.LoadMetrics[0];
        ServiceCorrelationDescription correlation = statelessService.ServiceCorrelations[0];

        VERIFY_ARE_EQUAL(statelessService.IsServiceGroup, false);
        VERIFY_ARE_EQUAL(statelessService.IsStateful, false);
        VERIFY_ARE_EQUAL(statelessService.ServiceTypeName, L"StatelessServiceName");
        VERIFY_ARE_EQUAL(statelessService.DefaultMoveCost, FABRIC_MOVE_COST_HIGH);
        VERIFY_ARE_EQUAL(statelessService.InstanceCount, 4);
        VERIFY_ARE_EQUAL(statelessService.Partition.Scheme, PartitionSchemeDescription::UniformInt64Range);
        VERIFY_ARE_EQUAL(statelessService.Partition.PartitionCount, 7);
        VERIFY_ARE_EQUAL(statelessService.Partition.LowKey, 11);
        VERIFY_ARE_EQUAL(statelessService.Partition.HighKey, 111);
        VERIFY_ARE_EQUAL(loadMetric.Name, L"SecondLoadMetric");
        VERIFY_ARE_EQUAL(loadMetric.PrimaryDefaultLoad, 5);
        VERIFY_ARE_EQUAL(loadMetric.SecondaryDefaultLoad, 6);
        VERIFY_ARE_EQUAL(loadMetric.Weight, WeightType::High);
        VERIFY_ARE_EQUAL(statelessService.PlacementConstraints, L"PlacementConstraintsForStatelessService");
        VERIFY_ARE_EQUAL(correlation.ServiceName, L"CorrelatedToXService");
        VERIFY_ARE_EQUAL(correlation.Scheme, ServiceCorrelationScheme::AlignedAffinity);

        ApplicationServiceDescription statefulGroupService = instance.ServiceTemplates[1];
        ServicePlacementPolicyDescription servicePlacement = statefulGroupService.ServicePlacementPolicies[0];
        ServiceGroupMemberDescription serviceMember = statefulGroupService.ServiceGroupMembers[0];

        VERIFY_ARE_EQUAL(statefulGroupService.IsServiceGroup, true);
        VERIFY_ARE_EQUAL(statefulGroupService.IsStateful, true);
        VERIFY_ARE_EQUAL(statefulGroupService.ServiceTypeName, L"StatefulServiceGroupName");
        VERIFY_ARE_EQUAL(statefulGroupService.DefaultMoveCost, FABRIC_MOVE_COST_MEDIUM);
        VERIFY_ARE_EQUAL(statefulGroupService.TargetReplicaSetSize, 4);
        VERIFY_ARE_EQUAL(statefulGroupService.MinReplicaSetSize, 3);
        VERIFY_ARE_EQUAL(statefulGroupService.ReplicaRestartWaitDuration, TimeSpan::FromSeconds(1));
        VERIFY_ARE_EQUAL(statefulGroupService.QuorumLossWaitDuration, TimeSpan::FromSeconds(2));
        VERIFY_ARE_EQUAL(statefulGroupService.StandByReplicaKeepDuration, TimeSpan::FromSeconds(3));
        VERIFY_ARE_EQUAL(statefulGroupService.Partition.Scheme, PartitionSchemeDescription::Named);
        VERIFY_ARE_EQUAL(statefulGroupService.Partition.PartitionNames[0], L"PartitionName");
        VERIFY_ARE_EQUAL(servicePlacement.domainName_, L"MyDomainName");
        VERIFY_ARE_EQUAL(servicePlacement.type_, FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN);
        VERIFY_ARE_EQUAL(serviceMember.ServiceTypeName, L"TypeName");
        VERIFY_ARE_EQUAL(serviceMember.MemberName, L"memberName");

        DefaultServiceDescription defaultService1 = instance.DefaultServices[0];
        ApplicationServiceDescription app1 = defaultService1.DefaultService;
        DefaultServiceDescription defaultService2 = instance.DefaultServices[1];
        ApplicationServiceDescription app2 = defaultService2.DefaultService;
        ServiceGroupMemberDescription defaultMember = app2.ServiceGroupMembers[0];
        DefaultServiceDescription defaultService3 = instance.DefaultServices[2];
        ApplicationServiceDescription app3 = defaultService3.DefaultService;

        VERIFY_ARE_EQUAL(app1.ServiceTypeName, L"StatefulService");
        VERIFY_ARE_EQUAL(app1.IsStateful, true);
        VERIFY_ARE_EQUAL(app1.IsServiceGroup, false);
        VERIFY_ARE_EQUAL(app1.Partition.Scheme, PartitionSchemeDescription::Singleton);
        VERIFY_ARE_EQUAL(defaultService1.Name, L"DefaultServiceName1");
        VERIFY_ARE_EQUAL(defaultService1.PackageActivationMode, ServicePackageActivationMode::Enum::SharedProcess);

        VERIFY_ARE_EQUAL(app2.IsStateful, false);
        VERIFY_ARE_EQUAL(app2.IsServiceGroup, true);
        VERIFY_ARE_EQUAL(app2.ServiceTypeName, L"ServiceName");
        VERIFY_ARE_EQUAL(app2.InstanceCount, 4);
        VERIFY_ARE_EQUAL(app2.Partition.Scheme, PartitionSchemeDescription::Singleton);
        VERIFY_ARE_EQUAL(defaultMember.MemberName, L"DefaultMemberName");
        VERIFY_ARE_EQUAL(defaultMember.ServiceTypeName, L"DefaultServiceName");
        VERIFY_ARE_EQUAL(defaultService2.Name, L"DefaultServiceName2");
        VERIFY_ARE_EQUAL(defaultService2.PackageActivationMode, ServicePackageActivationMode::Enum::SharedProcess);

        VERIFY_ARE_EQUAL(app3.ServiceTypeName, L"StatefulService");
        VERIFY_ARE_EQUAL(app3.IsStateful, true);
        VERIFY_ARE_EQUAL(app3.IsServiceGroup, false);
        VERIFY_ARE_EQUAL(app3.Partition.Scheme, PartitionSchemeDescription::Singleton);
        VERIFY_ARE_EQUAL(defaultService3.Name, L"DefaultServiceName3");
        VERIFY_ARE_EQUAL(defaultService3.ServiceDnsName, L"Default.Service.Name3");
        VERIFY_ARE_EQUAL(defaultService3.PackageActivationMode, ServicePackageActivationMode::Enum::ExclusiveProcess);
    }

    BOOST_FIXTURE_TEST_SUITE(ServiceModelSerializerTestSuite,ServiceModelSerializerTest)

    BOOST_AUTO_TEST_CASE(ServiceManifestSanity)
    {
        ServiceManifestDescription inputManifest;
        generateBaseServiceManifest(inputManifest);
        validateBaseServiceManifest(inputManifest);
        std::wstring fileName = L"SanityServiceManifest.xml";
        ErrorCode er = Serializer::WriteServiceManifest(fileName, inputManifest);
        VERIFY_IS_TRUE(er.IsSuccess());
        ServiceManifestDescription outputManifest;
        er = Parser::ParseServiceManifest(fileName, outputManifest);
        VERIFY_IS_TRUE(er.IsSuccess());
        validateBaseServiceManifest(outputManifest);
    }

    BOOST_AUTO_TEST_CASE(ServiceManifestAllServiceTypes)
    {
        ServiceManifestDescription inputManifest;
        generateAllServiceType(inputManifest);
        validateAllServiceType(inputManifest);
        std::wstring fileName = L"AllServiceTypeServiceManifest.xml";
        ErrorCode er = Serializer::WriteServiceManifest(fileName, inputManifest);
        VERIFY_IS_TRUE(er.IsSuccess());
        ServiceManifestDescription outputManifest;
        er = Parser::ParseServiceManifest(fileName, outputManifest);
        VERIFY_IS_TRUE(er.IsSuccess());
        validateAllServiceType(outputManifest);
    }

    BOOST_AUTO_TEST_CASE(ServiceManifestManyCodeConfigData)
    {
        ServiceManifestDescription inputManifest;
        generateManyCodeConfigData(inputManifest);
        validateManyCodeConfigData(inputManifest);
        std::wstring fileName = L"ManyCodeConfigDataServiceManifest.xml";
        ErrorCode er = Serializer::WriteServiceManifest(fileName, inputManifest);
        VERIFY_IS_TRUE(er.IsSuccess());
        ServiceManifestDescription outputManifest;
        er = Parser::ParseServiceManifest(fileName, outputManifest);
        VERIFY_IS_TRUE(er.IsSuccess());
        validateManyCodeConfigData(outputManifest);
    }

    BOOST_AUTO_TEST_CASE(ServiceManifestDiagnostics)
    {
        ServiceManifestDescription inputManifest;
        generateServiceDiagnostics(inputManifest);
        validateServiceDiagnostics(inputManifest);
        std::wstring fileName = L"DiagnosticsServiceManifest.xml";
        ErrorCode er = Serializer::WriteServiceManifest(fileName, inputManifest);
        VERIFY_IS_TRUE(er.IsSuccess());
        ServiceManifestDescription outputManifest;
        er = Parser::ParseServiceManifest(fileName, outputManifest);
        VERIFY_IS_TRUE(er.IsSuccess());
        validateServiceDiagnostics(outputManifest);
    }
    
    BOOST_AUTO_TEST_CASE(ServiceManifestResoruces)
    {
        ServiceManifestDescription inputManifest;
        generateServiceResources(inputManifest);
        validateServiceResources(inputManifest);
        std::wstring fileName = L"ResourcesServiceManifest.xml";
        ErrorCode er = Serializer::WriteServiceManifest(fileName, inputManifest);
        VERIFY_IS_TRUE(er.IsSuccess());
        ServiceManifestDescription outputManifest;
        er = Parser::ParseServiceManifest(fileName, outputManifest);
        VERIFY_IS_TRUE(er.IsSuccess());
        validateServiceResources(outputManifest);
    }

    BOOST_AUTO_TEST_CASE(ApplicationManifestSanity)
    {
        ApplicationManifestDescription inputManifest;
        generateBaseApplicationManifest(inputManifest);
        validateBaseApplicationManifest(inputManifest);
        std::wstring fileName = L"BaseApplicationManifest.xml";
        ErrorCode er = Serializer::WriteApplicationManifest(fileName, inputManifest);
        VERIFY_IS_TRUE(er.IsSuccess());
        ApplicationManifestDescription outputManifest;
        er = Parser::ParseApplicationManifest(fileName, outputManifest);
        Trace.WriteNoise(TestSource, "ParseApplicationPackage-ApplicationSanity={0}", er);
        VERIFY_IS_TRUE(er.IsSuccess());
        validateBaseApplicationManifest(outputManifest);
    }

    BOOST_AUTO_TEST_CASE(ApplicationManifestManyServices)
    {
        ApplicationManifestDescription inputManifest;
        generateApplicationServices(inputManifest);
        validateApplicationServices(inputManifest);
        std::wstring fileName = L"ServicesApplicationManifest.xml";
        ErrorCode er = Serializer::WriteApplicationManifest(fileName, inputManifest);
        Trace.WriteNoise(TestSource, "ParseApplicationManifest-ApplicationServices={0}", er);
        VERIFY_IS_TRUE(er.IsSuccess());
        ApplicationManifestDescription outputManifest;
        er = Parser::ParseApplicationManifest(fileName, outputManifest);
        Trace.WriteNoise(TestSource, "ParseApplicationManifest-ApplicationServies={0}", er);
        VERIFY_IS_TRUE(er.IsSuccess());
        validateApplicationServices(outputManifest);
    }

    BOOST_AUTO_TEST_CASE(ApplicationManifestParam)
    {
        ApplicationManifestDescription inputManifest;
        generateParametersApplication(inputManifest);
        validateParametersApplication(inputManifest);
        std::wstring fileName = L"ParametersApplicationManifest.xml";
        ErrorCode er = Serializer::WriteApplicationManifest(fileName, inputManifest);
        VERIFY_IS_TRUE(er.IsSuccess());
        ApplicationManifestDescription outputManifest;
        er = Parser::ParseApplicationManifest(fileName, outputManifest);
        Trace.WriteNoise(TestSource, "ParseApplicationManifest-ApplicationParam={0}", er);
        VERIFY_IS_TRUE(er.IsSuccess());
        validateParametersApplication(outputManifest);
    }

    BOOST_AUTO_TEST_CASE(ApplicationManifestPolicies)
    {
        ApplicationManifestDescription inputManifest;
        generatePoliciesApplication(inputManifest);
        validatePoliciesApplication(inputManifest);
        std::wstring fileName = L"PoliciesApplicationManifest.xml";
        ErrorCode er = Serializer::WriteApplicationManifest(fileName, inputManifest);
        Trace.WriteNoise(TestSource, "PoliciesApplicationManifest-ApplicationPolicies{0}", er);
        VERIFY_IS_TRUE(er.IsSuccess());
        ApplicationManifestDescription outputManifest;
        er = Parser::ParseApplicationManifest(fileName, outputManifest);
        Trace.WriteNoise(TestSource, "PoliciesApplicationManifest-ApplicationPolicies{0}", er);
        VERIFY_IS_TRUE(er.IsSuccess());
        validatePoliciesApplication(outputManifest);
    }

    BOOST_AUTO_TEST_CASE(ApplicationManifestDiagnostics)
    {
        ApplicationManifestDescription inputManifest;
        generateDiagnosticsApplication(inputManifest);
        validateDiagnosticsApplication(inputManifest);
        std::wstring fileName = L"DiagnosticsApplicationManifest.xml";
        ErrorCode er = Serializer::WriteApplicationManifest(fileName, inputManifest);
        Trace.WriteNoise(TestSource, "DiagnosticsApplicationManifest-ApplicationPolicies{0}", er);
        VERIFY_IS_TRUE(er.IsSuccess());
        ApplicationManifestDescription outputManifest;
        er = Parser::ParseApplicationManifest(fileName, outputManifest);
        Trace.WriteNoise(TestSource, "DiagnosticsApplicationManifest-ApplicationPolicies{0}", er);
        VERIFY_IS_TRUE(er.IsSuccess());
        validateDiagnosticsApplication(outputManifest);
    }

    BOOST_AUTO_TEST_CASE(ApplicationManifestCertificates)
    {
        ApplicationManifestDescription inputManifest;
        generateCertificatesApplication(inputManifest);
        validateCertificatesApplication(inputManifest);
        std::wstring fileName = L"CertificatesApplicationManifest.xml";
        ErrorCode er = Serializer::WriteApplicationManifest(fileName, inputManifest);
        Trace.WriteNoise(TestSource, "CertificatesApplicationManifest-ApplicationPolicies{0}", er);
        VERIFY_IS_TRUE(er.IsSuccess());
        ApplicationManifestDescription outputManifest;
        er = Parser::ParseApplicationManifest(fileName, outputManifest);
        Trace.WriteNoise(TestSource, "CertificatesApplicationManifest-ApplicationPolicies{0}", er);
        VERIFY_IS_TRUE(er.IsSuccess());
        validateCertificatesApplication(outputManifest);
    }

    BOOST_AUTO_TEST_CASE(ApplicationManifestPrincipals)
    {
        ApplicationManifestDescription inputManifest;
        generatePrincipalsApplication(inputManifest);
        validatePrincipalsApplication(inputManifest);
        std::wstring fileName = L"PrincipalsApplicationManifest.xml";
        ErrorCode er = Serializer::WriteApplicationManifest(fileName, inputManifest);
        Trace.WriteNoise(TestSource, "PrincipalsApplicationManifest-ApplicationPolicies{0}", er);
        VERIFY_IS_TRUE(er.IsSuccess());
        ApplicationManifestDescription outputManifest;
        er = Parser::ParseApplicationManifest(fileName, outputManifest);
        Trace.WriteNoise(TestSource, "PrincipalsApplicationManifest-ApplicationPolicies{0}", er);
        VERIFY_IS_TRUE(er.IsSuccess());
        validatePrincipalsApplication(outputManifest);
    }

    BOOST_AUTO_TEST_CASE(ServicePackageSanity)
    {
        ServicePackageDescription inputManifest;
        generateBaseServicePackage(inputManifest);
        validateBaseServicePackage(inputManifest);
        std::wstring fileName = L"BaseServicePackage.xml";
        ErrorCode er = Serializer::WriteServicePackage(fileName, inputManifest);
        Trace.WriteNoise(TestSource, "BaseServicePackage-ServicePackage{0}", er);
        VERIFY_IS_TRUE(er.IsSuccess());
        ServicePackageDescription outputManifest;
        er = Parser::ParseServicePackage(fileName, outputManifest);
        Trace.WriteNoise(TestSource, "BaseServicePackage-ServicePackage{0}", er);
        VERIFY_IS_TRUE(er.IsSuccess());
        validateBaseServicePackage(outputManifest);
    }

    BOOST_AUTO_TEST_CASE(ServicePackageConfigData)
    {
        ServicePackageDescription inputManifest;
        generateConfigDataServicePackage(inputManifest);
        validateConfigDataServicePackage(inputManifest);
        std::wstring fileName = L"ConfigDataServicePackage.xml";
        ErrorCode er = Serializer::WriteServicePackage(fileName, inputManifest);
        Trace.WriteNoise(TestSource, "ConfigDataServicePackage-ServicePackage{0}", er);
        VERIFY_IS_TRUE(er.IsSuccess());
        ServicePackageDescription outputManifest;
        er = Parser::ParseServicePackage(fileName, outputManifest);
        Trace.WriteNoise(TestSource, "ConfigDataServicePackage-ServicePackage{0}", er);
        VERIFY_IS_TRUE(er.IsSuccess());
        validateConfigDataServicePackage(outputManifest);
    }

    BOOST_AUTO_TEST_CASE(ApplicationPackageTest)
    {
        ApplicationPackageDescription inputManifest;
        generateApplicationPackage(inputManifest);
        validateApplicationPackage(inputManifest);
        std::wstring fileName = L"ApplicationPackage.xml";
        ErrorCode er = Serializer::WriteApplicationPackage(fileName, inputManifest);
        Trace.WriteNoise(TestSource, "ParseApplicationPackage-ApplicationPackage{0}", er);
        VERIFY_IS_TRUE(er.IsSuccess());
        ApplicationPackageDescription outputManifest;
        er = Parser::ParseApplicationPackage(fileName, outputManifest);
        Trace.WriteNoise(TestSource, "ParseApplicationPackage-ApplicationPackage{0}", er);
        VERIFY_IS_TRUE(er.IsSuccess());
        validateApplicationPackage(outputManifest);
    }

    BOOST_AUTO_TEST_CASE(ApplicationInstanceTest)
    {
        ApplicationInstanceDescription inputManifest;
        generateApplicationInstance(inputManifest);
        validateApplicationInstance(inputManifest);
        std::wstring fileName = L"ApplicationInstance.xml";
        ErrorCode er = Serializer::WriteApplicationInstance(fileName, inputManifest);
        Trace.WriteNoise(TestSource, "ParseApplicationInstance-ApplicationInstance{0}", er);
        VERIFY_IS_TRUE(er.IsSuccess());
        ApplicationInstanceDescription outputManifest;
        er = Parser::ParseApplicationInstance(fileName, outputManifest);
        Trace.WriteNoise(TestSource, "ParseApplicationInstance-ApplicationInstance{0}", er);
        VERIFY_IS_TRUE(er.IsSuccess());
        validateApplicationInstance(outputManifest);
    }
    BOOST_AUTO_TEST_SUITE_END()
}


