// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Test
{
    internal enum MismatchKind
    {
        PropertyMissingInManifest = 1,

        DataTypeMismatch = 2,

        FieldCountDiffer = 3,

        EventMissingInManifest = 4,

        EventNameWrongInMapping = 5,

        MappingMismatch = 6,

        EventVersionMismatch = 7,

        FieldPositionMismatch = 8,

        EnumMismatch = 9
    }
}