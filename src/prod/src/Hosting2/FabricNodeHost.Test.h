// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Hosting.h"
#include "Common/Common.h"
#include "FabricNode/fabricnode.h"

class TestFabricNodeHost
{
    DENY_COPY(TestFabricNodeHost)

public:
    TestFabricNodeHost();
    virtual ~TestFabricNodeHost();
            
    bool Open(Common::FabricNodeConfigSPtr && config = nullptr, Hosting2::IFabricUpgradeImplSPtr && testFabricUpgradeImpl = nullptr);
    bool Close();
  
    Fabric::FabricNodeSPtr const & GetFabricNode();
    Hosting2::HostingSubsystem & GetHosting();
    std::wstring const & GetTestDataRoot();
    std::wstring const & GetImageStoreRoot();
    std::wstring const & GetDeploymentRoot();
    std::wstring const & GetFabricUpgradeDeploymentRoot();
    std::wstring const & GetImageCacheRoot();
    
    static bool InitializeConfig();

private:
    bool SetupImageStore();
    bool ResetFolder(std::wstring const & folderPath);
    bool CleanupFolder(std::wstring const & folderPath);

private:    
    Fabric::FabricNodeSPtr fabricNode_;        
    std::shared_ptr<Hosting2::FabricActivationManager> fabricActivationManager_;    
    std::wstring workFolderRoot_;
    std::wstring imageStoreRoot_;
    std::wstring testDataRoot_;
    std::wstring imageCacheRoot_;
    std::wstring deploymentRoot_;
    std::wstring fabricUpgradeDeploymentRoot_;
    std::wstring fabricLogFolderRoot_;
    int retryCount_;
    int retryDelayInMilliSec_;
};
