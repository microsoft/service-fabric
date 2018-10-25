// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Threading;
    using System.Threading.Tasks;

    internal sealed class StreamChannelCoordinator : IUpgradeCoordinator
    {
        public const string TaskName = "SFRP StreamChannel poll";

        private static readonly TraceType TraceType = new TraceType("StreamChannelCoordinator");
        
        private readonly WrpStreamChannel streamChannel;
        private readonly IExceptionHandlingPolicy exceptionPolicy;
        
        private Task streamingTask;

        public StreamChannelCoordinator(            
            WrpStreamChannel streamChannel,
            IExceptionHandlingPolicy policy)
        {            
            streamChannel.ThrowIfNull(nameof(streamChannel));
            policy.ThrowIfNull(nameof(policy));
            
            this.streamChannel = streamChannel;
            this.exceptionPolicy = policy;
        }

        public string ListeningAddress
        {
            get { return string.Empty; }
        }

        public IEnumerable<Task> StartProcessing(CancellationToken token)
        {
            Trace.WriteInfo(TraceType, "StartProcessing called");

            ReleaseAssert.AssertIf(
                this.streamingTask != null && !this.streamingTask.IsCompleted,
                                   "StartProcessing should be called only once");

            this.streamingTask = this.RunStreamChannelAsync(token);
            
            return new List<Task>() { this.streamingTask};
        }

        private async Task RunStreamChannelAsync(CancellationToken token)
        {
            Trace.WriteInfo(TraceType, "RunStreamChannelAsync initialized");

            int continuousFailureCount = 0;

            while (!token.IsCancellationRequested)
            {
                try
                {
                    var streamChannelTask = this.streamChannel.StartStreamChannel(token);
                    token.Register(this.streamChannel.OnCancel, streamChannelTask);
                    await streamChannelTask;

                    continuousFailureCount = 0;
                    
                    // this.exceptionPolicy.ReportSuccess();

                    Trace.WriteInfo(TraceType, "RunStreamChannelAsync end");
                }
                catch (Exception e)
                {
                    Trace.WriteInfo(TraceType, "RunStreamChannelAsync throws {0}", e);                    
                    continuousFailureCount++;

                    // Temporary deactivation: RDBug 13000058:Stream channel failure to process requests are counted as continuous failures.
                    //
                    //      Stream channel failure to process requests are counted as continuous failures.
                    //      The failure count is not reset on stream connect and request successful process.
                    //      Will do this on a 6.3 CUX
                    //
                    // this.exceptionPolicy.ReportError(e, token.IsCancellationRequested);
                }

                // Linearly increase back-off with a max of 30 seconds in case of failures
                int delayInSeconds = Math.Min(continuousFailureCount, 30);

                await Task.Delay(
                    TimeSpan.FromSeconds(delayInSeconds),
                    token);
            }

            Trace.WriteInfo(TraceType, "RunStreamChannelAsync cancelled");
        }
    }
}
