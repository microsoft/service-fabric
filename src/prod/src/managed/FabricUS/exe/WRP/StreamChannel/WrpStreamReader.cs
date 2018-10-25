// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.IO;
    using System.Net;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    ///  In linux the stream doesn't support timeout,
    ///  we need to set timeout by ourself
    /// </summary>
    internal class WrpStreamReader : StreamReader
    {
        private int timeoutInMilliseconds = -1;
        private CancellationToken cancellationToken;

        public WrpStreamReader(Stream stream, CancellationToken cancellationToken)
            : base(stream)
        {
            this.cancellationToken = cancellationToken;
        }

        public void SetReadTimeout(int milliseconds)
        {
            if (milliseconds <= 0)
            {
                throw new ArgumentException(milliseconds.ToString());
            }

            Trace.WriteInfo(
                    new TraceType("WrpStreamReader"),
                    "WrpStreamReader:SetReadTimeout {0} ",
                    milliseconds);

            if (this.BaseStream.CanTimeout)
            {
                this.BaseStream.ReadTimeout = milliseconds;
            }
            else
            {
                Trace.WriteWarning(
                    new TraceType("WrpStreamReader"),
                    "WrpStreamReader:The stream doesn't support timeout"
                    );

                this.timeoutInMilliseconds = milliseconds;
            }
        }

        public override async Task<int> ReadAsync(char[] buffer, int index, int count)
        {
            return await ActionWithTimeout(() => base.ReadAsync(buffer, index, count));
        }

        public async Task<bool> IsEndOfStream()
        {
            return await ActionWithTimeout(() => Task.Run(()=> base.EndOfStream));
        }

        #region Private method

        private async Task<TResult> ActionWithTimeout<TResult>(Func<Task<TResult>> action)
        {
            var waitTask = action();
            if (await Task.WhenAny(waitTask, Task.Delay(timeoutInMilliseconds, this.cancellationToken)) == waitTask)
            {
                return await waitTask;
            }
            else
            {
                throw new WebException(
                    "WrpStreamReader: ActionWithTimeout timeout",
                    WebExceptionStatus.Timeout
                    );
            }
        }

        #endregion
    }
}