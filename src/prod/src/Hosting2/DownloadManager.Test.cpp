// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include "Hosting2/FabricNodeHost.Test.h"


using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;
using namespace Management;
using namespace ImageModel;
using namespace Management::FileStoreService;

const StringLiteral TraceType("DownloadManagerTest");

class DownloadManagerTestHelper
{
public:

    DownloadManagerTestHelper() : fabricNodeHost_(make_shared<TestFabricNodeHost>())
    {}

    DownloadManager & GetDownloadManager();

    bool VerifyDeployment(wstring const & deploymentPath, vector<wstring> const & expectedFiles);
    bool VerifySymbolicLinkDeployment(wstring const & deploymentPath, vector<wstring> const & expectedLinks);
    bool VerifyWriteInSymbolicDirectories(wstring appId, wstring const & deploymentPath, vector<wstring> const & expectedLinks);

    void SetupJbodDirectories(__out FabricNodeConfigSPtr & config);

    bool Setup();
    bool Cleanup();

public:
    vector<wstring> calculatorAppFiles;
    vector<wstring> calculatorPackageFiles;
    vector<wstring> sandboxedPQueueAppFiles;
    vector<wstring> sandboxedPQueuePackageFiles;
    vector<wstring> fabricUpgradeFiles;

    shared_ptr<TestFabricNodeHost> fabricNodeHost_;
};

class DownloadManagerTest
{
public:
    DownloadManagerTest() : downloadManagerTestHelper_() { BOOST_REQUIRE(Setup()); }
    TEST_CLASS_SETUP(Setup);
    ~DownloadManagerTest() { BOOST_REQUIRE(Cleanup()); }
    TEST_CLASS_CLEANUP(Cleanup);

    DownloadManagerTestHelper downloadManagerTestHelper_;
};

class DownloadAndActivateManagerTest
{
protected:
    DownloadAndActivateManagerTest() : downloadManagerTestHelper_() { BOOST_REQUIRE(Setup()); }
    TEST_CLASS_SETUP(Setup);
    ~DownloadAndActivateManagerTest() { BOOST_REQUIRE(Cleanup()); }
    TEST_CLASS_CLEANUP(Cleanup);

    DownloadManagerTestHelper downloadManagerTestHelper_;
};

BOOST_FIXTURE_TEST_SUITE(DownloadManagerTestSuite,DownloadManagerTest)

BOOST_AUTO_TEST_CASE(DownloadFabricUpgradePackageTest)
{    
    Trace.WriteInfo(TraceType, "DownloadFabricUpgradePackageTest Start");

     ManualResetEvent deployDone;    
     FabricVersion FabricVersion;
     auto error = FabricVersion::FromString(L"1.0.961.0:1.0", FabricVersion);
     VERIFY_IS_TRUE(error.IsSuccess());

     this->downloadManagerTestHelper_.GetDownloadManager().BeginDownloadFabricUpgradePackage(
         FabricVersion,
         [this, &deployDone](AsyncOperationSPtr const & operation) 
    {
        OperationStatus downloadStatus;
        auto error = this->downloadManagerTestHelper_.GetDownloadManager().EndDownloadFabricUpgradePackage(operation);
        VERIFY_IS_TRUE_FMT(error.IsSuccess(), "DownloadFabricUpgradePackage returned %d", error.ReadValue());
        VERIFY_IS_TRUE(this->downloadManagerTestHelper_.VerifyDeployment(this->downloadManagerTestHelper_.fabricNodeHost_->GetFabricUpgradeDeploymentRoot(), this->downloadManagerTestHelper_.fabricUpgradeFiles), L"VerifyFabricUpgradeDeployment");
        deployDone.Set();
    },
    AsyncOperationSPtr());

    VERIFY_IS_TRUE(deployDone.WaitOne(10000), L"DownloadFabricUpgradePackageTest completed before timeout.");
}

