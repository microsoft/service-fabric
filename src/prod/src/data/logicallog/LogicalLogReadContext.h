// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Log
    {
        struct LogicalLogReadContext
        {
            LogicalLogReadContext() : LogicalLogReadContext(0, nullptr, nullptr)
            {
            }

            LogicalLogReadContext(
                __in LONGLONG readLocation,
                __in_opt LogicalLogBuffer* readBuffer,
                __in_opt LogicalLogReadTask* nextReadTask)
                : ReadLocation(readLocation)
                , ReadBuffer(readBuffer)
                , NextReadTask(nextReadTask)
            {
            }

            LONGLONG ReadLocation;
            LogicalLogBuffer::SPtr ReadBuffer;
            LogicalLogReadTask::SPtr NextReadTask;
        };
    }
}
