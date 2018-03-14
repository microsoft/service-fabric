// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class NodeTransitionProgress : public Common::IFabricJsonSerializable
    {
    public:
        NodeTransitionProgress();
        Common::ErrorCode FromInternalInterface(Api::INodeTransitionProgressResultPtr &);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::State, state_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::NodeTransitionResult, result_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        Management::FaultAnalysisService::TestCommandProgressState::Enum state_;
        std::shared_ptr<Management::FaultAnalysisService::NodeTransitionResult> result_;
    };
}
