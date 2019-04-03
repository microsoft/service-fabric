// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace HttpApplicationGateway
{
    Common::Global<HttpApplicationGatewayEventSource> HttpApplicationGatewayEventSource::Trace = Common::make_global<HttpApplicationGatewayEventSource>();
}
