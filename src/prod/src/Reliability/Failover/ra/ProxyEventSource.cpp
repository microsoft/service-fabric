// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;

Global<RAPEventSource> RAPEventSource::Events = make_global<RAPEventSource>();
