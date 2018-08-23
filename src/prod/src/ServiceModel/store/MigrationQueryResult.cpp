// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;

void MigrationQueryResult::ToPublicApi(
    __in Common::ScopedHeap &,
    __out FABRIC_KEY_VALUE_STORE_MIGRATION_QUERY_RESULT & publicResult) const
{
    publicResult.CurrentPhase = MigrationPhase::ToPublic(this->CurrentPhase);
    publicResult.State = MigrationState::ToPublic(this->State);
    publicResult.NextPhase = MigrationPhase::ToPublic(this->NextPhase);
}

ErrorCode MigrationQueryResult::FromPublicApi(
    __in FABRIC_KEY_VALUE_STORE_MIGRATION_QUERY_RESULT const & publicResult)
{
    currentPhase_ = MigrationPhase::FromPublic(publicResult.CurrentPhase);
    state_ = MigrationState::FromPublic(publicResult.State);
    nextPhase_ = MigrationPhase::FromPublic(publicResult.NextPhase);

    return ErrorCodeValue::Success;
}

void MigrationQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w << "[phase=" << currentPhase_ << " state=" << state_ << " next=" << nextPhase_ << "]";
}
