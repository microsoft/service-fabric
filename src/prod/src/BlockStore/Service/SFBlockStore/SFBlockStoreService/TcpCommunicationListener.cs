// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
using System;
using System.Fabric;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.ServiceFabric.Services.Communication.Runtime;
using System.Net;
using System.Net.Sockets;
using System.Fabric.Description;
using System.IO;
using Microsoft.ServiceFabric.Services.Remoting.Client;
using Microsoft.ServiceFabric.Services.Client;
using System.Collections.Generic;
using System.Text;
using System.IO.Pipes;

namespace SFBlockstoreService
{
    public class SFBlockStoreCommunicationListener : ICommunicationListener
    {
        private StatefulServiceContext context;
        private int listeningPort;
        private List<TcpListener> tcpListeners;
        private SFBlockstoreService serviceBlockStore;

        public SFBlockStoreCommunicationListener(StatefulServiceContext context, SFBlockstoreService service)
        {
            this.context = context;
            this.serviceBlockStore = service;
        }

        public void Abort()
        {
            Console.WriteLine("SFBlockStoreService - shutdown due to Abort");
            ShutdownListeners();
        }

        public Task CloseAsync(CancellationToken cancellationToken)
        {
            Console.WriteLine("SFBlockStoreService - shutdown due to CloseAsync");
            cancellationToken.ThrowIfCancellationRequested();
            ShutdownListeners();

            return Task.FromResult(true);
        }

        private void ShutdownListeners()
        {
            if (tcpListeners != null)
            {
                foreach (TcpListener tcpListener in tcpListeners)
                {
                    Console.WriteLine("ShutdownListeners: Shutting down listener for {0}", tcpListener.LocalEndpoint.ToString());
                    tcpListener.Stop();
                }
            }
        }

        public Task<string> OpenAsync(CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();

            // Init NamedPipe Listener
            ThreadPool.QueueUserWorkItem((state) => this.InitAndProcessNPListener(cancellationToken));

            // Init the TCPListener and publish it
            string publishUri = InitTCPListener(cancellationToken);
            return Task.FromResult(publishUri);
        }

        private void InitAndProcessNPListener(CancellationToken cancellationToken)
        {
            // Create the security ACL to allow EVERYONE group to access this pipe for R/W/Pipe-Instance creation
            System.Security.Principal.SecurityIdentifier sid = new System.Security.Principal.SecurityIdentifier(System.Security.Principal.WellKnownSidType.WorldSid, null);
            PipeSecurity securityAcl = new PipeSecurity();
            securityAcl.AddAccessRule(new PipeAccessRule(sid, PipeAccessRights.ReadWrite | PipeAccessRights.Synchronize | PipeAccessRights.CreateNewInstance, System.Security.AccessControl.AccessControlType.Allow));

            // Loop to start a named pipe listener. When a connection is accepted, queue to the ThreadPool and initiate the next listener.
            while (!cancellationToken.IsCancellationRequested)
            {
                NamedPipeServerStream pipeServer = new NamedPipeServerStream("SFBlockStorePipe", PipeDirection.InOut, NamedPipeServerStream.MaxAllowedServerInstances, PipeTransmissionMode.Byte, PipeOptions.WriteThrough, GatewayPayloadConst.payloadHeaderSize, GatewayPayloadConst.payloadHeaderSize, securityAcl);
                pipeServer.WaitForConnection();
                ThreadPool.QueueUserWorkItem((state) => this.ProcessNPClient(pipeServer, cancellationToken));
            }
        }

