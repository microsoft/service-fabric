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

namespace BSGatewayService
{
    internal class TcpCommunicationListener : ICommunicationListener
    {
        private StatelessServiceContext context;
        private int listeningPort;
        private List<TcpListener> tcpListeners;
        private IBSManager bsManager;
        
        public TcpCommunicationListener(StatelessServiceContext context)
        {
            this.context = context;
        }

        public void Abort()
        {
            if (tcpListeners != null)
            {
                foreach(TcpListener tcpListener in tcpListeners)
                    tcpListener.Stop();
            }
        }

        public Task CloseAsync(CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();

            if (tcpListeners != null)
            {
                foreach (TcpListener tcpListener in tcpListeners)
                    tcpListener.Stop();
            }

            return Task.FromResult(true);
        }

        public Task<string> OpenAsync(CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();

            // Connect to the BSManager Service and target just a single partition for now.
            bsManager = ServiceProxy.Create<IBSManager>(new Uri("fabric:/BSGateway/BSManagerService"), new ServicePartitionKey(1));
           
            // Get the Port number from the service endpoint
            EndpointResourceDescription endpoint = context.CodePackageActivationContext.GetEndpoint("ServiceEndpoint");
            listeningPort = endpoint.Port;

            string uriPrefix = $"{endpoint.Protocol}://+:{endpoint.Port}";
            string nodeIPOrName = FabricRuntime.GetNodeContext().IPAddressOrFQDN;

            // Create the list of hostnames to bind to
            List<string> listHostnamesToBindTo = new List<string>();
            listHostnamesToBindTo.Add(nodeIPOrName);

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
            // 2) When developing the driver, it will connect to FQDN of the dev machine on which the service is running
            //    since the driver is usually developed in a VM (different from where service is deployed).
           listHostnamesToBindTo.Add(Dns.GetHostName());
           
            // Create the list to hold the tcpListener we create for IPV4 address bindings
            tcpListeners = new List<TcpListener>();

            foreach (string hostname in listHostnamesToBindTo)
            {
                // Create a new Tcp listner listening on the specified port
                IPHostEntry hostEntry = Dns.GetHostEntry(hostname);

                Console.WriteLine("BSGatewayService: Processing hostname {0} (resolved to {1})", hostname, hostEntry.HostName);

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

                            Console.WriteLine("BSGateway: Service is listening at {0}", address.ToString());

                            // Queue off a workitem in Threadpool that will process incoming requests
                            ThreadPool.QueueUserWorkItem((state) =>
                            {
                                this.ProcessTcpRequests(cancellationToken, tcpListener);
                            });
                        }
                        catch(Exception)
                        {
                            // REVIEW: Ignore any exceptions while starting a listening session.
                        }
                    }
                }
            }

            // REVIEW: Should we publish the URI of all the addresses we are listening on?
            // Return the Uri to be published by SF's naming service
            string publishUri = uriPrefix.Replace("+", FabricRuntime.GetNodeContext().IPAddressOrFQDN);
            return Task.FromResult(publishUri);
        }

        private void ProcessTcpRequests(CancellationToken cancellationToken, TcpListener tcpListener)
        {
            while(!cancellationToken.IsCancellationRequested)
            {
                TcpClient tcpClient = tcpListener.AcceptTcpClient();
                if (!cancellationToken.IsCancellationRequested)
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

        private void ProcessTcpClient(CancellationToken cancellationToken, TcpClient tcpClient)
        {
            if (!cancellationToken.IsCancellationRequested)
            {
                NetworkStream ns = null;
                byte[] arrData = null;
                int readBytes = 0;

                try
                {
                    try
                    {
                        ns = tcpClient.GetStream();
                    }
                    catch (InvalidOperationException)
                    {
                        Console.WriteLine("ProcessTCPClient: Unable to get NetworkStream to read data!");
                        tcpClient.Close();
                        ns = null;
                    }

                    while ((ns != null) && (tcpClient.Connected) && (!cancellationToken.IsCancellationRequested))
                    {
                        arrData = new byte[GatewayPayloadConst.payloadBufferSizeMax];

                        int iOffsetToReadInto = 0;
                        int iBufferAvailableLength = GatewayPayloadConst.payloadBufferSizeMax;
                        int iTotalRead = 0;

                        readBytes = ns.Read(arrData, iOffsetToReadInto, iBufferAvailableLength);
                        while (readBytes > 0)
                        {
                            // Increment the total bytes read
                            iTotalRead += readBytes;

                            iOffsetToReadInto += readBytes;
                            iBufferAvailableLength -= readBytes;
                            if (iBufferAvailableLength > 0)
                            {
                                readBytes = ns.Read(arrData, iOffsetToReadInto, iBufferAvailableLength);
                            }
                            else
                            {
                                Console.WriteLine("ProcessTCPClient: Read buffer full.");
                                break;
                            }
                        }

                        if (iTotalRead > 0)
                        {
                            Console.WriteLine("ProcessTCPClient: Read {0} bytes", iTotalRead);

                            GatewayPayload payload = GatewayPayload.From(arrData);

                            // Process the payload
                            try
                            {
                                payload.ProcessPayloadBlock(bsManager);
                            }
                            catch(Exception ex)
                            {
                                Console.WriteLine("ProcessTCPClient: Got an exception [{0} - {1}] while processing payload.",ex.GetType().ToString(), ex.Message);

                                // Build the payload information to be dumped.
                                StringBuilder sbPayloadInfo = new StringBuilder();
                                sbPayloadInfo.AppendLine(String.Format("Mode: {0:X}", payload.Mode));
                                sbPayloadInfo.AppendLine(String.Format("Offset: {0:X}", payload.OffsetToRW));
                                sbPayloadInfo.AppendLine(String.Format("Length to R/W: {0:X}", payload.LengthRW));
                                sbPayloadInfo.AppendLine(String.Format("DeviceID Length: {0:X}", payload.LengthDeviceID));
                                sbPayloadInfo.AppendLine(String.Format("DeviceID: {0}", payload.DeviceID));
                                sbPayloadInfo.AppendLine(String.Format("EndOfPayloadMarker: {0:X}", payload.EndOfPayloadMarker));

                                Console.WriteLine(sbPayloadInfo.ToString());

                                // Payload processing failed - Reflect in the payload status
                                payload.LengthRW = 0;
                                if (payload.EndOfPayloadMarker != GatewayPayloadConst.EndOfPayloadMarker)
                                {
                                    // We got a payload that was invalid, so reflect that 
                                    // in the mode we will send back
                                    payload.Mode = BlockMode.ServerInvalidPayload;
                                    Console.WriteLine("ProcessTCPClient: Client sent an invalid payload.");
                                }
                                else
                                {
                                    payload.Mode = BlockMode.OperationFailed;
                                }
                            }

                            // Send the Data back to the sender
                            byte[] arrDataToSendBack = payload.ToArray();
                            ns.Write(arrDataToSendBack, 0, arrDataToSendBack.Length);
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
                    Console.WriteLine("ProcessTCPClient: Got an exception [{0} - {1}] while handling incoming request.", ex.GetType().ToString(), ex.Message);
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