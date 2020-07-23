// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "TestSession.h"

namespace LBSimulator
{
    class RandomTestSession: public TestSession
    {
        DENY_COPY(RandomTestSession)

    public:
        RandomTestSession(int iterations, std::wstring const& label, bool autoMode, TestDispatcher& dispatcher);
        std::wstring GetInput();

    private:

        void ExecuteNodeDynamic();
        void ExecuteServiceDynamic();
        void ExecuteServiceAffinityDynamic(int randomIndex);
        void ExecuteLoadDynamic();
        std::wstring GetRandomBlockList();
        std::wstring GetMetricList(std::set<int> const& metricList);
        void CloseAllNode();
        void GenerateInitialNodeDynamic();
        std::wstring GetNodeCapacities();

        Common::Random random_;
        int targetIterations_;
        int iterations_;

        std::vector<bool> nodes_;

        struct ServiceData
        {
            ServiceData()
                : IsUp(false), AffinitizedService(-1), Metrics()
            {
            }

            ServiceData(bool isUp, int affinitizedService, std::set<int> && metrics)
                : IsUp(isUp), AffinitizedService(affinitizedService), Metrics(std::move(metrics))
            {
            }

            bool IsUp;
            int AffinitizedService;
            std::set<int> Metrics;
        };

        std::vector<ServiceData> services_;

        std::vector<Common::Uri> faultDomains_;

        std::vector<std::wstring> upgradeDomains_;

        std::vector<int> metrics_; // per partition load scale for this metric

        bool initialized_;
        Common::Stopwatch watch_;
    };
};
