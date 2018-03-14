// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class NodeTransitionResult : public ServiceModel::ClientServerMessageBody
        {
            DEFAULT_COPY_CONSTRUCTOR(NodeTransitionResult)

        public:

            NodeTransitionResult();

            NodeTransitionResult(NodeTransitionResult &&);

            __declspec(property(get = get_ErrorCode)) HRESULT ErrorCode;
            HRESULT const & get_ErrorCode() const { return errorCode_; }

            __declspec(property(get = get_NodeResult)) std::shared_ptr<Management::FaultAnalysisService::NodeResult> const& NodeResult;
            std::shared_ptr<Management::FaultAnalysisService::NodeResult> const& get_NodeResult() const { return nodeResult_; }
                   

            Common::ErrorCode FromPublicApi(FABRIC_NODE_TRANSITION_RESULT const &);
            Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_NODE_TRANSITION_RESULT & publicResult);
            
            void ToPublicApi();

            FABRIC_FIELDS_02(errorCode_, nodeResult_);


            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ErrorCode, errorCode_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::NodeResult, nodeResult_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            HRESULT errorCode_;
            std::shared_ptr<Management::FaultAnalysisService::NodeResult> nodeResult_;
        };
    }
}
