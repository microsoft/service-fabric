// ----------------------------------------------------------------------
//  <copyright file="FabricEventSourceConstants.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    using System;

    internal static class FabricEventSourceConstants
    {
        public const string OperationalChannelTableNameSuffix = "Ops";
        public const string OperationalChannelEventNameFormat = "_{0}{1}_{2}";
    }
}