// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace Reliability;

Common::Global<Reliability::ReliabilityEventSource> Reliability::ReliabilityEventSource::Events = Common::make_global<Reliability::ReliabilityEventSource>();

