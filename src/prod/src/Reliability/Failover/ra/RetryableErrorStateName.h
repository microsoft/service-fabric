// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace RetryableErrorStateName
        {
            // Used to index in an array
            enum Enum
            {
                None                                = 0,
                ReplicaOpen                         = 1,
                ReplicaReopen                       = 2,
                FindServiceRegistrationAtOpen       = 3,
                FindServiceRegistrationAtReopen     = 4,
                FindServiceRegistrationAtDrop       = 5,
                ReplicaChangeRoleAtCatchup          = 6,
                ReplicaClose                        = 7,
                ReplicaDelete                       = 8,
                LastValidEnum = ReplicaDelete
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum e);

            DECLARE_ENUM_STRUCTURED_TRACE(RetryableErrorStateName);
        }
    }
}

