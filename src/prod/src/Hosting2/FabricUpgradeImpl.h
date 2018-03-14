// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class FabricUpgradeImpl : 
        public Common::RootedObject,
        public IFabricUpgradeImpl
    {        
        DENY_COPY(FabricUpgradeImpl)

    public:
        FabricUpgradeImpl(
            Common::ComponentRoot const & root,
            __in HostingSubsystem & hosting);
        virtual ~FabricUpgradeImpl();

        Common::AsyncOperationSPtr BeginDownload(
            Common::FabricVersion const & fabricVersion,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDownload(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginValidateAndAnalyze(
            Common::FabricVersionInstance const & currentFabricVersionInstance,
            ServiceModel::FabricUpgradeSpecification const & fabricUpgradeSpec,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndValidateAndAnalyze(      
            __out bool & shouldCloseReplica,
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginUpgrade(
            Common::FabricVersionInstance const & currentFabricVersionInstance,
            ServiceModel::FabricUpgradeSpecification const & fabricUpgradeSpec,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);        
        Common::ErrorCode EndUpgrade(
            Common::AsyncOperationSPtr const & operation);

    private:
        __declspec(property(get=get_FabricNodeConfig)) Common::FabricNodeConfig & FabricNodeConfigObj;
        inline Common::FabricNodeConfig & get_FabricNodeConfig() const { return hosting_.FabricNodeConfigObj; }

        Common::ErrorCode ReadFabricDeployerTempFile(std::wstring const & filePath, std::wstring & fileContent);

        class ValidateFabricUpgradeAsyncOperation;
        class FabricUpgradeAsyncOperation;

    private:
        HostingSubsystem & hosting_;
        Management::ImageModel::WinFabRunLayoutSpecification const fabricUpgradeRunLayout_;        
    };
}
