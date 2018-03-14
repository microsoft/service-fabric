// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
 
namespace Query
{
    Common::Global<QueryEventSource> QueryEventSource::EventSource = Common::make_global<QueryEventSource>();
}
