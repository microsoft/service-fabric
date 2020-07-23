// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using Microsoft.AspNetCore.Mvc;
using Newtonsoft.Json.Linq;
using sfintegration.envoymodels;

namespace webapi.Controllers
{
    [Produces("application/json")]
    [Route("[controller]")]
    public class v1 : Controller
    {
        [HttpGet]
        public IActionResult GetAsync()
        {
            return Ok(
                new { partitions = SF_Services.partitions_, services = SF_Services.services_ }
                );
        }

        [HttpGet("envoydata")]
        public IActionResult GetList()
        {
            List<EnvoyClustersInformation> ret = new List<EnvoyClustersInformation>();
            foreach (var pID in SF_Services.partitions_)
            {
                ret.AddRange(SF_Services.EnvoyInformationForPartition(pID.Key));
            }
            return Ok(
                new { clusters = ret }
                );
        }

        [HttpGet("listeners/{service_cluster}/{service_node}")]
        public IActionResult GetListeners(string service_cluster, string service_node)
        {
            List<EnvoyListener> ret = new List<EnvoyListener>();
            if (EnvoyDefaults.gateway_map == null)
            {
                return Ok(new { listeners = ret });
            }
            int index = 0;
            foreach (var entry in EnvoyDefaults.gateway_map)
            {
                EnvoyListener info = new EnvoyListener(
                    "gateway_" + entry.Key, "tcp://" + EnvoyDefaults.gateway_listen_ip + ":" + entry.Key, "gateway_proxy", entry.Value);
                ret.Add(info);
                index++;
            }
            return Ok(
                new { listeners = ret }
                );
        }

        [HttpGet("clusters/{service_cluster}/{service_node}")]
        public IActionResult GetClusters(string service_cluster, string service_node)
        {
            List<EnvoyCluster> ret = new List<EnvoyCluster>();
            if (SF_Services.partitions_ == null)
            {
                return Ok(new { clusters = ret });
            }
            if (EnvoyDefaults.gateway_map != null)
            {
                foreach (var service in SF_Services.services_)
                {
                    if (!EnvoyDefaults.gateway_clusternames.Contains(service.Key))
                    {
                        continue;
                    }
                    EnvoyCluster info = new EnvoyCluster(service.Key);
                    ret.Add(info);
                }
            }
            else
            {
                foreach (var pID in SF_Services.partitions_)
                {
                    var info = SF_Services.EnvoyInformationForPartition(pID.Key);
                    foreach (var service in info)
                    {
                        ret.Add(service.cluster);
                    }
                }
                foreach (var service in SF_Services.services_)
                {
                    if (!service.Value.StatefulService)
                    {
                        continue;
                    }
                    EnvoyCluster info = new EnvoyCluster(service.Key);
                    ret.Add(info);
                }
            }

            return Ok(
                new { clusters = ret }
                );
        }

        // removes stateful header - PartitionKey and returns remaining headers
        // if it matches filters
        // not secondary and listername matches give name (if not null)
        Tuple<bool, List<JObject>> RemoveStatefulHeadersAndFilter(List<JObject> headers, string listenerName = null)
        {
            for (var i = headers.Count - 1; i >= 0; i--)
            {
                var header = headers[i];
                var headerName = (string)header.GetValue("name");
                if (headerName == "SecondaryReplicaIndex")
                {
                    return new Tuple<bool, List<JObject>>(false, null);
                }
                if (headerName == "ListenerName" && listenerName != null && (string)header.GetValue("value") != listenerName)
                {
                    return new Tuple<bool, List<JObject>>(false, null);
                }
                if (headerName == "PartitionKey")
                {
                    headers.RemoveAt(i);
                }
            }
            return new Tuple<bool, List<JObject>>(true, headers);
        }

        [HttpGet("routes/{name}/{service_cluster}/{service_node}")]
        public IActionResult GetRoutes(string name, string service_cluster, string service_node)
        {
            List<EnvoyVirtualHost> virtual_hosts = new List<EnvoyVirtualHost>();
            List<EnvoyRoute> ret = new List<EnvoyRoute>();
            if (SF_Services.partitions_ == null)
            {
                virtual_hosts.Add(new EnvoyVirtualHost(
                    name,
                    new List<string>() { "*" },
                    ret));
                return Ok(new
                {
                    virtual_hosts
                });
            }
            if (name.StartsWith("gateway_config|"))
            {
                return GetRoutesForGateway(name, virtual_hosts);
            }

            foreach (var pID in SF_Services.partitions_)
            {
                var info = SF_Services.EnvoyInformationForPartition(pID.Key);
                foreach (var service in info)
                {
                    ret.AddRange(service.routes);
                }
            }
            foreach (var service in SF_Services.services_)
            {
                if (!service.Value.StatefulService)
                {
                    continue;
                }
                string routeConfigForPartition = service.Value.Partitions[0].ToString() + "|" + service.Value.EndpointIndex.ToString() + "|0";
                var info = SF_Services.EnvoyInformationForPartition(service.Value.Partitions[0]);
                foreach (var serviceInfo in info)
                {
                    if (serviceInfo.cluster.name != routeConfigForPartition)
                    {
                        continue;
                    }
                    var routes = serviceInfo.routes;
                    foreach (var route in routes)
                    {
                        route.cluster = service.Key;
                        var tuple = RemoveStatefulHeadersAndFilter(route.headers);
                        if (!tuple.Item1)
                        {
                            continue;
                        }
                        route.prefix_rewrite = "/";
                        ret.Add(route);
                    }
                }
            }
            virtual_hosts.Add(new EnvoyVirtualHost(
                name,
                new List<string>() { "*" },
                ret));
            return Ok(
                new
                {
                    virtual_hosts,
                    EnvoyDefaults.response_headers_to_remove
                });
        }

