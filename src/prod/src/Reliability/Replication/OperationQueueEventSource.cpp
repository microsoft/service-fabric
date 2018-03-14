// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

Common::Global<OperationQueueEventSource> OperationQueueEventSource::Events = Common::make_global<OperationQueueEventSource>();

} // end namespace ReplicationComponent
} // end namespace Reliability
