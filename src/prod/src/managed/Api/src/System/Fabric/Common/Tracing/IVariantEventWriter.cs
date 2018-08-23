// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    internal interface IVariantEventWriter
    {
        void VariantWrite(
            ref GenericEventDescriptor genericEventDescriptor,
            int argCount,
            Variant v0 = default(Variant),
            Variant v1 = default(Variant),
            Variant v2 = default(Variant),
            Variant v3 = default(Variant),
            Variant v4 = default(Variant),
            Variant v5 = default(Variant),
            Variant v6 = default(Variant),
            Variant v7 = default(Variant),
            Variant v8 = default(Variant),
            Variant v9 = default(Variant),
            Variant v10 = default(Variant),
            Variant v11 = default(Variant),
            Variant v12 = default(Variant),
            Variant v13 = default(Variant));

#if DotNetCoreClrLinux
        // TODO - Following code will be removed once fully transitioned to structured traces in Linux
        void VariantWriteLinuxUnstructured(
            ref GenericEventDescriptor genericEventDescriptor,
            int argCount,
            Variant v0 = default(Variant),
            Variant v1 = default(Variant),
            Variant v2 = default(Variant),
            Variant v3 = default(Variant),
            Variant v4 = default(Variant),
            Variant v5 = default(Variant),
            Variant v6 = default(Variant),
            Variant v7 = default(Variant),
            Variant v8 = default(Variant),
            Variant v9 = default(Variant),
            Variant v10 = default(Variant),
            Variant v11 = default(Variant),
            Variant v12 = default(Variant),
            Variant v13 = default(Variant));
#endif

        bool IsEnabled(
            byte level,
            long keywords);
    }
}