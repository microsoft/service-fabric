// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        // This class is a member of the failover unit proxy
        // It maintains the read and write status of the partition in the FUP
        // It contains the logic needed for updating the ComStatefulServicePartition
        // as required
        class ReadWriteStatusState
        {
        public:
            ReadWriteStatusState();

            bool TryUpdate(
                AccessStatus::Enum readStatus,
                AccessStatus::Enum writeStatus);

            __declspec(property(get = get_Current)) ReadWriteStatusValue Current;
            ReadWriteStatusValue get_Current() const;

        private:
            AccessStatus::Enum readStatus_;
            AccessStatus::Enum writeStatus_;
        };
    }
}



