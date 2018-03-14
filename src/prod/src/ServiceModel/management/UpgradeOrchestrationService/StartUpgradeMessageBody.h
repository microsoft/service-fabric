// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace UpgradeOrchestrationService
    {
        class StartUpgradeMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            StartUpgradeMessageBody() : startUpgradeDescription_() { }

            StartUpgradeMessageBody(StartUpgradeDescription const & startUpgradeDescription) : startUpgradeDescription_(startUpgradeDescription) { }

            __declspec(property(get = get_StartUpgradeDescription)) StartUpgradeDescription const & Description;

            StartUpgradeDescription const & get_StartUpgradeDescription() const { return startUpgradeDescription_; }

            FABRIC_FIELDS_01(startUpgradeDescription_);

        private:
            StartUpgradeDescription startUpgradeDescription_;
        };
    }
}
