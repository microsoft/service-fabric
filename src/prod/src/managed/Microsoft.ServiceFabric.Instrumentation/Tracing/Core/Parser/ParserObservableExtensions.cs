// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Parser
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Parsers;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Reader;

    public static class ParserObservableExtensions
    {
        public static IObservable<T> ObserveAsync<T>(this ITraceRecordParser parser, ITraceRecordReader reader, CancellationToken token) where T : TraceRecord
        {
            var ret = new TraceRecordObservable<T>(
                reader,
                (callback, subscriptionId) => parser.SubscribeAsync(callback, token),
                (callback, subscriptionId) => parser.UnSubscribeAsync(callback, token));
            return ret;
        }

        private class TraceRecordObservable<T> : IObservable<T> where T : TraceRecord
        {
            private ITraceRecordReader traceReader;

            /// <summary>
            /// This is the Add action - the one we can use to register a new Callback for a particular Trace
            /// </summary>
            private Action<Func<T, CancellationToken, Task>, object> addHandler;

            /// <summary>
            /// This is the remove action - the one we use to unregister an existing callback registration for a specific trace.
            /// </summary>
            private Action<Func<T, CancellationToken, Task>, object> removeHandler;

            public TraceRecordObservable(
                ITraceRecordReader reader,
                Action<Func<T, CancellationToken, Task>, object> addHandler,
                Action<Func<T, CancellationToken, Task>, object> removeHandler)
            {
                this.traceReader = reader;
                this.addHandler = addHandler;
                this.removeHandler = removeHandler;
            }

            /// <inheritdoc />
            public IDisposable Subscribe(IObserver<T> observer)
            {
                return new TraceRecordSubscription(
                    (data, token) =>
                    {
                        var cloned = data.Clone();
                        observer.OnNext((T)cloned);
                        return Task.FromResult(true);
                    },
                    observer.OnCompleted,
                    this);
            }

            /// <summary>
            /// A helper class that connects 'callbackMethod' and 'completedCallbackMethod' to the 'observable' and 
            /// unhooks them when 'Dispose' is called.  
            /// </summary>
            private class TraceRecordSubscription : IDisposable
            {
                private Func<T, CancellationToken, Task> callbackMethod;
                private Action completedCallbackMethod;
                private TraceRecordObservable<T> observable;

                internal TraceRecordSubscription(
                    Func<T, CancellationToken, Task> callbackMethod,
                    Action completedCallbackMethod,
                    TraceRecordObservable<T> observable)
                {
                    // Used for Despose
                    this.callbackMethod = callbackMethod;
                    this.completedCallbackMethod = completedCallbackMethod;
                    this.observable = observable;

                    // Ask the parser to register the registered callback
                    this.observable.addHandler(this.callbackMethod, this);

                    // And the comleted callbackMethod handler
                    this.observable.traceReader.Completed += this.completedCallbackMethod;
                }

                /// <inheritdoc />
                public void Dispose()
                {
                    this.observable.traceReader.Completed -= this.completedCallbackMethod;
                    this.observable.removeHandler(this.callbackMethod, this);
                }
            }
        }
    }
}