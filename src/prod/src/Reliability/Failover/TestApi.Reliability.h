// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ReliabilityTestApi
{
    class ReliabilityTestHelper
    {
        DENY_COPY(ReliabilityTestHelper);

    public:
        ReliabilityTestHelper(Reliability::IReliabilitySubsystem & reliability);

        __declspec(property(get=get_Reliability)) Reliability::IReliabilitySubsystem & Reliability;
        Reliability::IReliabilitySubsystem & get_Reliability() const; 

        // These functions return the FM test helper
        // They return null if FM is not present
        FailoverManagerComponentTestApi::FailoverManagerTestHelperUPtr GetFailoverManager();

        FailoverManagerComponentTestApi::FailoverManagerTestHelperUPtr GetFailoverManagerMaster();

        ReconfigurationAgentComponentTestApi::ReconfigurationAgentTestHelperUPtr GetRA();            

        static Common::ErrorCode DeleteAllFMStoreData();

    private:
        Reliability::ReliabilitySubsystem & reliability_;
    };
}

