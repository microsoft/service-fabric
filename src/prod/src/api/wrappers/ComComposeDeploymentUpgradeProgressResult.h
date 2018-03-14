// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // f18bb56f-8c42-4b07-af72-0cd7fcf6436f
    static const GUID CLSID_ComComposeDeploymentUpgradeProgressResult =
    {0xf18bb56f, 0x8c42, 0x4b07, {0xaf, 0x72, 0xc, 0xd7, 0xfc, 0xf6, 0x43, 0x6f}};
    
    class ComComposeDeploymentUpgradeProgressResult
        : public IFabricComposeDeploymentUpgradeProgressResult
        , public Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComComposeDeploymentUpgradeProgressResult);

        BEGIN_COM_INTERFACE_LIST(ComComposeDeploymentUpgradeProgressResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricComposeDeploymentUpgradeProgressResult)
            COM_INTERFACE_ITEM(IID_IFabricComposeDeploymentUpgradeProgressResult, IFabricComposeDeploymentUpgradeProgressResult)
            COM_INTERFACE_ITEM(CLSID_ComComposeDeploymentUpgradeProgressResult, ComComposeDeploymentUpgradeProgressResult)
        END_COM_INTERFACE_LIST()

    public:
        ComComposeDeploymentUpgradeProgressResult(
            ServiceModel::ComposeDeploymentUpgradeProgress const & internalProgress);
        
        const FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_PROGRESS * get_UpgradeProgress();

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_PROGRESS> upgradeProgress_;
    };
}
