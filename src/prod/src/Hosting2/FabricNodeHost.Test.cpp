// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Hosting2/FabricNodeHost.Test.h"

using namespace std;
using namespace Common;
using namespace Fabric;
using namespace Hosting2;

const StringLiteral TraceType("TestFabricNodeHost");

TestFabricNodeHost::TestFabricNodeHost()
    : fabricNode_(nullptr),
    fabricActivationManager_(),
    retryCount_(5),
    retryDelayInMilliSec_(2000)
{
}

TestFabricNodeHost::~TestFabricNodeHost()
{
}

bool TestFabricNodeHost::Open(Common::FabricNodeConfigSPtr && config, IFabricUpgradeImplSPtr && testFabricUpgradeImpl)
{

    if(!config)
    {
        config = std::make_shared<FabricNodeConfig>();
    }

    config->StartApplicationPortRange = 1000;
    config->EndApplicationPortRange = 5000;

    auto hostingTestDir = L"Hosting2.Test";
    config->WorkingDir = Path::Combine(hostingTestDir, L"Data");
    config->WorkingDir = Path::Combine(config->WorkingDir, File::GetTempFileName());
    
    // V2 store stack doesn't support relative paths
    //
    config->WorkingDir = Path::GetFullPath(config->WorkingDir);
    workFolderRoot_ = config->WorkingDir;

    if (!ResetFolder(workFolderRoot_)) { return false; }


    // setup image store
    if (!this->SetupImageStore()) { return false; }
     auto error = HostingSubsystem::GetDeploymentFolder(*config, config->WorkingDir, this->deploymentRoot_);
    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceType, "Failed to get DeploymentFolder");
        return false;
    }
    this->imageCacheRoot_ = HostingSubsystem::GetImageCacheFolder(*config, config->WorkingDir);
    this->fabricUpgradeDeploymentRoot_ = HostingSubsystem::GetFabricUpgradeDeploymentFolder(*config, config->WorkingDir);

    if (!Path::IsPathRooted(this->deploymentRoot_))
    {
        this->deploymentRoot_ = Path::Combine(Directory::GetCurrentDirectoryW(), this->deploymentRoot_);
    }

    if (!Path::IsPathRooted(this->fabricUpgradeDeploymentRoot_))
    {
        this->fabricUpgradeDeploymentRoot_ = Path::Combine(Directory::GetCurrentDirectoryW(), this->fabricUpgradeDeploymentRoot_);
    }

     if (!Path::IsPathRooted(this->imageCacheRoot_))
    {
        this->imageCacheRoot_ = Path::Combine(Directory::GetCurrentDirectoryW(), this->imageCacheRoot_);
    }

    // reset deployment and image cache folders
    if (!ResetFolder(this->deploymentRoot_)) { return false; }
    if (!ResetFolder(this->fabricUpgradeDeploymentRoot_)) { return false; }
    if (!ResetFolder(this->imageCacheRoot_)) {  return false; }

    Trace.WriteInfo(TraceType, "Creating and Opening FabricActivationManager");
    AutoResetEvent openEvent(false);
    ErrorCode openError;
    bool retval = true;
    this->fabricActivationManager_ = make_shared<FabricActivationManager>(false, true);

    wstring currentDirectory = Directory::GetCurrentDirectoryW();
    wstring fabricHostPath;

    Environment::GetEnvironmentVariableW(L"_NTTREE", fabricHostPath);
    //Setting FabricDataRoot because this is checked in fabric node open
    Environment::SetEnvironmentVariableW(L"FabricDataRoot", config->WorkingDir);

    auto fabricLogDir = Path::GetFullPath(Path::Combine(hostingTestDir, L"FabricLog"));
    fabricLogFolderRoot_ = fabricLogDir;
    if (!ResetFolder(fabricLogFolderRoot_)) { return false; }

    //Setting FabricLogRoot because this is checked in FabricActivationManager open
    Environment::SetEnvironmentVariableW(L"FabricLogRoot", fabricLogFolderRoot_);

    Directory::SetCurrentDirectoryW(fabricHostPath);

     fabricActivationManager_->BeginOpen(FabricHostConfig::GetConfig().StartTimeout,
        [this, &openEvent, &openError] (AsyncOperationSPtr const& operation)
    {
        openError = this->fabricActivationManager_->EndOpen(operation);
        openEvent.Set();
    },
        fabricActivationManager_->CreateAsyncOperationRoot()
        );

    Trace.WriteInfo(TraceType, "Waiting for FabricActivationManager to open");

    if (!openEvent.WaitOne(TimeSpan::FromMinutes(1)))
    {
        Trace.WriteWarning(TraceType, "FabricActivationManager open timed out.");
        retval = false;
    }
    if (!openError.IsSuccess())
    {
        Trace.WriteError(TraceType, "FabricActivationManager failed to open with error", openError);
        retval = false;
    }
    Directory::SetCurrentDirectoryW(currentDirectory);
    if(!retval)
    {
        return retval;
    }

    Trace.WriteInfo(TraceType, "Creating FabricNode");

    auto createError = FabricNode::Create(
        config,
        this->fabricNode_);

    if(!createError.IsSuccess())
    {
        Trace.WriteWarning(TraceType, "FabricNode failed to open with {0}.", createError);
        return false;
    }

    if(testFabricUpgradeImpl)
    {
        GetHosting().FabricUpgradeManagerObj->Test_SetFabricUpgradeImpl(testFabricUpgradeImpl);
    }

    bool retVal = true;
    AutoResetEvent openCompleted(false);

    for (int retryCount = 0; retryCount < retryCount_; ++retryCount)
    {
        Trace.WriteInfo(TraceType, "Opening FabricNode. RetryCount = {0}", retryCount);

        this->fabricNode_->BeginOpen(
            TimeSpan::MaxValue,
            [this, &openCompleted, &openError](AsyncOperationSPtr const& operation)
        {
            openError = this->fabricNode_->EndOpen(operation);
            Trace.WriteInfo(TraceType, "FabricNode::EndOpen returns {0}", openError);
            openCompleted.Set();
        },
            AsyncOperationSPtr());

        Trace.WriteInfo(TraceType, "Waiting for CreateAndOpen of FabricNode to be completed");
        if (!openCompleted.WaitOne(TimeSpan::FromMinutes(1)) || !openError.IsSuccess())
        {
            openCompleted.Reset();
            retVal = false;
            Sleep(retryDelayInMilliSec_);
            continue;
        }

        retVal = true;
        break;
    }

    return retVal;
}

