// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Parser
{
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;

    public static class HelpfulExtensions
    {
        /// <summary>
        /// Allows us to buffer all the rows that will be in the Observable stream and return a awaitable task for this buffering.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="observable"></param>
        /// <returns></returns>
        public static Task<IList<T>> BufferAllAsync<T>(this IObservable<T> observable)
        {
            List<T> result = new List<T>();
            object gate = new object();
            TaskCompletionSource<IList<T>> finalTask = new TaskCompletionSource<IList<T>>();

            observable.Subscribe(
                new AllPurposeObserver<T>
                {
                    OnNextCallback = value =>
                    {
                        lock (gate)
                        {
                            result.Add(value);
                        }
                    },
                    OnCompletedCallback = () => finalTask.SetResult(result.AsReadOnly()),
                    OnErrorCallback = exception => finalTask.TrySetException(exception),
                });

            return finalTask.Task;
        }

        private class AllPurposeObserver<T> : IObserver<T>
        {
            public Action<T> OnNextCallback { private get; set; }

            public Action OnCompletedCallback { private get; set; }

            public Action<Exception> OnErrorCallback { private get; set; }

            public void OnNext(T value)
            {
                this.OnNextCallback(value);
            }

            public void OnError(Exception error)
            {
                this.OnErrorCallback(error);
            }

            public void OnCompleted()
            {
                this.OnCompletedCallback();
            }
        }
    }
}