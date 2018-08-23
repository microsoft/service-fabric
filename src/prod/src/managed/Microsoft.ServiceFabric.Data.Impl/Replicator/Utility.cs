// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.IO;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    using Microsoft.Win32.SafeHandles;

    /// <summary>
    /// Replicator utility class.
    /// </summary>
    internal static class Utility
    {
        public static void SetIoPriorityHint(SafeFileHandle safeFileHandle, Kernel32Types.PRIORITY_HINT priorityHintInfo)
        {
            Kernel32Types.FileInformation fileInformation = new Kernel32Types.FileInformation();
            fileInformation.FILE_IO_PRIORITY_HINT_INFO.PriorityHint = priorityHintInfo;

            bool isSet = FabricFile.SetFileInformationByHandle(
                safeFileHandle,
                Kernel32Types.FILE_INFO_BY_HANDLE_CLASS.FileIoPriorityHintInfo,
                ref fileInformation,
                Marshal.SizeOf(fileInformation.FILE_IO_PRIORITY_HINT_INFO));

            if (isSet == false)
            {
                int status = Marshal.GetLastWin32Error();
                Assert(false, "SetFileInformationByHandle failed: ErrorCode: {0}", status);
            }
        }

#if !DotNetCoreClr
        // A few tests depends on this. Remove this after clearing up the tests.
        internal static object[] ArgListToParams(RuntimeArgumentHandle handle)
        {
            var iter = new ArgIterator(handle);
            var count = iter.GetRemainingCount();
            var args = new object[count];

            for (var i = 0; i < count; i++)
            {
                args[i] = TypedReference.ToObject(iter.GetNextArg());
            }

            return args;
        }
#endif

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format)
        {
            if (condition == false)
            {
                CodingError(format, new object[0]);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, long param1)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, long param1, long param2)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, string param1)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, LogRecordType param1)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, TransactionState param1, long param2)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, ReplicaRole param1)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, DrainingStream param1)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, Exception param1)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, ApplyContext param1)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, string param1, long param2)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, Metadata param1)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, Uri param1)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, StateManagerApplyType param1)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, ReplicationMetadata param1)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, bool param1)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, LogicalSequenceNumber param1)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, LogicalSequenceNumber param1, LogicalSequenceNumber param2)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, bool param1, bool param2)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, LogicalSequenceNumber param1, RecoveredOrCopiedCheckpointLsn param2, bool param3)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2, param3);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, string param1, LogicalSequenceNumber param2, LogicalSequenceNumber param3)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2, param3);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, string param1, long param2, long param3)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2, param3);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, long param1, long param2, int param3, StateManagerApplyType param4)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2, param3, param4);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, DrainingStream param1, LogRecordType param2)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, bool param1, LogRecordType param2)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, CheckpointState param1, CheckpointState param2)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, LogicalSequenceNumber param1, PhysicalSequenceNumber param2)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, string param1, long param2, long param3, long param4)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2, param3, param4);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, ulong param1)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, LogRecord param1)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, PhysicalSequenceNumber param1, PhysicalSequenceNumber param2)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, string param1, string param2)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, string param1, string param2, string param3)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2, param3);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, LogicalSequenceNumber param1, long param2, bool param3)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2, param3);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, LogicalSequenceNumber param1, LogicalSequenceNumber param2, LogRecordType param3)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2, param3);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, LogicalSequenceNumber param1, LogRecordType param2)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, RecordProcessingMode param1)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, DrainingStream param1, LogicalSequenceNumber param2, PhysicalSequenceNumber param3)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2, param3);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, OperationLogRecord param1, BeginCheckpointLogRecord param2)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, ReplicationMetadata param1, Metadata param2)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, long param1, Metadata param2, long param3)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2, param3);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, Metadata param1, long param2, long param3)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2, param3);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, CopyStage param1)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, long param1, TransactionState param2)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, ulong param1, ulong param2)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, string param1, bool param2, string param3, string param4)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2, param3, param4);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, BeginCheckpointLogRecord param1, LogRecord param2)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, TruncateHeadLogRecord param1, LogRecord param2)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1, param2);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, CheckpointState param1)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, TruncationState param1)
        {
            if (condition == false)
            {
                var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, param1);
                FailFast(failFastMessage);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Assert(bool condition, string format, params object[] list)
        {
            if (condition == false)
            {
                CodingError(format, list);

                // AMW - Force break into debugger for ease of debugging
                Debugger.Break();
            }
        }

        internal static void FailFast(string failFastMessage)
        {
            var processId = Process.GetCurrentProcess().Id;
            var threadId = Thread.CurrentThread.ManagedThreadId;
            var component = string.Format(
                System.Globalization.CultureInfo.InvariantCulture,
                "{0}:{1}@{2}",
                processId,
                threadId,
                DateTime.UtcNow);
            var errorMessage = string.Format(
                System.Globalization.CultureInfo.InvariantCulture,
                "Assert or Coding error occurred.\n{0}",
                Environment.StackTrace);

            AppTrace.TraceSource.WriteError(
                component,
                @"Assert or Coding error with message {0}. Stack : {1}",
                failFastMessage,
                errorMessage);

            if (Debugger.IsAttached)
            {
                Debugger.Break();
            }

            if (IsDebuggerPresent())
            {
                OutputDebugString(component);
                OutputDebugString(errorMessage);
                DebugBreak();
            }

            Environment.FailFast(failFastMessage);

            return;
        }

        internal static void CodingError(string format, params object[] args)
        {
            var failFastMessage = string.Format(System.Globalization.CultureInfo.InvariantCulture, format, args);
            FailFast(failFastMessage);
        }

        internal static void CreateHardLinkOrCopyFile(string sourceFileName, string destinationFileName)
        {
            if (!FabricFile.Exists(sourceFileName))
            {
                return;
            }

            // Since the source file won't be changed, we can simply create a hardlink
            if (!FabricFile.CreateHardLink(destinationFileName, sourceFileName))
            {
                // Hardlink can fail, e.g. if the destination is on another drive or the drive isn't NTFS.
                // Fallback to a full file copy.
                FabricFile.Copy(sourceFileName, destinationFileName, overwrite: true);
            }
        }

        internal static Exception FlattenException(Exception e, out int innerHResult)
        {
            if (e.InnerException != null)
            {
                innerHResult = e.InnerException.HResult;
            } 
            else 
            {
                innerHResult = 0;
            }

            var exception = e as AggregateException;
            if (exception != null)
            {
                e = exception.Flatten().InnerException;
            }

            return e;
        }

        internal static Type GetSerializedType(string typeName)
        {
            try
            {
                var simpleTypeName = GetSimpleTypeName(typeName);
                var type = Type.GetType(simpleTypeName);
                if (type != null)
                {
                    return type;
                }
            }
            catch (Exception)
            {
            }

            try
            {
                return Type.GetType(typeName);
            }
            catch (Exception)
            {
            }

            return null;
        }

        internal static string GetSimpleTypeName(Type type)
        {
            var typeName = type.AssemblyQualifiedName;
            return GetSimpleTypeName(typeName);
        }

        internal static long GetOperationSize(OperationData operationData)
        {
            long size = 0;
            if (operationData != null && operationData.Count > 0)
            {
                foreach (var segment in operationData)
                {
                    size += segment.Count;
                }
            }
            return size;
        }

        /// <summary>
        /// Transforms type names from assembly qualified format to a simpler format for serialization.
        /// 
        /// Transforms type names from:
        /// - Namespace.Foo`1[[System.Int32, mscorlib, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089]], MyAssembly, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null
        /// to:
        /// - Namespace.Foo`1[[System.Int32, mscorlib]], MyAssembly
        /// </summary>
        /// <param name="typeName">Type name to transform.</param>
        /// <returns>Type name usable for replicated serialization.</returns>
        internal static string GetSimpleTypeName(string typeName)
        {
            var writer = new StringBuilder(typeName.Length);
            var currentIndex = 0;
            var commaActive = false;

            // Walk the string and only append characters associated with the type name and assembly name
            while (currentIndex < typeName.Length)
            {
                // Commas are the important delimiters between entities
                if (typeName[currentIndex] == ',')
                {
                    if (commaActive)
                    {
                        // If this is the second comma we've seen, it delimits extraneous
                        // entities (Version, Culture, PublicKeyToken, etc.) that we can discard
                        while (currentIndex < typeName.Length && typeName[currentIndex] != ']')
                        {
                            currentIndex++;
                        }

                        commaActive = false;
                        continue;
                    }
                    else
                    {
                        commaActive = true;
                    }
                }
                else if (typeName[currentIndex] == '[')
                {
                    // This is the start of a generic type
                    commaActive = false;
                }

                writer.Append(typeName[currentIndex]);
                currentIndex++;
            }

            return writer.ToString();
        }

        internal static void ProcessUnexpectedException(
            string component,
            ITracer tracer,
            string messagePrefix,
            Exception e)
        {
            int innerHResult = 0;
            var flattenedException = FlattenException(e, out innerHResult);
            tracer.WriteError(
                component,
                Environment.NewLine + "    Encountered unexpected exception " + messagePrefix + ": {0}" + Environment.NewLine +
                "    Message: {1} HResult: 0x{2:X8}" + Environment.NewLine + "{3}",
                flattenedException.GetType().ToString(),
                flattenedException.Message,
                flattenedException.HResult != 0 ? flattenedException.HResult : innerHResult,
                flattenedException.StackTrace);

            // We should never arrive here
            Assert(false, "ProcessUnexpectedException");
        }

        internal static async Task ReadAsync(Stream stream, byte[] buffer)
        {
            int numberOfBytesRead = 0, totalNumberOfBytesToRead = buffer.Length;

            do
            {
                var numberOfBytesToRead = totalNumberOfBytesToRead - numberOfBytesRead;
                var currentNumberOfBytesRead = await stream.ReadAsync(buffer, numberOfBytesRead, numberOfBytesToRead).ConfigureAwait(false);
                Assert(
                    currentNumberOfBytesRead > 0,
                    "ReadAsync: currentNumberOfBytesRead ({0}) > 0",
                    currentNumberOfBytesRead);
                numberOfBytesRead += currentNumberOfBytesRead;
            } while (numberOfBytesRead < totalNumberOfBytesToRead);

            return;
        }

        internal static string ToString(Epoch epoch)
        {
            return string.Format("{0}:{1:x}", epoch.DataLossNumber, epoch.ConfigurationNumber);
        }

        [DllImport("kernel32.dll")]
        private static extern void DebugBreak();

        [DllImport("kernel32.dll")]
        private static extern bool IsDebuggerPresent();

        [DllImport("kernel32.dll")]
        private static extern void OutputDebugString(string message);
    }
}