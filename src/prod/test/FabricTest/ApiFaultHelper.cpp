// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace TestCommon;
using namespace FabricTest;

StringLiteral const ApiFaultHelper::TraceSource("FabricTest.ApiFaultHelper");

ApiFaultHelper::ApiFaultHelper()
{
    randomApiFaultsEnabled_ = true;

    maxApiDelayInterval_ = Common::TimeSpan::FromSeconds(1);
    minApiDelayInterval_ = Common::TimeSpan::FromSeconds(1);
}

void ApiFaultHelper::AddFailure(
    __in wstring const & nodeId,
    __in wstring const & serviceName,
    __in wstring const & strFailureIn)
{
    wstring strFailure = strFailureIn;
    StringUtility::TrimWhitespaces(strFailure);
    StringUtility::ToLower(strFailure);

    failureStore_.AddBehavior(nodeId, serviceName, strFailure);

    Federation::NodeId parsedNodeId;
    if (Federation::NodeId::TryParse(nodeId, parsedNodeId))
    {
        TestHooks::FaultInjector::GetGlobalFaultInjector().InjectPermanentError(
            strFailure, 
            ErrorCodeValue::OperationFailed, 
            parsedNodeId.ToString(),
            serviceName);
    }
    else if (nodeId == L"*")
    {
        TestHooks::FaultInjector::GetGlobalFaultInjector().InjectPermanentError(
            strFailure, 
            ErrorCodeValue::OperationFailed, 
            serviceName);
    }
    else
    {
        TestSession::FailTest("Invalid node ID for fault injection: '{0}'", nodeId);
    }

    TestSession::WriteInfo(
        TraceSource,
        "ApiFaultHelper::AddFailure - nodeId({0}) serviceName({1}) failureType({2})",
        nodeId,
        serviceName,
        strFailure);
}

void ApiFaultHelper::RemoveFailure(
    __in wstring const & nodeId,
    __in wstring const & serviceName,
    __in wstring const & strFailureIn)
{
    wstring strFailure = strFailureIn;
    StringUtility::TrimWhitespaces(strFailure);
    StringUtility::ToLower(strFailure);

    failureStore_.RemoveBehavior(nodeId, serviceName, strFailure);

    TestHooks::FaultInjector::GetGlobalFaultInjector().RemoveError(strFailure);

    TestSession::WriteInfo(
        TraceSource,
        "ApiFaultHelper::RemoveFailure - nodeId({0}) serviceName({1}) failureType({2})",
        nodeId,
        serviceName,
        strFailure);
}

wchar_t easytolower(wchar_t in){
    if(in<=static_cast<wchar_t>('Z') && in>=static_cast<wchar_t>('A'))
        return in-static_cast<wchar_t>(('Z'-'z'));
    return in;
}

namespace
{
    wstring ComponentNameToString(ApiFaultHelper::ComponentName compName)
    {
        switch (compName)
        {
        case ApiFaultHelper::Service:
            return L"service.";
        case ApiFaultHelper::Provider:
            return L"provider.";
        case ApiFaultHelper::Replicator:
            return L"replicator.";
        case ApiFaultHelper::ServiceOperationPump:
            return L"pump.";
        case ApiFaultHelper::Hosting:
            return L"hosting.";
        case ApiFaultHelper::SP2:
            return L"sp2.";
        case ApiFaultHelper::SP2HighFrequencyApi:
            return L"sp2highfrequencyapi.";
        default:
            Assert::CodingError("Unknown component name {0}", static_cast<int>(compName));
        };
    }

    wstring ApiNameToString(ApiFaultHelper::ApiName api)
    {
        switch (api)
        {
        case ApiFaultHelper::DownloadApplication:
            return L"downloadapplication";
        case ApiFaultHelper::AnalyzeApplicationUpgrade:
            return L"analyzeapplicationupgrade";
        case ApiFaultHelper::UpgradeApplication:
            return L"upgradeapplication";
        case ApiFaultHelper::DownloadFabric:
            return L"downloadfabric";
        case ApiFaultHelper::ValidateFabricUpgrade:
            return L"validatefabricupgrade";
        case ApiFaultHelper::FabricUpgrade:
            return L"fabricupgrade";
        case ApiFaultHelper::FindServiceTypeRegistration:
            return L"findservicetyperegistration";
        case ApiFaultHelper::GetHostId:
            return L"gethostid";
        default:
            Assert::CodingError("Unknown api name {0}", static_cast<int>(api));
        }
    }

    wstring FaultTypeToString(ApiFaultHelper::FaultType faultType)
    {
        switch (faultType)
        {
        case ApiFaultHelper::Failure:
            return L"";
        case ApiFaultHelper::ReportFaultPermanent:
            return L".reportfault.permanent";
        case ApiFaultHelper::ReportFaultTransient:
            return L".reportfault.transient";
        case ApiFaultHelper::Delay:
            return L".delay";
        case ApiFaultHelper::SetLocationWhenActive:
            return L".setlocationwhenactive";
        case ApiFaultHelper::Block:
            return L".block";
        default:
            Assert::CodingError("Unknown fault type {0}", static_cast<int>(faultType));
        };
    }
}

std::wstring ApiFaultHelper::GetFailureName(ComponentName compName, ApiName api, FaultType faultType) const
{
    return GetFailureName(compName, ApiNameToString(api), faultType);
}

wstring ApiFaultHelper::GetFailureName(ComponentName compName, wstring const& operationName, FaultType faultType) const
{
    wstring strFailure = L"";
    strFailure += ComponentNameToString(compName);

    strFailure += operationName;

    strFailure += FaultTypeToString(faultType);

    StringUtility::ToLower(strFailure);

    return strFailure;
}