        private void ProcessNPClient(NamedPipeServerStream pipeServer, CancellationToken cancellationToken)
        {
            if (!cancellationToken.IsCancellationRequested)
            {
                byte[] arrHeader = null;

                try
                {
                    while (pipeServer.IsConnected && (!cancellationToken.IsCancellationRequested))
                    {
                        // Fetch the header from the stream
                        arrHeader = new byte[GatewayPayloadConst.payloadHeaderSize];
                        int iTotalRead = FetchDataFromTransport(null, pipeServer, arrHeader, GatewayPayloadConst.payloadHeaderSize);
                        if (iTotalRead > 0)
                        {
                            // Did we get a buffer of expected size?
                            if (iTotalRead != GatewayPayloadConst.payloadHeaderSize)
                            {
                                throw new InvalidOperationException(String.Format("SFBlockstoreService::ProcessNPClient: Malformed request of size {0} encountered; expected size is {1}", iTotalRead, GatewayPayloadConst.payloadHeaderSize));
                            }

                            GatewayPayload payload = GatewayPayload.ParseHeader(arrHeader, pipeServer, this);

                            // Process the payload
                            try
                            {
                                payload.ProcessPayload(serviceBlockStore, cancellationToken);
                            }
                            catch (Exception ex)
                            {
                                HandleProcessPayloadException(ref payload, ex);
                            }

                            // Send the Data back to the sender
                            payload.SendResponse();
                        }
                    }
                }
                catch (IOException)
                {
                    // Network connection was closed
                }
                catch (ObjectDisposedException)
                {
                    // Network connection was closed
                }
                catch (Exception ex)
                {
                    Console.WriteLine("SFBlockstoreService::ProcessNPClient: Got an exception [{0} - {1}] while handling incoming request.", ex.GetType().ToString(), ex.Message);
                }

                // If we are here, we are not in a position to process the network requests and thus, should close
                // the connection.
                if (pipeServer != null)
                {
                    pipeServer.Close();
                }
            }
        }

        private static GatewayPayload HandleProcessPayloadException(ref GatewayPayload payload, Exception ex)
        {
            Console.WriteLine("SFBlockstoreService::HandleProcessPayloadException: Got an exception [{0} - {1}] while processing payload.", ex.GetType(), ex.Message);

            // Build the payload information to be dumped.
            StringBuilder sbPayloadInfo = new StringBuilder();
            sbPayloadInfo.AppendLine(String.Format("Mode: {0:X}", payload.Mode));
            sbPayloadInfo.AppendLine(String.Format("Offset: {0:X}", payload.OffsetToRW));
            sbPayloadInfo.AppendLine(String.Format("Length to R/W: {0:X}", payload.SizePayloadBuffer));
            sbPayloadInfo.AppendLine(String.Format("DeviceID Length: {0:X}", payload.LengthDeviceID));
            sbPayloadInfo.AppendLine(String.Format("DeviceID: {0}", payload.DeviceID));
            sbPayloadInfo.AppendLine(String.Format("EndOfPayloadMarker: {0:X}", payload.EndOfPayloadMarker));

            Console.WriteLine(sbPayloadInfo.ToString());

            // TODO: Can we do this better?
            // Payload processing failed - Reflect in the payload status
            uint expectedResponsePayloadSize = (uint)GatewayPayloadConst.blockSizeManagementRequest;
            if (payload.Mode == BlockMode.Read)
            {
                if (payload.UseDMA)
                {
                    // We do not have any payload to send for the DMA scenario.
                    expectedResponsePayloadSize = 0;
                }
                else
                {
                    // Read request's receiver is expecting the intended size
                    expectedResponsePayloadSize = payload.SizePayloadBuffer;
                }
            }
            else if (payload.Mode == BlockMode.Write)
            {
                // Write request's receiver is expected only the payload header.
                expectedResponsePayloadSize = 0;
            }
            else
            {
                // All other requests (e.g. LU registration) expected the default (BlockSizeManagementRequest sized) payload size.
            }

            payload.SizePayloadBuffer = expectedResponsePayloadSize;
            payload.PayloadData = null;

            if (payload.EndOfPayloadMarker != GatewayPayloadConst.EndOfPayloadMarker)
            {
                // We got a payload that was invalid, so reflect that 
                // in the mode we will send back
                payload.Mode = BlockMode.ServerInvalidPayload;
                Console.WriteLine("SFBlockstoreService::HandleProcessPayloadException: Client sent an invalid payload.");
            }
            else
            {
                payload.Mode = BlockMode.OperationFailed;
            }

            return payload;
        }

