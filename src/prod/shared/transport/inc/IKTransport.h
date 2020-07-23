// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

__interface IKTransport : IKBase
{
    NTSTATUS CreateReceiveEndpoint(
        __in LPCWSTR Address,
        __in LPCWSTR Principal,
        __out IKReceiveEndpoint ** Endpoint);

    NTSTATUS CreateSendEndpoint(
        __in LPCWSTR Address,
        __in LPCWSTR Via,
        __in LPCWSTR Principal,
        __out IKSendEndpoint ** Endpoint);

    NTSTATUS CreateReplyEndpoint(
        __in LPCWSTR Address,
        __in LPCWSTR Via,
        __in LPCWSTR Principal,
        __out IKReplyEndpoint ** Endpoint);
};


