// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Fabric;
using System.Fabric.Query;
using Newtonsoft.Json.Linq;
using ServiceFabric.Helpers;
using Newtonsoft.Json;
using System.Net.NetworkInformation;
using System.Net;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Runtime.InteropServices;
using Gateway.Models;
using sfintegration.envoymodels;

namespace webapi
{

    public class ListenerFilterConfig
    {
        public enum ListenerFilterType
        {
            Tcp,
            Http
        }

        public ListenerFilterConfig(string name, ListenerFilterType type, string clusterName)
        {
            Name = name;
            Type = type;
            ClusterName = clusterName;
        }

        public string Name { get; private set; }
        public ListenerFilterType Type { get; private set; }

        public string ClusterName { get; private set; }
    }

    public class ListenerHttpFilterConfig : ListenerFilterConfig
    {
        public ListenerHttpFilterConfig(string name, string clusterName, IList<HttpHostNameConfig> hosts)
            : base(name, ListenerFilterConfig.ListenerFilterType.Http, clusterName)
        {
            Hosts = hosts;
        }

        public IList<HttpHostNameConfig> Hosts { get; private set; }
    }

    public class EnvoyDefaults
    {
        private static int indentLevel = 0;
        private static string indentSpaces = "";
        public enum IndentOperation
        {
            NoChange,
            BeginLevel,
            EndLevel
        }
        public static void LogMessage(string message, IndentOperation indentOp = IndentOperation.NoChange)
        {
            if (indentOp == IndentOperation.EndLevel)
            {
                indentLevel--;
                if (indentLevel < 0) indentLevel = 0;
                indentSpaces = new string(' ', indentLevel * 2);
            }
            // Get the local time zone and the current local time and year.
            DateTime currentDate = DateTime.UtcNow;
            System.Console.WriteLine("[{0}][info][sfintegration] {2}{1}", currentDate.ToString("yyyy-MM-dd HH:mm:ss.fffZ"), message, indentSpaces);
            if (indentOp == IndentOperation.BeginLevel)
            {
                indentLevel++;
                indentSpaces = new string(' ', indentLevel * 2);
            }
        }
        static EnvoyDefaults()
        {
            var connectTimeout = Environment.GetEnvironmentVariable("HttpRequestConnectTimeoutMs");
            if (connectTimeout != null)
            {
                try
                {
                    connect_timeout_ms = Convert.ToInt32(connectTimeout);
                }
                catch { }
            }

            var requestTimeout = Environment.GetEnvironmentVariable("DefaultHttpRequestTimeoutMs");
            if (requestTimeout != null)
            {
                try
                {
                    timeout_ms = Convert.ToInt32(requestTimeout);
                }
                catch { }
            }

            var removeResponseHeaders = Environment.GetEnvironmentVariable("RemoveServiceResponseHeaders");
            if (removeResponseHeaders != null)
            {
                try
                {
                    response_headers_to_remove.AddRange(removeResponseHeaders.Replace(" ", "").Split(','));
                }
                catch { }
            }

            var useHttps = Environment.GetEnvironmentVariable("UseHttps");
            if (useHttps != null && useHttps == "true")
            {
                var verify_certificate_hash = Environment.GetEnvironmentVariable("ServiceCertificateHash");

                var subjectAlternateName = Environment.GetEnvironmentVariable("ServiceCertificateAlternateNames");
                if (subjectAlternateName != null)
                {
                    try
                    {
                        verify_subject_alt_name.AddRange(subjectAlternateName.Replace(" ", "").Split(','));
                    }
                    catch { }
                }
                cluster_ssl_context = new EnvoyClusterSslContext(verify_certificate_hash, verify_subject_alt_name);
            }

            host_ip = Environment.GetEnvironmentVariable("Fabric_NodeIPOrFQDN");
            if (host_ip == null || host_ip == "localhost")
            {
                bool runningInContainer = Environment.GetEnvironmentVariable("__STANDALONE_TESTING__") == null;
                if (runningInContainer)
                {
                    // running in a container outside of Service Fabric or 
                    // running in a container on local dev cluster
                    host_ip = GetInternalGatewayAddress();
                }
                else if (Environment.GetEnvironmentVariable("__USE_LOCALHOST__") == null)
                {
                    // running outside of Service Fabric or 
                    // running on local dev cluster
                    host_ip = GetIpAddress();
                }
                else
                {
                    host_ip = "127.0.0.1";
                }
            }
            var port = Environment.GetEnvironmentVariable("ManagementPort");
            if (port != null)
            {
                management_port = port;
            }

            client_cert_subject_name = Environment.GetEnvironmentVariable("SF_ClientCertCommonName");
            var issuer_thumbprints = Environment.GetEnvironmentVariable("SF_ClientCertIssuerThumbprints");
            if (!string.IsNullOrWhiteSpace(issuer_thumbprints))
            {
                client_cert_issuer_thumbprints = issuer_thumbprints.Split(',');
            }
            var server_common_names = Environment.GetEnvironmentVariable("SF_ClusterCertCommonNames");
            if (!string.IsNullOrWhiteSpace(server_common_names))
            {
                server_cert_common_names = server_common_names.Split(',');
            }
            var server_issuer_thumbprints = Environment.GetEnvironmentVariable("SF_ClusterCertIssuerThumbprints");
            if (!string.IsNullOrWhiteSpace(server_issuer_thumbprints))
            {
                server_cert_issuer_thumbprints = server_issuer_thumbprints.Split(',');
            }

            if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
            {
                fabricUri = new String[] { host_ip + ":" + management_port };
            }
            else
            {
                fabricUri = null;
            }

            LogMessage(String.Format("Management Endpoint={0}:{1}", host_ip, management_port));
            if (fabricUri != null)
            {
                LogMessage(String.Format("Fabric Uri ={0}", fabricUri));
            }
            else
            {
                LogMessage(String.Format("Fabric Uri =[]"));
            }

            if (client_cert_subject_name != null)
            {
                LogMessage(String.Format("SF_ClientCertCommonName={0}", client_cert_subject_name));
            }
            if (issuer_thumbprints != null)
            {
                LogMessage(String.Format("SF_ClientCertIssuerThumbprints={0}", issuer_thumbprints));
            }
            if (server_cert_common_names != null)
            {
                LogMessage(String.Format("SF_ClusterCertCommonNames={0}", server_cert_common_names));
            }
            if (server_issuer_thumbprints != null)
            {
                LogMessage(String.Format("  ={0}", server_issuer_thumbprints));
            }

            var gateway_listen_network = "";
            LogMessage("Getting Gateway_Config");
            string gateway_config = Environment.GetEnvironmentVariable("Gateway_Config");
            if (gateway_config != null && gateway_config != "")
            {
                LogMessage(String.Format("Gateway_Config={0}", gateway_config));
                var gatewayProperties = JsonConvert.DeserializeObject<GatewayResourceDescription>(gateway_config);

                gateway_listen_network = gatewayProperties.SourceNetwork.Name;

                gateway_map = new Dictionary<string, List<ListenerFilterConfig>>();
                gateway_clusternames = new HashSet<string>();
                if (gatewayProperties.Tcp != null)
                {
                    foreach (var e in gatewayProperties.Tcp)
                    {
                        string serviceName = e.Destination.EnvoyServiceName();
                        if (!gateway_map.ContainsKey(e.Port.ToString()) ||
                            gateway_map[e.Port.ToString()] == null)
                        {
                            gateway_map[e.Port.ToString()] = new List<ListenerFilterConfig>();
                        }
                        gateway_map[e.Port.ToString()].Add(
                            new ListenerFilterConfig(
                                e.Name,
                                ListenerFilterConfig.ListenerFilterType.Tcp,
                                serviceName));
                        gateway_clusternames.Add(serviceName);
                    }
                }
                if (gatewayProperties.Http != null)
                {
                    foreach (var httpConfig in gatewayProperties.Http)
                    {
                        string configName = "gateway_config|" + httpConfig.Port;
                        if (!gateway_map.ContainsKey(httpConfig.Port.ToString()) ||
                            gateway_map[httpConfig.Port.ToString()] == null)
                        {
                            gateway_map[httpConfig.Port.ToString()] = new List<ListenerFilterConfig>();
                        }
                        gateway_map[httpConfig.Port.ToString()].Add(
                            new ListenerHttpFilterConfig(
                                httpConfig.Name,
                                configName,
                                httpConfig.Hosts));
                        foreach (var host in httpConfig.Hosts)
                        {
                            foreach (var e in host.Routes)
                            {
                                var serviceName = e.Destination.EnvoyServiceName();
                                gateway_clusternames.Add(serviceName);
                            }
                        }
                    }
                }
            }
            else
                LogMessage("Gateway_Config=null");

            if (gateway_listen_network != null)
            {
                LogMessage(String.Format("Gateway_Listen_Network={0}", gateway_listen_network));
                gateway_listen_network = "[" + gateway_listen_network + "]";
                var env = Environment.GetEnvironmentVariables();
                var keys = env.Keys;
                foreach (System.Collections.DictionaryEntry de in env)
                {
                    var variable = de.Key.ToString();
                    if (variable.Contains(gateway_listen_network) && variable.StartsWith("Fabric_NET"))
                    {
                        gateway_listen_ip = de.Value.ToString();
                        break;
                    }
                }
            }
            else
                LogMessage("Gateway_Listen_Network=null");

            LogMessage("gateway_listen_ip=" + gateway_listen_ip);
        }
        private static string GetInternalGatewayAddress()
        {
            foreach (NetworkInterface networkInterface in NetworkInterface.GetAllNetworkInterfaces())
            {
                if (networkInterface.GetIPProperties().GatewayAddresses != null &&
                    networkInterface.GetIPProperties().GatewayAddresses.Count > 0)
                {
                    foreach (GatewayIPAddressInformation gatewayAddr in networkInterface.GetIPProperties().GatewayAddresses)
                    {
                        if (gatewayAddr.Address.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork)
                        {
                            return gatewayAddr.Address.ToString();
                        }
                    }
                }
            }
            throw new ArgumentNullException("internalgatewayaddress");
        }

