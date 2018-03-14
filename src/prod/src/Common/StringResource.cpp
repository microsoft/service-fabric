// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

StringLiteral TraceComponent = "StringResource";

wstring StringResource::Get(uint id)
{
    wstring buffer;
    buffer.resize(CommonConfig::GetConfig().MinResourceStringBufferSizeInWChars);

    for (;;)
    {
        // LoadString will truncate and null-terminate if the buffer is too small.
        // Double the buffer and re-read (up to a limit) until we are sure
        // that no truncation has occurred. Return the truncated string if
        // we reach the buffer limit.
        //
        // Null termination takes up a character in buffer, but is not included in
        // the returned count.
        //
        size_t count = LoadStringResource(id, &buffer[0], static_cast<int>(buffer.size()));

        if (count > 0)
        {
            if (count < (buffer.size() - 1))
            {
                // buffer is not full (no truncation)
                buffer.resize(count);
                return buffer;
            }
            else
            {
                size_t nextSize = buffer.size() * 2;
                if (static_cast<int>(nextSize) <= CommonConfig::GetConfig().MaxResourceStringBufferSizeInWChars)
                {
                    // buffer is full (possible truncation)
                    buffer.clear();
                    buffer.resize(nextSize);
                } 
                else
                {
                    Trace.WriteWarning(
                        TraceComponent,
                        "truncated resource string id {0}: [{1}, {2}]",
                        id,
                        CommonConfig::GetConfig().MinResourceStringBufferSizeInWChars,
                        CommonConfig::GetConfig().MaxResourceStringBufferSizeInWChars);

                    buffer.resize(count);
                    return buffer;
                }
            }
        }
        else
        {
            int error = GetLastError();

            Trace.WriteWarning(
                TraceComponent,
                "failed to read resource string id {0}: error = {1}",
                id,
                error);

            buffer.clear();

            return buffer;
        }
    } // end for(;;)
}
