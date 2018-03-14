// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::LoggingReplicator;
using namespace Data::LogRecordLib;
using namespace Common;

// Class level methods
FlushedRecordInfo::FlushedRecordInfo(FlushedRecordInfo && other)
    : lsn_(other.lsn_)
    , psn_(other.psn_)
    , recordPosition_(other.recordPosition_)
    , recordType_(other.recordType_)
{
}

FlushedRecordInfo::FlushedRecordInfo(
    __in LogRecordType::Enum const & recordType,
    __in FABRIC_SEQUENCE_NUMBER LSN,
    __in FABRIC_SEQUENCE_NUMBER PSN,
    __in ULONG64 recordPosition)
    : recordType_(recordType)
    , lsn_(LSN)
    , psn_(PSN)
    , recordPosition_(recordPosition)
{
}

void FlushedRecordInfo::Update(
    __in LogRecordType::Enum const & recordType,
    __in FABRIC_SEQUENCE_NUMBER LSN,
    __in FABRIC_SEQUENCE_NUMBER PSN,
    __in ULONG64 recordPosition)
{
    recordType_ = recordType;
    lsn_ = LSN;
    psn_ = PSN;
    recordPosition_ = recordPosition;
}

void FlushedRecordInfo::Reset()
{
    lsn_ = FABRIC_INVALID_SEQUENCE_NUMBER;
    psn_ = FABRIC_INVALID_SEQUENCE_NUMBER;
    recordPosition_ = 0;
    recordType_ = LogRecordType::Enum::Invalid;
}

void FlushedRecordInfo::WriteTo(Common::TextWriter & writer, Common::FormatOptions const &) const
{
    writer.Write(
        "\r\nType:{0} Lsn:{1} Psn:{2} Pos:{3}",
        recordType_,
        lsn_,
        psn_,
        recordPosition_);
}

void FlushedRecordInfo::WriteToEtw(uint16 contextSequenceId) const
{
    EventSource::Events->FlushedRecordInfo(
        contextSequenceId,
        recordType_,
        lsn_,
        psn_,
        recordPosition_);
}