BOOST_AUTO_TEST_CASE(DownloadApplicationPackageTest)
{
    Trace.WriteInfo(TraceType, "DownloadApplicationPackageTest Start");

    ManualResetEvent deployDone;
    
    ApplicationIdentifier appId;
    auto error = ApplicationIdentifier::FromString(L"SandboxPersistentQueueApp_App1", appId);
    VERIFY_IS_TRUE(error.IsSuccess(), L"ApplicationIdentifier::FromString");

    this->downloadManagerTestHelper_.GetDownloadManager().BeginDownloadApplicationPackage(
        appId,
        ApplicationVersion(),
        ServicePackageActivationContext(),
        L"",
        1,
        [this, &deployDone](AsyncOperationSPtr const & operation) 
    {
        ApplicationPackageDescription appDesc;
        OperationStatus downloadStatus;
        auto error = this->downloadManagerTestHelper_.GetDownloadManager().EndDownloadApplicationPackage(operation, downloadStatus, appDesc);
        Trace.WriteNoise(TraceType, "DownloadStatus = {0}", downloadStatus);
        VERIFY_IS_TRUE(downloadStatus.FailureCount == 0, L"downloadStatus.FailureCount == 0");
        VERIFY_IS_TRUE(downloadStatus.LastError.IsSuccess(), L"downloadStatus.LastError.IsSuccess()");
        VERIFY_IS_TRUE(downloadStatus.State == OperationState::Completed, L"downloadStatus.State == OperationState::Completed");
        VERIFY_IS_TRUE_FMT(error.IsSuccess(), "DownloadApplicationPackageDocument returned %d", error.ReadValue());
        VERIFY_IS_TRUE(this->downloadManagerTestHelper_.VerifyDeployment(this->downloadManagerTestHelper_.fabricNodeHost_->GetDeploymentRoot(), this->downloadManagerTestHelper_.sandboxedPQueueAppFiles), L"VerifyAppPackageDeployment");
        deployDone.Set();
    },
    AsyncOperationSPtr());

    VERIFY_IS_TRUE(deployDone.WaitOne(10000), L"DownloadApplicationPackageDocument completed before timeout.");
}

BOOST_AUTO_TEST_CASE(DownloadServicePackageTest)
{
    Trace.WriteInfo(TraceType, "DownloadServicePackageTest Start");

    ManualResetEvent deployDone;
    
    ServicePackageIdentifier packageId;
    auto error = ServicePackageIdentifier::FromString(L"SandboxPersistentQueueApp_App1:PersistentQueueServicePackage", packageId);
    VERIFY_IS_TRUE(error.IsSuccess(), L"ServicePackageIdentifier::FromString");

    this->downloadManagerTestHelper_.GetDownloadManager().BeginDownloadServicePackage(
        packageId,
        ServicePackageVersion(),
        ServicePackageActivationContext(),
        L"",
        1,
        [this, &deployDone](AsyncOperationSPtr const & operation) 
    {
        ServicePackageDescription packageDesc;
        OperationStatus downloadStatus;
        auto error = this->downloadManagerTestHelper_.GetDownloadManager().EndDownloadServicePackage(operation, downloadStatus, packageDesc);
        Trace.WriteNoise(TraceType, "DownloadStatus = {0}", downloadStatus);
        VERIFY_IS_TRUE(downloadStatus.FailureCount == 0, L"downloadStatus.FailureCount == 0");
        VERIFY_IS_TRUE(downloadStatus.LastError.IsSuccess(), L"downloadStatus.LastError.IsSuccess()");
        VERIFY_IS_TRUE(downloadStatus.State == OperationState::Completed, L"downloadStatus.State == OperationState::Completed");
        VERIFY_IS_TRUE_FMT(error.IsSuccess(), "DownloadServicePackageTest returned %d", error.ReadValue());
        VERIFY_IS_TRUE(this->downloadManagerTestHelper_.VerifyDeployment(this->downloadManagerTestHelper_.fabricNodeHost_->GetDeploymentRoot(), this->downloadManagerTestHelper_.sandboxedPQueuePackageFiles), L"VerifyPackageDeployment");
        deployDone.Set();
    },
    AsyncOperationSPtr());

    VERIFY_IS_TRUE(deployDone.WaitOne(10000), L"DownloadServicePackageTest completed before timeout.");
}

