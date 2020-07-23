// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Fabric;
using System.Fabric.Description;
using Microsoft.ServiceFabric.Services.Client;
using System.Collections.Generic;

namespace ServiceFabric
{
    namespace Helpers
    {
        /// <summary>
        /// Class to setup ResolveNotifications to update Fabric Client's Endpoint cache.
        /// </summary>
        public class EnableResolveNotifications
        {
            static private FabricClient client;

            static EnableResolveNotifications()
            {
                client = null;
            }

            static void InitClient(FabricClient cli, EventHandler handler)
            {
                if (client == null)
                {
                    if (cli == null)
                    {
                        client = new FabricClient();
                    }
                    else
                    {
                        client = cli;
                    }
                    if (handler != null)
                    {
                        client.ServiceManager.ServiceNotificationFilterMatched += handler;
                    }

                    ServicePartitionResolver resolver =
                        new ServicePartitionResolver(
                            delegate
                            {
                                return client;
                            }
                        );
                    ServicePartitionResolver.SetDefault(resolver);
                }
            }

            public static void RegisterNotificationFilter(string filter, FabricClient cli = null, EventHandler handler = null)
            {
                InitClient(cli, handler);
                client.ServiceManager.RegisterServiceNotificationFilterAsync(
                    new ServiceNotificationFilterDescription(
                        new Uri(filter),
                        true,
                        false
                    )
                );
            }
        }
    }
}