// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::RepairManager;

StringLiteral const TraceComponent("RepairTaskContext");

RepairTaskContext::RepairTaskContext()
    : StoreData()
    , task_()
    , lock_()
    , needsRefresh_(false)
    , getStatusOnly_(false)
{
}

RepairTaskContext::RepairTaskContext(RepairTask const & task)
    : StoreData()
    , task_(task)
    , lock_()
    , needsRefresh_(false)
    , getStatusOnly_(false)
{
}

RepairTaskContext::RepairTaskContext(RepairTaskContext && other)
    : StoreData(move(other))
    , task_(move(other.task_))
    , lock_()
    , needsRefresh_(other.needsRefresh_)
    , getStatusOnly_(other.getStatusOnly_)
{
}

RepairTaskContext const & RepairTaskContext::operator = (RepairTaskContext const & other)
{
    StoreData::operator = (other);

    task_ = other.task_;
    needsRefresh_ = other.needsRefresh_;
    getStatusOnly_ = other.getStatusOnly_;

    return *this;
}

RepairTaskContext::~RepairTaskContext()
{
}

std::wstring const & RepairTaskContext::get_Type() const
{
    return Constants::StoreType_RepairTaskContext;
}

std::wstring RepairTaskContext::ConstructKey() const
{
    return task_.TaskId;
}

std::wstring RepairTaskContext::ConstructDeactivationBatchId() const
{
    wstring batchId(ServiceModel::Constants::NodeDeactivationTaskIdPrefixRM.cbegin());
    batchId.append(task_.TaskId);
    return batchId;
}

void RepairTaskContext::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("RepairTaskContext[{0}]", task_);
}