BOOST_AUTO_TEST_CASE(DownloadServicePackageWithDebugParametersTest)
{

    Trace.WriteInfo(TraceType, "DownloadServicePackageWithDebugParametersTest Start");

    ManualResetEvent deployDone;

    // Enable ProcessDebugging flag to enable debug parameters.
    HostingConfig::GetConfig().set_EnableProcessDebugging(true);

    ServicePackageIdentifier packageId;
    auto error = ServicePackageIdentifier::FromString(L"SandboxPersistentQueueApp_App2:PersistentQueueServicePackage", packageId);
    VERIFY_IS_TRUE(error.IsSuccess(), L"ServicePackageIdentifier::FromString");

    this->downloadManagerTestHelper_.GetDownloadManager().BeginDownloadServicePackage(
        packageId,
        ServicePackageVersion(),
        ServicePackageActivationContext(),
        L"",
        1,
        [this, &deployDone](AsyncOperationSPtr const & operation)
    {
        vector<wstring> expectedLinks;
        expectedLinks.push_back(L"SandboxPersistentQueueApp_App2\\PersistentQueueServicePackage.PersistentQueueService.Code.1.0");
        expectedLinks.push_back(L"SandboxPersistentQueueApp_App2\\PersistentQueueServicePackage.PersistentQueueService.Config.1.0");
        expectedLinks.push_back(L"SandboxPersistentQueueApp_App2\\PersistentQueueServicePackage.PersistentQueueService.Data.1.0");
        
        ServicePackageDescription packageDesc;
        OperationStatus downloadStatus;
        auto error = this->downloadManagerTestHelper_.GetDownloadManager().EndDownloadServicePackage(operation, downloadStatus, packageDesc);
        Trace.WriteNoise(TraceType, "DownloadStatus = {0}", downloadStatus);
        VERIFY_IS_TRUE(downloadStatus.FailureCount == 0, L"downloadStatus.FailureCount == 0");
        VERIFY_IS_TRUE(downloadStatus.LastError.IsSuccess(), L"downloadStatus.LastError.IsSuccess()");
        VERIFY_IS_TRUE(downloadStatus.State == OperationState::Completed, L"downloadStatus.State == OperationState::Completed");
        VERIFY_IS_TRUE_FMT(error.IsSuccess(), "DownloadServicePackageTest returned %d", error.ReadValue());
        VERIFY_IS_TRUE(this->downloadManagerTestHelper_.VerifySymbolicLinkDeployment(this->downloadManagerTestHelper_.fabricNodeHost_->GetDeploymentRoot(), expectedLinks), L"VerifySymbolicLinkDeployment");
        deployDone.Set();
    },
        AsyncOperationSPtr());

    VERIFY_IS_TRUE(deployDone.WaitOne(10000), L"DownloadServicePackageTest completed before timeout.");

    HostingConfig::GetConfig().set_EnableProcessDebugging(false);
}

