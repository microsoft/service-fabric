// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class StartChaosDescription
            : public Serialization::FabricSerializable
        {
            DENY_COPY(StartChaosDescription)

        public:

            StartChaosDescription();
            StartChaosDescription(StartChaosDescription &&);
            StartChaosDescription & operator = (StartChaosDescription && other);

            explicit StartChaosDescription(std::unique_ptr<ChaosParameters> &&);

            __declspec(property(get = get_ChaosParameters)) std::unique_ptr<ChaosParameters> const & ChaosParametersUPtr;

            std::unique_ptr<ChaosParameters> const & get_ChaosParameters() const { return chaosParametersUPtr_; }

            Common::ErrorCode FromPublicApi(FABRIC_START_CHAOS_DESCRIPTION const &);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_START_CHAOS_DESCRIPTION &) const;

            FABRIC_FIELDS_01(
                chaosParametersUPtr_);

        private:

            std::unique_ptr<ChaosParameters> chaosParametersUPtr_;
        };
    }
}
