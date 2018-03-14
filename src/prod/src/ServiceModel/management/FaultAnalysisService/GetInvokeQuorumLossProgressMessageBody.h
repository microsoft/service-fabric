// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class GetInvokeQuorumLossProgressMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            GetInvokeQuorumLossProgressMessageBody() : operationId_() { }
			
			GetInvokeQuorumLossProgressMessageBody(Common::Guid const & operationId) : operationId_(operationId) { }

            __declspec(property(get = get_OperationId)) Common::Guid const & OperationId;

            Common::Guid const & get_OperationId() const { return operationId_; }

            FABRIC_FIELDS_01(operationId_);

        private:
            Common::Guid operationId_;
        };
    }
}