#if !defined(PLATFORM_UNIX)
BOOST_AUTO_TEST_CASE(DownloadServicePackageInParallelTest)
{
    Trace.WriteInfo(TraceType, "DownloadServicePackageInParallelTest Start");

    ServicePackageIdentifier packageId;
    ErrorCode error = ServicePackageIdentifier::FromString(L"SandboxPersistentQueueApp_App1:PersistentQueueServicePackage", packageId);
    VERIFY_IS_TRUE(error.IsSuccess(), L"ServicePackageIdentifier::FromString");

    bool generatedTestFile(true);
    wstring tempFileName(L"Download.Parallel.Data");
    wstring tempTestFilePath = Path::Combine(Directory::GetCurrentDirectory(), tempFileName);
    int sizeInMB = 1500;
    std::vector<char> buffer(1024, 0);
    std::ofstream ofstream(tempTestFilePath.c_str(), std::ios::binary | std::ios::out);
    for (int i = 0; i < 1024 * sizeInMB; i++)
    {
        if (!ofstream.write(&buffer[0], buffer.size()))
        {
            generatedTestFile = false;
            break;
        }
    }

    VERIFY_IS_TRUE_FMT(generatedTestFile, "Problem writring to file: {0}", tempTestFilePath);
    wstring remotePath = L"Store\\SandboxPersistentQueueApp\\PersistentQueueServicePackage.PersistentQueueService.Code.1.0\\" + tempFileName;
    error = this->downloadManagerTestHelper_.GetDownloadManager().ImageStore->UploadContent(
        remotePath,
        tempTestFilePath,
        TimeSpan::FromSeconds(30),
        Management::ImageStore::CopyFlag::Enum::Overwrite);

    if (!error.IsSuccess())
    {
        generatedTestFile = false;
    }
    else
    {
        this->downloadManagerTestHelper_.sandboxedPQueuePackageFiles.push_back(L"SandboxPersistentQueueApp_App1\\PersistentQueueServicePackage.PersistentQueueService.Code.1.0\\" + tempFileName);
    }

    VERIFY_IS_TRUE_FMT(generatedTestFile, "Problem uploading file: {0}", remotePath);
    Trace.WriteInfo(TraceType, "DownloadServicePackageInParallelTest upload completed {0}", remotePath);

    int const operationCountInParallel = 5;
    int index = operationCountInParallel - 1;
    int64 completionTime[operationCountInParallel + 1] = { DateTime::Now().Ticks };
    OperationStatus downloadStatus[operationCountInParallel];
    atomic_uint64 pendingOperationCount(operationCountInParallel);

    ManualResetEvent deployDone;
    do
    {
        
        this->downloadManagerTestHelper_.GetDownloadManager().BeginDownloadServicePackage(
            packageId,
            ServicePackageVersion(),
            ServicePackageActivationContext(Guid::NewGuid()),
            L"",
            1,
            [this, &deployDone, index, &pendingOperationCount, &completionTime, &downloadStatus](AsyncOperationSPtr const & operation)
        {
            ServicePackageDescription packageDesc;
            auto error = this->downloadManagerTestHelper_.GetDownloadManager().EndDownloadServicePackage(operation, downloadStatus[index], packageDesc);
            Trace.WriteNoise(TraceType, "DownloadStatus = {0}, Round = {1}", downloadStatus[index], index);
            completionTime[index] = DateTime::Now().Ticks;
            
            uint64 pendingOperations = --pendingOperationCount;

            if (pendingOperations == 0)
            {
                deployDone.Set();
            }
        },
            AsyncOperationSPtr());
    } while (--index >= 0);

    error = deployDone.Wait();
    VERIFY_IS_TRUE(error.IsSuccess(), L"DownloadServicePackageInParallelTest failed.");

    for (int i = 0; i < operationCountInParallel; i++)
    {
        VERIFY_IS_TRUE_FMT(downloadStatus[i].FailureCount == 0, L"downloadStatus.FailureCount == 0, Round = {0}", i);
        VERIFY_IS_TRUE_FMT(downloadStatus[i].LastError.IsSuccess(), L"downloadStatus.LastError.IsSuccess(), Round = {0}", i);
        VERIFY_IS_TRUE_FMT(downloadStatus[i].State == OperationState::Completed, L"downloadStatus.State == OperationState::Completed, Round = {0}", i);
    }
    VERIFY_IS_TRUE_FMT(this->downloadManagerTestHelper_.VerifyDeployment(this->downloadManagerTestHelper_.fabricNodeHost_->GetDeploymentRoot(), this->downloadManagerTestHelper_.sandboxedPQueuePackageFiles), L"VerifyPackageDeployment");
    TimeSpan primaryCompletionTime = TimeSpan::FromTicks(completionTime[operationCountInParallel] - completionTime[0]);
    Trace.WriteInfo(TraceType, "Primary completed after {0} ms", primaryCompletionTime.get_Milliseconds());
    int SecondaryCount = operationCountInParallel - 1;
    int64 maxAllowedGap = TimeSpan::FromMilliseconds(100).Ticks;
    for (int i = 0; i < SecondaryCount; i++)
    {
        TimeSpan secondaryCompletionTime = TimeSpan::FromTicks(completionTime[SecondaryCount - i] - completionTime[0]);
        Trace.WriteInfo(TraceType, "Secondary {0} completed after {1} ms", i + 1, secondaryCompletionTime.get_Milliseconds());
        VERIFY_IS_TRUE((primaryCompletionTime < secondaryCompletionTime) || (maxAllowedGap > std::abs(completionTime[operationCountInParallel] - completionTime[SecondaryCount - i])), L"VerfiyCompletionTimeSpan");
    }

    error = this->downloadManagerTestHelper_.GetDownloadManager().ImageStore->RemoveRemoteContent(remotePath);
    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceType, "Problem deleting file: {0}", remotePath);
    }
}
#endif

