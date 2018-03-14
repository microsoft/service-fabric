// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

Common::Global<Federation::SiteNodeEventSource> Federation::SiteNodeEventSource::Events = Common::make_global<Federation::SiteNodeEventSource>();
Common::Global<Federation::ArbitrationEventSource> Federation::ArbitrationEventSource::Events = Common::make_global<Federation::ArbitrationEventSource>();
Common::Global<Federation::VoteManagerEventSource> Federation::VoteManagerEventSource::Events = Common::make_global<Federation::VoteManagerEventSource>();
Common::Global<Federation::VoteProxyEventSource> Federation::VoteProxyEventSource::Events = Common::make_global<Federation::VoteProxyEventSource>();
Common::Global<Federation::BroadcastEventSource> Federation::BroadcastEventSource::Events = Common::make_global<Federation::BroadcastEventSource>();
