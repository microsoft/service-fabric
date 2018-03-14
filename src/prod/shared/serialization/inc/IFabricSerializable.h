// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Serialization
{
    __interface IFabricSerializable
    {
        NTSTATUS Write(__in IFabricSerializableStream * stream);

        NTSTATUS Read(__in IFabricSerializableStream * stream);

        NTSTATUS GetTypeInformation(__out FabricTypeInformation & typeInformation) const;

        NTSTATUS SetUnknownData(__in ULONG scope, __in FabricIOBuffer buffer);

        NTSTATUS GetUnknownData(__in ULONG scope, __out FabricIOBuffer & data);

        void ClearUnknownData();
    };
}
