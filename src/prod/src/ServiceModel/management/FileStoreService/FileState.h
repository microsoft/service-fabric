// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        namespace FileState
        {
            enum Enum
            {
                // Final state for V1 KVS stack after performing
                // filesystem work
                //
                Available_V1 = 0,

                // Transient state before actual filesystem work
                // is performed on primary to make the file available
                // on its share or delete the file from the share
                //
                Updating = 1,
                Deleting = 2,

                // Two-Phase final state for V2 KVS/TStore stack after
                // performing filesystem work.
                //
                // V1 and V2 protocols cannot be running in the same
                // cluster since we do not support simple upgrade
                // of a cluster to enable the TStore stack.
                //
                // However, to support easier data migration in the future:
                //
                // Replicating cannot be the same value as Available_V1
                // because a V2 secondary must take action to copy files
                // from a V2 primary, but at the same time it must not
                // mistakenly interpret Replicating file from a V2 primary
                // as a stable Available_V1 file from a V1 primary.
                //
                // Committed can be the same value as Available_V1
                // to support potentially downgrading the code.
                // However, notification processing logic is slightly
                // simpler if we keep the values different and downgrading
                // back from V2 to V1 is an expensive and unlikely path
                // since we would have already incurred the cost of
                // migrating from V1 to V2.
                //
                Replicating = 3,
                Committed = 4,
            };

            bool IsStable(Enum const &);

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
            std::wstring ToString(Enum const & val);
        }
    }
}
