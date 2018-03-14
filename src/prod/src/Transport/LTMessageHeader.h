// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

//Need to this trace message ID for lease transport
struct LTMessageHeader
{
    unsigned char MajorVersion = 1;
    unsigned char MinorVersion = 0;
    unsigned int MessageHeaderSize = 0;
    unsigned int MessageSize = 0;
    LARGE_INTEGER MessageIdentifier = {};

    static ULONGLONG GetMessageId(unsigned char const* messageBuffer, unsigned int size) 
    {
        if (size > sizeof(LTMessageHeader)) 
        {
            auto messageHeader = reinterpret_cast<LTMessageHeader const*>(messageBuffer);

            if (messageHeader->MessageHeaderSize > sizeof(LTMessageHeader) && messageHeader->MajorVersion == 1)
            {
                return messageHeader->MessageIdentifier.QuadPart;
            }
        }

        return 0;
    }
};
