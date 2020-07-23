// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

__interface IKReplyEndpoint : IKEndpoint
{
    NTSTATUS SendRequest(
        __in IKAllocator * Allocator,
        __in IKMessageReceiver * Receiver,
        __in IKMessage * Message);
};

