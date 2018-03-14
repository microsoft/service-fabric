// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace TestHooks;

Global<NodeServiceEntityStore<Signal>> ApiSignalHelper::signalStore_ = make_global<NodeServiceEntityStore<Signal>>();
StringLiteral const ApiSignalHelper::TraceSource("FabricTest.ApiSignalHelper");

void ApiSignalHelper::AddSignal(
    wstring const& nodeId,
    wstring const& serviceName,
    wstring const& strSignal)
{
    auto signal = signalStore_->GetEntity(nodeId, serviceName, strSignal);
    if (signal != nullptr)
    {
        signal->SetSignal();
        return;
    }

    Signal::FailTestFunctionPtr fnPtr = [](wstring const & msg) { CODING_ASSERT("{0}", msg); };

    shared_ptr<Signal> s = make_shared<Signal>(fnPtr, wformatString("{0}-{1}-{2}", strSignal, nodeId, serviceName));
    s->SetSignal();

    signalStore_->AddEntity(nodeId, serviceName, strSignal, s);
}

void ApiSignalHelper::ResetAllSignals()
{
    for (auto & it : signalStore_->GetAll())
    {
        if (it->IsSet())
        {
            it->ResetSignal();
        }
    }
}

void ApiSignalHelper::ResetSignal(
    wstring const& nodeId,
    wstring const& serviceName,
    wstring const& strSignal)
{
    auto signal = signalStore_->GetEntity(nodeId, serviceName, strSignal);
    ASSERT_IF(signal == nullptr, "Signal not found in store {0} {1} {2}", nodeId, serviceName, strSignal);

    signal->ResetSignal();
}

bool ApiSignalHelper::IsSignalSet(
    wstring const& nodeId,
    wstring const& serviceName,
    wstring const& strSignal)
{
    auto signal = signalStore_->GetEntity(nodeId, serviceName, strSignal);
    if (signal == nullptr)
    {
        return false;
    }

    bool rv = signal->IsSet();
    Trace.WriteInfo(TraceSource, "ApiSignalHelper::IsSignalSet - Signal found {0} {1} {2}. Value = {3}", nodeId, serviceName, strSignal, rv);
    return rv;
}

bool ApiSignalHelper::IsSignalSet(
    NodeId const& nodeId,
    wstring const& serviceName,
    wstring const& strSignal)
{
    return ApiSignalHelper::IsSignalSet(nodeId.ToString(), serviceName, strSignal);
}

void ApiSignalHelper::WaitForSignalHit(
    wstring const& nodeId,
    wstring const& serviceName,
    wstring const& strSignal)
{
    auto signal = signalStore_->GetEntity(nodeId, serviceName, strSignal);
    if (signal == nullptr)
    {
        return;
    }

    signal->WaitForSignalHit();
}


void ApiSignalHelper::WaitForSignalReset(
    wstring const& nodeId,
    wstring const& serviceName,
    wstring const& strSignal)
{
    auto signal = signalStore_->GetEntity(nodeId, serviceName, strSignal);
    if (signal == nullptr)
    {
        return;
    }

    signal->WaitForSignalReset();
}

void ApiSignalHelper::WaitForSignalReset(
    NodeId const& nodeId,
    wstring const& serviceName,
    wstring const& strSignal)
{
    ApiSignalHelper::WaitForSignalReset(nodeId.ToString(), serviceName, strSignal);
}

void ApiSignalHelper::WaitForSignalIfSet(
    TestHookContext const & context,
    wstring const& strSignal)
{
    if (IsSignalSet(context.NodeId, context.ServiceName, strSignal))
    {
        WaitForSignalReset(context.NodeId, context.ServiceName, strSignal);
    }
}

void ApiSignalHelper::FailIfSignalHit(
    wstring const& nodeId,
    wstring const& serviceName,
    wstring const& strSignal,
    Common::TimeSpan timeout)
{
    auto signal = signalStore_->GetEntity(nodeId, serviceName, strSignal);
    if (signal == nullptr)
    {
        return;
    }

    signal->FailIfSignalHit(timeout);
}
