// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        namespace PeriodicTaskName
        {
            enum Enum
            {
                BG = 0,
                CreateContexts = 1,
                FTBackgroundManager = 2,
                AddThreadContexts = 3,
                LoadCache = 4,
                NodeCache = 5,
                ServiceCache = 6,
                ServiceCleanup = 7,
                AddServiceContexts = 8,
                LastValidEnum = AddServiceContexts
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const& value);

            DECLARE_ENUM_STRUCTURED_TRACE(PeriodicTaskName);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        namespace QueueName
        {
            enum Enum
            {
                CommonQueue = 0,
                FailoverUnitQueue = 1,
                QueryQueue = 2,
                LastValidEnum = QueryQueue
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const& value);

            DECLARE_ENUM_STRUCTURED_TRACE(QueueName);
        }
    }
}
