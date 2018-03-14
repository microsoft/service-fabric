// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Transport;

Global<MessageHeaderTrace> MessageHeaderTrace::singleton_ = GetSingleton();

Global<MessageHeaderTrace> MessageHeaderTrace::StaticInit()
{
    Global<MessageHeaderTrace> singleton = make_global<MessageHeaderTrace>();

    // Register all headers in transport
    singleton->RegisterHeader<ActorHeader>();
    singleton->RegisterHeader<ActionHeader>();
    singleton->RegisterHeader<ExpectsReplyHeader>();
    singleton->RegisterHeader<FabricActivityHeader>();
    singleton->RegisterHeader<FaultHeader>();
    singleton->RegisterHeader<ForwardMessageHeader>();
    singleton->RegisterHeader<HighPriorityHeader>();
    singleton->RegisterHeader<IdempotentHeader>();
    singleton->RegisterHeader<IpcHeader>();
    singleton->RegisterHeader<MessageIdHeader>();
    singleton->RegisterHeader<RelatesToHeader>();
    singleton->RegisterHeader<RequestInstanceHeader>();
    singleton->RegisterHeader<RetryHeader>();
    singleton->RegisterHeader<TimeoutHeader>();
    singleton->RegisterHeader<ClientAuthHeader>();

#ifndef PLATFORM_UNIX
    singleton->RegisterHeader<SecurityHeader>();
#endif

    return singleton;
}

Global<MessageHeaderTrace> MessageHeaderTrace::GetSingleton()
{
    static Global<MessageHeaderTrace> singleton = StaticInit();

    return singleton;
}

MessageHeaderTrace::MessageHeaderTrace()
{
}

void MessageHeaderTrace::Trace(Common::TextWriter & w, MessageHeaders::HeaderReference & headerReference)
{
    MessageHeaderTrace & singleton = *(MessageHeaderTrace::GetSingleton());

    MessageHeaderTraceFunc* func = nullptr;
    {
        AcquireReadLock lock(singleton.typeMapLock_);

        auto iter = singleton.headerTraceFuncMap_.find(headerReference.Id());
        if (iter != singleton.headerTraceFuncMap_.end())
        {
            func = &(iter->second);
        }
    }

    if (func)
    {
        (*func)(headerReference, w);
    }
    else
    {
        w << L'(' << headerReference.Id() << L':' << headerReference.SerializedHeaderObjectSize() << L":Unknown)";
    }
}
