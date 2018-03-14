// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace SystemServices;

template <class TMessageHandler>
void SystemServiceMessageHandlers<TMessageHandler>::Clear()
{
    AcquireWriteLock lock(handlersLock_);

    handlers_.clear();
}

template <class TMessageHandler>
void SystemServiceMessageHandlers<TMessageHandler>::SetHandler(SystemServiceLocation const & location, TMessageHandler const & handler)
{
    AcquireWriteLock lock(handlersLock_);

    // Replace existing handler if any
    handlers_[SystemServiceMessageFilter(location)] = handler;
}

template <class TMessageHandler>
void SystemServiceMessageHandlers<TMessageHandler>::RemoveHandler(SystemServiceLocation const & location)
{
    AcquireWriteLock lock(handlersLock_);

    handlers_.erase(SystemServiceMessageFilter(location));
}

template <class TMessageHandler>
TMessageHandler SystemServiceMessageHandlers<TMessageHandler>::GetHandler(__in Transport::MessageUPtr & message)
{
    AcquireReadLock lock(handlersLock_);

    // We can optimize this lookup for high-replica density scenarios, but
    // should the direct messaging transport even be shared in such cases?
    //
    for (auto it : handlers_)
    {
        auto & filter = const_cast<SystemServiceMessageFilter &>(it.first);

        if (filter.Match(*message))
        {
            return it.second;
        }
    }

    return nullptr;
}

template class SystemServices::SystemServiceMessageHandlers<IpcMessageHandler>;
template class SystemServices::SystemServiceMessageHandlers<DemuxerMessageHandler>;

