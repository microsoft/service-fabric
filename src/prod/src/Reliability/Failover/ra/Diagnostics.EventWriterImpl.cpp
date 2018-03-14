// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"
#include "Diagnostics.EventWriterImpl.h"
#include "Infrastructure.EventSource.h"
#include <memory>

using namespace std;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace Diagnostics;


void EventWriterImpl::Trace(IEventData const && eventData)
{
    RAEventSource::Events->TraceEventData(move(eventData), nodeName_);
}
