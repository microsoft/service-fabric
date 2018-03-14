// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace FabricTest
{
    class TestFabricUpgradeHostingImpl : public Hosting2::IFabricUpgradeImpl, protected Common::TextTraceComponent<Common::TraceTaskCodes::TestSession> 
    {
        DENY_COPY(TestFabricUpgradeHostingImpl);      
    
    public:
        class TestValidateUpgradeAsyncOperation;
        class TestFabricUpgradeAsyncOperation;

        TestFabricUpgradeHostingImpl(std::weak_ptr<FabricTest::FabricNodeWrapper> testNodeWPtr)
        {
            testNodeWPtr_ = testNodeWPtr;
        }

        Common::AsyncOperationSPtr BeginDownload(
            Common::FabricVersion const & fabricVersion,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);        

        ErrorCode EndDownload(Common::AsyncOperationSPtr const & operation);

        AsyncOperationSPtr BeginValidateAndAnalyze(
            Common::FabricVersionInstance const & currentFabricVersionInstance,
            ServiceModel::FabricUpgradeSpecification const & fabricUpgradeSpec,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        ErrorCode EndValidateAndAnalyze(
            __out bool & shouldCloseReplica,
            Common::AsyncOperationSPtr const & operation);

        AsyncOperationSPtr BeginUpgrade(
            Common::FabricVersionInstance const & currentFabricVersionInstance,
            ServiceModel::FabricUpgradeSpecification const & fabricUpgradeSpec,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        ErrorCode EndUpgrade(Common::AsyncOperationSPtr const & operation);

    public:
        static bool ShouldFailDownload;
        static bool ShouldFailValidation;
        static bool ShouldFailUpgrade;

    private:
        std::weak_ptr<FabricTest::FabricNodeWrapper> testNodeWPtr_;          
    };
}