        private string InitTCPListener(CancellationToken cancellationToken)
        {
            // Get the Port number from the service endpoint
            EndpointResourceDescription endpoint = context.CodePackageActivationContext.GetEndpoint("ServiceEndpoint");
            listeningPort = endpoint.Port;

            string uriPrefix = $"{endpoint.Protocol}://+:{endpoint.Port}";
            string nodeIPOrName = FabricRuntime.GetNodeContext().IPAddressOrFQDN;

            // Create the list of hostnames to bind to
            List<string> listHostnamesToBindTo = new List<string>();
            listHostnamesToBindTo.Add(nodeIPOrName);
            listHostnamesToBindTo.Add("localhost");

            // We will add the hostname to the list of names the service will listen on. 
            //
            // 1) When deployed to the cluster, the kernel driver will connect to the hostname
            //    to interact with the BSGateway (stateless) service. Aside from colocated connection,
            //    we need to do this since attempt to connect to the FQDN of the cluster results in a bugcheck
            //    way down in NT's Event-trace infrastructure.
            //
            //    REVIEW: We should determine a better way (than binding hostname's IP addresses to service port)
            //            to determine the address service uses on each node.
            //
            // 2) When developing the driver, it will allow connection to FQDN of the dev machine on which the service is running
            //    since the driver is usually developed in a VM (different from where service is deployed).
            listHostnamesToBindTo.Add(Dns.GetHostName());

            // Create the list to hold the tcpListener we create for IPV4 address bindings
            tcpListeners = new List<TcpListener>();

            foreach (string hostname in listHostnamesToBindTo)
            {
                // Create a new Tcp listner listening on the specified port
                IPHostEntry hostEntry = Dns.GetHostEntry(hostname);

                Console.WriteLine("SFBlockstoreService: Processing hostname {0} (resolved to {1})", hostname, hostEntry.HostName);

                foreach (IPAddress address in hostEntry.AddressList)
                {
                    // REVIEW: Support IPV6 as well. Bind to every IPV4 address
                    // corresponding to the hostname.
                    if (address.AddressFamily == AddressFamily.InterNetwork)
                    {
                        try
                        {
                            TcpListener tcpListener = new TcpListener(address, listeningPort);
                            tcpListener.Start();

                            tcpListeners.Add(tcpListener);

                            Console.WriteLine("SFBlockstoreService: Service is listening at {0}", address.ToString());

                            // Queue off a workitem in Threadpool that will process incoming requests
                            ThreadPool.QueueUserWorkItem((state) =>
                            {
                                this.ProcessTcpRequests(cancellationToken, tcpListener);
                            });
                        }
                        catch (Exception ex)
                        {
                            Console.WriteLine("SFBlockstoreService: Service was unable to listen on host {0}, address {1}, due to exception: {2} - {3}\n", hostname, address.ToString(), ex.GetType().ToString(), ex.Message);
                        }
                    }
                }
            }

            // REVIEW: Should we publish the URI of all the addresses we are listening on?
            // Return the Uri to be published by SF's naming service
            string publishUri = uriPrefix.Replace("+", FabricRuntime.GetNodeContext().IPAddressOrFQDN);
            return publishUri;
        }

        private void ProcessTcpRequests(CancellationToken cancellationToken, TcpListener tcpListener)
        {
            while(!cancellationToken.IsCancellationRequested)
            {
                TcpClient tcpClient = null;

                try
                {
                    tcpClient = tcpListener.AcceptTcpClient();
                }
                catch(Exception ex)
                {
                    // Catch any exception to ensure that service keeps on listening for incoming requests.
                    Console.WriteLine("ProcessTcpRequests: Caught an exception of the type {0}, with message {1}, while accepting incoming connections.", ex.GetType().ToString(), ex.Message);
                }

                if ((tcpClient != null) && (!cancellationToken.IsCancellationRequested))
                {
                    // Queue off a workitem in Threadpool that will process the accepted request so that
                    // we continue to accept other incoming connections.
                    ThreadPool.QueueUserWorkItem((state) =>
                    {
                        this.ProcessTcpClient(cancellationToken, tcpClient);
                    });
                }
            }
        }

        private static int FetchDataFromNetworkStream(NetworkStream ns, byte[] arrReadDest, int MaxLengthToRead)
        {
            int iOffsetToReadInto = 0;
            int iBufferAvailableLength = MaxLengthToRead;
            int iTotalRead = 0;
            int readBytes = 0;

            readBytes = ns.Read(arrReadDest, iOffsetToReadInto, iBufferAvailableLength);
            while (readBytes > 0)
            {
                // Increment the total bytes read and move to the next offset to read from.
                iTotalRead += readBytes;
                iOffsetToReadInto += readBytes;

                // Adjust available bytes in the buffer and read if there is some capacity left.
                iBufferAvailableLength -= readBytes;
                if (iBufferAvailableLength > 0)
                {
                   readBytes = ns.Read(arrReadDest, iOffsetToReadInto, iBufferAvailableLength);
                }
                else
                {
                    break;
                }
            }

            return iTotalRead;
        }

