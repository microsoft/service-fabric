// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Common;
    using Common.Tracing;
    using Diagnostics.CodeAnalysis;
    using Globalization;
    using Interop;
    using Runtime.InteropServices;

    /// <summary>
    /// Typed wrapper over string, to prevent accidentally omitting the first parameter of the Trace methods.
    /// </summary>
    internal sealed class TraceType
    {
        public TraceType(string name)
            : this(name, string.Empty)
        {
        }

        public TraceType(string name, string id)
        {
            this.Name = name;
            this.Id = id;
        }

        public string Name { get; private set; }

        public string Id { get; private set; }
    }

    internal static class Trace
    {
        private static readonly FabricEvents.ExtensionsEvents traceSource = FabricEvents.ExtensionsEvents.GetEventSource(FabricEvents.Tasks.InfrastructureService);

        static Trace()
        {
            ConsoleOutputEnabled = true;
        }

        public static bool ConsoleOutputEnabled { get; set; }

        private static void ConditionalConsoleWriteLine(string format, object[] args)
        {
            if (ConsoleOutputEnabled)
            {
                Console.WriteLine(format, args);
            }
        }

        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
        public static void WriteNoise(this TraceType traceType, string format, params object[] args)
        {
            traceSource.WriteNoiseWithId(traceType.Name, traceType.Id, format, args);
            ConditionalConsoleWriteLine(format, args);
        }

        public static void WriteInfo(this TraceType traceType, string format, params object[] args)
        {
            traceSource.WriteInfoWithId(traceType.Name, traceType.Id, format, args);
            ConditionalConsoleWriteLine(format, args);
        }

        public static void WriteWarning(this TraceType traceType, string format, params object[] args)
        {
            traceSource.WriteWarningWithId(traceType.Name, traceType.Id, format, args);
            ConditionalConsoleWriteLine(format, args);
        }
        
        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
        public static void WriteError(this TraceType traceType, string format, params object[] args)
        {
            traceSource.WriteErrorWithId(traceType.Name, traceType.Id, format, args);
            ConditionalConsoleWriteLine(format, args);
        }

        public static void ConsoleWriteLine(this TraceType traceType, string format, params object[] args)
        {
            traceSource.WriteInfoWithId(traceType.Name, traceType.Id, format, args);
            Console.WriteLine(format, args);
        }

        public static COMException CreateException(this TraceType traceType, NativeTypes.FABRIC_ERROR_CODE error, string format, params object[] args)
        {
            string message = string.Format(CultureInfo.InvariantCulture, format, args);
            var exception = new COMException(message, (int)error);

            traceSource.WriteExceptionAsWarningWithId(traceType.Name, traceType.Id, exception, string.Empty);

            return exception;
        }

        /// <summary>
        /// Writes an aggregated event for consumption by listeners (e.g. TraceViewer).
        /// Here, the typeNameSuffix and key parameter combination has to be distinct for each event.
        /// If they are the same, then DCA will forward only the last event.
        /// So, don't use this for dumping snapshot views. (i.e. poll every x seconds and dump state)
        /// Instead use this for dumping state changes where you can tolerate loss of this message downstream.
        /// </summary>
        /// <remarks>
        /// Also, use this only for low volume data since DCA has to handle a distinct Id each time at the VM level.
        /// </remarks>
        /// <example>
        /// traceType.WriteAggregatedEvent(
        ///     "CompletedRepairTask"
        ///     "Azure/PlatformMaintenance/1e56e313-0e1e-47d0-85f2-9f75e4833a4f/4/5644"
        ///     "{0}",
        ///     repairTask.ToJson());
        /// </example>
        public static void WriteAggregatedEvent(
            this TraceType traceType, 
            string typeNameSuffix, 
            string key, 
            string format, 
            params object[] args)
        {
            traceType.Validate("traceType");
            typeNameSuffix.Validate("typeNameSuffix");
            key.Validate("key");

            // E.g. WARP-Prod-CDM-CDM05PrdApp01@CompletedRepairTask@Azure/PlatformMaintenance/1e56e313-0e1e-47d0-85f2-9f75e4833a4f/4/5644
            string id = "{0}@{1}@{2}".ToString(traceType.Id, typeNameSuffix, key);
            string message = string.Format(CultureInfo.InvariantCulture, format, args);

            // this shows up in TraceViewer under the Type column as
            // InfrastructureServiceAggregatedEvent@WARP-Prod-CDM-CDM05PrdApp01@CompletedRepairTask@Azure/PlatformMaintenance/1e56e313-0e1e-47d0-85f2-9f75e4833a4f/4/5644
            FabricEvents.Events.InfrastructureServiceAggregatedEvent(id, message);            
        }
    }
}