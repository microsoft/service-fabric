// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class IFabricUpgradeImpl
    {        
    public:        
        virtual ~IFabricUpgradeImpl() {}

        virtual Common::AsyncOperationSPtr BeginDownload(
            Common::FabricVersion const & fabricVersion,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndDownload(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginValidateAndAnalyze(
            Common::FabricVersionInstance const & currentFabricVersionInstance,
            ServiceModel::FabricUpgradeSpecification const & fabricUpgradeSpec,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndValidateAndAnalyze(
            __out bool & shouldCloseReplica,
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginUpgrade(
            Common::FabricVersionInstance const & currentFabricVersionInstance,
            ServiceModel::FabricUpgradeSpecification const & fabricUpgradeSpec,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;        
        virtual Common::ErrorCode EndUpgrade(
            Common::AsyncOperationSPtr const & operation) = 0;
    };
}
