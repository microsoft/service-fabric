// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class CancelTestCommandMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            CancelTestCommandMessageBody() : description_() { }

            CancelTestCommandMessageBody(CancelTestCommandDescription const & description) : description_(description) { }

            __declspec(property(get = get_CancelTestCommandDescription)) CancelTestCommandDescription const & Description;

            CancelTestCommandDescription const & get_CancelTestCommandDescription() const { return description_; }

            FABRIC_FIELDS_01(description_);

        private:
            CancelTestCommandDescription description_;
        };
    }
}