BOOST_AUTO_TEST_CASE(ApplicationDownloadSpecificationTest)
{
    Trace.WriteInfo(TraceType, "ApplicationDownloadSpecificationTest Start");

    {
        ManualResetEvent deployDone;
     
        ApplicationDownloadSpecification appDownloadSpec;
        appDownloadSpec.ApplicationId = ApplicationIdentifier(L"CalculatorApp", 0);
        appDownloadSpec.AppVersion = ApplicationVersion();
        appDownloadSpec.PackageDownloads.push_back(ServicePackageDownloadSpecification(L"CalculatorServicePackage", RolloutVersion()));
        appDownloadSpec.PackageDownloads.push_back(ServicePackageDownloadSpecification(L"CalculatorGatewayPackage", RolloutVersion()));
    
        this->downloadManagerTestHelper_.GetDownloadManager().BeginDownloadApplication(
            appDownloadSpec,
            [this, &deployDone](AsyncOperationSPtr const & operation) 
        {
            OperationStatusMapSPtr downloadStatus;
            auto error = this->downloadManagerTestHelper_.GetDownloadManager().EndDownloadApplication(operation, downloadStatus);
            Trace.WriteNoise(TraceType, "DownloadStatus = {0}", *downloadStatus);
            VERIFY_IS_TRUE_FMT(error.IsSuccess(), "DownloadApplication returned %d", error.ReadValue());
            VERIFY_IS_TRUE(this->downloadManagerTestHelper_.VerifyDeployment(this->downloadManagerTestHelper_.fabricNodeHost_->GetDeploymentRoot(), this->downloadManagerTestHelper_.calculatorAppFiles), L"VerifyAppPackageDeployment");
            VERIFY_IS_TRUE(this->downloadManagerTestHelper_.VerifyDeployment(this->downloadManagerTestHelper_.fabricNodeHost_->GetDeploymentRoot(), this->downloadManagerTestHelper_.calculatorPackageFiles), L"VerifyPackageDeployment");
            deployDone.Set();
        },
        AsyncOperationSPtr());
    
        VERIFY_IS_TRUE(deployDone.WaitOne(10000), L"DownloadApplicationTest completed before timeout.");
    }

    {
        ManualResetEvent deployDone;
     
        ApplicationDownloadSpecification appDownloadSpec;
        appDownloadSpec.ApplicationId = ApplicationIdentifier(L"CalculatorApp", 0);
        appDownloadSpec.AppVersion = ApplicationVersion();
        appDownloadSpec.PackageDownloads.push_back(ServicePackageDownloadSpecification(L"CalculatorServicePackage", RolloutVersion()));
        appDownloadSpec.PackageDownloads.push_back(ServicePackageDownloadSpecification(L"CalculatorGatewayPackage", RolloutVersion()));
    
        this->downloadManagerTestHelper_.GetDownloadManager().BeginDownloadApplication(
            appDownloadSpec,
            [this, &deployDone](AsyncOperationSPtr const & operation) 
        {
            OperationStatusMapSPtr downloadStatus;
            auto error = this->downloadManagerTestHelper_.GetDownloadManager().EndDownloadApplication(operation, downloadStatus);
            Trace.WriteNoise(TraceType, "DownloadStatus = {0}", *downloadStatus);
            VERIFY_IS_TRUE_FMT(error.IsSuccess(), "DownloadApplication returned %d", error.ReadValue());
            VERIFY_IS_TRUE(this->downloadManagerTestHelper_.VerifyDeployment(this->downloadManagerTestHelper_.fabricNodeHost_->GetDeploymentRoot(), this->downloadManagerTestHelper_.calculatorAppFiles), L"VerifyAppPackageDeployment");
            VERIFY_IS_TRUE(this->downloadManagerTestHelper_.VerifyDeployment(this->downloadManagerTestHelper_.fabricNodeHost_->GetDeploymentRoot(), this->downloadManagerTestHelper_.calculatorPackageFiles), L"VerifyPackageDeployment");
            deployDone.Set();
        },
        AsyncOperationSPtr());
    
        VERIFY_IS_TRUE(deployDone.WaitOne(10000), L"DownloadApplicationTest completed before timeout.");
    }
}

