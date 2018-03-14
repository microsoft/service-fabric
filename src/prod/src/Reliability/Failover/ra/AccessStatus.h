// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace AccessStatus
        {
            enum Enum
            {
                // Partition cannot respond at this time, retry later.
                TryAgain = 0,
                
                // Partition cannot accept read/write operations since its not the Primary.
                NotPrimary = 1,
                
                // Partition cannot accept write operations since there is not a write quorum
                NoWriteQuorum = 2,
                
                // Partition can accept store operations.
                Granted = 3,

                LastValidEnum = Granted
            };

            std::wstring ToString(AccessStatus::Enum accessStatus);

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
            ::FABRIC_SERVICE_PARTITION_ACCESS_STATUS ConvertToPublicAccessStatus(AccessStatus::Enum toConvert);

            DECLARE_ENUM_STRUCTURED_TRACE(AccessStatus);
        }
    }
}
