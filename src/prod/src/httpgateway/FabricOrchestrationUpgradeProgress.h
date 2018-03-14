// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    //
    // This is type used to send the data from the internal interface as JSON stream.
    //
    class FabricOrchestrationUpgradeProgress : public Common::IFabricJsonSerializable
    {
    public:
        FabricOrchestrationUpgradeProgress();
        Common::ErrorCode FromInternalInterface(Api::IFabricOrchestrationUpgradeStatusResultPtr &upgDesc);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::UpgradeState, upgradeState_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ProgressStatus, progressStatus_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ConfigVersion, configVersion_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Details, details_)
            END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        FABRIC_UPGRADE_STATE upgradeState_;
        uint32 progressStatus_;
        std::wstring configVersion_;
        std::wstring details_;
    };

}
