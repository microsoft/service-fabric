// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System.Fabric.Common;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Threading.Tasks;
    using System.Threading;

    internal sealed class UpdatePackageRetriever
    {
        const string queryStringFormat = @"IsInstalled=0 and Type='Software' and IsHidden=1 and CategoryIDs contains '{0}'";
        private readonly string queryString;
        private NativeMethods.IUpdateSession3 updateSession;

        public UpdatePackageRetriever()
        {
            this.queryString = string.Format(queryStringFormat, Constants.WUSCoordinator.CategoryId);
            this.updateSession = (NativeMethods.IUpdateSession3)new NativeMethods.UpdateSession();
            this.updateSession.ClientApplicationID = "Upgrade Service";
        }

        public async Task<NativeMethods.IUpdateCollection> GetAvailableUpdates(
            TimeSpan timeout,
            CancellationToken token)
        {
            var searchOperation = SearchWindowsUpdateAsync(updateSession);
            return await AwaitForOperationToComplete(searchOperation, timeout, token, "Serach");
        }

        public async Task<ClusterUpgradeCommandParameter> DownloadWindowsUpdate(
            NativeMethods.IUpdate update, 
            TimeSpan timeout,
            CancellationToken token)
        {
            if (update == null)
            {
                return null;
            }

            if (!update.IsDownloaded)
            {
                var downloadOperation = DownloadUpdateAsync(updateSession, update);
                await AwaitForOperationToComplete(downloadOperation, timeout, token, "Download");
            }

            if (update.IsDownloaded)
            {
                string dir = Path.Combine(Path.GetTempPath(), Path.GetRandomFileName());
                var info = new DirectoryInfo(dir);
                if (!info.Exists)
                {
                    info.Create();
                }

                update.CopyFromCache(dir, false);                
                var cabFilePath = Directory.GetFiles(dir, "*.cab").FirstOrDefault();
                if (cabFilePath != null)
                {
                    var cabVersion = CabFileOperations.GetCabVersion(cabFilePath);
                    return new ClusterUpgradeCommandParameter()
                    {
                        CodeFilePath = cabFilePath,
                        CodeVersion = cabVersion
                    };
                }
            }

            return null;
        }

        private Task<NativeMethods.IUpdateCollection> SearchWindowsUpdateAsync(NativeMethods.IUpdateSession3 updateSession)
        {
            var updateSearcher = updateSession.CreateUpdateSearcher();
            TaskCompletionSource<NativeMethods.IUpdateCollection> tcs = new TaskCompletionSource<NativeMethods.IUpdateCollection>();
            var job = updateSearcher.BeginSearch(this.queryString, new SearchCompletedCallback(updateSearcher, tcs), null);
            return tcs.Task;
        }

        private Task<NativeMethods.IDownloadResult> DownloadUpdateAsync(NativeMethods.IUpdateSession3 updateSession, NativeMethods.IUpdate update)
        {
            NativeMethods.IUpdateDownloader updateDownloader = updateSession.CreateUpdateDownloader();
            updateDownloader.Updates = (NativeMethods.IUpdateCollection)new NativeMethods.UpdateCollection();
            updateDownloader.Updates.Add(update);
            TaskCompletionSource<NativeMethods.IDownloadResult> tcs = new TaskCompletionSource<NativeMethods.IDownloadResult>();
            var job = updateDownloader.BeginDownload(
                new DownloadProgressCallback(),
                new DownloadCompletedCallback(updateDownloader, tcs),
                null);
            return tcs.Task;
        }

        private static async Task<T> AwaitForOperationToComplete<T>(Task<T> operationTask, TimeSpan timeout, CancellationToken token, string operationName)
        {
            using (var source = CancellationTokenSource.CreateLinkedTokenSource(token))
            {
                var completedTask = await Task.WhenAny(operationTask, Task.Delay(timeout, source.Token));
                if (completedTask == operationTask)
                {
                    source.Cancel();
                    return await operationTask;
                }
                else
                {
                    if (completedTask.IsCanceled)
                    {
                        throw new TaskCanceledException(string.Format(CultureInfo.InvariantCulture, "Operation: {0} was cancelled", operationName));
                    }

                    throw new TimeoutException(string.Format(CultureInfo.InvariantCulture, "Operation: {0} was didn't complete within given time", operationName));
                }
            }
        }
    }

    internal class SearchCompletedCallback : NativeMethods.ISearchCompletedCallback
    {
        private readonly NativeMethods.IUpdateSearcher updateSearcher;
        private readonly TaskCompletionSource<NativeMethods.IUpdateCollection> tcs;

        public SearchCompletedCallback(NativeMethods.IUpdateSearcher updateSearcher, TaskCompletionSource<NativeMethods.IUpdateCollection> tcs)
        {
            this.updateSearcher = updateSearcher;
            this.tcs = tcs;
        }

        public void Invoke(NativeMethods.ISearchJob searchJob, NativeMethods.ISearchCompletedCallbackArgs callbackArgs)
        {
            var result = updateSearcher.EndSearch(searchJob);
            if (result.ResultCode == NativeMethods.OperationResultCode.Aborted)
            {
                this.tcs.SetCanceled();
            }
            else if (result.ResultCode == NativeMethods.OperationResultCode.Succeeded)
            {
                if (result.Updates.Count == 0)
                {
                    this.tcs.SetResult(null);
                    return;
                }

                this.tcs.SetResult(result.Updates);
            }
            else
            {
                this.tcs.SetException(new COMException("Search for update failed with result code: " + result.ResultCode));
            }
        }
    }

    internal class DownloadProgressCallback : NativeMethods.IDownloadProgressChangedCallback
    {
        public void Invoke(NativeMethods.IDownloadJob downloadJob, NativeMethods.IDownloadProgressChangedCallbackArgs callbackArgs)
        {
        }
    }

    internal class DownloadCompletedCallback : NativeMethods.IDownloadCompletedCallback
    {
        private readonly TaskCompletionSource<NativeMethods.IDownloadResult> tcs;
        private readonly NativeMethods.IUpdateDownloader updateDownloader;

        public DownloadCompletedCallback(NativeMethods.IUpdateDownloader updateDownloader, TaskCompletionSource<NativeMethods.IDownloadResult> tcs)
        {
            this.tcs = tcs;
            this.updateDownloader = updateDownloader;
        }

        public void Invoke(NativeMethods.IDownloadJob downloadJob, NativeMethods.IDownloadCompletedCallbackArgs callbackArgs)
        {
            var result = updateDownloader.EndDownload(downloadJob);
            if (result.ResultCode == NativeMethods.OperationResultCode.Aborted)
            {
                this.tcs.SetCanceled();
            }
            else if (result.ResultCode == NativeMethods.OperationResultCode.Succeeded)
            {
                this.tcs.SetResult(result);
            }
            else
            {
                this.tcs.TrySetException(new COMException("Search for update failed with result code: " + result.ResultCode));
            }
        }
    }
}