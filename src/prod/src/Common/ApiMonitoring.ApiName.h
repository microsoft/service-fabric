// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace ApiMonitoring
    {
        namespace ApiName
        {
            enum Enum
            {
                Invalid = 0,

                // General lifecycle APIs
                Open = 1,
                Close = 2,
                Abort = 3,

                // IStateProvider Interface API's
                GetNextCopyState = 4,
                GetNextCopyContext = 5,
                UpdateEpoch = 6,
                OnDataLoss = 7,

                // IStatefulServiceReplica API's
                ChangeRole = 8,

                // IReplicator APIs 
                CatchupReplicaSet = 9,
                BuildReplica = 10,
                RemoveReplica = 11,
                GetStatus = 12,
                UpdateCatchupConfiguration = 13,
                UpdateCurrentConfiguration = 14,
                GetQuery = 15,

                LastValidEnum = GetQuery,
                COUNT = LastValidEnum + 1
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            DECLARE_ENUM_STRUCTURED_TRACE(ApiName);
        }

        typedef std::vector<ApiName::Enum> ApiNameList;
    }
}


