// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "Common/Common.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#if defined(PLATFORM_UNIX)
#define DLABEL
#define PATHSEP L"/"
#else
#define DLABEL L"C:"
#define PATHSEP L"\\"
#endif

namespace ImageModelTests
{
	using namespace std;
	using namespace Common;
	using namespace Management;
	using namespace ImageModel;


	struct ModuleSetup {
		ModuleSetup()
		{
			// load tracing configuration
			Config cfg;
			Common::TraceProvider::LoadConfiguration(cfg);

			//return TRUE;
		}
	};

    const StringLiteral TestSource = "ImageModelTest";

    class ImageModelTest
    {
    };

	BOOST_GLOBAL_FIXTURE(ModuleSetup);

    BOOST_FIXTURE_TEST_SUITE(ImageModelTestSuite,ImageModelTest)

    BOOST_AUTO_TEST_CASE(RelativeBuildLayoutTest)
    {

        // tests the build layout for the following canned application
        // ApplicationManifest
        //      ServiceManifest(TestServiceManifest1)
        //          CodePackage(Code1)
        //          ConfigPackage(Code1.Config)
        //          DataPackage(Data1)
        //      ServiceManifest(TestServiceManifest2)
        //

        ImageModel::BuildLayoutSpecification bl;
        bl.GetApplicationManifestFile();

        Trace.WriteNoise(TestSource, "GetApplicationManifestFile()={0}", 
            bl.GetApplicationManifestFile());
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            bl.GetApplicationManifestFile(), 
            L"ApplicationManifest.xml"));

        Trace.WriteNoise(TestSource, "GetServiceManifestFile(TestServiceManifest1)={0}", 
            bl.GetServiceManifestFile(L"TestServiceManifest1"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            bl.GetServiceManifestFile(L"TestServiceManifest1"), 
            L"TestServiceManifest1" PATHSEP L"ServiceManifest.xml"));

        Trace.WriteNoise(TestSource, "GetServiceManifestFile(TestServiceManifest2)={0}", 
            bl.GetServiceManifestFile(L"TestServiceManifest2"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            bl.GetServiceManifestFile(L"TestServiceManifest2"), 
            L"TestServiceManifest2" PATHSEP L"ServiceManifest.xml"));

        Trace.WriteNoise(TestSource, "GetCodePackageFolder(TestServiceManifest1, Code1)={0}", 
            bl.GetCodePackageFolder(L"TestServiceManifest1", L"Code1"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            bl.GetCodePackageFolder(L"TestServiceManifest1", L"Code1"), 
            L"TestServiceManifest1" PATHSEP L"Code1"));

        Trace.WriteNoise(TestSource, "GetConfigPackageFolder(TestServiceManifest1, Code1.Config)={0}", 
            bl.GetConfigPackageFolder(L"TestServiceManifest1", L"Code1.Config"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            bl.GetConfigPackageFolder(L"TestServiceManifest1", L"Code1.Config"), 
            L"TestServiceManifest1" PATHSEP L"Code1.Config"));

        Trace.WriteNoise(TestSource, "GetDataPackageFolder(TestServiceManifest1, Data1)={0}", 
            bl.GetDataPackageFolder(L"TestServiceManifest1", L"Data1"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            bl.GetDataPackageFolder(L"TestServiceManifest1", L"Data1"), 
            L"TestServiceManifest1" PATHSEP L"Data1"));

        Trace.WriteNoise(TestSource, "GetSettingsFile(bl.GetConfigPackageFolder(TestServiceManifest1, Code1.Config))={0}", 
            bl.GetSettingsFile(bl.GetConfigPackageFolder(L"TestServiceManifest1", L"Code1.Config")));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            bl.GetSettingsFile(bl.GetConfigPackageFolder(L"TestServiceManifest1", L"Code1.Config")), 
            L"TestServiceManifest1" PATHSEP L"Code1.Config" PATHSEP L"Settings.xml"));
    }

    BOOST_AUTO_TEST_CASE(AbsoluteLocalBuildLayoutTest)
    {
        ImageModel::BuildLayoutSpecification bl(DLABEL PATHSEP L"Resources" PATHSEP L"WAFab");
        bl.GetApplicationManifestFile();

        Trace.WriteNoise(TestSource, "GetApplicationManifestFile()={0}", 
            bl.GetApplicationManifestFile());
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            bl.GetApplicationManifestFile(), 
            DLABEL PATHSEP L"Resources" PATHSEP L"WAFab" PATHSEP L"ApplicationManifest.xml"));

        Trace.WriteNoise(TestSource, "GetServiceManifestFile(TestServiceManifest1)={0}", 
            bl.GetServiceManifestFile(L"TestServiceManifest1"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            bl.GetServiceManifestFile(L"TestServiceManifest1"), 
            DLABEL PATHSEP L"Resources" PATHSEP L"WAFab" PATHSEP L"TestServiceManifest1" PATHSEP L"ServiceManifest.xml"));

        Trace.WriteNoise(TestSource, "GetServiceManifestFile(TestServiceManifest2)={0}", 
            bl.GetServiceManifestFile(L"TestServiceManifest2"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            bl.GetServiceManifestFile(L"TestServiceManifest2"), 
            DLABEL PATHSEP L"Resources" PATHSEP L"WAFab" PATHSEP L"TestServiceManifest2" PATHSEP L"ServiceManifest.xml"));

        Trace.WriteNoise(TestSource, "GetCodePackageFolder(TestServiceManifest1, Code1)={0}", 
            bl.GetCodePackageFolder(L"TestServiceManifest1", L"Code1"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            bl.GetCodePackageFolder(L"TestServiceManifest1", L"Code1"), 
            DLABEL PATHSEP L"Resources" PATHSEP L"WAFab" PATHSEP L"TestServiceManifest1" PATHSEP L"Code1"));

        Trace.WriteNoise(TestSource, "GetConfigPackageFolder(TestServiceManifest1, Code1.Config)={0}", 
            bl.GetConfigPackageFolder(L"TestServiceManifest1", L"Code1.Config"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            bl.GetConfigPackageFolder(L"TestServiceManifest1", L"Code1.Config"), 
            DLABEL PATHSEP L"Resources" PATHSEP L"WAFab" PATHSEP L"TestServiceManifest1" PATHSEP L"Code1.Config"));

        Trace.WriteNoise(TestSource, "GetDataPackageFolder(TestServiceManifest1, Data1)={0}", 
            bl.GetDataPackageFolder(L"TestServiceManifest1", L"Data1"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            bl.GetDataPackageFolder(L"TestServiceManifest1", L"Data1"), 
            DLABEL PATHSEP L"Resources" PATHSEP L"WAFab" PATHSEP L"TestServiceManifest1" PATHSEP L"Data1"));

        Trace.WriteNoise(TestSource, "GetSettingsFile(bl.GetConfigPackageFolder(TestServiceManifest1, Code1.Config))={0}", 
            bl.GetSettingsFile(bl.GetConfigPackageFolder(L"TestServiceManifest1", L"Code1.Config")));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            bl.GetSettingsFile(bl.GetConfigPackageFolder(L"TestServiceManifest1", L"Code1.Config")), 
            DLABEL PATHSEP L"Resources" PATHSEP L"WAFab" PATHSEP L"TestServiceManifest1" PATHSEP L"Code1.Config" PATHSEP L"Settings.xml"));
    }

