// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Diagnostics;
using System.Fabric.Common;
using System.Fabric.Common.Tracing;
using System.Fabric.Interop;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Win32.SafeHandles;

namespace System.Fabric.BackupRestore
{
    internal class BackupRestoreUtility
    {
        private const string TraceType = "BackupRestoreUtility";

        internal static FabricEvents.ExtensionsEvents TraceSource { get; set; }

        internal static string TrimToLength(string str, int length)
        {
            return (str.Length <= length) ? str : str.Substring(0, length);
        }

        internal static async Task CopyDirectory(string sourceDir, string destinationDir)
        {
            var sourceDirInfo = new DirectoryInfo(sourceDir);
            foreach (var directory in sourceDirInfo.GetDirectories())
            {
                var destinationSubdirName = destinationDir + "\\" + directory.Name;
                
                // Create directory
                Directory.CreateDirectory(destinationSubdirName);

                // Copy sub directories
                await CopyDirectory(directory.FullName, destinationSubdirName);
            }

            // Now enumerate files in this directory
            foreach (var file in sourceDirInfo.GetFiles())
            {
                using (var sourceStream = File.OpenRead(file.FullName))
                {
                    using (var destinationStream = File.Create(destinationDir + "\\" + file.Name))
                    {
                        await sourceStream.CopyToAsync(destinationStream);
                    }
                }
            }
        }
        
        internal static void PerformWithRetries<T>(Action<T> worker, T context, RetriableOperationExceptionHandler exceptionHandler)
        {
            var initialRetryIntervalMs = BackupRestoreManager.InitialRetryInterval.Milliseconds;
            var maxRetryCount = BackupRestoreManager.MaxRetryCount;
            var maxRetryIntervalMs = BackupRestoreManager.MaxRetryInterval.Milliseconds;

            PerformWithRetries(worker, context, exceptionHandler, initialRetryIntervalMs, maxRetryCount, maxRetryIntervalMs);
        }

        internal static void PerformWithRetries<T>(Action<T> worker, T context, RetriableOperationExceptionHandler exceptionHandler, int maxRetryCount)
        {
            var initialRetryIntervalMs = BackupRestoreManager.InitialRetryInterval.Milliseconds;
            var maxRetryIntervalMs = BackupRestoreManager.MaxRetryInterval.Milliseconds;

            PerformWithRetries(worker, context, exceptionHandler, initialRetryIntervalMs, maxRetryCount, maxRetryIntervalMs);
        }

        internal static void PerformIOWithRetries(Action worker, int maxRetryCount)
        {
            var initialRetryIntervalMs = BackupRestoreManager.InitialRetryInterval.Milliseconds;
            var maxRetryIntervalMs = BackupRestoreManager.MaxRetryInterval.Milliseconds;

            PerformWithRetries<object>(
                       obj => worker(),
                       null,
                       new RetriableOperationExceptionHandler(RetriableIOExceptionHandler),
                       initialRetryIntervalMs,
                       maxRetryCount,
                       maxRetryIntervalMs);
        }

        internal static void PerformIOWithRetries(Action worker, int initialRetryIntervalMs, int maxRetryCount, int maxRetryIntervalMs)
        {
            PerformWithRetries<object>(
                       obj => worker(),
                       null,
                       new RetriableOperationExceptionHandler(RetriableIOExceptionHandler),
                       initialRetryIntervalMs,
                       maxRetryCount,
                       maxRetryIntervalMs);
        }

        internal static void PerformIOWithRetries<T>(Action<T> worker, T context, int maxRetryCount)
        {
            var initialRetryIntervalMs = BackupRestoreManager.InitialRetryInterval.Milliseconds;
            var maxRetryIntervalMs = BackupRestoreManager.MaxRetryInterval.Milliseconds;

            PerformWithRetries(
                       worker,
                       context,
                       new RetriableOperationExceptionHandler(RetriableIOExceptionHandler),
                       initialRetryIntervalMs,
                       maxRetryCount,
                       maxRetryIntervalMs);
        }

