// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "data/txnreplicator/logrecordlib/TracingHeaders.h"

#include "DrainingStream.h"
#include "LoggerEngine.h"
#include "LogReaderRange.h"
#include "FlushedRecordInfo.h"
#include "PeriodicCheckpointTruncationState.h"
#include "EventSource.h" // Include eventsource here after all enums