#if !defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(AbsoluteServerBuildLayoutTest)
    {
        ImageModel::BuildLayoutSpecification bl(L"\\winfabfs\\ImageStore\\");
        bl.GetApplicationManifestFile();

        Trace.WriteNoise(TestSource, "GetApplicationManifestFile()={0}", 
            bl.GetApplicationManifestFile());
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            bl.GetApplicationManifestFile(), 
            L"\\winfabfs\\ImageStore\\ApplicationManifest.xml"));

        Trace.WriteNoise(TestSource, "GetServiceManifestFile(TestServiceManifest1)={0}", 
            bl.GetServiceManifestFile(L"TestServiceManifest1"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            bl.GetServiceManifestFile(L"TestServiceManifest1"), 
            L"\\winfabfs\\ImageStore\\TestServiceManifest1\\ServiceManifest.xml"));

        Trace.WriteNoise(TestSource, "GetServiceManifestFile(TestServiceManifest2)={0}", 
            bl.GetServiceManifestFile(L"TestServiceManifest2"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            bl.GetServiceManifestFile(L"TestServiceManifest2"), 
            L"\\winfabfs\\ImageStore\\TestServiceManifest2\\ServiceManifest.xml"));

        Trace.WriteNoise(TestSource, "GetCodePackageFolder(TestServiceManifest1, Code1)={0}", 
            bl.GetCodePackageFolder(L"TestServiceManifest1", L"Code1"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            bl.GetCodePackageFolder(L"TestServiceManifest1", L"Code1"), 
            L"\\winfabfs\\ImageStore\\TestServiceManifest1\\Code1"));

        Trace.WriteNoise(TestSource, "GetConfigPackageFolder(TestServiceManifest1, Code1.Config)={0}", 
            bl.GetConfigPackageFolder(L"TestServiceManifest1", L"Code1.Config"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            bl.GetConfigPackageFolder(L"TestServiceManifest1", L"Code1.Config"), 
            L"\\winfabfs\\ImageStore\\TestServiceManifest1\\Code1.Config"));

        Trace.WriteNoise(TestSource, "GetDataPackageFolder(TestServiceManifest1, Data1)={0}", 
            bl.GetDataPackageFolder(L"TestServiceManifest1", L"Data1"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            bl.GetDataPackageFolder(L"TestServiceManifest1", L"Data1"), 
            L"\\winfabfs\\ImageStore\\TestServiceManifest1\\Data1"));

        Trace.WriteNoise(TestSource, "GetSettingsFile(bl.GetConfigPackageFolder(TestServiceManifest1, Code1.Config))={0}", 
            bl.GetSettingsFile(bl.GetConfigPackageFolder(L"TestServiceManifest1", L"Code1.Config")));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            bl.GetSettingsFile(bl.GetConfigPackageFolder(L"TestServiceManifest1", L"Code1.Config")), 
            L"\\winfabfs\\ImageStore\\TestServiceManifest1\\Code1.Config\\Settings.xml"));
    }
