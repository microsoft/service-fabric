// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "../../utilities/Data.Utilities.Public.h" // SFStatus.h requires ktl.h
#include "../../logicallog/LogicalLog.Public.h" // SFStatus.h requires ktl.h
#include <SfStatus.h>

#include "../common/TransactionalReplicator.Common.Public.h"
#include "../common/TransactionalReplicator.Common.Internal.h"

#include "TracingHeaders.h"

#include "Pointers.h"
#include "LogRecord.h"
#include "LogicalLogRecord.h"
#include "PhysicalLogRecord.h"
#include "BarrierLogRecord.h"
#include "BackupLogRecord.h"
#include "UpdateEpochLogRecord.h"
#include "ProgressVectorEntry.h"
#include "SharedProgressVectorEntry.h"
#include "CopyModeResult.h"
#include "ProgressVector.h"
#include "CopyContextParameters.h"
#include "TransactionLogRecord.h"
#include "BeginTransactionOperationLogRecord.h"
#include "EndTransactionLogRecord.h"
#include "OperationLogRecord.h"
#include "IndexingLogRecord.h"
#include "InformationLogRecord.h"
#include "LogHeadRecord.h"
#include "TruncateTailLogRecord.h"
#include "TruncateHeadLogRecord.h"
#include "BeginCheckpointLogRecord.h"
#include "EndCheckpointLogRecord.h"
#include "CompleteCheckpointLogRecord.h"
#include "InvalidLogRecords.h"

// ILogManagerReadOnly and PhysicalLogReader
#include "IPhysicalLogReader.h"
#include "ILogManagerReadOnly.h"
#include "LogRecords.h"
#include "PhysicalLogReader.h"
#include "RecoveryPhysicalLogReader.h"

#include "CopyStageBuffers.h"
#include "CopyHeader.h"
#include "CopyMetadata.h"
#include "CopyContext.h"
