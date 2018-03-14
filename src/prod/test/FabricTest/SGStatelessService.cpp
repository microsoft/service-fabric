// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace TestCommon;
using namespace FabricTest;

//
// Constructor/Destructor.
//
SGStatelessService::SGStatelessService(
    __in NodeId nodeId,
    __in LPCWSTR serviceType,
    __in LPCWSTR serviceName,
    __in ULONG initializationDataLength,
    __in const byte* initializationData,
    __in FABRIC_INSTANCE_ID instanceId)
    : nodeId_(nodeId)
    , serviceType_(serviceType)
    , serviceName_(serviceName)
    , instanceId_(instanceId)
    , workItemReportMetrics_(NULL)
{
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatelessService::SGStatelessService - ctor - serviceType({2}) serviceName({3})",
        nodeId,
        this,
        serviceType,
        serviceName);

    wstring init;
    if (0 < initializationDataLength)
    {
        init = wstring(reinterpret_cast<const wchar_t*>(initializationData));
    }

    StringCollection collection;
    StringUtility::Split(init, collection, wstring(L" "));

    settings_ = make_unique<TestCommon::CommandLineParser>(collection);
}

SGStatelessService::~SGStatelessService()
{
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatelessService::~SGStatelessService - dtor - serviceType({2}) serviceName({3})",
        nodeId_,
        this,
        serviceType_,
        serviceName_);
}

HRESULT SGStatelessService::OnOpen(IFabricStatelessServicePartition* partition)
{
    partition_.SetAndAddRef(partition);

    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatelessService::OnOpen - serviceType({2}) serviceName({3}) - partition_({4})",
        nodeId_,
        this,
        serviceType_,
        serviceName_,
        partition);

    if (settings_->GetBool(L"reportload"))
    {
        workItemReportMetrics_ = ::CreateThreadpoolWork(&ThreadPoolWorkItemReportMetrics, this, NULL);
        selfReportMetrics_ = shared_from_this();
        ::SubmitThreadpoolWork(workItemReportMetrics_);
    }

    return S_OK;
}

HRESULT SGStatelessService::OnClose()
{
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatelessService::OnClose - serviceType({2}) serviceName({3}) - partition_({4})",
        nodeId_,
        this,
        serviceType_,
        serviceName_,
        partition_.GetRawPointer());

    StopWorkLoad();

    return S_OK;
}

void SGStatelessService::OnAbort()
{
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatelessService::OnAbort - serviceType({2}) serviceName({3}) - partition_({4})",
        nodeId_,
        this,
        serviceType_,
        serviceName_,
        partition_.GetRawPointer());

    StopWorkLoad();
}

void SGStatelessService::StopWorkLoad()
{
    if (NULL != workItemReportMetrics_)
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatelessService::StopWorkload - serviceType({2}) serviceName({3}) - Stopping report metrics",
            nodeId_,
            this,
            serviceType_,
            serviceName_);

        ::WaitForThreadpoolWorkCallbacks(workItemReportMetrics_, FALSE);
        ::CloseThreadpoolWork(workItemReportMetrics_);
        workItemReportMetrics_ = NULL;

        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatelessService::StopWorkload - serviceType({2}) serviceName({3}) - Stopped report metrics",
            nodeId_,
            this,
            serviceType_,
            serviceName_);
    }
}

void SGStatelessService::ReportMetricsRandom()
{
    const ULONG maxMetricCount = 10;
    const ULONG minMetricCount = 1;

    const int maxScale = 7000;

    map<wstring, ULONG> loads;
    vector<FABRIC_LOAD_METRIC> metrics;

    ULONG count = random_.Next(minMetricCount, maxMetricCount);

    wstring metricTrace;
    StringWriter metricWriter(metricTrace);

    for (ULONG i = 0; i < count; ++i)
    {
        wstring name;
        StringWriter(name).Write("metric{0}", i);

        ULONG load = static_cast<ULONG>(random_.Next(maxScale));

        loads.insert(make_pair(name, load));

        FABRIC_LOAD_METRIC metric = { 0 };
        metric.Name = loads.find(name)->first.c_str();
        metric.Value = load;

        metrics.push_back(metric);

        if (metricTrace.empty())
        {
            metricWriter.Write("{0} = {1}", name, load);
        }
        else
        {
            wstring current = metricTrace;
            metricTrace.clear();
            metricWriter.Write("{0}; {1} = {2}", current, name, load);
        }
    }

    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatelessService::PrimaryReportMetricsRandom - serviceType({2}) serviceName({3}) - Calling ReportLoad with {4} metrics - {5}",
        nodeId_,
        this,
        serviceType_,
        serviceName_,
        count,
        metricTrace);

    HRESULT hr = this->partition_->ReportLoad(static_cast<ULONG>(metrics.size()), metrics.data());

    if (FAILED(hr))
    {
        TestSession::FailTest(
            "Node({0}) - ({1}) - SGStatelessService::PrimaryReportMetricsRandom - serviceType({2}) serviceName({3}) - ReportLoad should succeed but failed with {4}",
            nodeId_,
            this,
            serviceType_,
            serviceName_,
            hr
            );
    }
    else
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatelessService::PrimaryReportMetricsRandom - serviceType({2}) serviceName({3}) - ReportLoad completed as expected (hr={4})",
            nodeId_,
            this,
            serviceType_,
            serviceName_,
            hr);
    }
}

void CALLBACK SGStatelessService::ThreadPoolWorkItemReportMetrics(
    __inout PTP_CALLBACK_INSTANCE Instance,
    __inout_opt PVOID Context,
    __inout PTP_WORK Work)
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Work);

    SGStatelessService* service = (SGStatelessService*)Context;
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatelessService::ThreadPoolWorkItemReportMetrics - serviceType({2}) serviceName({3}) - Context({4})",
        service->nodeId_,
        service,
        service->serviceType_,
        service->serviceName_,
        Context);

    for (int i = 0; i < 10; ++i)
    {
        service->ReportMetricsRandom();
        ::Sleep(500);
    }

    service->selfReportMetrics_.reset();
}

bool SGStatelessService::ShouldFailOn(ApiFaultHelper::ComponentName compName, wstring operationName, ApiFaultHelper::FaultType faultType)
{
    return ApiFaultHelper::Get().ShouldFailOn(nodeId_, serviceName_, compName, operationName, faultType);
}

std::wstring const SGStatelessService::StatelessServiceType = L"SGStatelessService";

StringLiteral const SGStatelessService::TraceSource("FabricTest.ServiceGroup.SGStatelessService");
