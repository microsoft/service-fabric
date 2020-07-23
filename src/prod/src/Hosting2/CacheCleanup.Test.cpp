// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <wextestclass.h>
#include <wexstring.h>

#include "Hosting2\FabricNodeHost.Test.h"
#include "Management\ImageModel\ImageModel.h"

using namespace WEX::Common;
using namespace WEX::TestExecution;
using namespace WEX::Logging;

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;
using namespace Management::ImageModel;

const StringLiteral TraceType("CacheCleanupTestClass");

// ********************************************************************************************************************

class CacheCleanupTestClass : 
    public WEX::TestClass<CacheCleanupTestClass> 
{
public:
    CacheCleanupTestClass();
    TEST_CLASS(CacheCleanupTestClass);
    
    TEST_METHOD_SETUP(Setup);
    TEST_METHOD_CLEANUP(Cleanup);

    TEST_METHOD(CleanupAppInstanceFolder);
    TEST_METHOD(CleanupInstanceAndAppTypeVersion);
    TEST_METHOD(CleanupAppTypeFolder);

private:
    HostingSubsystem & GetHosting();    
    void ValidateImageCacheAgainstImageStore();

private:
    shared_ptr<TestFabricNodeHost> fabricNodeHost_;    

    unique_ptr<StoreLayoutSpecification> imageStoreLayout_;
    unique_ptr<StoreLayoutSpecification> imageCacheLayout_;
    unique_ptr<RunLayoutSpecification> runLayout_;

    static GlobalWString CalculatorAppTypeName;
    static GlobalWString CalculatorAppId;    
    static GlobalWString CalculatorServiceManifestName;    
    static GlobalWString CalculatorGatewayServiceManifestName;    
};

GlobalWString CacheCleanupTestClass::CalculatorAppTypeName = make_global<wstring>(L"CalculatorApp");
GlobalWString CacheCleanupTestClass::CalculatorAppId = make_global<wstring>(L"CalculatorApp_App0");
GlobalWString CacheCleanupTestClass::CalculatorServiceManifestName = make_global<wstring>(L"CalculatorServicePackage");
GlobalWString CacheCleanupTestClass::CalculatorGatewayServiceManifestName = make_global<wstring>(L"CalculatorGatewayPackage");

CacheCleanupTestClass::CacheCleanupTestClass()
    : WEX::TestClass<CacheCleanupTestClass>(),
    fabricNodeHost_(make_shared<TestFabricNodeHost>())
{    
}

HostingSubsystem & CacheCleanupTestClass::GetHosting()
{
    return this->fabricNodeHost_->GetHosting();
}

bool CacheCleanupTestClass::Setup()
{
    HostingConfig::GetConfig().EnableActivateNoWindow = true;
    
    wstring nttree;
    if (!Environment::GetEnvironmentVariableW(L"_NTTREE", nttree, Common::NOTHROW()))
    {
        nttree = Directory::GetCurrentDirectoryW();
    }
     
    if (!this->fabricNodeHost_->Open()) { return false; }

    wstring imageStoreFolder = this->fabricNodeHost_->GetImageStoreRoot();
    wstring imageCacheFolder = this->fabricNodeHost_->GetImageCacheRoot();
    wstring deployedApplicationsFolder = this->fabricNodeHost_->GetDeploymentRoot();

    imageStoreLayout_ = make_unique<StoreLayoutSpecification>(imageStoreFolder);
    imageCacheLayout_ = make_unique<StoreLayoutSpecification>(imageCacheFolder);
    runLayout_ = make_unique<RunLayoutSpecification>(deployedApplicationsFolder);

    auto error = Directory::Copy(imageStoreFolder, imageCacheFolder, true);
    if (!error.IsSuccess())
    {
        Trace.WriteError(
            TraceType,
            "Failed to copy content from ImageStoreRoot to ImageCacheRoot. ImageStore:{0}, ImageCache:{1}, Error:{2}",
            imageStoreFolder,
            imageCacheFolder,
            error);
        return false;
    }

    // Not converting to RunLayout since we dont care about the content inside the
    // ApplicationInstance folder
    error = Directory::Copy(
        imageCacheLayout_->GetApplicationInstanceFolder(*CalculatorAppTypeName, *CalculatorAppId),
        runLayout_->GetApplicationFolder(*CalculatorAppId),
        true);

    if (!error.IsSuccess())
    {
        Trace.WriteError(
            TraceType,
            "Failed to copy content to DeployedApplication. DeployedApplicationFolder:{0}, Error:{1}",
            deployedApplicationsFolder,            
            error);
        return false;
    }

    return true;
}