        private static string GetIpAddress()
        {
            var hostname = Dns.GetHostName();
            IPHostEntry hostEntry = Dns.GetHostEntry(Dns.GetHostName());
            IPAddress ipAddress = hostEntry.AddressList.FirstOrDefault(
                ip =>
                    (ip.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork));
            if (ipAddress != null)
            {
                return ipAddress.ToString();
            }
            throw new InvalidOperationException("HostIpAddress");
        }

        public static string LocalHostFixup(string host)
        {
            if (host == null || host == "localhost")
            {
                return host_ip;
            }
            return host;
        }

        public static int connect_timeout_ms = 5000;

        public static int timeout_ms = 120000;

        public static List<string> response_headers_to_remove = new List<string>();

        public static string verify_certificate_hash;

        public static List<string> verify_subject_alt_name = new List<string>();

        public static EnvoyClusterSslContext cluster_ssl_context;

        public static string host_ip;

        public static string management_port = "19000";

        public static string[] fabricUri;

        public static string client_cert_subject_name;

        public static string[] client_cert_issuer_thumbprints;

        public static string[] server_cert_common_names;

        public static string[] server_cert_issuer_thumbprints;

        public static bool is_gateway_config = false;

        public static string gateway_listen_ip = "0.0.0.0";

        public static Dictionary<string, List<ListenerFilterConfig>> gateway_map;
        public static HashSet<string> gateway_clusternames;
    }
    public class EnvoyClustersInformation
    {
        public EnvoyClustersInformation(string name, List<EnvoyRoute> routes, List<EnvoyHost> hosts, bool isHttps = false)
        {
            cluster = new EnvoyCluster(name, EnvoyDefaults.connect_timeout_ms, isHttps, EnvoyDefaults.cluster_ssl_context);
            this.routes = routes;
            this.hosts = hosts;
        }