double ApiFaultHelper::GetRandomFaultRatio(ComponentName compName, FaultType faultType) const
{
    wstring strFailure = ComponentNameToString(compName);

    switch (faultType)
    {
    case Failure:
        strFailure+= L"fail";
        break;
    case ReportFaultPermanent:
    case ReportFaultTransient:
        strFailure += L"reportfault";
        break;
    case Delay:
        strFailure += L"delay";
        break;
    case SetLocationWhenActive:
        strFailure += L"setlocationwhenactive";
    };

    StringUtility::ToLower(strFailure);

    return GetRandomFaultRatio(strFailure);
}

double ApiFaultHelper::GetRandomFaultRatio(wstring const& name) const
{
    auto iter = randomFaults_.find(name);
    if(iter != randomFaults_.end())
    {
        return iter->second;
    }

    return 0;
}

bool ApiFaultHelper::ShouldFailOn(
    __in NodeId nodeId,
    __in wstring const& serviceName,
    __in ComponentName compName,
    __in wstring const& operationName,
    __in FaultType faultType) const
{
    return ShouldFailOn(nodeId.ToString(), serviceName, compName, operationName, faultType);
}

bool ApiFaultHelper::ShouldFailOn(
    __in wstring const& nodeName,
    __in wstring const& serviceName,
    __in ComponentName compName,
    __in wstring const& operationName,
    __in FaultType faultType) const
{
    bool shouldFail = false;
    wstring strFailure = GetFailureName(compName, operationName, faultType);

    shouldFail = failureStore_.IsBehaviorSet(nodeName, serviceName, strFailure);

    if(!shouldFail && randomApiFaultsEnabled_)
    {
        double ratio = GetRandomFaultRatio(compName, faultType);
        shouldFail = GetRandomDouble() < ratio;
    }

    if(faultType == Failure)
    {
        strFailure += L".fail";
    }

    if(shouldFail)
    {
        TestSession::WriteInfo(
            TraceSource,
            L"Failed",
            "Failure: nodeId({0}) serviceName({1}) failureType({2})",
            nodeName,
            serviceName,
            strFailure);
    }
    else
    {
        TestSession::WriteNoise(
            TraceSource,
            L"Skipped",
            "No Failure injected: nodeId({0}) serviceName({1}) failureType({2})",
            nodeName,
            serviceName,
            strFailure);
    }

    return shouldFail;
}


void ApiFaultHelper::SetRandomApiFaultsEnabled(bool value)
{
    if (value == randomApiFaultsEnabled_)
    {
        return;
    }

    TestSession::WriteInfo(TraceSource, "Setting RandomApiFaultsEnabled to {0}", value);
    randomApiFaultsEnabled_ = value;
}

bool ApiFaultHelper::ShouldFailOn(
    __in Federation::NodeId nodeId,
    __in wstring const& serviceName,
    __in ComponentName compName,
    __in ApiName apiName,
    __in FaultType faultType) const
{
    return ShouldFailOn(nodeId, serviceName, compName, ApiNameToString(apiName), faultType);
}

void ApiFaultHelper::SetMaxApiDelayInterval(Common::TimeSpan maxApiDelayInterval)
{
    maxApiDelayInterval_ = maxApiDelayInterval;
}

void ApiFaultHelper::SetMinApiDelayInterval(Common::TimeSpan minApiDelayInterval)
{
    minApiDelayInterval_ = minApiDelayInterval;
}

void ApiFaultHelper::SetRandomFaults(map<wstring, double, Common::IsLessCaseInsensitiveComparer<std::wstring>> randomFaults)
{
    randomFaults_ = randomFaults;
}

Common::TimeSpan ApiFaultHelper::GetApiDelayInterval()
{
    if (maxApiDelayInterval_ == minApiDelayInterval_)
    {
        return maxApiDelayInterval_;
    }
    else
    {
        Random random;
        int delaySec = random.Next((int)minApiDelayInterval_.TotalSeconds(), (int)maxApiDelayInterval_.TotalSeconds());
        return Common::TimeSpan::FromSeconds(delaySec);
    }
}

double ApiFaultHelper::GetRandomDouble() const
{
    AcquireWriteLock grab(randomLock_);
    double randomValue = random_.NextDouble();
    return randomValue;
}

Common::TimeSpan ApiFaultHelper::GetAsyncCompleteDelayTime()
{
    double asyncCompletedApiRatio = GetRandomFaultRatio(L"AsyncCompleted.Delay");

    if (asyncCompletedApiRatio > 0)
    {
        if (GetRandomDouble() < asyncCompletedApiRatio)
        {
            Common::Random random;
            int maxDelaySec = (int)maxApiDelayInterval_.TotalSeconds();
            int delaySec = random.Next(1, maxDelaySec);

            TestSession::WriteNoise(
                TraceSource,
                "ApiFaultHelper::GetAsyncCompleteDelayTime - Delay: sec({0})",
                delaySec);

            return Common::TimeSpan::FromSeconds(delaySec);
        }
    }

    return Common::TimeSpan::Zero;
}

INIT_ONCE ApiFaultHelper::initOnce_;
Global<ApiFaultHelper> ApiFaultHelper::singleton_;
BOOL CALLBACK ApiFaultHelper::InitConfigFunction(PINIT_ONCE, PVOID, PVOID *)
{
    singleton_ = Common::Global<ApiFaultHelper>(new ApiFaultHelper());
    return TRUE;
}

ApiFaultHelper & ApiFaultHelper::Get()
{
    PVOID lpContext = NULL;
    BOOL result = ::InitOnceExecuteOnce(&ApiFaultHelper::initOnce_, ApiFaultHelper::InitConfigFunction, NULL, &lpContext);
    ASSERT_IF(!result, "Failed to initialize ApiFaultHelper singleton");
    return *(ApiFaultHelper::singleton_);
}