        internal static void PerformIOWithRetries<T>(Action<T> worker, T context, int initialRetryIntervalMs, int maxRetryCount, int maxRetryIntervalMs)
        {
            PerformWithRetries(
                       worker,
                       context,
                       new RetriableOperationExceptionHandler(RetriableIOExceptionHandler),
                       initialRetryIntervalMs,
                       maxRetryCount,
                       maxRetryIntervalMs);
        }

        internal static void PerformIOWithRetries(Action worker)
        {
            PerformWithRetries<object>(
                       obj => worker(),
                       null,
                       new RetriableOperationExceptionHandler(RetriableIOExceptionHandler));
        }

        internal static void PerformIOWithRetries<T>(Action<T> worker, T context)
        {
            PerformWithRetries(
                       worker,
                       context,
                       new RetriableOperationExceptionHandler(RetriableIOExceptionHandler));
        }

        internal static Task<TResult> PerformIOWithRetriesAsync<T, TResult>(Func<T, CancellationToken, Task<TResult>> worker, T context, CancellationToken cancellationToken)
        {
            var initialRetryIntervalMs = BackupRestoreManager.InitialRetryInterval.Milliseconds;
            var maxRetryCount = BackupRestoreManager.MaxRetryCount;
            var maxRetryIntervalMs = BackupRestoreManager.MaxRetryInterval.Milliseconds;
            return PerformWithRetriesAsync<T, TResult>(worker, context, cancellationToken, new RetriableOperationExceptionHandler(RetriableIOExceptionHandler), initialRetryIntervalMs, maxRetryCount, maxRetryIntervalMs);
        }

        internal static Task PerformIOWithRetriesAsync<T>(Func<T, CancellationToken, Task> worker, T context, CancellationToken cancellationToken,
            int initialRetryIntervalMs, int maxRetryCount, int maxRetryIntervalMs)
        {

            return PerformWithRetriesAsync<T>(worker, context, cancellationToken, new RetriableOperationExceptionHandler(RetriableIOExceptionHandler),
                initialRetryIntervalMs, maxRetryCount, maxRetryIntervalMs);
        }

        internal static void PerformWithRetries<T>(Action<T> worker, T context, RetriableOperationExceptionHandler exceptionHandler, int initialRetryIntervalMs, int maxRetryCount, int maxRetryIntervalMs)
        {
            var retryCount = 0;
            var retryIntervalMs = initialRetryIntervalMs;

            for (;;)
            {
                try
                {
                    // Perform the operation
                    worker(context);
                    break;
                }
                catch (Exception e)
                {
                    // Invoke the exception handler if present
                    var response = exceptionHandler.Handler(e);

                    if (RetriableOperationExceptionHandler.Response.Abort == response)
                    {
                        // The nature of the failure is such that we should not retry
                        throw;
                    }

                    // Should retry the operation
                    if (retryCount < maxRetryCount)
                    {
                        // We should retry the operation after an interval
                        Thread.Sleep(retryIntervalMs);

                        // Update the retry count
                        retryCount++;

                        // Update the interval to wait between retries. We are using a backoff mechanism here. 
                        // The caller is responsible for ensuring that this doesn't overflow by providing a 
                        // reasonable combination of initialRetryIntervalMS and maxRetryCount.
                        int nextRetryIntervalMs;
                        checked
                        {
                            nextRetryIntervalMs = retryIntervalMs * 2;
                        }

                        if (nextRetryIntervalMs <= maxRetryIntervalMs)
                        {
                            retryIntervalMs = nextRetryIntervalMs;
                        }
                    }
                    else
                    {
                        // The operation failed even after the maximum number of retries
                        throw;
                    }
                }
            }
        }

        internal static Task PerformFabricOperationWithRetriesAsync<T>(Func<T, CancellationToken, Task> worker, T context, CancellationToken cancellationToken)
        {
            return PerformWithRetriesAsync(worker, context, cancellationToken,
                new RetriableOperationExceptionHandler(RetriableFabricExceptionHandler),
                BackupRestoreManager.InitialRetryInterval.Milliseconds, BackupRestoreManager.MaxRetryCount,
                BackupRestoreManager.MaxRetryInterval.Milliseconds);
        }

