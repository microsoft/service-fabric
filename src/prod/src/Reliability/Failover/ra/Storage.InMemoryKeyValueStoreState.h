// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Storage
        {
            // Non thread safe in memory key value store state
            class InMemoryKeyValueStoreState
            {
            public:
                Common::ErrorCode Enumerate(
                    Storage::Api::RowType::Enum type,
                    __out std::vector<Storage::Api::Row>&  rows);

                Common::ErrorCode PerformOperation(
                    Storage::Api::OperationType::Enum operationType,
                    Storage::Api::RowIdentifier const & rowId,
                    Storage::Api::RowData && bytes);

            private:
                std::map<Storage::Api::RowIdentifier, Storage::Api::RowData> state_;
            };
        }
    }
}
