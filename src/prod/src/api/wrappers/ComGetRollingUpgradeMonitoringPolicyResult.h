// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    static const GUID CLSID_ComGetRollingUpgradeMonitoringPolicyResult = 
    { 
        /* d696d458-e400-4c7a-b7a1-98b0b3910b61 */
        0xd696d458,
        0xe400,
        0x4c7a,
        { 0xb7, 0xa1, 0x98, 0xb0, 0xb3, 0x91, 0x0b, 0x61 }
    };
    class ComGetRollingUpgradeMonitoringPolicyResult :
        public IFabricGetRollingUpgradeMonitoringPolicyResult,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComGetRollingUpgradeMonitoringPolicyResult)

        BEGIN_COM_INTERFACE_LIST(ComGetRollingUpgradeMonitoringPolicyResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricGetRollingUpgradeMonitoringPolicyResult)
            COM_INTERFACE_ITEM(IID_IFabricGetRollingUpgradeMonitoringPolicyResult, IFabricGetRollingUpgradeMonitoringPolicyResult)
            COM_INTERFACE_ITEM(CLSID_ComGetRollingUpgradeMonitoringPolicyResult, ComGetRollingUpgradeMonitoringPolicyResult)
        END_COM_INTERFACE_LIST()

    public:
        explicit ComGetRollingUpgradeMonitoringPolicyResult(
            ServiceModel::RollingUpgradeMonitoringPolicy const & policy);
        virtual ~ComGetRollingUpgradeMonitoringPolicyResult();

        // 
        // IFabricGetRollingUpgradeMonitoringPolicyResult methods
        // 
        const FABRIC_ROLLING_UPGRADE_MONITORING_POLICY * STDMETHODCALLTYPE get_Policy();

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_ROLLING_UPGRADE_MONITORING_POLICY> policy_;
    };
}
