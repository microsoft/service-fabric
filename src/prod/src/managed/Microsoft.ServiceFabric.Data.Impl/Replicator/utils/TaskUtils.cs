// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Globalization;
    using System.Linq;
    using System.Reflection;
    using System.Runtime.CompilerServices;
    using System.Threading.Tasks;
    using System.Fabric.Common.Tracing;

    internal static class TaskUtils
    {
#if DEBUG && !DotNetCoreClr
        private static readonly ConditionalWeakTable<object, Tracker> WatchedObjects =
            new ConditionalWeakTable<object, Tracker>();
#endif

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static T GCUnwatch<T>(this T @object)
        {
#if DEBUG && !DotNetCoreClr
            Tracker tracker;
            if (WatchedObjects.TryGetValue(@object, out tracker))
            {
                WatchedObjects.Remove(@object);
            }

#endif
            return @object;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static T GCWatch<T>(this T @object, Action<Tracker> callback, object state = null)
        {
#if DEBUG && !DotNetCoreClr
            WatchedObjects.Add(@object, new Tracker(@object, callback, state));
#endif
            return @object;
        }

        internal sealed class Tracker
        {
            public readonly object Object;

            public readonly object State;

            private readonly Action<Tracker> callback;

            public Tracker(object @object, Action<Tracker> callback, object state)
            {
                this.Object = @object;
                this.State = state;
                this.callback = callback;
            }

            ~Tracker()
            {
                this.callback(this);
            }
        }
    }

    [SuppressMessage("Microsoft.StyleCop.CSharp.MaintainabilityRules", "SA1402:FileMayOnlyContainASingleClass", Justification = "Too many perf counter classes")
    ]
    internal static class ReplicatorTaskExtensions
    {
#if DEBUG && !DotNetCoreClr
        // We do not build this in RELEASE
        private static FieldInfo contingentPropertiesFI = null;

        private static FieldInfo exceptionsHolderFI = null;

        private static FieldInfo isHandledFI = null;
#endif

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Task Track(
            this Task task,
            ITracer tracer,
            string tag = null,
            [CallerMemberName] string callerMemberName = null,
            [CallerFilePath] string callerPathName = null,
            [CallerLineNumber] int callerLineNumber = 0)
        {
            return task.GCWatch(
                TaskGC,
                new TaskTrackInfo(tracer, tag, callerMemberName, callerPathName, callerLineNumber));
        }

        internal static Task IgnoreException(this Task task)
        {
            // See TaskScheduler.UnobservedTaskException & log via AggregateException's Flatten()
            task.ContinueWith(t => t.Exception, TaskContinuationOptions.OnlyOnFaulted);
            return task;
        }

        internal static Task<TResult> IgnoreException<TResult>(this Task<TResult> task)
        {
            // See TaskScheduler.UnobservedTaskException & log via AggregateException's Flatten()
            task.ContinueWith(t => t.Exception, TaskContinuationOptions.OnlyOnFaulted);
            return task;
        }

        internal static Task IgnoreException(this Task<Task> taskOfTask)
        {
            var task = taskOfTask.Unwrap();
            return task.IgnoreException();
        }

        internal static Task<TResult> IgnoreException<TResult>(this Task<Task<TResult>> taskOfTask)
        {
            var task = taskOfTask.Unwrap();
            return task.IgnoreException();
        }

        internal static void IgnoreExceptionVoid(this Task task)
        {
            // See TaskScheduler.UnobservedTaskException & log via AggregateException's Flatten()
            task.ContinueWith(t => t.Exception, TaskContinuationOptions.OnlyOnFaulted);
        }

        internal static void IgnoreExceptionVoid(this Task<Task> taskOfTask)
        {
            var task = taskOfTask.Unwrap();
            task.IgnoreExceptionVoid();
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static TaskCompletionSource<T> Track<T>(
            this TaskCompletionSource<T> tcs,
            ITracer tracer,
            string tag = null,
            [CallerMemberName] string callerMemberName = null,
            [CallerFilePath] string callerPathName = null,
            [CallerLineNumber] int callerLineNumber = 0)
        {
            tcs.Task.Track(tracer, tag, callerMemberName, callerPathName, callerLineNumber);
            return tcs;
        }

#if DEBUG && !DotNetCoreClr
        internal static bool HasUnobservedExceptions(this Task task)
        {
            // t.Task.m_contingentProperties.m_exceptionsHolder.m_isHandled
            if (contingentPropertiesFI == null)
            {
                contingentPropertiesFI =
                    typeof(Task).GetTypeInfo().DeclaredFields.First(fi => fi.Name == "m_contingentProperties");
            }

            var contingentProperties = contingentPropertiesFI.GetValue(task);
            if (contingentProperties == null)
            {
                return false;
            }

            if (exceptionsHolderFI == null)
            {
                exceptionsHolderFI =
                    contingentProperties.GetType()
                        .GetTypeInfo()
                        .DeclaredFields.First(fi => fi.Name == "m_exceptionsHolder");
            }

            var exceptionsHolder = exceptionsHolderFI.GetValue(contingentProperties);
            if (exceptionsHolder == null)
            {
                return false;
            }

            if (isHandledFI == null)
            {
                isHandledFI =
                    exceptionsHolder.GetType().GetTypeInfo().DeclaredFields.First(fi => fi.Name == "m_isHandled");
            }

            return !(bool) isHandledFI.GetValue(exceptionsHolder);
        }
#endif

        private static void TaskGC(TaskUtils.Tracker tracker)
        {
#if DEBUG && !DotNetCoreClr
            var task = (Task) tracker.Object;
            var tti = (TaskTrackInfo) tracker.State;
            if (!task.HasUnobservedExceptions())
            {
                return;
            }

            var ex = string.Format(
                CultureInfo.InvariantCulture,
                "TaskGC - Unobserved Task Exception at {0},{1},{2}",
                tti.CallerMemberName,
                tti.CallerPathName,
                tti.CallerLineNumber);

            // var st = task.Exception.StackTrace;
            FabricEvents.Events.Exception_TStatefulServiceReplica(
                tti.Tracer.Type,
                ex,
                task.Exception.GetType().ToString(),
                task.Exception.Message,
                task.Exception.HResult,
                task.Exception.StackTrace);
#endif
        }

        private sealed class TaskTrackInfo
        {
            public readonly int CallerLineNumber;

            public readonly string CallerMemberName;

            public readonly string CallerPathName;

            public readonly string Tag;

            public readonly ITracer Tracer;

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public TaskTrackInfo(
                ITracer tracer,
                string tag,
                [CallerMemberName] string callerMemberName = null,
                [CallerMemberName] string callerPathName = null,
                [CallerLineNumber] int callerLineNumber = 0)
            {
                this.Tracer = tracer;
                this.Tag = tag;
                this.CallerMemberName = callerMemberName;
                this.CallerPathName = callerPathName;
                this.CallerLineNumber = callerLineNumber;
            }
        }
    }
}