// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

CopyStageBuffers::CopyStageBuffers(__in KAllocator & allocator)
    : KObject()
    , KShared()
{
    BinaryWriter bw(allocator);

    bw.Write(static_cast<int>(CopyStage::Enum::CopyState));
    copyStateOperation_ = bw.GetBuffer(0);

    bw.set_Position(0);
    bw.Write(static_cast<int>(CopyStage::Enum::CopyProgressVector));
    copyProgressVectorOperation_ = bw.GetBuffer(0);

    bw.set_Position(0);
    bw.Write(static_cast<int>(CopyStage::Enum::CopyFalseProgress));
    copyFalseProgressOperation_ = bw.GetBuffer(0);

    bw.set_Position(0);
    bw.Write(static_cast<int>(CopyStage::Enum::CopyLog));
    copyLogOperation_ = bw.GetBuffer(0);
}

CopyStageBuffers::~CopyStageBuffers()
{
}

CopyStageBuffers::SPtr CopyStageBuffers::Create(
    __in KAllocator & allocator)
{
    CopyStageBuffers * pointer = _new(COPYSTAGEBUFFERS_TAG, allocator) CopyStageBuffers(allocator);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return CopyStageBuffers::SPtr(pointer);
}