        private IActionResult GetRoutesForGateway(string name, List<EnvoyVirtualHost> ret)
        {
            var segments = name.Split("|");
            if (segments.Length != 2 ||
                EnvoyDefaults.gateway_map == null ||
                !EnvoyDefaults.gateway_map.ContainsKey(segments[1]))
            {
                return Ok(
                    new
                    {
                        virtual_hosts = new[]
                        {
                            new {
                                name = "Dummy",
                                domains = new List<string>() { "*" },
                                routes = ret
                            }
                        }
                    });
            }


            var filterConfigs = EnvoyDefaults.gateway_map[segments[1]];
            foreach (var config in filterConfigs)
            {
                if (config.Type == ListenerFilterConfig.ListenerFilterType.Http)
                {
                    var httpHosts = ((ListenerHttpFilterConfig)config).Hosts;
                    foreach (var host in httpHosts)
                    {
                        var routes = new List<EnvoyRoute>();
                        foreach (var route in host.Routes)
                        {
                            var serviceName = route.Destination.EnvoyServiceName();
                            if (!SF_Services.services_.ContainsKey(serviceName))
                            {
                                continue;
                            }
                            var service = SF_Services.services_[serviceName];
                            foreach (var partition in service.Partitions)
                            {
                                string routeConfigForPartition = partition.ToString() + "|" + service.EndpointIndex.ToString() + "|0";
                                var pId = partition;
                                var info = SF_Services.EnvoyInformationForPartition(pId);
                                foreach (var serviceInfo in info)
                                {
                                    if (serviceInfo.cluster.name != routeConfigForPartition)
                                    {
                                        continue;
                                    }
                                    foreach (var envoyRoute in serviceInfo.routes)
                                    {
                                        var tuple = RemoveStatefulHeadersAndFilter(envoyRoute.headers, route.Destination.EndpointName);
                                        if (!tuple.Item1)
                                        {
                                            continue;
                                        }

                                        // We should filter only on user specified headers
                                        envoyRoute.headers = new List<JObject>();
                                        envoyRoute.prefix = route.Match.Path.Value;
                                        envoyRoute.prefix_rewrite = route.Match.Path.Rewrite;
                                        if (route.Match.Headers != null)
                                        {
                                            foreach (var header in route.Match.Headers)
                                            {
                                                var envoyHeader = new JObject();
                                                envoyHeader.Add("name", header.Name);
                                                envoyHeader.Add("value", header.Value);
                                                envoyRoute.headers.Add(envoyHeader);
                                            }
                                        }
                                        envoyRoute.cluster = serviceName;
                                        routes.Add(envoyRoute);
                                    }
                                }
                            }

                        }
                        var domain = new List<string>() { host.Name };
                        var virtual_host = new EnvoyVirtualHost(host.Name,
                            domain,
                            routes);
                        ret.Add(virtual_host);
                    }
                }
            }

            return Ok(
                new
                {
                    virtual_hosts = ret
                });
        }

        [HttpGet("registration/{routeConfig}")]
        public IActionResult GetHosts(string routeConfig)
        {
            List<EnvoyHost> ret = new List<EnvoyHost>();

            var nameSegements = routeConfig.Split('|');
            // Deal with service name cluster as opposed to a partition cluster
            if (nameSegements[2] == "-2")
            {
                if (!SF_Services.services_.ContainsKey(routeConfig))
                {
                    return Ok(new { hosts = ret });
                }
                var service = SF_Services.services_[routeConfig];
                foreach (var partition in service.Partitions)
                {
                    string routeConfigForPartition = partition.ToString() + "|" + service.EndpointIndex.ToString() + "|0";
                    var pId = partition;
                    var info = SF_Services.EnvoyInformationForPartition(pId);
                    foreach (var serviceInfo in info)
                    {
                        if (serviceInfo.cluster.name != routeConfigForPartition)
                        {
                            continue;
                        }
                        ret.AddRange(serviceInfo.hosts);
                    }
                }
            }
            else
            {
                Guid pId = new Guid(nameSegements[0]);
                var info = SF_Services.EnvoyInformationForPartition(pId);
                foreach (var service in info)
                {
                    if (service.cluster.name != routeConfig)
                    {
                        continue;
                    }
                    ret.AddRange(service.hosts);
                }
            }
            return Ok(
                new { hosts = ret }
                );
        }
    }
}
