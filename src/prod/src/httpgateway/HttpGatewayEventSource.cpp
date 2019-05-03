// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace HttpGateway
{
    Common::Global<HttpGatewayEventSource> HttpGatewayEventSource::Trace = Common::make_global<HttpGatewayEventSource>();
}
