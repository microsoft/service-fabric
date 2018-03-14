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
            namespace Api
            {
                /*
                    The RA abstraction around a key value store.

                    Each row has an id, type and associated data

                    There are several implementations:
                    - The LocalStoreAdapter which translates Ese storage into the RA interface
                    - The replicated store adater 
                    - The in memory implementation
                */
                class IKeyValueStore
                {
                    DENY_COPY(IKeyValueStore);
                public:
                    IKeyValueStore() {}

                    virtual ~IKeyValueStore() {}

                    virtual Common::ErrorCode Enumerate(
                        Storage::Api::RowType::Enum type,
                        __out std::vector<Storage::Api::Row>&  rows) = 0;

                    /*
                        Perform an operation to a row with a specific id and type
                        
                        Behavior special notes:
                        - Undefined behavior if the rowType is changed for a row
                        - Concurrent calls for differet ids must succeed
                        - Undefined behavior if concurrent calls are made for the same id with operation Type != insert
                        - For Insert+Insert one must succeed and the other fail with StoreWriteConflict
                    */
                    virtual Common::AsyncOperationSPtr BeginStoreOperation(
                        Storage::Api::OperationType::Enum operationType,
                        Storage::Api::RowIdentifier const & id,
                        Storage::Api::RowData && bytes,
                        Common::TimeSpan const timeout,
                        Common::AsyncCallback const & callback,
                        Common::AsyncOperationSPtr const & parent) = 0;

                    virtual Common::ErrorCode EndStoreOperation(Common::AsyncOperationSPtr const & operation) = 0;

                    virtual void Close() = 0;
                };
            }
        }
    }
}



