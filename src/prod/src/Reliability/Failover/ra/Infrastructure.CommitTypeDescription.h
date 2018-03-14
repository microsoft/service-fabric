// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            struct CommitTypeDescription
            {
                CommitTypeDescription() :
                    StoreOperationType(Storage::Api::OperationType::Insert),
                    IsInMemoryOperation(true)
                {
                }

                Storage::Api::OperationType::Enum StoreOperationType;
                bool IsInMemoryOperation;
            };
        }
    }
}