        [JsonProperty]
        public EnvoyCluster cluster;

        [JsonProperty]
        public List<EnvoyRoute> routes;

        [JsonProperty]
        public List<EnvoyHost> hosts;
    }

    public class SF_EndpointInstance
    {
        public SF_EndpointInstance(ServiceEndpointRole role, Uri uri)
        {
            role_ = role;
            endpoint_ = uri;
        }

        [JsonProperty(PropertyName = "role")]
        public ServiceEndpointRole role_;

        [JsonProperty(PropertyName = "endpoint")]
        public Uri endpoint_;
    }

    public class SF_Endpoint
    {
        public SF_Endpoint(string name)
        {
            this.Name = name;
        }

        public void AddInstance(ServiceEndpointRole role, Uri uri)
        {
            if (instances == null)
            {
                instances = new List<SF_EndpointInstance>();
            }
            instances.Add(new SF_EndpointInstance(role, uri));
            instances.Sort((x, y) =>
            {
                if (x.role_ - y.role_ != 0)
                    return x.role_ - y.role_;
                return string.CompareOrdinal(x.endpoint_.AbsolutePath, y.endpoint_.AbsolutePath);
            });
        }

        public int InstanceCount()
        {
            if (instances == null)
            {
                return 0;
            }
            return instances.Count;
        }

        public SF_EndpointInstance GetAt(int index)
        {
            return instances[index];
        }

        [JsonProperty]
        public string Name;

        [JsonProperty]
        private List<SF_EndpointInstance> instances;
    }
    public class SF_Partition
    {
        public SF_Partition(Uri serviceName, ServiceKind serviceKind, ServicePartitionInformation partitionInformation, ServiceEndpointsVersion version, List<SF_Endpoint> listeners)
        {
            serviceName_ = serviceName;
            serviceKind_ = serviceKind;
            partitionInformation_ = partitionInformation;
            version_ = version;
            listeners_ = listeners;
        }

        [JsonProperty(PropertyName = "service_name")]
        public Uri serviceName_;

        [JsonProperty(PropertyName = "service_kind")]
        public ServiceKind serviceKind_;

        [JsonProperty(PropertyName = "partition_information")]
        public ServicePartitionInformation partitionInformation_;

        [JsonProperty(PropertyName = "version")]
        public ServiceEndpointsVersion version_;

        [JsonProperty(PropertyName = "listeners")]
        public List<SF_Endpoint> listeners_;
    }

    public class ServicePartitions
    {
        [JsonProperty]
        public int EndpointIndex;

        [JsonProperty]
        public bool StatefulService;

        [JsonProperty]
        public List<Guid> Partitions;
    }