bool CacheCleanupTestClass::Cleanup()
{
    return fabricNodeHost_->Close();
}

void CacheCleanupTestClass::CleanupAppInstanceFolder()
{
    wstring appInstanceFolderToDelete = L"CalculatorApp_App1";

    auto error = Directory::Copy(
        imageCacheLayout_->GetApplicationInstanceFolder(*CalculatorAppTypeName, *CalculatorAppId),
        imageCacheLayout_->GetApplicationInstanceFolder(*CalculatorAppTypeName, appInstanceFolderToDelete),
        true);

    VERIFY_IS_TRUE(error.IsSuccess(), L"Create new ApplicationInstance folder in ImageCache");

    error = Directory::Copy(
        imageCacheLayout_->GetApplicationInstanceFolder(*CalculatorAppTypeName, appInstanceFolderToDelete),
        runLayout_->GetApplicationFolder(appInstanceFolderToDelete),
        true);

    VERIFY_IS_TRUE(error.IsSuccess(), L"Create new ApplicationInstance folder in Applications folder");

    //this->GetHosting().DownloadManagerObj->Test_CleanupCache();

    this->ValidateImageCacheAgainstImageStore();

    VERIFY_IS_FALSE(Directory::Exists(runLayout_->GetApplicationFolder(appInstanceFolderToDelete)));
}

void CacheCleanupTestClass::CleanupInstanceAndAppTypeVersion()
{
    auto error = Directory::Delete(
        imageStoreLayout_->GetApplicationInstanceFolder(*CalculatorAppTypeName, *CalculatorAppId),
        true,
        true);

    VERIFY_IS_TRUE(error.IsSuccess(), L"Delete ApplicationInstance folder in ImageStore");

    error = File::Delete2(
        imageStoreLayout_->GetServiceManifestFile(*CalculatorAppTypeName, *CalculatorGatewayServiceManifestName, L"1.0"));

    VERIFY_IS_TRUE(error.IsSuccess(), L"Delete CalculatorGatewayServiceManifestName");

    error = Directory::Delete(
        imageStoreLayout_->GetCodePackageFolder(*CalculatorAppTypeName, *CalculatorGatewayServiceManifestName, L"CalculatorGateway.Code", L"1.0"),
        true,
        true);

    VERIFY_IS_TRUE(error.IsSuccess(), L"Delete CalculatorGatewayServiceManifestName CodePackage");

    //this->GetHosting().DownloadManagerObj->Test_CleanupCache();

    this->ValidateImageCacheAgainstImageStore();

    VERIFY_IS_FALSE(Directory::Exists(runLayout_->GetApplicationFolder(*CalculatorAppId)));
}

void CacheCleanupTestClass::CleanupAppTypeFolder()
{
    auto error = Directory::Delete(
        imageStoreLayout_->GetApplicationTypeFolder(*CalculatorAppTypeName),
        true,
        true);

    VERIFY_IS_TRUE(error.IsSuccess(), L"Delete AppType folder in ImageStore");

    //this->GetHosting().DownloadManagerObj->Test_CleanupCache();

    this->ValidateImageCacheAgainstImageStore();
}

void CacheCleanupTestClass::ValidateImageCacheAgainstImageStore()
{
    wstring imageStoreFolder = this->fabricNodeHost_->GetImageStoreRoot();
    wstring imageCacheFolder = this->fabricNodeHost_->GetImageCacheRoot();

    vector<wstring> imageStoreFiles = Directory::Find(imageStoreFolder, L"*.*", UINT_MAX, true);
    for(auto iter = imageStoreFiles.begin(); iter != imageStoreFiles.end(); ++iter)
    {
        StringUtility::Replace<wstring>(*iter, imageStoreFolder, L"");
    }

    vector<wstring> imageCacheFiles = Directory::Find(imageCacheFolder, L"*.*", UINT_MAX, true);
    for(auto iter = imageCacheFiles.begin(); iter != imageCacheFiles.end(); ++iter)
    {
        StringUtility::Replace<wstring>(*iter, imageCacheFolder, L"");
    }
    
    VERIFY_IS_TRUE(StringUtility::AreStringCollectionsEqualCaseInsensitive(imageStoreFiles, imageCacheFiles));
}