        private static int FetchDataFromNamedPipe(NamedPipeServerStream pipeServer, byte[] arrReadDest, int MaxLengthToRead)
        {
            int iOffsetToReadInto = 0;
            int iRemainingLengthToRead = MaxLengthToRead;
            int iTotalRead = 0;
            int iCurrentLengthToRead = iRemainingLengthToRead % GatewayPayloadConst.maxNamedPipePayloadSize;
            if (iCurrentLengthToRead == 0)
            {
                iCurrentLengthToRead = GatewayPayloadConst.maxNamedPipePayloadSize;
            }

            while (iRemainingLengthToRead > 0)
            {
                int readBytes = pipeServer.Read(arrReadDest, iOffsetToReadInto, iCurrentLengthToRead);
                if (readBytes == 0)
                {
                    // Break out of the loop since end of stream is reached.
                    break;
                }

                // Increment the total bytes read and move to the next offset to read from.
                iTotalRead += readBytes;
                iOffsetToReadInto += readBytes;
                iRemainingLengthToRead -= readBytes;
                iCurrentLengthToRead = GatewayPayloadConst.maxNamedPipePayloadSize;
            }

            return iTotalRead;
        }

        public static int FetchDataFromTransport(NetworkStream ns, NamedPipeServerStream pipeServer, byte[] arrReadDest, int MaxLengthToRead)
        {
            int iTotalRead = 0;

            if (ns != null)
            {
                iTotalRead = FetchDataFromNetworkStream(ns, arrReadDest, MaxLengthToRead);
            }
            else
            {
                iTotalRead = FetchDataFromNamedPipe(pipeServer, arrReadDest, MaxLengthToRead);
            }

            return iTotalRead;
        }

        private void ProcessTcpClient(CancellationToken cancellationToken, TcpClient tcpClient)
        {
            if (!cancellationToken.IsCancellationRequested)
            {
                NetworkStream ns = null;
                byte[] arrHeader = null;
                
                try
                {
                    try
                    {
                        ns = tcpClient.GetStream();
                    }
                    catch (InvalidOperationException)
                    {
                        Console.WriteLine("SFBlockstoreService::ProcessTCPClient: Unable to get NetworkStream to read data!");
                        tcpClient.Close();
                        ns = null;
                    }

                    while ((ns != null) && (tcpClient.Connected) && (!cancellationToken.IsCancellationRequested))
                    {
                        // Fetch the header from the stream
                        arrHeader = new byte[GatewayPayloadConst.payloadHeaderSize];
                        int iTotalRead = FetchDataFromTransport(ns, null, arrHeader, GatewayPayloadConst.payloadHeaderSize);
                        if (iTotalRead > 0)
                        {
                            // Did we get a buffer of expected size?
                            if (iTotalRead != GatewayPayloadConst.payloadHeaderSize)
                            {
                                throw new InvalidOperationException(String.Format("SFBlockstoreService::ProcessTCPClient: Malformed request of size {0} encountered; expected size is {1}", iTotalRead, GatewayPayloadConst.payloadHeaderSize));
                            }

                            GatewayPayload payload = GatewayPayload.ParseHeader(arrHeader, ns, this);

                            // Process the payload
                            try
                            {
                                payload.ProcessPayload(serviceBlockStore, cancellationToken);
                            }
                            catch (Exception ex)
                            {
                                HandleProcessPayloadException(ref payload, ex);
                            }

                            // Send the Data back to the sender
                            payload.SendResponse();
                        }
                        else
                        {
                            //Remote socket is terminated.
                            break;
                        }
                    }
                }
                catch(IOException)
                {
                    // Network connection was closed
                }
                catch(ObjectDisposedException)
                {
                    // Network connection was closed
                }
                catch (Exception ex)
                {
                    Console.WriteLine("SFBlockstoreService::ProcessTCPClient: Got an exception [{0} - {1}] while handling incoming request.", ex.GetType().ToString(), ex.Message);
                }

                // If we are here, we are not in a position to process the network requests and thus, should close
                // the connection.
                if (tcpClient != null)
                {
                    tcpClient.Close();
                }
            }
        }
    }
}
