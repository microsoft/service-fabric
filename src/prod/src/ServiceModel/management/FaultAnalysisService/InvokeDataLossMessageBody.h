// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class InvokeDataLossMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            InvokeDataLossMessageBody() : invokeDataLossDescription_() { }

            InvokeDataLossMessageBody(InvokeDataLossDescription const & invokeDataLossDescription) : invokeDataLossDescription_(invokeDataLossDescription) { }

            __declspec(property(get = get_InvokeDataLossDescription)) InvokeDataLossDescription const & DataLossDescription;

            InvokeDataLossDescription const & get_InvokeDataLossDescription() const { return invokeDataLossDescription_; }

            FABRIC_FIELDS_01(invokeDataLossDescription_);

        private:
            InvokeDataLossDescription invokeDataLossDescription_;
        };
    }
}