        internal static async Task PerformWithRetriesAsync<T>(Func<T, CancellationToken, Task> worker, T context, CancellationToken cancellationToken, RetriableOperationExceptionHandler exceptionHandler, int initialRetryIntervalMs, int maxRetryCount, int maxRetryIntervalMs)
        {
            var retryCount = 0;
            var retryIntervalMs = initialRetryIntervalMs;

            for (;;)
            {
                try
                {
                    cancellationToken.ThrowIfCancellationRequested();

                    // Perform the operation
                    await worker(context, cancellationToken);
                    return;
                }
                catch (Exception e)
                {
                    // Invoke the exception handler if present
                    var response = exceptionHandler.Handler(e);

                    if (RetriableOperationExceptionHandler.Response.Abort == response)
                    {
                        // The nature of the failure is such that we should not retry
                        throw;
                    }

                    // Should retry the operation
                    if (retryCount < maxRetryCount)
                    {
                        // We should retry the operation after an interval
                        await Task.Delay(retryIntervalMs, cancellationToken);

                        // Update the retry count
                        retryCount++;

                        // Update the interval to wait between retries. We are using a backoff mechanism here. 
                        // The caller is responsible for ensuring that this doesn't overflow by providing a 
                        // reasonable combination of initialRetryIntervalMS and maxRetryCount.
                        int nextRetryIntervalMs;
                        checked
                        {
                            nextRetryIntervalMs = retryIntervalMs * 2;
                        }

                        if (nextRetryIntervalMs <= maxRetryIntervalMs)
                        {
                            retryIntervalMs = nextRetryIntervalMs;
                        }
                    }
                    else
                    {
                        // The operation failed even after the maximum number of retries
                        throw;
                    }
                }
            }
        }

        internal static async Task<bool> TryPerformOperationWithWaitAsync<T>(Func<T, CancellationToken, Task> worker, Func<bool> condition, T context,
            CancellationToken cancellationToken, int initialRetryIntervalMs, int maxRetryCount, int maxRetryIntervalMs)
        {
            var retryCount = 0;
            var retryIntervalMs = initialRetryIntervalMs;

            for (; ; )
            {
                if (condition())
                {
                    cancellationToken.ThrowIfCancellationRequested();
                    // Perform the operation
                    await worker(context, cancellationToken);
                    return true;
                }
                else
                {
                    // Should retry the operation
                    if (retryCount < maxRetryCount)
                    {
                        // We should retry the operation after an interval
                        await Task.Delay(retryIntervalMs, cancellationToken);

                        // Update the retry count
                        retryCount++;

                        // Update the interval to wait between retries. We are using a backoff mechanism here. 
                        // The caller is responsible for ensuring that this doesn't overflow by providing a 
                        // reasonable combination of initialRetryIntervalMS and maxRetryCount.
                        int nextRetryIntervalMs;
                        checked
                        {
                            nextRetryIntervalMs = retryIntervalMs * 2;
                        }

                        if (nextRetryIntervalMs <= maxRetryIntervalMs)
                        {
                            retryIntervalMs = nextRetryIntervalMs;
                        }
                    }
                    else
                    {
                        TraceSource.WriteError(TraceType, "Retry count exausted for operation : {0}.", worker.GetType().Name);
                        return false;
                    }
                }
            }
        }



        internal static TResult PerformWithRetries<T, TResult>(Func<T, TResult> worker, T context, RetriableOperationExceptionHandler exceptionHandler)
        {
            var initialRetryIntervalMs = BackupRestoreManager.InitialRetryInterval.Milliseconds;
            var maxRetryCount = BackupRestoreManager.MaxRetryCount;
            var maxRetryIntervalMs = BackupRestoreManager.MaxRetryInterval.Milliseconds;

            return PerformWithRetries<T, TResult>(worker, context, exceptionHandler, initialRetryIntervalMs, maxRetryCount, maxRetryIntervalMs);
        }