    public class SF_Services
    {
        public static Dictionary<Guid, SF_Partition> partitions_;

        public static Dictionary<string, ServicePartitions> services_;

        // Temporary lists to handle initialization race
        static Dictionary<Guid, SF_Partition> partitionsAdd_ = new Dictionary<Guid, SF_Partition>();

        static Dictionary<Guid, SF_Partition> partitionsRemove_ = new Dictionary<Guid, SF_Partition>();

        static object lock_ = new object();

        static private FabricClient client;
        static SF_Services()
        {
            try
            {
                partitions_ = null;

                if (!string.IsNullOrWhiteSpace(EnvoyDefaults.client_cert_subject_name) || 
                    EnvoyDefaults.client_cert_issuer_thumbprints != null)
                {
                    EnvoyDefaults.LogMessage("Client: Creating, secure");

                    X509Credentials creds = new X509Credentials();
                    if (!string.IsNullOrWhiteSpace(EnvoyDefaults.client_cert_subject_name))
                    {
                        creds.FindType = X509FindType.FindBySubjectName;
                        creds.FindValue = EnvoyDefaults.client_cert_subject_name;
                        if (EnvoyDefaults.client_cert_issuer_thumbprints != null)
                        {
                            foreach (var issuer in EnvoyDefaults.client_cert_issuer_thumbprints)
                            {
                                creds.IssuerThumbprints.Add(issuer);
                            }
                        }
                    }
                    else
                    {
                        creds.FindType = X509FindType.FindByThumbprint;
                        creds.FindValue = EnvoyDefaults.client_cert_issuer_thumbprints[0];
                    }

                    if (EnvoyDefaults.server_cert_common_names != null)
                    {
                        foreach (var commonName in EnvoyDefaults.server_cert_common_names)
                        {
                            creds.RemoteCommonNames.Add(commonName);
                        }
                    }
                    else if (!string.IsNullOrWhiteSpace(EnvoyDefaults.client_cert_subject_name))
                    {
                        creds.RemoteCommonNames.Add(EnvoyDefaults.client_cert_subject_name);
                    }
                    if (EnvoyDefaults.server_cert_issuer_thumbprints != null)
                    {
                        foreach (var issuer in EnvoyDefaults.server_cert_issuer_thumbprints)
                        {
                            creds.RemoteCertThumbprints.Add(issuer);
                        }
                    }
                    else if (EnvoyDefaults.client_cert_issuer_thumbprints != null)
                    {
                        foreach (var issuer in EnvoyDefaults.client_cert_issuer_thumbprints)
                        {
                            creds.RemoteCertThumbprints.Add(issuer);
                        }
                    }
                    creds.StoreLocation = StoreLocation.LocalMachine;
                    creds.StoreName = "/var/lib/sfcerts";

                    client = new FabricClient(creds, EnvoyDefaults.fabricUri);
                }
                else
                {
                        EnvoyDefaults.LogMessage("Client: Creating, nonsecure");
                        client = new FabricClient(EnvoyDefaults.fabricUri);
                }
                EnvoyDefaults.LogMessage("Client sucessfully created");

                EnableResolveNotifications.RegisterNotificationFilter("fabric:", client, Handler);
                EnvoyDefaults.LogMessage("Notification handler sucessfully set");
            }
            catch (Exception e)
            {
                EnvoyDefaults.LogMessage(String.Format("Error={0}", e));
            }
        }

        private static string CalculateNameForService(SF_Partition partition, int listenerIndex)
        {
            StringBuilder serviceName = new StringBuilder(partition.serviceName_.PathAndQuery.Substring(1).Replace('/', '_'));
            if (partition.listeners_[listenerIndex].Name != "")
            {
                serviceName.Append("_");
                serviceName.Append(partition.listeners_[listenerIndex].Name);
            }
            serviceName.Append("|*|-2");

            return serviceName.ToString();
        }

        // Assumes partitions_ is not null and the data structures are already locked
        private static void RemovePartitionFromService(Guid partitionId)
        {
            EnvoyDefaults.LogMessage(String.Format("Removed: {0}", partitionId));
            foreach (var service in services_)
            {
                service.Value.Partitions.RemoveAll(x => x == partitionId);
            }
            foreach (var service in services_.Where(x => x.Value.Partitions.Count == 0).ToList())
            {
                services_.Remove(service.Key);
            }
        }

        private static void AddPartitionToService(Guid partitionId, SF_Partition partitionInfo)
        {
            EnvoyDefaults.LogMessage(String.Format("Added: {0}={1}", partitionId,
                JsonConvert.SerializeObject(partitionInfo)));
            for (int listenerIndex = 0; listenerIndex < partitionInfo.listeners_.Count; listenerIndex++)
            {
                string serviceName = CalculateNameForService(partitionInfo, listenerIndex);
                if (!services_.ContainsKey(serviceName))
                {
                    services_[serviceName] = new ServicePartitions
                    {
                        EndpointIndex = listenerIndex,
                        StatefulService = (partitionInfo.serviceKind_ == ServiceKind.Stateful),
                        Partitions = new List<Guid>()
                    };
                }
                var entry = services_[serviceName].Partitions.Find(x => x == partitionId);
                if (entry == Guid.Empty)
                {
                    services_[serviceName].Partitions.Add(partitionId);
                }
            }
        }

