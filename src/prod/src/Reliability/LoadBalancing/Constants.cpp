// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Constants.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

// Configuration
Common::StringLiteral const Constants::DummyPLBSource("DummyPLB");

// TODO: verify other load metrics should not have the same name with it
Common::GlobalWString const Constants::MoveCostMetricName = make_global<wstring>(L"_MoveCost_");

Common::GlobalWString const Constants::FaultDomainIdScheme = make_global<wstring>(L"fd");
Common::GlobalWString const Constants::UpgradeDomainIdScheme = make_global<wstring>(L"ud");

Common::GlobalWString const Constants::ImplicitNodeType= make_global<wstring>(L"NodeType");
Common::GlobalWString const Constants::ImplicitNodeName= make_global<wstring>(L"NodeName");
Common::GlobalWString const Constants::ImplicitUpgradeDomainId= make_global<wstring>(L"UpgradeDomain");
Common::GlobalWString const Constants::ImplicitFaultDomainId= make_global<wstring>(L"FaultDomain");

double const Constants::MetricWeightHigh = 1;
double const Constants::MetricWeightMedium = 0.3;
double const Constants::MetricWeightLow = 0.1;
double const Constants::MetricWeightZero = 0.0;

double const Constants::SmallValue = 0.001;

vector<GlobalWString> const Constants::DefaultServicesNameList = [] { 
    vector<GlobalWString> v;
    v.push_back(make_global<wstring>(L"ClusterManagerService"));
    v.push_back(make_global<wstring>(L"RepairManagerService"));
    v.push_back(make_global<wstring>(L"FileStoreService"));
    v.push_back(make_global<wstring>(L"TokenValidationService"));
    v.push_back(make_global<wstring>(L"NamingService"));
    v.push_back(make_global<wstring>(L"InfrastructureService"));
    return v;
} ();