        internal static TResult PerformWithRetries<T, TResult>(Func<T, TResult> worker, T context, RetriableOperationExceptionHandler exceptionHandler, int maxRetryCount)
        {
            var initialRetryIntervalMs = BackupRestoreManager.InitialRetryInterval.Milliseconds;
            var maxRetryIntervalMs = BackupRestoreManager.MaxRetryInterval.Milliseconds;

            return PerformWithRetries<T, TResult>(worker, context, exceptionHandler, initialRetryIntervalMs, maxRetryCount, maxRetryIntervalMs);
        }

        internal static TResult PerformWithRetries<T, TResult>(Func<T, TResult> worker, T context, RetriableOperationExceptionHandler exceptionHandler, int initialRetryIntervalMs, int maxRetryCount, int maxRetryIntervalMs)
        {
            int retryCount = 0;
            int retryIntervalMs = initialRetryIntervalMs;

            for (;;)
            {
                try
                {
                    // Perform the operation
                    return worker(context);
                }
                catch (Exception e)
                {
                    // Invoke the exception handler if present
                    RetriableOperationExceptionHandler.Response response = exceptionHandler.Handler(e);

                    if (RetriableOperationExceptionHandler.Response.Abort == response)
                    {
                        // The nature of the failure is such that we should not retry
                        throw;
                    }

                    // Should retry the operation
                    if (retryCount < maxRetryCount)
                    {
                        // We should retry the operation after an interval
                        Thread.Sleep(retryIntervalMs);

                        // Update the retry count
                        retryCount++;

                        // Update the interval to wait between retries. We are using a backoff mechanism here. 
                        // The caller is responsible for ensuring that this doesn't overflow by providing a 
                        // reasonable combination of initialRetryIntervalMS and maxRetryCount.
                        int nextRetryIntervalMs;
                        checked
                        {
                            nextRetryIntervalMs = retryIntervalMs * 2;
                        }

                        if (nextRetryIntervalMs <= maxRetryIntervalMs)
                        {
                            retryIntervalMs = nextRetryIntervalMs;
                        }
                    }
                    else
                    {
                        // The operation failed even after the maximum number of retries
                        throw;
                    }
                }
            }
        }

        internal static async Task<TResult> PerformWithRetriesAsync<T, TResult>(Func<T, CancellationToken, Task<TResult>> worker, T context, CancellationToken cancellationToken, RetriableOperationExceptionHandler exceptionHandler, int initialRetryIntervalMs, int maxRetryCount, int maxRetryIntervalMs)
        {
            var retryCount = 0;
            var retryIntervalMs = initialRetryIntervalMs;

            for (;;)
            {
                try
                {
                    cancellationToken.ThrowIfCancellationRequested();

                    // Perform the operation
                    return await worker(context, cancellationToken);
                }
                catch (Exception e)
                {
                    // Invoke the exception handler if present
                    var response = exceptionHandler.Handler(e);

                    if (RetriableOperationExceptionHandler.Response.Abort == response)
                    {
                        // The nature of the failure is such that we should not retry
                        throw;
                    }

                    // Should retry the operation
                    if (retryCount < maxRetryCount)
                    {
                        // We should retry the operation after an interval
                        await Task.Delay(retryIntervalMs, cancellationToken);

                        // Update the retry count
                        retryCount++;

                        // Update the interval to wait between retries. We are using a backoff mechanism here. 
                        // The caller is responsible for ensuring that this doesn't overflow by providing a 
                        // reasonable combination of initialRetryIntervalMS and maxRetryCount.
                        int nextRetryIntervalMs;
                        checked
                        {
                            nextRetryIntervalMs = retryIntervalMs * 2;
                        }

                        if (nextRetryIntervalMs <= maxRetryIntervalMs)
                        {
                            retryIntervalMs = nextRetryIntervalMs;
                        }
                    }
                    else
                    {
                        // The operation failed even after the maximum number of retries
                        throw;
                    }
                }
            }
        }

