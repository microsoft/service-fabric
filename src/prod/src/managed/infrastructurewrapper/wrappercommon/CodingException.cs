// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.Fabric.InfrastructureWrapper
{
    using System;
    using System.Diagnostics;
    using System.Globalization;

    internal class CodingException : Exception
    {
        public CodingException(string message)
            : base(string.Format(CultureInfo.InvariantCulture, "Coding Error in {0}:{1}", new StackFrame(1, true).GetMethod().Name, message))
        {
        }
    }
}