bool TestFabricNodeHost::Close()
{
    Trace.WriteInfo(TraceType, "Closing FabricNode");
    bool retval = true;
    AutoResetEvent closeCompleted(false);
    ErrorCode closeError;

    this->fabricNode_->BeginClose(
        TimeSpan::MaxValue,
        [this, &closeCompleted, &closeError](AsyncOperationSPtr const& operation)
    {
        closeError = this->fabricNode_->EndClose(operation);
        closeCompleted.Set();
    },
        this->fabricNode_->CreateAsyncOperationRoot());

    Trace.WriteInfo(TraceType, "Waiting for FabricNode to be closed.");
    if (!closeCompleted.WaitOne(TimeSpan::FromMinutes(1)))
    {
        retval = false;
    }
    if (!closeError.IsSuccess())
    {
        retval = false;
    }

    closeCompleted.Reset();
    this->fabricActivationManager_->BeginClose(
        TimeSpan::MaxValue,
        [this,&closeCompleted, &closeError] (AsyncOperationSPtr const& operation)
    {
        closeError = this->fabricActivationManager_->EndClose(operation);
        Trace.WriteInfo(TraceType, "FabricNode::EndClose returns {0}", closeError);
        closeCompleted.Set();
    },
    this->fabricActivationManager_->CreateAsyncOperationRoot());

    Trace.WriteInfo(TraceType, "Waiting for FabricActivationManager to be closed.");
    if (!closeCompleted.WaitOne(TimeSpan::FromMinutes(1)))
    {
        retval = false;
    }
    if (!closeError.IsSuccess())
    {
        retval = false;
    }

    Directory::Delete(imageStoreRoot_, true, true).ReadValue();
    Directory::Delete(workFolderRoot_, true, true).ReadValue();

    return true;
}

