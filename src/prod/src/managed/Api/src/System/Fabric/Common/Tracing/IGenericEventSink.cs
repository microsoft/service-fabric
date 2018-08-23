// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    using System.Diagnostics.Tracing;

    internal interface IGenericEventSink
    {
        void SetPath(string filename);

        void SetOption(string option);

        void Write(string processName, string v1, string v2, EventLevel informational, string formattedMessage);
    }
}