BOOST_AUTO_TEST_CASE(ApplicationSymbolicLinkTest)
{
    // Test the creation of symbolic links for the application work directories 
    // Download manager sets up the symbolic link to the directories in the HostingConfig LogicalApplicationDirectories map
    Trace.WriteInfo(TraceType, "ApplicationSymbolicLinkTest Start");

    ManualResetEvent deployDone;

    ApplicationIdentifier appId;
    auto error = ApplicationIdentifier::FromString(L"SandboxPersistentQueueApp_App1", appId);
    VERIFY_IS_TRUE(error.IsSuccess(), L"ApplicationIdentifier::FromString");

    this->downloadManagerTestHelper_.GetDownloadManager().BeginDownloadApplicationPackage(
        appId,
        ApplicationVersion(),
        ServicePackageActivationContext(),
        L"",
        1,
        [this, &deployDone](AsyncOperationSPtr const & operation)
    {
        vector<wstring> expectedLinks;
        expectedLinks.push_back(L"SandboxPersistentQueueApp_App1\\work\\ApplicationDedicatedDataLog");
        expectedLinks.push_back(L"SandboxPersistentQueueApp_App1\\work\\ApplicationCheckpointFiles");
        expectedLinks.push_back(L"SandboxPersistentQueueApp_App1\\work\\Backup");
        expectedLinks.push_back(L"SandboxPersistentQueueApp_App1\\work\\Foo");
        expectedLinks.push_back(L"SandboxPersistentQueueApp_App1\\log");

        ApplicationPackageDescription appDesc;
        OperationStatus downloadStatus;
        auto error = this->downloadManagerTestHelper_.GetDownloadManager().EndDownloadApplicationPackage(operation, downloadStatus, appDesc);
        Trace.WriteNoise(TraceType, "DownloadStatus = {0}", downloadStatus);
        VERIFY_IS_TRUE(downloadStatus.FailureCount == 0, L"downloadStatus.FailureCount == 0");
        VERIFY_IS_TRUE(downloadStatus.LastError.IsSuccess(), L"downloadStatus.LastError.IsSuccess()");
        VERIFY_IS_TRUE(downloadStatus.State == OperationState::Completed, L"downloadStatus.State == OperationState::Completed");
        VERIFY_IS_TRUE_FMT(error.IsSuccess(), "DownloadApplicationPackageDocument returned %d", error.ReadValue());
        VERIFY_IS_TRUE(this->downloadManagerTestHelper_.VerifyDeployment(this->downloadManagerTestHelper_.fabricNodeHost_->GetDeploymentRoot(), this->downloadManagerTestHelper_.sandboxedPQueueAppFiles), L"VerifyAppPackageDeployment");
        VERIFY_IS_TRUE(this->downloadManagerTestHelper_.VerifySymbolicLinkDeployment(this->downloadManagerTestHelper_.fabricNodeHost_->GetDeploymentRoot(), expectedLinks), L"VerifySymbolicLinkDeployment");
        //Verify the write through the symbolic links actually happens to the directories they are pointing to
        VERIFY_IS_TRUE(this->downloadManagerTestHelper_.VerifyWriteInSymbolicDirectories(L"SandboxPersistentQueueApp_App1", this->downloadManagerTestHelper_.fabricNodeHost_->GetDeploymentRoot(), expectedLinks));
        deployDone.Set();
    },
        AsyncOperationSPtr());

    VERIFY_IS_TRUE(deployDone.WaitOne(10000), L"DownloadApplicationPackageDocument completed before timeout.");

}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(DownloadAndActivateTestSuite, DownloadAndActivateManagerTest)

BOOST_AUTO_TEST_CASE(TestAbortApplication)
{
    Trace.WriteInfo(TraceType, "TestAbortApplication Start");

    ApplicationIdentifier appId;
    auto error = ApplicationIdentifier::FromString(L"SandboxPersistentQueueApp_App1", appId);

    //Downloads And Activates the Application 1.0

    wstring serviceTypeName = L"PersistentQueueService";

    ServicePackageIdentifier packageId;
    error = ServicePackageIdentifier::FromString(L"SandboxPersistentQueueApp_App1:PersistentQueueServicePackage", packageId);
    VERIFY_IS_TRUE(error.IsSuccess(), L"ServicePackageIdentifier::FromString");

    ServiceTypeIdentifier serviceTypeId = ServiceTypeIdentifier(packageId, serviceTypeName);
    ServicePackageVersionInstance servicePackageVersionInstance;
    VersionedServiceTypeIdentifier versionedServiceTypeId = VersionedServiceTypeIdentifier(serviceTypeId, servicePackageVersionInstance);

    {
        ManualResetEvent downloadEvent;

        this->downloadManagerTestHelper_.fabricNodeHost_->GetHosting().ApplicationManagerObj->BeginDownloadAndActivate(
            1,
            versionedServiceTypeId,
            ServicePackageActivationContext(),
            L"",
            L"SandboxPersistentQueueApp",
            [this, &downloadEvent](AsyncOperationSPtr const & operation)
        {
            auto error = this->downloadManagerTestHelper_.fabricNodeHost_->GetHosting().ApplicationManagerObj->EndDownloadAndActivate(operation);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::Success), L"Download and activation Succeded");
            downloadEvent.Set();
        },
            AsyncOperationSPtr());

        VERIFY_IS_TRUE(downloadEvent.WaitOne(10000), L"Download and Activate completed before timeout.");
    }

    std::vector<Application2SPtr> applications;

    this->downloadManagerTestHelper_.fabricNodeHost_->GetHosting().ApplicationManagerObj->GetApplications(applications, L"SandboxPersistentQueueApp");

    VERIFY_IS_TRUE(applications.size() == 1, L"Expected only 1 application");

    applications[0]->Abort();

    VERIFY_IS_TRUE(applications[0]->GetState() == Application2::Aborted, L"Application state should be aborted.")
}

BOOST_AUTO_TEST_SUITE_END()


DownloadManager & DownloadManagerTestHelper::GetDownloadManager()
{
    return *this->fabricNodeHost_->GetHosting().DownloadManagerObj;
}