#endif
    BOOST_AUTO_TEST_CASE(RelativeStoreLayoutTest)
    {
        // WinFabTestTeamAppType (Version: 3.0)
        //     ServiceManifest: WinFabTestTeamService (Version: 2.0)
        //         Code Package: UserServiceHost (Version: Nylon_1.1)
        //         Config Package: UserServiceHost.Config (Version: Cotton)
        //         Data Package:: UserServiceHost.Data (Version: 2.0)
        // Package: WinFabTestTeamService,Rollout6521
        // Override for WinFabTestTeamService,Rollout6520
        ImageModel::StoreLayoutSpecification sl;

        Trace.WriteNoise(TestSource, "sl.GetApplicationManifestFile(L\"WinFabTestTeamAppType\", L\"3.0\")={0}", 
            sl.GetApplicationManifestFile(L"WinFabTestTeamAppType", L"3.0"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetApplicationManifestFile(L"WinFabTestTeamAppType", L"3.0"),
            L"Store" PATHSEP L"WinFabTestTeamAppType" PATHSEP L"ApplicationManifest.3.0.xml"));

        Trace.WriteNoise(TestSource, "sl.GetServiceManifestFile(L\"WinFabTestTeamAppType\", L\"WinFabTestTeamService\", L\"2.0\")={0}", 
            sl.GetServiceManifestFile(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"2.0"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetServiceManifestFile(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"2.0"),
            L"Store" PATHSEP L"WinFabTestTeamAppType" PATHSEP L"WinFabTestTeamService.Manifest.2.0.xml"));

        Trace.WriteNoise(TestSource, "sl.GetCodePackageFolder(L\"WinFabTestTeamAppType\", L\"WinFabTestTeamService\", L\"UserServiceHost\", L\"Nylon_1.1\")={0}", 
            sl.GetCodePackageFolder(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"UserServiceHost", L"Nylon_1.1"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetCodePackageFolder(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"UserServiceHost", L"Nylon_1.1"),
            L"Store" PATHSEP L"WinFabTestTeamAppType" PATHSEP L"WinFabTestTeamService.UserServiceHost.Nylon_1.1"));

        Trace.WriteNoise(TestSource, "sl.GetCodePackageFolder(L\"WinFabTestTeamAppType\", L\"WinFabTestTeamService\", L\"UserServiceHost.Config\", L\"Cotton\")={0}", 
            sl.GetConfigPackageFolder(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"UserServiceHost.Config", L"Cotton"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetConfigPackageFolder(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"UserServiceHost.Config", L"Cotton"),
            L"Store" PATHSEP L"WinFabTestTeamAppType" PATHSEP L"WinFabTestTeamService.UserServiceHost.Config.Cotton"));

        wstring configFolder = sl.GetConfigPackageFolder(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"UserServiceHost.Config", L"Cotton");
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetSettingsFile(configFolder),
            L"Store" PATHSEP L"WinFabTestTeamAppType" PATHSEP L"WinFabTestTeamService.UserServiceHost.Config.Cotton" PATHSEP L"Settings.xml"));

        Trace.WriteNoise(TestSource, "sl.GetCodePackageFolder(L\"WinFabTestTeamAppType\", L\"WinFabTestTeamService\", L\"Data\", L\"2.0\")={0}", 
            sl.GetDataPackageFolder(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"UserServiceHost.Data", L"2.0"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetDataPackageFolder(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"UserServiceHost.Data", L"2.0"),
            L"Store" PATHSEP L"WinFabTestTeamAppType" PATHSEP L"WinFabTestTeamService.UserServiceHost.Data.2.0"));

        Trace.WriteNoise(TestSource, "sl.GetApplicationInstanceFile(L\"WinFabTestTeamAppType\", L\"B80DF87E-D725-45EA-A73B-6CB60E654121\", L\"Rollout6521\")={0}", 
            sl.GetApplicationInstanceFile(L"WinFabTestTeamAppType", L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"Rollout6521"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetApplicationInstanceFile(L"WinFabTestTeamAppType", L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"Rollout6521"),
            L"Store" PATHSEP L"WinFabTestTeamAppType" PATHSEP L"apps" PATHSEP L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"ApplicationInstance.Rollout6521.xml"));

        Trace.WriteNoise(TestSource, "sl.GetServicePackageFile(L\"WinFabTestTeamAppType\", L\"B80DF87E-D725-45EA-A73B-6CB60E654121\", L\"WinFabTestTeamService\", L\"Rollout6521\")={0}", 
            sl.GetServicePackageFile(L"WinFabTestTeamAppType", L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"Rollout6521"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetServicePackageFile(L"WinFabTestTeamAppType", L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"Rollout6521"),
            L"Store" PATHSEP L"WinFabTestTeamAppType" PATHSEP L"apps" PATHSEP L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"WinFabTestTeamService.Package.Rollout6521.xml"));
    }

     BOOST_AUTO_TEST_CASE(AboluteLocalStoreLayoutTest)
    {
        // WinFabTestTeamAppType (Version: 3.0)
        //     ServiceManifest: WinFabTestTeamService (Version: 2.0)
        //         Code Package: UserServiceHost (Version: Nylon_1.1)
        //         Config Package: UserServiceHost.Config (Version: Cotton)
        //         Data Package:: UserServiceHost.Data (Version: 2.0)
        // Package: WinFabTestTeamService,Rollout6521
        // Override for WinFabTestTeamService,Rollout6520
        ImageModel::StoreLayoutSpecification sl(DLABEL PATHSEP L"Resources" PATHSEP L"WAFab" PATHSEP);

        Trace.WriteNoise(TestSource, "sl.GetApplicationManifestFile(L\"WinFabTestTeamAppType\", L\"3.0\")={0}", 
            sl.GetApplicationManifestFile(L"WinFabTestTeamAppType", L"3.0"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetApplicationManifestFile(L"WinFabTestTeamAppType", L"3.0"),
            DLABEL PATHSEP L"Resources" PATHSEP L"WAFab" PATHSEP L"Store" PATHSEP L"WinFabTestTeamAppType" PATHSEP L"ApplicationManifest.3.0.xml"));

        Trace.WriteNoise(TestSource, "sl.GetServiceManifestFile(L\"WinFabTestTeamAppType\", L\"WinFabTestTeamService\", L\"2.0\")={0}", 
            sl.GetServiceManifestFile(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"2.0"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetServiceManifestFile(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"2.0"),
            DLABEL PATHSEP L"Resources" PATHSEP L"WAFab" PATHSEP L"Store" PATHSEP L"WinFabTestTeamAppType" PATHSEP L"WinFabTestTeamService.Manifest.2.0.xml"));

        Trace.WriteNoise(TestSource, "sl.GetCodePackageFolder(L\"WinFabTestTeamAppType\", L\"WinFabTestTeamService\", L\"UserServiceHost\", L\"Nylon_1.1\")={0}", 
            sl.GetCodePackageFolder(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"UserServiceHost", L"Nylon_1.1"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetCodePackageFolder(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"UserServiceHost", L"Nylon_1.1"),
            DLABEL PATHSEP L"Resources" PATHSEP L"WAFab" PATHSEP L"Store" PATHSEP L"WinFabTestTeamAppType" PATHSEP L"WinFabTestTeamService.UserServiceHost.Nylon_1.1"));

        Trace.WriteNoise(TestSource, "sl.GetCodePackageFolder(L\"WinFabTestTeamAppType\", L\"WinFabTestTeamService\", L\"UserServiceHost.Config\", L\"Cotton\")={0}", 
            sl.GetConfigPackageFolder(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"UserServiceHost.Config", L"Cotton"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetConfigPackageFolder(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"UserServiceHost.Config", L"Cotton"),
            DLABEL PATHSEP L"Resources" PATHSEP L"WAFab" PATHSEP L"Store" PATHSEP L"WinFabTestTeamAppType" PATHSEP L"WinFabTestTeamService.UserServiceHost.Config.Cotton"));

        wstring configFolder = sl.GetConfigPackageFolder(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"UserServiceHost.Config", L"Cotton");
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetSettingsFile(configFolder),
            DLABEL PATHSEP L"Resources" PATHSEP L"WAFab" PATHSEP L"Store" PATHSEP L"WinFabTestTeamAppType" PATHSEP L"WinFabTestTeamService.UserServiceHost.Config.Cotton" PATHSEP L"Settings.xml"));
        
        Trace.WriteNoise(TestSource, "sl.GetCodePackageFolder(L\"WinFabTestTeamAppType\", L\"WinFabTestTeamService\", L\"Data\", L\"2.0\")={0}", 
            sl.GetDataPackageFolder(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"UserServiceHost.Data", L"2.0"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetDataPackageFolder(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"UserServiceHost.Data", L"2.0"),
            DLABEL PATHSEP L"Resources" PATHSEP L"WAFab" PATHSEP L"Store" PATHSEP L"WinFabTestTeamAppType" PATHSEP L"WinFabTestTeamService.UserServiceHost.Data.2.0"));

        Trace.WriteNoise(TestSource, "sl.GetApplicationInstanceFile(L\"WinFabTestTeamAppType\", L\"B80DF87E-D725-45EA-A73B-6CB60E654121\", L\"Rollout6521\")={0}", 
            sl.GetApplicationInstanceFile(L"WinFabTestTeamAppType", L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"Rollout6521"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetApplicationInstanceFile(L"WinFabTestTeamAppType", L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"Rollout6521"),
            DLABEL PATHSEP L"Resources" PATHSEP L"WAFab" PATHSEP L"Store" PATHSEP L"WinFabTestTeamAppType" PATHSEP L"apps" PATHSEP L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"ApplicationInstance.Rollout6521.xml"));

        Trace.WriteNoise(TestSource, "sl.GetServicePackageFile(L\"WinFabTestTeamAppType\", L\"B80DF87E-D725-45EA-A73B-6CB60E654121\", L\"WinFabTestTeamService\", L\"Rollout6521\")={0}", 
            sl.GetServicePackageFile(L"WinFabTestTeamAppType", L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"Rollout6521"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetServicePackageFile(L"WinFabTestTeamAppType", L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"Rollout6521"),
            DLABEL PATHSEP L"Resources" PATHSEP L"WAFab" PATHSEP L"Store" PATHSEP L"WinFabTestTeamAppType" PATHSEP L"apps" PATHSEP L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"WinFabTestTeamService.Package.Rollout6521.xml"));
    }

