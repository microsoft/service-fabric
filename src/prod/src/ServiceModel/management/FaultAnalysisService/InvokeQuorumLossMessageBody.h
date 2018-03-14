// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class InvokeQuorumLossMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            InvokeQuorumLossMessageBody() : invokeQuorumLossDescription_() { }

            InvokeQuorumLossMessageBody(InvokeQuorumLossDescription const & invokeQuorumLossDescription) : invokeQuorumLossDescription_(invokeQuorumLossDescription) { }

            __declspec(property(get = get_InvokeQuorumLossDescription)) InvokeQuorumLossDescription const & QuorumLossDescription;

            InvokeQuorumLossDescription const & get_InvokeQuorumLossDescription() const { return invokeQuorumLossDescription_; }

            FABRIC_FIELDS_01(invokeQuorumLossDescription_);

        private:
            InvokeQuorumLossDescription invokeQuorumLossDescription_;
        };
    }
}
