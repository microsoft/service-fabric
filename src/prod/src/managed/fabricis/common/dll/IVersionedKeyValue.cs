// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    internal interface IVersionedKeyValue
    {
        string Key { get; }

        string VersionKey { get; }

        string Value { get; }

        Int64 Version { get; }
    }
}