//----------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//----------------------------------------------------------------

#if DotNetCoreClrLinux
namespace Microsoft.ServiceFabric.ContainerServiceClient
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Net.Sockets;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    class UnixDomainSocketHttpMessageHandler : ContainerServiceHttpMessageHandler
    {
        private const string TraceType = "UnixDomainSocketHttpMessageHandler";
        private const int ClientConnectionTimeoutMilliSeconds = 30000;

        private readonly UnixDomainSocketEndPoint socketEndpoint;
        private readonly ContainerServiceClient client;


        internal UnixDomainSocketHttpMessageHandler(ContainerServiceClient client, string pipeString)
        {
            this.client = client;
            this.socketEndpoint = new UnixDomainSocketEndPoint(pipeString);
        }

        internal override async Task<BufferedClientStream> AcquireClientStreamAsync(CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();

            Socket socket = null;
            NetworkStream networkStream = null;
            BufferedClientStream clientStream = null;

            try
            {
                socket = new Socket(AddressFamily.Unix, SocketType.Stream, ProtocolType.Unspecified);
                await socket.ConnectAsync(this.socketEndpoint);

                networkStream = new NetworkStream(socket, true);

                clientStream = new BufferedClientStream(networkStream);
            }
            catch (Exception ex)
            {
                if (networkStream != null)
                {
                    networkStream.Dispose();
                }
                else if(socket != null)
                {
                    socket.Dispose();
                }

                var errMsg = this.GetConnectionFailedErrorMessage(ex);
                AppTrace.TraceSource.WriteWarning(TraceType, errMsg);
                throw new FabricException(errMsg, FabricErrorCode.InvalidOperation);
            }

            return clientStream;
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
                "Failed to connect to DockerService at socket '{0}'. IsDockerServiceManagedBySF={1}",
                this.socketEndpoint.ToString(),
                this.client.IsContainerServiceManaged);

            if (this.client.IsContainerServiceManaged == false)
            {
                sb.Append("Please check if Docker service is up and running on the machine.");
            }

            sb.Append($"Error={ex}");

            return sb.ToString();
        }
    }
}
#endif
