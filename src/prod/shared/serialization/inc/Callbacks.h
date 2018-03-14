// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Serialization
{
    typedef void (*FabricCompletionCallback)(void * state);

    __interface IFabricSerializable;

    typedef IFabricSerializable * (*FabricActivatorFunction)(FabricTypeInformation);
}