bool DownloadManagerTestHelper::VerifyDeployment(wstring const & deploymentPath, vector<wstring> const & expectedFiles)
{
    for(auto iter = expectedFiles.cbegin(); iter != expectedFiles.cend(); ++iter)
    {
        wstring filePath = Path::Combine(deploymentPath,*iter);
        Trace.WriteNoise(TraceType, "Verifying deployment of {0}", filePath);
        if (!File::Exists(filePath))
        {
            Trace.WriteWarning(TraceType, "File {0} not found.", filePath);
            return false;
        }
    }

    return true;
}

bool DownloadManagerTestHelper::VerifySymbolicLinkDeployment(wstring const & deploymentPath, vector<wstring> const & expectedLinks)
{
    for (auto iter = expectedLinks.cbegin(); iter != expectedLinks.cend(); ++iter)
    {
        wstring path = Path::Combine(deploymentPath, *iter);
        Trace.WriteNoise(TraceType, "Verifying {0} is symbolic link", path);
        if (!Directory::IsSymbolicLink(path))
        {
            Trace.WriteWarning(TraceType, "Path {0} is not symbolic link.", path);
            return false;
        }
    }

    return true;
}

bool DownloadManagerTestHelper::VerifyWriteInSymbolicDirectories(wstring appId, wstring const & deploymentPath, vector<wstring> const & expectedLinks)
{
    ErrorCode error = ErrorCode(ErrorCodeValue::Success);
    for (auto iter = expectedLinks.cbegin(); iter != expectedLinks.cend(); ++iter)
    {
        wstring pathToTestDir = Path::Combine(Path::Combine(deploymentPath, *iter), L"testDir");
        Trace.WriteNoise(TraceType, "Creating {0} using symbolic link", pathToTestDir);
        error = Directory::Create2(pathToTestDir);
        if (!error.IsSuccess())
        {
           Trace.WriteWarning(TraceType, "Unable to create Directory Path {0} exists.", pathToTestDir);
           return false;
        }
    }
    //Verify that the directories exist at the path that symbolic link is poinitng to
    Common::FabricNodeConfig::KeyStringValueCollectionMap logicalApplicationDirectories = fabricNodeHost_->GetHosting().FabricNodeConfigObj.LogicalApplicationDirectories;

    wstring nodeId = fabricNodeHost_->GetHosting().NodeId;
    for (auto iter = logicalApplicationDirectories.begin(); iter != logicalApplicationDirectories.end(); ++iter)
    {
        wstring pathToTestDir = Path::Combine(iter->second, nodeId);
        pathToTestDir = Path::Combine(Path::Combine(pathToTestDir, appId), L"testDir");
        Trace.WriteNoise(TraceType, "Verifying {0} using exists", pathToTestDir);
        if (!Directory::Exists(pathToTestDir))
        {
            Trace.WriteWarning(TraceType, "Path {0} doesnot exist.", pathToTestDir);
            return false;
        }
    }
    return true;
}