#if !defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(AboluteServerStoreLayoutTest)
    {
        // WinFabTestTeamAppType (Version: 3.0)
        //     ServiceManifest: WinFabTestTeamService (Version: 2.0)
        //         Code Package: UserServiceHost (Version: Nylon_1.1)
        //         Config Package: UserServiceHost.Config (Version: Cotton)
        //         Data Package:: UserServiceHost.Data (Version: 2.0)
        // Package: WinFabTestTeamService,Rollout6521
        // Override for WinFabTestTeamService,Rollout6520
        ImageModel::StoreLayoutSpecification sl(L"\\server\\imagestore");

        Trace.WriteNoise(TestSource, "sl.GetApplicationManifestFile(L\"WinFabTestTeamAppType\", L\"3.0\")={0}", 
            sl.GetApplicationManifestFile(L"WinFabTestTeamAppType", L"3.0"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetApplicationManifestFile(L"WinFabTestTeamAppType", L"3.0"),
            L"\\server\\imagestore\\Store\\WinFabTestTeamAppType\\ApplicationManifest.3.0.xml"));

        Trace.WriteNoise(TestSource, "sl.GetServiceManifestFile(L\"WinFabTestTeamAppType\", L\"WinFabTestTeamService\", L\"2.0\")={0}", 
            sl.GetServiceManifestFile(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"2.0"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetServiceManifestFile(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"2.0"),
            L"\\server\\imagestore\\Store\\WinFabTestTeamAppType\\WinFabTestTeamService.Manifest.2.0.xml"));

        Trace.WriteNoise(TestSource, "sl.GetCodePackageFolder(L\"WinFabTestTeamAppType\", L\"WinFabTestTeamService\", L\"UserServiceHost\", L\"Nylon_1.1\")={0}", 
            sl.GetCodePackageFolder(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"UserServiceHost", L"Nylon_1.1"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetCodePackageFolder(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"UserServiceHost", L"Nylon_1.1"),
            L"\\server\\imagestore\\Store\\WinFabTestTeamAppType\\WinFabTestTeamService.UserServiceHost.Nylon_1.1"));

        Trace.WriteNoise(TestSource, "sl.GetCodePackageFolder(L\"WinFabTestTeamAppType\", L\"WinFabTestTeamService\", L\"UserServiceHost.Config\", L\"Cotton\")={0}", 
            sl.GetConfigPackageFolder(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"UserServiceHost.Config", L"Cotton"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetConfigPackageFolder(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"UserServiceHost.Config", L"Cotton"),
            L"\\server\\imagestore\\Store\\WinFabTestTeamAppType\\WinFabTestTeamService.UserServiceHost.Config.Cotton"));

        wstring configFolder = sl.GetConfigPackageFolder(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"UserServiceHost.Config", L"Cotton");
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetSettingsFile(configFolder),
            L"\\server\\imagestore\\Store\\WinFabTestTeamAppType\\WinFabTestTeamService.UserServiceHost.Config.Cotton\\Settings.xml"));

        Trace.WriteNoise(TestSource, "sl.GetCodePackageFolder(L\"WinFabTestTeamAppType\", L\"WinFabTestTeamService\", L\"Data\", L\"2.0\")={0}", 
            sl.GetDataPackageFolder(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"UserServiceHost.Data", L"2.0"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetDataPackageFolder(L"WinFabTestTeamAppType", L"WinFabTestTeamService", L"UserServiceHost.Data", L"2.0"),
            L"\\server\\imagestore\\Store\\WinFabTestTeamAppType\\WinFabTestTeamService.UserServiceHost.Data.2.0"));

        Trace.WriteNoise(TestSource, "sl.GetApplicationInstanceFile(L\"WinFabTestTeamAppType\", L\"B80DF87E-D725-45EA-A73B-6CB60E654121\", L\"Rollout6521\")={0}", 
            sl.GetApplicationInstanceFile(L"WinFabTestTeamAppType", L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"Rollout6521"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetApplicationInstanceFile(L"WinFabTestTeamAppType", L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"Rollout6521"),
            L"\\server\\imagestore\\Store\\WinFabTestTeamAppType\\apps\\B80DF87E-D725-45EA-A73B-6CB60E654121\\ApplicationInstance.Rollout6521.xml"));

        Trace.WriteNoise(TestSource, "sl.GetServicePackageFile(L\"WinFabTestTeamAppType\", L\"B80DF87E-D725-45EA-A73B-6CB60E654121\", L\"WinFabTestTeamService\", L\"Rollout6521\")={0}", 
            sl.GetServicePackageFile(L"WinFabTestTeamAppType", L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"Rollout6521"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            sl.GetServicePackageFile(L"WinFabTestTeamAppType", L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"Rollout6521"),
            L"\\server\\imagestore\\Store\\WinFabTestTeamAppType\\apps\\B80DF87E-D725-45EA-A73B-6CB60E654121\\WinFabTestTeamService.Package.Rollout6521.xml"));
    }
