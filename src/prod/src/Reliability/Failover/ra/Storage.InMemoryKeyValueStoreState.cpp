// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Storage;
using namespace Storage::Api;

ErrorCode InMemoryKeyValueStoreState::Enumerate(
    RowType::Enum type,
    __out std::vector<Row>&  rows)
{
    for (auto const & it : state_)
    {
        if (it.first.Type != type)
        {
            continue;
        }

        Row r;
        r.Id = it.first;
        r.Data = it.second;
        rows.push_back(std::move(r));
    }

    return ErrorCode::Success();
}

ErrorCode InMemoryKeyValueStoreState::PerformOperation(
    OperationType::Enum operationType,
    RowIdentifier const & rowId,
    RowData && bytes)
{
    auto it = state_.find(rowId);

    switch (operationType)
    {
    case OperationType::Insert:
        if (it != state_.end())
        {
            return ErrorCodeValue::StoreWriteConflict;
        }

        state_.insert(make_pair(rowId, move(bytes)));
        break;

    case OperationType::Update:
        if(it == state_.end())
        {
            return ErrorCodeValue::StoreRecordNotFound;
        }
        
        it->second = move(bytes);
        break;

    case OperationType::Delete:
        if (it == state_.end())
        {
            return ErrorCodeValue::StoreRecordNotFound;
        }

        state_.erase(it);
        break;

    default:
        Assert::CodingError("Unknown operation type {0}", static_cast<int>(operationType));
        break;
    }

    return ErrorCode::Success();
}
