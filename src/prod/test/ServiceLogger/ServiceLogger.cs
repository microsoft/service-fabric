// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace MS.Test.WinFabric.Logging
{
    using System;
    using System.Diagnostics;
    using System.Diagnostics.Eventing;
    using System.Fabric;
    using System.Linq;
    using System.Reflection;
    using Transaction = Microsoft.ServiceFabric.Replicator.Transaction;

    public class ServiceLogger : IDisposable
    {
        private EventProvider tracingEtw;
        private bool isDisposed = false;

        protected EventProvider Trace { get { return tracingEtw; } }

        public string ServiceName { get; set; }
        public string PartitionIdentifier { get; set; }
        public string ReplicaIdentifier { get; set; }
        public string ReplicaLabel { get; set; }

        enum LoggingMode
        {
            Generic,
            Service,
            Host,
        }

        LoggingMode Mode { get; set; }

        public ServiceLogger(Guid etwProviderGuid)
        {
            tracingEtw = new EventProvider(etwProviderGuid);
            Mode = LoggingMode.Generic;
        }

        public void InitializeService(ServiceInitializationParameters initializationParameters)
        {
            Mode = LoggingMode.Service;

            if (initializationParameters == null)
            {
                ServiceName = "unknown";
                PartitionIdentifier = "unknown";
                ReplicaIdentifier = "unknown";
                ReplicaLabel = "R";
            }
            else if (initializationParameters is StatefulServiceInitializationParameters)
            {
                ServiceName = initializationParameters.ServiceName.OriginalString;
                ReplicaIdentifier = ((StatefulServiceInitializationParameters)initializationParameters).ReplicaId.ToString();
                PartitionIdentifier = initializationParameters.PartitionId.ToString();
                ReplicaLabel = "R";
            }
            else if (initializationParameters is StatelessServiceInitializationParameters)
            {
                ServiceName = initializationParameters.ServiceName.OriginalString;
                ReplicaIdentifier = ((StatelessServiceInitializationParameters)initializationParameters).InstanceId.ToString();
                PartitionIdentifier = initializationParameters.PartitionId.ToString();
                ReplicaLabel = "I";
            }
            else
            {
                ServiceName = initializationParameters.ServiceName.OriginalString;
                ReplicaIdentifier = "n/a";
                PartitionIdentifier = initializationParameters.PartitionId.ToString();
                ReplicaLabel = "R";
            }
        }

        public void InitializeHost(string hostName)
        {
            Mode = LoggingMode.Host;
        }

        public static void WriteWithColor(string message, ConsoleColor color)
        {
            Console.ForegroundColor = color;
            Console.WriteLine(message);
        }

        public void Log(string message)
        {
            Log(LogLevel.Information, message);
        }

        public void Log(LogLevel level, string message)
        {
            string componentName = GetCallerComponentName();

            switch (Mode)
            {
                case LoggingMode.Generic:
                    Trace.WriteMessageEvent(string.Format("[PID:{0}] [>{1}] {2}", Process.GetCurrentProcess().Id, componentName, message), (byte)level, 0);
                    break;

                case LoggingMode.Service:
                    Trace.WriteMessageEvent(string.Format("[S:{0}] [P:{1}] [{2}:{3}] [>{4}] {5}", ServiceName, PartitionIdentifier, ReplicaLabel, ReplicaIdentifier, componentName, message), (byte)level, 0);
                    break;

                case LoggingMode.Host:
                    Trace.WriteMessageEvent(string.Format("[HostName:{0}] [PID:{1}] [>{2}] {3}", ServiceName, Process.GetCurrentProcess().Id, componentName, message), (byte)level, 0);
                    break;
            }
        }

        public void Log(Transaction tx, string message)
        {
            Log(tx, LogLevel.Information, message);
        }

        public void Log(Transaction tx, LogLevel level, string message)
        {
            string componentName = GetCallerComponentName();

            switch (Mode)
            {
                case LoggingMode.Generic:
                    Trace.WriteMessageEvent(string.Format("[PID:{0}] [TX:{1}] [>{2}] {3}", Process.GetCurrentProcess().Id, GetTransactionId(tx), componentName, message), (byte)level, 0);
                    break;

                case LoggingMode.Service:
                    Trace.WriteMessageEvent(string.Format("[S:{0}] [P:{1}] [{2}:{3}] [TX:{4}] [>{5}] {6}", ServiceName, PartitionIdentifier, ReplicaLabel, ReplicaIdentifier, GetTransactionId(tx), componentName, message), (byte)level, 0);
                    break;

                case LoggingMode.Host:
                    Trace.WriteMessageEvent(string.Format("[HostName:{0}] [PID:{1}] [TX:{2}] [>{3}] {4}", ServiceName, Process.GetCurrentProcess().Id, GetTransactionId(tx), componentName, message), (byte)level, 0);
                    break;
            }
        }

        public void Log(string format, params object[] args)
        {
            Log(LogLevel.Information, format, args);
        }

        public void Log(LogLevel level, string format, params object[] args)
        {
            string componentName = GetCallerComponentName();

            switch (Mode)
            {
                case LoggingMode.Generic:
                    Trace.WriteMessageEvent(string.Format("[PID:{0}] [>{1}] {2}", Process.GetCurrentProcess().Id, componentName, string.Format(format, args)), (byte)level, 0);
                    break;

                case LoggingMode.Service:
                    Trace.WriteMessageEvent(string.Format("[S:{0}] [P:{1}] [{2}:{3}] [>{4}] {5}", ServiceName, PartitionIdentifier, ReplicaLabel, ReplicaIdentifier, componentName, string.Format(format, args)), (byte)level, 0);
                    break;

                case LoggingMode.Host:
                    Trace.WriteMessageEvent(string.Format("[HostName:{0}] [PID:{1}] [>{2}] {3}", ServiceName, Process.GetCurrentProcess().Id, componentName, string.Format(format, args)), (byte)level, 0);
                    break;
            }
        }

        public void Log(Transaction tx, string format, params object[] args)
        {
            Log(tx, LogLevel.Information, format, args);
        }

        public void Log(Transaction tx, LogLevel level, string format, params object[] args)
        {
            string componentName = GetCallerComponentName();

            switch (Mode)
            {
                case LoggingMode.Generic:
                    Trace.WriteMessageEvent(string.Format("[PID:{0}] [TX:{1}] [>{2}] {3}", Process.GetCurrentProcess().Id, GetTransactionId(tx), componentName, string.Format(format, args)), (byte)level, 0);
                    break;

                case LoggingMode.Service:
                    Trace.WriteMessageEvent(string.Format("[S:{0}] [P:{1}] [{2}:{3}] [TX:{4}] [>{5}] {6}", ServiceName, PartitionIdentifier, ReplicaLabel, ReplicaIdentifier, GetTransactionId(tx), componentName, string.Format(format, args)), (byte)level, 0);
                    break;

                case LoggingMode.Host:
                    Trace.WriteMessageEvent(string.Format("[HostName:{0}] [PID:{1}] [TX:{2}] [>{3}] {4}", ServiceName, Process.GetCurrentProcess().Id, GetTransactionId(tx), componentName, string.Format(format, args)), (byte)level, 0);
                    break;
            }
        }

        private static string GetTransactionId(Transaction tx)
        {
            return tx.ToString().Split(':').First();
        }

        private static string GetCallerComponentName()
        {
            string callerClassName, callerMethodName;

            try
            {
                for (int i = 2; ; i++)
                {
                    StackFrame frame = new StackFrame(i, false);
                    MethodBase method = frame.GetMethod();
                    callerClassName = method.DeclaringType.Name;
                    callerMethodName = method.Name;

                    if (callerClassName != "ServiceLogger")
                    {
                        break;
                    }
                }
            }
            catch (StackOverflowException)
            {
                callerClassName = "Unknown";
                callerMethodName = "Unknown";
            }

            return string.Concat(callerClassName, "-", callerMethodName);
        }

        public void Dispose()
        {
            if (!isDisposed)
            {
                Trace.Dispose();
                isDisposed = true;
            }
        }
    }
}