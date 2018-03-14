// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace ServiceModel
{
    Common::Global<ServiceModelEventSource> ServiceModelEventSource::Trace = Common::make_global<ServiceModelEventSource>();
}
