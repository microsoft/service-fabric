// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;

namespace
{
    bool perMessageTraceDisabled[Actor::EndValidEnum];
    INIT_ONCE initOnce = INIT_ONCE_STATIC_INIT;
    const StringLiteral TraceType("PerMessageTracing");

    BOOL CALLBACK InitFunction(PINIT_ONCE, PVOID, PVOID *)
    {
        auto disableList = TransportConfig::GetConfig().PerMessageTraceDisableList;
        vector<wstring> tokens;
        StringUtility::Split<wstring>(disableList, tokens, L",");
        if (tokens.empty())
        {
            textTrace.WriteInfo(TraceType, "no actor is disabled");
            return TRUE;
        }

        map<wstring, Actor::Enum, IsLessCaseInsensitiveComparer<wstring>> actorMap;
        for (int actor = Actor::Empty; actor < Actor::EndValidEnum; ++actor)
        {
            perMessageTraceDisabled[actor] = false;

            wstring key;
            StringWriter sw(key);
            Actor::WriteToTextWriter(sw, (Actor::Enum)actor);
            actorMap.emplace(move(key), (Actor::Enum)actor);
        }

        for (auto const & token : tokens)
        {
            auto iter = actorMap.find(token);
            if (iter != actorMap.cend())
            {
                perMessageTraceDisabled[iter->second] = true;
                textTrace.WriteInfo(TraceType, "disabled for actor {0}", iter->second);
                continue;
            }

            textTrace.WriteError(TraceType, "skipped invalid actor {0}", token);
        }

        return TRUE;
    }
}

#ifdef PLATFORM_UNIX

namespace
{
    EventLoopPool* eventLoopPool = nullptr;
    INIT_ONCE initPoolOnce = INIT_ONCE_STATIC_INIT;

    BOOL CALLBACK InitEventLoopPool(PINIT_ONCE, PVOID, PVOID*)
    {
        // create a dedicated EventLoopPool for isolation
        eventLoopPool = new EventLoopPool(L"Transport");
        return TRUE;
    }
}

EventLoopPool* IDatagramTransport::GetDefaultTransportEventLoopPool()
{
    PVOID lpContext = NULL;
    BOOL  bStatus = ::InitOnceExecuteOnce(
        &initPoolOnce,
        InitEventLoopPool,
        nullptr,
        nullptr);

    ASSERT_IF(!bStatus, "Failed to initialize EventLoopPool for Transport");
    return eventLoopPool; 
}

#endif

IDatagramTransport::IDatagramTransport()
{
    auto bStatus = ::InitOnceExecuteOnce(
        &initOnce,
        InitFunction,
        nullptr,
        nullptr);

    ASSERT_IF(!bStatus, "IDatagramTransport: InitOnceExecuteOnce failed");
}

IDatagramTransport::~IDatagramTransport()
{
}

bool IDatagramTransport::IsPerMessageTraceDisabled(Actor::Enum actor)
{
    return (Actor::Empty <= actor) && (actor < Actor::EndValidEnum) && perMessageTraceDisabled[actor];
}

ISendTarget::SPtr IDatagramTransport::ResolveTarget(
    std::wstring const & address,
    std::wstring const & targetId,
    uint64 instance)
{
    return Resolve(address, targetId, L"", instance);
}

ISendTarget::SPtr IDatagramTransport::ResolveTarget(
    std::wstring const & address,
    std::wstring const & targetId,
    std::wstring const & sspiTarget,
    uint64 instance)
{
    return Resolve(address, targetId, sspiTarget, instance);
}

ISendTarget::SPtr IDatagramTransport::ResolveTarget(NamedAddress const & namedAddress)
{
    return Resolve(namedAddress.Address, namedAddress.Name, L"", 0);
}

std::wstring IDatagramTransport::TargetAddressToTransportAddress(std::wstring const & targetAddress)
{
    // Need to parse "targetAddress" since there may be things following transport address, e.g. replication passing something like this:
    // "host.bricks.com:49904/4216d69d-e661-4f48-9f4c-89d7172a57ab-129722966844577360". The guid suffix is for replication demuxing.
    // As long as we want to share ISendTarget for the same TCP transport address, we should get rid of such suffixes.
    auto spliterPosition = targetAddress.find_first_of(L'/');
    if (spliterPosition == std::wstring::npos)
    {
        return targetAddress;
    }

    return targetAddress.substr(0, spliterPosition);
}

void IDatagramTransport::Test_Reset()
{
    Common::Assert::CodingError("Not implemented");
}

namespace
{
    auto healthCallbackLock = make_global<RwLock>();
    auto healthCallback = make_global<IDatagramTransport::HealthReportingCallback>();
    StringLiteral const TraceHealth("Health");
}

void IDatagramTransport::SetHealthReportingCallback(HealthReportingCallback && callback)
{
    AcquireWriteLock grab(*healthCallbackLock);

    if (*healthCallback)
    {
        textTrace.WriteInfo(TraceHealth, "healthCallback already set");
        return;
    }

    *healthCallback = move(callback);
}

void IDatagramTransport::RerportHealth(SystemHealthReportCode::Enum reportCode, wstring const & dynamicProperty, wstring const & description, TimeSpan ttl)
{
    HealthReportingCallback callback;
    {
        AcquireReadLock grab(*healthCallbackLock);
        callback = *healthCallback;
    }

    if (!callback)
    {
        textTrace.WriteNoise(
            TraceHealth,
            "ingnore health report as healthCallback is empty, reportCode = {0}, dynamicProperty = '{1}', description = {2}, ttl = {3}",
            reportCode, dynamicProperty, description, ttl);
        return;
    }

    callback(reportCode, dynamicProperty, description, ttl);
}
