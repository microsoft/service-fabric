// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LoggingReplicator;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;

LogReaderRange const LogReaderRange::DefaultLogReaderRangeValue = LogReaderRange(MAXLONG64, MAXULONG64, L"Default", LogReaderType::Enum::Default);

LogReaderRange::LogReaderRange()
    : KObject()
    , fullcopyStartingLsn_(MAXLONG64)
    , referenceCount_(1)
    , logReaderType_(LogReaderType::Enum::Default)
    , startingRecordPosition_(MAXULONG64)
    , readerName_()
{
    BOOLEAN result = readerName_.Concat(L"Default");
    ASSERT_IFNOT(
        result == TRUE,
        "LogReaderRange : Failed to concatenate 'Default'"); // otherwise reader is too long
}

LogReaderRange::LogReaderRange(
    __in LONG64 fullcopyStartingLsn,
    __in ULONG64 startingRecordPosition,
    __in KStringView const & readerName,
    __in LogReaderType::Enum logReaderType)
    : KObject()
    , fullcopyStartingLsn_(fullcopyStartingLsn)
    , referenceCount_(1)
    , logReaderType_(logReaderType)
    , startingRecordPosition_(startingRecordPosition)
    , readerName_()
{
    BOOLEAN result = readerName_.Concat(readerName);

    ASSERT_IFNOT(
        result == TRUE,
        "LogReaderRange : Failed to concatenate {0}",
        Data::Utilities::ToStringLiteral(readerName)); // otherwise reader is too long
}

LogReaderRange::LogReaderRange(__in LogReaderRange && other)
    : fullcopyStartingLsn_(other.fullcopyStartingLsn_)
    , referenceCount_(other.referenceCount_)
    , logReaderType_(other.logReaderType_)
    , startingRecordPosition_(other.startingRecordPosition_)
    , readerName_(move(other.readerName_))
{
}

LogReaderRange::LogReaderRange(__in LogReaderRange const & other)
    : fullcopyStartingLsn_(other.fullcopyStartingLsn_)
    , referenceCount_(other.referenceCount_)
    , logReaderType_(other.logReaderType_)
    , startingRecordPosition_(other.startingRecordPosition_)
    , readerName_(other.readerName_)
{
}

LogReaderRange & LogReaderRange::operator=(LogReaderRange const & other)
{
    if (this != &other)
    {
        fullcopyStartingLsn_ = other.fullcopyStartingLsn_;
        referenceCount_ = other.referenceCount_;
        logReaderType_ = other.logReaderType_;
        startingRecordPosition_ = other.startingRecordPosition_;
        readerName_ = other.readerName_;
    }

    return *this;
}

LogReaderRange const & LogReaderRange::DefaultLogReaderRange()
{
    return DefaultLogReaderRangeValue;
}

void LogReaderRange::AddRef()
{
    ASSERT_IFNOT(
        startingRecordPosition_ != MAXULONG64,
        "LogReaderRange::AddRef : startingRecordPosition_ cannot be equal to MAXULONG64");

    ++referenceCount_;
}

int LogReaderRange::Release()
{
    ASSERT_IFNOT(
        startingRecordPosition_ != MAXULONG64,
        "LogReaderRange::Release : startingRecordPosition_ cannot be equal to MAXULONG64");

    --referenceCount_;

    return referenceCount_;
}
