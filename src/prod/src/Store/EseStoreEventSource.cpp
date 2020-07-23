// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Store
{
    Common::Global<EseStoreEventSource> EseStoreEventSource::Trace = Common::make_global<EseStoreEventSource>();
}
