// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

__interface IKMessageReceiver : IKBase
{
    void MessageReceived(
        __in IKSendEndpoint * From,
        __in IKMessage * Message);
};
