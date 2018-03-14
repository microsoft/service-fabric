// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class StartChaosMessageBody : public ServiceModel::ClientServerMessageBody
        {
            DENY_COPY(StartChaosMessageBody)

        public:
            StartChaosMessageBody() : startChaosDescription_() { }

            StartChaosMessageBody(StartChaosDescription && startChaosDescription) : startChaosDescription_(std::move(startChaosDescription)) { }

            __declspec(property(get = get_StartChaosDescription)) StartChaosDescription const & ChaosDescription;

            StartChaosDescription const & get_StartChaosDescription() const { return startChaosDescription_; }

            FABRIC_FIELDS_01(startChaosDescription_);

        private:
            StartChaosDescription startChaosDescription_;
        };
    }
}
