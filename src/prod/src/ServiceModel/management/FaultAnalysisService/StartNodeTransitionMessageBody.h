// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {    
        class StartNodeTransitionMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            StartNodeTransitionMessageBody() : description_() { }

            StartNodeTransitionMessageBody(StartNodeTransitionDescription const & description) : description_(description) { }

            __declspec(property(get = get_StartNodeTransitionDescription)) StartNodeTransitionDescription const & Description;

            StartNodeTransitionDescription const & get_StartNodeTransitionDescription() const { return description_; }

            FABRIC_FIELDS_01(description_);

        private:
            StartNodeTransitionDescription description_;
        };
    }
}


