// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

__interface IKMessage : IKBase
{
    VOID Clone(__out IKMessage ** MessageClone);
};