bool DownloadManagerTestHelper::Setup()
{
    calculatorAppFiles.clear();
    calculatorPackageFiles.clear();
    sandboxedPQueueAppFiles.clear();
    sandboxedPQueuePackageFiles.clear();

    calculatorAppFiles.push_back(L"CalculatorApp_App0\\App.1.0.xml");

    calculatorPackageFiles.push_back(L"CalculatorApp_App0\\CalculatorServicePackage.Manifest.1.0.xml");
    calculatorPackageFiles.push_back(L"CalculatorApp_App0\\CalculatorServicePackage.Package.1.0.xml");
    calculatorPackageFiles.push_back(L"CalculatorApp_App0\\CalculatorServicePackage.CalculatorService.Code.1.0\\Common.dll.txt");
    calculatorPackageFiles.push_back(L"CalculatorApp_App0\\CalculatorServicePackage.CalculatorService.Code.1.0\\Service.exe.txt");
    calculatorPackageFiles.push_back(L"CalculatorApp_App0\\CalculatorServicePackage.CalculatorService.Code.1.0\\Service.pdb.txt");
    calculatorPackageFiles.push_back(L"CalculatorApp_App0\\CalculatorServicePackage.CalculatorService.Code.1.0\\Binaries\\Shared.DllTxt");
    calculatorPackageFiles.push_back(L"CalculatorApp_App0\\CalculatorServicePackage.CalculatorService.Config.1.0\\Settings.xml");
    calculatorPackageFiles.push_back(L"CalculatorApp_App0\\CalculatorServicePackage.CalculatorService.Data.1.0\\Service.pdb.txt");
    
    calculatorPackageFiles.push_back(L"CalculatorApp_App0\\CalculatorGatewayPackage.Package.1.0.xml");
    calculatorPackageFiles.push_back(L"CalculatorApp_App0\\CalculatorGatewayPackage.Manifest.1.0.xml");
    calculatorPackageFiles.push_back(L"CalculatorApp_App0\\CalculatorGatewayPackage.CalculatorGateway.Code.1.0\\Gateway.exe.txt");
    
    sandboxedPQueueAppFiles.push_back(L"SandboxPersistentQueueApp_App1\\App.1.0.xml");
    
    sandboxedPQueuePackageFiles.push_back(L"SandboxPersistentQueueApp_App1\\PersistentQueueServicePackage.Package.1.0.xml");
    sandboxedPQueuePackageFiles.push_back(L"SandboxPersistentQueueApp_App1\\PersistentQueueServicePackage.Manifest.1.0.xml");
    sandboxedPQueuePackageFiles.push_back(L"SandboxPersistentQueueApp_App1\\PersistentQueueServicePackage.PersistentQueueService.Code.1.0\\Common.dll.txt");
    sandboxedPQueuePackageFiles.push_back(L"SandboxPersistentQueueApp_App1\\PersistentQueueServicePackage.PersistentQueueService.Code.1.0\\SandboxPersistentQueueService.exe.txt");

    fabricUpgradeFiles.push_back(L"ClusterManifest.1.0.xml");

// TODO: Get codeVersion from FabricCodeVersion instead of hardcoded 
#if defined(PLATFORM_UNIX)    
    Common::LinuxPackageManagerType::Enum packageManagerType;
    auto error = FabricEnvironment::GetLinuxPackageManagerType(packageManagerType);
    ASSERT_IF(!error.IsSuccess(), "GetLinuxPackageManagerType failed. Type: {0}. Error: {1}", packageManagerType, error);
    CODING_ERROR_ASSERT(packageManagerType != Common::LinuxPackageManagerType::Enum::Unknown);
    switch (packageManagerType)
    {
    case Common::LinuxPackageManagerType::Enum::Deb:
        fabricUpgradeFiles.push_back(L"servicefabric_1.0.961.0.deb");    
        break;
    case Common::LinuxPackageManagerType::Enum::Rpm:
        fabricUpgradeFiles.push_back(L"servicefabric_1.0.961.0.rpm");    
        break;
    }
#else
    fabricUpgradeFiles.push_back(L"WindowsFabric.1.0.961.0.msi");
#endif

    FabricNodeConfigSPtr config = std::make_shared<FabricNodeConfig>();
    SetupJbodDirectories(config);
    return fabricNodeHost_->Open(move(config));
}

void DownloadManagerTestHelper::SetupJbodDirectories(__out FabricNodeConfigSPtr & config)
{
    Common::FabricNodeConfig::KeyStringValueCollectionMap logicalApplicationDirectoryDirMap;

    std::wstring currentDirectory = Directory::GetCurrentDirectory();
    logicalApplicationDirectoryDirMap.insert(make_pair(L"ApplicationDedicatedDataLog", Path::Combine(currentDirectory, L"ApplicationDedicatedDataLog")));
    logicalApplicationDirectoryDirMap.insert(make_pair(L"ApplicationCheckpointFiles", Path::Combine(currentDirectory, L"ApplicationCheckpointFiles")));
    logicalApplicationDirectoryDirMap.insert(make_pair(L"Backup", Path::Combine(currentDirectory, L"Backup")));
    logicalApplicationDirectoryDirMap.insert(make_pair(L"Foo", Path::Combine(currentDirectory, L"Foo")));
    //Should work if there is a typo in the directory name
    logicalApplicationDirectoryDirMap.insert(make_pair(L"LOg", Path::Combine(currentDirectory, L"LOg")));

    config->LogicalApplicationDirectories = logicalApplicationDirectoryDirMap;
}

bool DownloadManagerTestHelper::Cleanup()
{
    return fabricNodeHost_->Close();
}

bool DownloadManagerTest::Setup()
{
    return this->downloadManagerTestHelper_.Setup();
}

bool DownloadManagerTest::Cleanup()
{
    return this->downloadManagerTestHelper_.Cleanup();
}

bool DownloadAndActivateManagerTest::Setup()
{
    HostingConfig::GetConfig().EndpointProviderEnabled = true;
    HostingConfig::GetConfig().RunAsPolicyEnabled = true;
    HostingConfig::GetConfig().FirewallPolicyEnabled = true;

    return this->downloadManagerTestHelper_.Setup();
}

bool DownloadAndActivateManagerTest::Cleanup()
{
    return this->downloadManagerTestHelper_.Cleanup();
}