        private static void Handler(Object sender, EventArgs eargs)
        {
            EnvoyDefaults.LogMessage("Notification Handler: Start", EnvoyDefaults.IndentOperation.BeginLevel);
            try
            {
                var notification = ((FabricClient.ServiceManagementClient.ServiceNotificationEventArgs)eargs).Notification;
                if (notification.Endpoints.Count == 0)
                {
                    //remove
                    lock (lock_)
                    {
                        if (partitions_ != null)
                        {
                            partitions_.Remove(notification.PartitionId);
                            RemovePartitionFromService(notification.PartitionId);
                        }
                        else
                        {
                            partitionsAdd_.Remove(notification.PartitionId);
                            partitionsRemove_[notification.PartitionId] = null;
                        }
                        EnvoyDefaults.LogMessage("Notification Handler: End", EnvoyDefaults.IndentOperation.EndLevel);
                        return;
                    }
                }

                List<SF_Endpoint> listeners = new List<SF_Endpoint>();
                ServiceEndpointRole role = ServiceEndpointRole.Invalid;
                EnvoyDefaults.LogMessage(String.Format("{0}: Start Enumerating Replicas", notification.PartitionId),
                    EnvoyDefaults.IndentOperation.BeginLevel);
                EnvoyDefaults.LogMessage(String.Format("{0}: Replica Count = {1}", notification.PartitionId, 
                    notification.Endpoints.Count()));
                foreach (var notificationEndpoint in notification.Endpoints)
                {
                    if (notificationEndpoint.Address.Length == 0)
                    {
                        EnvoyDefaults.LogMessage("_Node = <Empty>");
                        continue;
                    }

                    EnvoyDefaults.LogMessage(String.Format("_Node = {0}", notificationEndpoint.Address));
                    JObject addresses;
                    try
                    {
                        addresses = JObject.Parse(notificationEndpoint.Address);
                    }
                    catch
                    {
                        continue;
                    }

                    var notificationListeners = addresses["Endpoints"].Value<JObject>();
                    foreach (var notificationListener in notificationListeners)
                    {
                        int listenerIndex = listeners.FindIndex(x => x.Name == notificationListener.Key);
                        if (listenerIndex == -1)
                        {
                            listeners.Add(new SF_Endpoint(notificationListener.Key));
                            listenerIndex = listeners.Count - 1;
                        }
                        try
                        {
                            var listenerAddressString = notificationListener.Value.ToString();
                            EnvoyDefaults.LogMessage(String.Format("AddressString = {0}", listenerAddressString));
                            var listenerAddress = new UriBuilder(listenerAddressString).Uri;
                            if (listenerAddress.HostNameType == UriHostNameType.Dns)
                            {
                                var ipaddrs = Dns.GetHostAddresses(listenerAddress.Host);
                                foreach (var ipaddr in ipaddrs)
                                {
                                    if (ipaddr.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork)
                                    {
                                        var saddrstring = ipaddr.ToString();
                                        if (saddrstring.StartsWith("172"))
                                        {
                                            listenerAddress = new Uri(listenerAddress.Scheme + "://" +
                                                saddrstring +
                                                ":" + listenerAddress.Port + listenerAddress.PathAndQuery);
                                            break;
                                        }
                                    }
                                }
                            }
                            listeners[listenerIndex].AddInstance(notificationEndpoint.Role, listenerAddress);
                        }
                        catch (System.Exception e)
                        {
                            EnvoyDefaults.LogMessage(String.Format("Error={0}", e));
                        }
                    }
                    if (role == ServiceEndpointRole.Invalid)
                    {
                        role = notificationEndpoint.Role;
                    }
                }
                EnvoyDefaults.LogMessage(String.Format("{0}: End Enumerating Replicas", notification.PartitionId),
                    EnvoyDefaults.IndentOperation.EndLevel);

                // Remove any listeners without active endpoints
                listeners.RemoveAll(x => x.InstanceCount() == 0);

                if (listeners.Count == 0)
                {
                    EnvoyDefaults.LogMessage("Notification Handler: End", EnvoyDefaults.IndentOperation.EndLevel);
                    return;
                }

                var partitionInfo = new SF_Partition(notification.ServiceName,
                    (role == ServiceEndpointRole.Stateless) ? ServiceKind.Stateless : ServiceKind.Stateful,
                    notification.PartitionInfo,
                    notification.Version,
                    listeners
                    );

                lock (lock_)
                {
                    if (partitions_ != null)
                    {
                        partitions_[notification.PartitionId] = partitionInfo;
                        AddPartitionToService(notification.PartitionId, partitionInfo);
                    }
                    else
                    {
                        partitionsRemove_.Remove(notification.PartitionId);
                        partitionsAdd_[notification.PartitionId] = partitionInfo;
                    }
                }
            }
            catch (Exception e)
            {
                EnvoyDefaults.LogMessage(String.Format("Error={0}", e.Message));
                EnvoyDefaults.LogMessage(String.Format("Error={0}", e.StackTrace));
            }
            EnvoyDefaults.LogMessage("Notification Handler: End", EnvoyDefaults.IndentOperation.EndLevel);
        }

