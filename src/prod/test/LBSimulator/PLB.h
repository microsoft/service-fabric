// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Reliability/LoadBalancing/PLBEventSource.h"
#include "Reliability/LoadBalancing/Placement.h"
#include "Reliability/LoadBalancing/Checker.h"
#include "Reliability/LoadBalancing/SearcherSettings.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class Placement;
        typedef std::unique_ptr<Placement> PlacementUPtr;
        class Checker;
        typedef std::unique_ptr<Checker> CheckerUPtr;
    }
}

namespace LBSimulator
{
    class PLB
    {
    public:
        PLB();
        void Load(std::wstring const& fileName);
        void RunCreation();
        void RunConstraintCheck();
        void RunLoadBalancing();

    private:
        static Reliability::LoadBalancingComponent::PLBEventSource const trace_;
        Reliability::LoadBalancingComponent::PlacementUPtr pl_;
        Reliability::LoadBalancingComponent::CheckerUPtr checker_;
        Reliability::LoadBalancingComponent::SearcherSettings settings_;
    };
}