FabricNodeSPtr const & TestFabricNodeHost::GetFabricNode()
{
    return this->fabricNode_;
}

Hosting2::HostingSubsystem & TestFabricNodeHost::GetHosting()
{
    return dynamic_cast<Hosting2::HostingSubsystem&>(this->fabricNode_->Test_Hosting);
}

bool TestFabricNodeHost::SetupImageStore()
{
    wstring nttree;
    if (Environment::GetEnvironmentVariableW(L"_NTTREE", nttree, Common::NOTHROW()))
    {
#if !defined(PLATFORM_UNIX)
        testDataRoot_ = Path::Combine(nttree, L"FabricUnitTests\\Hosting.Test.Data");
#else
        testDataRoot_ = Path::Combine(nttree, L"test/Hosting.Test.Data");
#endif
    }
    else
    {
        testDataRoot_ = Path::Combine(Directory::GetCurrentDirectoryW(), L"Hosting.Test.Data");
    }

    if (!Directory::Exists(testDataRoot_))
    {
        Trace.WriteWarning(TraceType, "Directory '{0}' does not exist.", testDataRoot_);
        return false;
    }


    imageStoreRoot_ = Path::Combine(workFolderRoot_, L"ImageStoreRoot");

    if (!Path::IsPathRooted(this->imageStoreRoot_))
    {
        this->imageStoreRoot_ = Path::Combine(Directory::GetCurrentDirectoryW(), this->imageStoreRoot_);
    }

    auto error = Directory::Copy(Path::Combine(testDataRoot_, L"ImageStoreRoot"), imageStoreRoot_, true);
    if(!error.IsSuccess())
    {
        Trace.WriteWarning(TraceType, "Could not copy ImageStore. Error:{0}", error);
        return false;
    }

    Management::ManagementConfig::GetConfig().ImageStoreConnectionString = SecureString(L"file:" + imageStoreRoot_);

    return true;
}

wstring const & TestFabricNodeHost::GetImageStoreRoot()
{
    return this->imageStoreRoot_;
}

wstring const & TestFabricNodeHost::GetTestDataRoot()
{
    return this->testDataRoot_;
}

wstring const & TestFabricNodeHost::GetDeploymentRoot()
{
    return this->deploymentRoot_;
}

wstring const & TestFabricNodeHost::GetFabricUpgradeDeploymentRoot()
{
    return this->fabricUpgradeDeploymentRoot_;
}

wstring const & TestFabricNodeHost::GetImageCacheRoot()
{
    return this->imageCacheRoot_;
}


bool TestFabricNodeHost::ResetFolder(wstring const & folderPath)
{
    if (!CleanupFolder(folderPath)) { return false; }

    auto error = Directory::Create2(folderPath);
    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceType, "Create directory \"{0}\" failed", folderPath);
        return false;
    }

    return true;
}

bool TestFabricNodeHost::CleanupFolder(wstring const & folderPath)
{
    int retryCount = 0;
    if (Directory::Exists(folderPath))
    {
        auto error = Directory::Delete(folderPath, true);
        if(!error.IsSuccess())
        {
            Trace.WriteWarning(
                TraceType,
                "Failed to delete {0} on try {0}",
                folderPath,
                retryCount);

            ::Sleep(500);

            if (++retryCount == 10)
            {
                return false;
            }
        }
    }

    return true;
}