        /// <summary>
        /// This function gathers the state of the cluster on startup and caches the information
        /// Changes to cluster state are handled through notifications.
        /// 
        /// Capture information for each replica for every service running in the cluster.
        /// </summary>
        /// <returns></returns>
        public static async Task InitializePartitionData()
        {
            EnvoyDefaults.LogMessage("InitializePartitionData started");

            // Populate data locally
            Dictionary<Guid, SF_Partition> partitionData = new Dictionary<Guid, SF_Partition>();

            var queryManager = client.QueryManager;

            ApplicationList applications = null;
            try
            {
                applications = await queryManager.GetApplicationListAsync();
            }
            catch (Exception e)
            {
                EnvoyDefaults.LogMessage("GetApplicationListAsync: Failed");
                EnvoyDefaults.LogMessage(String.Format("Error={0}", e.Message));
                EnvoyDefaults.LogMessage(String.Format("Error={0}", e.StackTrace));
                Environment.FailFast("GetApplicationListAsync failed");
            }
            EnvoyDefaults.LogMessage("GetApplicationListAsync: Succeeded");
            EnvoyDefaults.LogMessage(String.Format("GetApplicationListAsync: Application Count = {0}", applications.Count()));
            EnvoyDefaults.LogMessage("Start Enumerating Applications", EnvoyDefaults.IndentOperation.BeginLevel);
            foreach (var application in applications)
            {
                EnvoyDefaults.LogMessage(String.Format("{0}: Start Enumerating Services", application.ApplicationName), 
                    EnvoyDefaults.IndentOperation.BeginLevel);
                var services = await queryManager.GetServiceListAsync(application.ApplicationName);
                EnvoyDefaults.LogMessage(String.Format("{0}: Service Count = {1}", application.ApplicationName, services.Count()));
                foreach (var service in services)
                {
                    EnvoyDefaults.LogMessage(String.Format("{0}: Start Enumerating Partitions", service.ServiceName),
                        EnvoyDefaults.IndentOperation.BeginLevel);
                    var partitions = await queryManager.GetPartitionListAsync(service.ServiceName);
                    EnvoyDefaults.LogMessage(String.Format("{0}: Partition Count = {1}", service.ServiceName, partitions.Count()));
                    foreach (var partition in partitions)
                    {
                        List<SF_Endpoint> listeners = new List<SF_Endpoint>();

                        EnvoyDefaults.LogMessage(String.Format("{0}: Start Enumerating Replicas", partition.PartitionInformation.Id),
                            EnvoyDefaults.IndentOperation.BeginLevel);
                        var replicas = await queryManager.GetReplicaListAsync(partition.PartitionInformation.Id);
                        EnvoyDefaults.LogMessage(String.Format("{0}: Replica Count = {1}", partition.PartitionInformation.Id, replicas.Count()));
                        foreach (var replica in replicas)
                        {
                            if (replica.ReplicaAddress.Length == 0)
                            {
                                EnvoyDefaults.LogMessage(String.Format("{0} = <Empty>", replica.NodeName));
                                continue;
                            }

                            EnvoyDefaults.LogMessage(String.Format("{0} = {1}", replica.NodeName, replica.ReplicaAddress));
                            JObject addresses;
                            try
                            {
                                addresses = JObject.Parse(replica.ReplicaAddress);
                            }
                            catch
                            {
                                continue;
                            }

                            var replicaListeners = addresses["Endpoints"].Value<JObject>();
                            foreach (var replicaListener in replicaListeners)
                            {
                                var role = ServiceEndpointRole.Stateless;
                                if (partition.ServiceKind == ServiceKind.Stateful)
                                {
                                    var statefulRole = ((StatefulServiceReplica)replica).ReplicaRole;
                                    switch (statefulRole)
                                    {
                                        case ReplicaRole.Primary:
                                            role = ServiceEndpointRole.StatefulPrimary;
                                            break;
                                        case ReplicaRole.ActiveSecondary:
                                            role = ServiceEndpointRole.StatefulSecondary;
                                            break;
                                        default:
                                            role = ServiceEndpointRole.Invalid;
                                            break;
                                    }
                                }
                                int listenerIndex = listeners.FindIndex(x => x.Name == replicaListener.Key);
                                if (listenerIndex == -1)
                                {
                                    listeners.Add(new SF_Endpoint(replicaListener.Key));
                                    listenerIndex = listeners.Count - 1;
                                }
                                try
                                {
                                    var listenerAddressString = replicaListener.Value.ToString();
                                    EnvoyDefaults.LogMessage(String.Format("AddressString = {0}", listenerAddressString));
                                    var listenerAddress = new UriBuilder(listenerAddressString).Uri;
                                    if (listenerAddress.HostNameType == UriHostNameType.Dns)
                                    {
                                        var ipaddrs = Dns.GetHostAddresses(listenerAddress.Host);
                                        foreach (var ipaddr in ipaddrs)
                                        {
                                            if (ipaddr.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork)
                                            {
                                                var saddrstring = ipaddr.ToString();
                                                if (saddrstring.StartsWith("172"))
                                                {
                                                    listenerAddress = new Uri(listenerAddress.Scheme + "://" +
                                                        saddrstring +
                                                        ":" + listenerAddress.Port + listenerAddress.PathAndQuery);
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                    listeners[listenerIndex].AddInstance(role, listenerAddress);
                                }
                                catch (System.Exception e)
                                {
                                    EnvoyDefaults.LogMessage(String.Format("Error={0}", e));
                                }
                            }
                            EnvoyDefaults.LogMessage(String.Format("{0}: End Enumerating Replicas", partition.PartitionInformation.Id),
                                EnvoyDefaults.IndentOperation.EndLevel);
                        }

                        // Remove any listeners without active endpoints
                        listeners.RemoveAll(x => x.InstanceCount() == 0);

                        if (listeners.Count != 0)
                        {
                            var partitionInfo = new SF_Partition(service.ServiceName,
                                service.ServiceKind,
                                partition.PartitionInformation,
                                null,
                                listeners);

                            partitionData[partition.PartitionInformation.Id] = partitionInfo;
                        }
                        EnvoyDefaults.LogMessage(String.Format("{0}: End Enumerating Partitions", service.ServiceName),
                                EnvoyDefaults.IndentOperation.EndLevel);
                    }
                    EnvoyDefaults.LogMessage(String.Format("{0}: End Enumerating Services", application.ApplicationName),
                                EnvoyDefaults.IndentOperation.EndLevel);
                }
                EnvoyDefaults.LogMessage("End Enumerating Applications",
                                EnvoyDefaults.IndentOperation.EndLevel);
            }

            // Process changes received through notifications
            lock (lock_)
            {
                foreach (var partition in partitionsAdd_)
                {
                    partitionData[partition.Key] = partition.Value;
                }
                foreach (var partition in partitionsRemove_)
                {
                    partitionData.Remove(partition.Key);
                }

                // Finally update global state, populate Service List and log details
                partitions_ = partitionData;
                services_ = new Dictionary<string, ServicePartitions>();
                foreach (var partition in partitionData)
                {
                    AddPartitionToService(partition.Key, partition.Value);
                    EnvoyDefaults.LogMessage(String.Format("Added: {0}={1}", partition.Key,
                        JsonConvert.SerializeObject(partition.Value)));
                }

                partitionsRemove_ = null;
                partitionsAdd_ = null;
            }
        }

        public static List<EnvoyClustersInformation> EnvoyInformationForPartition(Guid partitionId)
        {
            lock (lock_)
            {
                List<EnvoyClustersInformation> ret = new List<EnvoyClustersInformation>();
                if (partitions_ == null)
                {
                    return ret;
                }

                SF_Partition partition;
                if (!partitions_.TryGetValue(partitionId, out partition))
                {
                    return ret;
                }

                //List<RouteData> ret = new List<RouteData>();
                // stateless - partitionId | endpoint Index | -1
                // stateful - partitionId | endpoint Index | replica Index
                string prefix = partition.serviceName_.AbsolutePath;
                if (!prefix.EndsWith("/"))
                {
                    prefix += "/";
                }
                for (int endpointIndex = 0; endpointIndex < partition.listeners_.Count; endpointIndex++)
                {
                    var ep = partition.listeners_[endpointIndex];
                    List<int> replicaIndexes;
                    bool statefulPartition = false;
                    if (partition.serviceKind_ == ServiceKind.Stateless)
                    {
                        // For stateless, path is same for all replicas so we need to iterate just once
                        replicaIndexes = new List<int>() { 0 };
                    }
                    else
                    {
                        replicaIndexes = new List<int>();
                        replicaIndexes.AddRange(Enumerable.Range(0, ep.InstanceCount()));
                        statefulPartition = true;
                    }
                    JObject endpointHeader = null;
                    if (ep.Name != "")
                    {
                        endpointHeader = new JObject();
                        endpointHeader.Add("name", "ListenerName");
                        endpointHeader.Add("value", ep.Name);
                    }

                    for (int index = 0; index < replicaIndexes.Count; index++)
                    {
                        List<EnvoyRoute> routeData = new List<EnvoyRoute>();
                        List<EnvoyHost> hostData = new List<EnvoyHost>();
                        string cluster = partitionId.ToString() + "|" + endpointIndex.ToString() + "|" + replicaIndexes[index].ToString();
                        string prefix_rewrite = ep.GetAt(index).endpoint_.AbsolutePath;
                        List<JObject> headers = new List<JObject>();
                        if (endpointHeader != null)
                        {
                            headers.Add(endpointHeader);
                        }
                        if (ep.GetAt(index).role_ == ServiceEndpointRole.StatefulSecondary)
                        {
                            JObject statefulSecondaryIndex = new JObject();
                            statefulSecondaryIndex.Add("name", "SecondaryReplicaIndex");
                            statefulSecondaryIndex.Add("value", replicaIndexes[index].ToString());
                            headers.Add(statefulSecondaryIndex);
                        }
                        if (statefulPartition)
                        {
                            hostData.Add(new EnvoyHost(ep.GetAt(index).endpoint_.Host, ep.GetAt(index).endpoint_.Port));
                            var partitionKind = partition.partitionInformation_.Kind;
                            if (partitionKind == ServicePartitionKind.Int64Range)
                            {
                                //if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
                                //{
                                //    // Remove once Windows has Envoy 1.6
                                //    continue;
                                //}

                                var rangePartitionInformation = (Int64RangePartitionInformation)partition.partitionInformation_;
                                if (rangePartitionInformation.LowKey == Int64.MinValue &&
                                    rangePartitionInformation.HighKey == Int64.MaxValue)
                                {
                                    routeData.Add(new EnvoyRoute(cluster, prefix, prefix_rewrite, headers, EnvoyDefaults.timeout_ms));
                                }
                                else
                                {
                                    // Envoy - start and end of the range using half-open interval semantics [start, end)
                                    var rangeEnd = rangePartitionInformation.HighKey;
                                    if (rangePartitionInformation.HighKey == Int64.MaxValue)
                                    {
                                        JObject maxValHeader = new JObject();
                                        maxValHeader.Add("name", "PartitionKey");
                                        maxValHeader.Add("value", Int64.MaxValue.ToString());

                                        List<JObject> maxValHeaders = new List<JObject>(headers);
                                        maxValHeaders.Add(maxValHeader);
                                        routeData.Add(new EnvoyRoute(cluster, prefix, prefix_rewrite, maxValHeaders, EnvoyDefaults.timeout_ms));
                                    }
                                    else
                                    {
                                        rangeEnd++;
                                    }
                                    JObject partitionKeyHeader = new JObject();
                                    partitionKeyHeader.Add("name", "PartitionKey");

                                    JObject range_match = new JObject();
                                    range_match.Add("start", rangePartitionInformation.LowKey);
                                    range_match.Add("end", rangeEnd);
                                    partitionKeyHeader.Add("range_match", range_match);

                                    List<JObject> keyHeaders = new List<JObject>(headers);
                                    keyHeaders.Add(partitionKeyHeader);
                                    routeData.Add(new EnvoyRoute(cluster, prefix, prefix_rewrite, keyHeaders, EnvoyDefaults.timeout_ms));
                                }
                            }
                            else if (partitionKind == ServicePartitionKind.Named)
                            {
                                var namedPartitionInformation = (NamedPartitionInformation)partition.partitionInformation_;

                                JObject partitionKeyHeader = new JObject();
                                partitionKeyHeader.Add("name", "PartitionKey");
                                partitionKeyHeader.Add("value", namedPartitionInformation.Name);

                                List<JObject> keyHeaders = new List<JObject>(headers);
                                keyHeaders.Add(partitionKeyHeader);
                                routeData.Add(new EnvoyRoute(cluster, prefix, prefix_rewrite, keyHeaders, EnvoyDefaults.timeout_ms));
                            }
                        }
                        else
                        {
                            // For stateless, capture addresses for all listeners for the one route
                            for (int i = 0; i < ep.InstanceCount(); i++)
                            {
                                var address = ep.GetAt(i);
                                hostData.Add(new EnvoyHost(address.endpoint_.Host, address.endpoint_.Port));
                            }
                            routeData.Add(new EnvoyRoute(cluster, prefix, prefix_rewrite, headers, EnvoyDefaults.timeout_ms));
                        }
                        if (ep.GetAt(index).endpoint_.Scheme == "https")
                        {
                            if (EnvoyDefaults.cluster_ssl_context != null)
                            {
                                ret.Add(new EnvoyClustersInformation(cluster, routeData, hostData, true));
                            }
                            else
                            {
                                // Log information indicating that this end point is skipped
                            }
                        }
                        else
                        {
                            ret.Add(new EnvoyClustersInformation(cluster, routeData, hostData, false));
                        }
                    }
                }
                return ret;
            }
        }
    }
}