        internal static Task PerformActionWithRetriesAsync<T1, T2>(Func<T1, T2, CancellationToken, Task> worker, T1 param1, T2 param2, CancellationToken cancellationToken, RetriableOperationExceptionHandler exceptionHandler)
        {
            var initialRetryIntervalMs = BackupRestoreManager.InitialRetryInterval.Milliseconds;
            var maxRetryCount = BackupRestoreManager.MaxRetryCount;
            var maxRetryIntervalMs = BackupRestoreManager.MaxRetryInterval.Milliseconds;

            return PerformActionWithRetriesAsync<T1, T2>(worker, param1, param2, cancellationToken, exceptionHandler, initialRetryIntervalMs, maxRetryCount, maxRetryIntervalMs);
        }

        internal static Task PerformActionWithRetriesAsync<T1, T2>(Func<T1, T2, CancellationToken, Task> worker, T1 param1, T2 param2, CancellationToken cancellationToken, RetriableOperationExceptionHandler exceptionHandler, int initialRetryIntervalMs, int maxRetryCount, int maxRetryIntervalMs)
        {
            var retryCount = 0;
            var retryIntervalMs = initialRetryIntervalMs;

            for (;;)
            {
                try
                {
                    cancellationToken.ThrowIfCancellationRequested();

                    // Perform the operation
                    return worker(param1, param2, cancellationToken);
                }
                catch (Exception e)
                {
                    // Invoke the exception handler if present
                    var response = exceptionHandler.Handler(e);

                    if (RetriableOperationExceptionHandler.Response.Abort == response)
                    {
                        // The nature of the failure is such that we should not retry
                        throw;
                    }

                    // Should retry the operation
                    if (retryCount < maxRetryCount)
                    {
                        // We should retry the operation after an interval
                        Task.Delay(retryIntervalMs, cancellationToken).Wait();

                        // Update the retry count
                        retryCount++;

                        // Update the interval to wait between retries. We are using a backoff mechanism here. 
                        // The caller is responsible for ensuring that this doesn't overflow by providing a 
                        // reasonable combination of initialRetryIntervalMS and maxRetryCount.
                        int nextRetryIntervalMs;
                        checked
                        {
                            nextRetryIntervalMs = retryIntervalMs * 2;
                        }

                        if (nextRetryIntervalMs <= maxRetryIntervalMs)
                        {
                            retryIntervalMs = nextRetryIntervalMs;
                        }
                    }
                    else
                    {
                        // The operation failed even after the maximum number of retries
                        throw;
                    }
                }
            }
        }
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
                Debug.Assert(false, String.Format("SetFileInformationByHandle failed: ErrorCode: {0}", status));
            }
        }

        private static RetriableOperationExceptionHandler.Response RetriableIOExceptionHandler(Exception e)
        {
            if ((e is IOException) || (e is FabricException))
            {
                // If the path is too long, there's no point retrying. Otherwise,
                // we can retry.
                if (false == (e is PathTooLongException))
                {
                    // Retry
                    TraceSource.WriteWarning(
                        TraceType,
                        "Exception encountered when performing retriable I/O operation. Operation will be retried if retry limit has not been reached. Exception information: {0}.",
                        e);
                    return RetriableOperationExceptionHandler.Response.Retry;
                }
            }

            // Give up
            TraceSource.WriteError(
                TraceType,
                "Exception encountered when performing retriable I/O operation. Exception information: {0}.",
                e);
            return RetriableOperationExceptionHandler.Response.Abort;
        }

        private static RetriableOperationExceptionHandler.Response RetriableFabricExceptionHandler(Exception ex)
        {
            var e = ex;

            if (ex is AggregateException)
            {
                e = ex.InnerException;
            }

            if (e is TimeoutException || e is FabricTransientException)
            {
                
                // Retry
                TraceSource.WriteWarning(
                    TraceType,
                    "Exception encountered when performing fabric operation. Operation will be retried if retry limit has not been reached. Exception information: {0}.",
                    e);
                return RetriableOperationExceptionHandler.Response.Retry;
            }

            // Give up
            TraceSource.WriteError(
                TraceType,
                "Exception encountered when performing fabric operation. Exception information: {0}.",
                e);
            return RetriableOperationExceptionHandler.Response.Abort;
        }
    }
}