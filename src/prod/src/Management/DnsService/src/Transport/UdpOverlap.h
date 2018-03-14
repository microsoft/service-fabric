// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    enum SOCKET_OP_TYPE
    {
        SOCKET_OP_TYPE_INVALID = 0,
        SOCKET_OP_TYPE_READ = 1,
        SOCKET_OP_TYPE_WRITE = 2
    };

    struct UDP_OVERLAPPED : OVERLAPPED
    {
        SOCKET_OP_TYPE _opType;
        IUdpAsyncOp* _pOverlapOp;

        UDP_OVERLAPPED()
        {
            _opType = SOCKET_OP_TYPE_INVALID;
            _pOverlapOp = nullptr;
        }
    };
}
