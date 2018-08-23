// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.IO.Pipes;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    internal class NamedPipeHttpMessageHandler : ContainerServiceHttpMessageHandler
    {
        private const string TraceType = "NamedPipeHttpMessageHandler";
        private const int ClientConnectionTimeoutMilliSeconds = 15000;

        private readonly string serverName;
        private readonly string pipeName;
        private readonly ContainerServiceClient client;

        internal NamedPipeHttpMessageHandler(ContainerServiceClient client, string pipeName)
        {
            this.client = client;
            this.serverName = ".";
            this.pipeName = pipeName;
        }

        internal override Task<BufferedClientStream> AcquireClientStreamAsync(CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();

            NamedPipeClientStream pipeClientStream = null;
            BufferedClientStream clientStream = null;

            try
            {
                pipeClientStream = new NamedPipeClientStream(this.serverName, this.pipeName);
                pipeClientStream.Connect(ClientConnectionTimeoutMilliSeconds);

                clientStream = new BufferedClientStream(pipeClientStream);
            }
            catch(ArgumentOutOfRangeException ex)
            {
                ReleaseAssert.Failfast($"Connecting to pipe encountered: {ex}.");
            }
            catch(InvalidOperationException ex)
            {
                ReleaseAssert.Failfast($"Connecting to pipe encountered: {ex}.");
            }
            catch (Exception ex)
            {
                if (pipeClientStream != null)
                {
                    pipeClientStream.Dispose();
                }

                var errMsg = this.GetConnectionFailedErrorMessage(ex);
                AppTrace.TraceSource.WriteWarning(TraceType, errMsg);
                throw new FabricException(errMsg, FabricErrorCode.OperationTimedOut);
            }

            return Task.FromResult(clientStream);
        }

        internal override void ReleaseClientStream(BufferedClientStream clientStream)
        {
            if (clientStream != null)
            {
                clientStream.Dispose();
            }
        }

        private string GetConnectionFailedErrorMessage(Exception ex)
        {
            var sb = new StringBuilder();

            sb.AppendFormat(
                "Failed to connect to DockerService at named pipe '{0}'. IsDockerServiceManagedBySF={1}",
                this.pipeName,
                this.client.IsContainerServiceManaged);

            sb.Append($"OrignalException={ex}");

            return sb.ToString();
        }
    }
}