// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Typed wrapper over string, to prevent accidentally omitting the first parameter of the Trace methods.
    /// </summary>
    public sealed class TraceType
    {
        public TraceType(string name)
        {
            this.Name = name;
        }

        public string Name { get; private set; }
    }

    internal class Trace
    {
        private static FabricEvents.ExtensionsEvents traceSource = FabricEvents.ExtensionsEvents.GetEventSource(FabricEvents.Tasks.InfrastructureService);

        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
        public static void WriteNoise(TraceType traceType, string format, params object[] args)
        {
            traceSource.WriteNoise(traceType.Name, format, args);
        }

        public static void WriteInfo(TraceType traceType, string format, params object[] args)
        {
            traceSource.WriteInfo(traceType.Name, format, args);
        }

        public static void WriteWarning(TraceType traceType, string format, params object[] args)
        {
            traceSource.WriteWarning(traceType.Name, format, args);
        }

        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
        public static void WriteError(TraceType traceType, string format, params object[] args)
        {
            traceSource.WriteError(traceType.Name, format, args);
        }

        public static void ConsoleWriteLine(TraceType traceType, string format, params object[] args)
        {
            traceSource.WriteInfo(traceType.Name, format, args);
        }

        public static COMException CreateException(TraceType traceType, NativeTypes.FABRIC_ERROR_CODE error, string format, params object[] args)
        {
            string message = string.Format(CultureInfo.InvariantCulture, format, args);
            var exception = new COMException(message, (int)error);

            traceSource.WriteExceptionAsWarning(traceType.Name, exception);

            return exception;
        }
    }
}