#endif

    BOOST_AUTO_TEST_CASE(RelativeRunLayoutTest)
    {
        // WinFabTestTeamAppType (Version: 3.0)
        //     ServiceManifest: WinFabTestTeamService (Version: 2.0)
        //         Code Package: UserServiceHost (Version: Nylon_1.1)
        //         Config Package: UserServiceHost.Config (Version: Cotton)
        //         Data Package:: UserServiceHost.Data (Version: 2.0)
        // Package: WinFabTestTeamService,Rollout6521
        // Override for WinFabTestTeamService,Rollout6520
        ImageModel::RunLayoutSpecification rl;

        Trace.WriteNoise(
            TestSource,
            "rl.GetApplicationFolder(L\"B80DF87E-D725-45EA-A73B-6CB60E654121\")={0}",
            rl.GetApplicationFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121"));

        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            rl.GetApplicationFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121"),
            L"B80DF87E-D725-45EA-A73B-6CB60E654121"));

        Trace.WriteNoise(
            TestSource,
            "rl.GetApplicationLogFolder(L\"B80DF87E-D725-45EA-A73B-6CB60E654121\")={0}",
            rl.GetApplicationLogFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            rl.GetApplicationLogFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121"),
            L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"log"));

        Trace.WriteNoise(
            TestSource,
            "rl.GetApplicationWorkFolder(L\"B80DF87E-D725-45EA-A73B-6CB60E654121\")={0}",
            rl.GetApplicationWorkFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            rl.GetApplicationWorkFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121"),
            L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"work"));

        Trace.WriteNoise(
            TestSource,
            "rl.GetCodePackageFolder(L\"B80DF87E-D725-45EA-A73B-6CB60E654121\", L\"WinFabTestTeamService\", L\"UserServiceHost\", L\"Nylon_1.1\")={0}",
            rl.GetCodePackageFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"UserServiceHost", L"Nylon_1.1", false));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            rl.GetCodePackageFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"UserServiceHost", L"Nylon_1.1", false),
            L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"WinFabTestTeamService.UserServiceHost.Nylon_1.1"));

        Trace.WriteNoise(
            TestSource,
            "rl.GetConfigPackageFolder(L\"B80DF87E-D725-45EA-A73B-6CB60E654121\", L\"WinFabTestTeamService\", L\"UserServiceHost.Config\", L\"Cotton\")={0}",
            rl.GetConfigPackageFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"UserServiceHost.Config", L"Cotton", false));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            rl.GetConfigPackageFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"UserServiceHost.Config", L"Cotton", false),
            L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"WinFabTestTeamService.UserServiceHost.Config.Cotton"));

        wstring configFolder = rl.GetConfigPackageFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"UserServiceHost.Config", L"Cotton", false);
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            rl.GetSettingsFile(configFolder),
            L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"WinFabTestTeamService.UserServiceHost.Config.Cotton" PATHSEP L"Settings.xml"));

        Trace.WriteNoise(
            TestSource,
            "rl.GetDataPackageFolder(L\"B80DF87E-D725-45EA-A73B-6CB60E654121\", L\"WinFabTestTeamService\", L\"UserServiceHost.Data\", L\"2.0\")={0}",
            rl.GetDataPackageFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"UserServiceHost.Data", L"2.0", false));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            rl.GetDataPackageFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"UserServiceHost.Data", L"2.0", false),
            L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"WinFabTestTeamService.UserServiceHost.Data.2.0"));

        Trace.WriteNoise(
            TestSource,
            "rl.GetServiceManifestFile(L\"B80DF87E-D725-45EA-A73B-6CB60E654121\", L\"WinFabTestTeamService\", L\"2.0\")={0}",
            rl.GetServiceManifestFile(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"2.0"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            rl.GetServiceManifestFile(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"2.0"),
            L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"WinFabTestTeamService.Manifest.2.0.xml"));


        Trace.WriteNoise(
            TestSource,
            "rl.GetServicePackageFile(L\"B80DF87E-D725-45EA-A73B-6CB60E654121\", L\"WinFabTestTeamService\", L\"Rollout6521\")={0}",
            rl.GetServicePackageFile(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"Rollout6521"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            rl.GetServicePackageFile(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"Rollout6521"),
            L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"WinFabTestTeamService.Package.Rollout6521.xml"));

        Trace.WriteNoise(
            TestSource,
            "rl.GetServicePackageContextFile(L\"B80DF87E-D725-45EA-A73B-6CB60E654121\", L\"WinFabTestTeamService\")={0}",
            rl.GetCurrentServicePackageFile(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L""));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            rl.GetCurrentServicePackageFile(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L""),
            L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"WinFabTestTeamService.Package.Current.xml"));

        Trace.WriteNoise(
            TestSource,
            "rl.GetApplicationInstanceFile(L\"B80DF87E-D725-45EA-A73B-6CB60E654121\", L\"2.0\")={0}",
            rl.GetApplicationPackageFile(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"2.0"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            rl.GetApplicationPackageFile(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"2.0"),
            L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"App.2.0.xml"));
    }

    BOOST_AUTO_TEST_CASE(AbsoluteLocalRunLayoutTest)
    {
        // WinFabTestTeamAppType (Version: 3.0)
        //     ServiceManifest: WinFabTestTeamService (Version: 2.0)
        //         Code Package: UserServiceHost (Version: Nylon_1.1)
        //         Config Package: UserServiceHost.Config (Version: Cotton)
        //         Data Package:: UserServiceHost.Data (Version: 2.0)
        // Package: WinFabTestTeamService,Rollout6521
        // Override for WinFabTestTeamService,Rollout6520
        ImageModel::RunLayoutSpecification rl(DLABEL PATHSEP L"temp" PATHSEP L"RunLayout" PATHSEP L"[NodeName]");

        Trace.WriteNoise(
            TestSource,
            "rl.GetApplicationFolder(L\"B80DF87E-D725-45EA-A73B-6CB60E654121\")={0}",
            rl.GetApplicationFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121"));

        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            rl.GetApplicationFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121"),
            DLABEL PATHSEP L"temp" PATHSEP L"RunLayout" PATHSEP L"[NodeName]" PATHSEP L"B80DF87E-D725-45EA-A73B-6CB60E654121"));

        Trace.WriteNoise(
            TestSource,
            "rl.GetApplicationLogFolder(L\"B80DF87E-D725-45EA-A73B-6CB60E654121\")={0}",
            rl.GetApplicationLogFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            rl.GetApplicationLogFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121"),
            DLABEL PATHSEP L"temp" PATHSEP L"RunLayout" PATHSEP L"[NodeName]" PATHSEP L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"log"));

        Trace.WriteNoise(
            TestSource,
            "rl.GetApplicationWorkFolder(L\"B80DF87E-D725-45EA-A73B-6CB60E654121\")={0}",
            rl.GetApplicationWorkFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            rl.GetApplicationWorkFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121"),
            DLABEL PATHSEP L"temp" PATHSEP L"RunLayout" PATHSEP L"[NodeName]" PATHSEP L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"work"));

        Trace.WriteNoise(
            TestSource,
            "rl.GetCodePackageFolder(L\"B80DF87E-D725-45EA-A73B-6CB60E654121\", L\"WinFabTestTeamService\", L\"UserServiceHost\", L\"Nylon_1.1\")={0}",
            rl.GetCodePackageFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"UserServiceHost", L"Nylon_1.1", false));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            rl.GetCodePackageFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"UserServiceHost", L"Nylon_1.1", false),
            DLABEL PATHSEP L"temp" PATHSEP L"RunLayout" PATHSEP L"[NodeName]" PATHSEP L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"WinFabTestTeamService.UserServiceHost.Nylon_1.1"));

        Trace.WriteNoise(
            TestSource,
            "rl.GetConfigPackageFolder(L\"B80DF87E-D725-45EA-A73B-6CB60E654121\", L\"WinFabTestTeamService\", L\"UserServiceHost.Config\", L\"Cotton\")={0}",
            rl.GetConfigPackageFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"UserServiceHost.Config", L"Cotton", false));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            rl.GetConfigPackageFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"UserServiceHost.Config", L"Cotton", false),
            DLABEL PATHSEP L"temp" PATHSEP L"RunLayout" PATHSEP L"[NodeName]" PATHSEP L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"WinFabTestTeamService.UserServiceHost.Config.Cotton"));

        wstring configFolder = rl.GetConfigPackageFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"UserServiceHost.Config", L"Cotton", false);
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            rl.GetSettingsFile(configFolder),
            DLABEL PATHSEP L"temp" PATHSEP L"RunLayout" PATHSEP L"[NodeName]" PATHSEP L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"WinFabTestTeamService.UserServiceHost.Config.Cotton" PATHSEP L"Settings.xml"));

        Trace.WriteNoise(
            TestSource,
            "rl.GetDataPackageFolder(L\"B80DF87E-D725-45EA-A73B-6CB60E654121\", L\"WinFabTestTeamService\", L\"UserServiceHost.Data\", L\"2.0\")={0}",
            rl.GetDataPackageFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"UserServiceHost.Data", L"2.0", false));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            rl.GetDataPackageFolder(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"UserServiceHost.Data", L"2.0", false),
            DLABEL PATHSEP L"temp" PATHSEP L"RunLayout" PATHSEP L"[NodeName]" PATHSEP L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"WinFabTestTeamService.UserServiceHost.Data.2.0"));

        Trace.WriteNoise(
            TestSource,
            "rl.GetServiceManifestFile(L\"B80DF87E-D725-45EA-A73B-6CB60E654121\", L\"WinFabTestTeamService\", L\"2.0\")={0}",
            rl.GetServiceManifestFile(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"2.0"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            rl.GetServiceManifestFile(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"2.0"),
            DLABEL PATHSEP L"temp" PATHSEP L"RunLayout" PATHSEP L"[NodeName]" PATHSEP L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"WinFabTestTeamService.Manifest.2.0.xml"));

        Trace.WriteNoise(
            TestSource,
            "rl.GetServicePackageFile(L\"B80DF87E-D725-45EA-A73B-6CB60E654121\", L\"WinFabTestTeamService\", L\"Rollout6521\")={0}",
            rl.GetServicePackageFile(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"Rollout6521"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            rl.GetServicePackageFile(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L"Rollout6521"),
            DLABEL PATHSEP L"temp" PATHSEP L"RunLayout" PATHSEP L"[NodeName]" PATHSEP L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"WinFabTestTeamService.Package.Rollout6521.xml"));

        Trace.WriteNoise(
            TestSource,
            "rl.GetServicePackageContextFile(L\"B80DF87E-D725-45EA-A73B-6CB60E654121\", L\"WinFabTestTeamService\")={0}",
            rl.GetCurrentServicePackageFile(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L""));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(
            rl.GetCurrentServicePackageFile(L"B80DF87E-D725-45EA-A73B-6CB60E654121", L"WinFabTestTeamService", L""),
            DLABEL PATHSEP L"temp" PATHSEP L"RunLayout" PATHSEP L"[NodeName]" PATHSEP L"B80DF87E-D725-45EA-A73B-6CB60E654121" PATHSEP L"WinFabTestTeamService.Package.Current.xml"));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
