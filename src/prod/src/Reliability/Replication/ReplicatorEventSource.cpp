// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

Common::Global<ReplicatorEventSource> ReplicatorEventSource::Events = Common::make_global<ReplicatorEventSource>();

} // end namespace ReplicationComponent
} // end namespace Reliability
