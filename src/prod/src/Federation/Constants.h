// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class Constants
    {
    public:
        static Common::GlobalWString SeedNodeVoteType;
        static Common::GlobalWString SqlServerVoteType;
        static Common::GlobalWString WindowsAzureVoteType;

        static Common::GlobalWString GlobalTimestampEpochName;

        static char const* RejectedTokenKey;
        static char const* NeighborHeadersIgnoredKey;

        static Common::StringLiteral VersionTraceType;
    };
};
