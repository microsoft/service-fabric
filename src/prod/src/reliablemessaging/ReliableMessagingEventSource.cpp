// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace ReliableMessaging
{
        Common::Global<ReliableMessagingEventSource> ReliableMessagingEventSource::Events = Common::make_global<ReliableMessagingEventSource>();
}

