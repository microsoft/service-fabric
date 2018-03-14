// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define LT_FRAMETYPE_MESSAGE 1
#define LT_FRAMETYPE_CONNECT 2

struct LTFrameHeader // Lease Transport Frame Header
{
    unsigned char FrameType = LT_FRAMETYPE_MESSAGE;
    unsigned char Reserved1 = 0;
    unsigned short Reserved2 = 0;
    unsigned int FrameSize = 0;
    unsigned int Reserved3 = 0;

    void SetFrameSize(unsigned int frameSize)
    {
        FrameSize = frameSize;

        static_assert(sizeof(*this) == 12, "unexpected frame header size");
        if (FrameSize == sizeof(*this))
        {
            FrameType = LT_FRAMETYPE_CONNECT;
        }
    }
};

