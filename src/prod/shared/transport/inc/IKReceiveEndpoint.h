// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

__interface IKReceiveEndpoint : IKBase
{
    NTSTATUS Open(
        __in IKAllocator * Allocator,
        __in IKMessageReceiver * Receiver);

    NTSTATUS Close();
};
