// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Hosting.ContainerActivatorService
{
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using System.Fabric.Common;
    using System.IO;
    using System.Text;
    using Newtonsoft.Json;
    using System.Fabric;
    using Microsoft.ServiceFabric.ContainerServiceClient.Config;
    using Microsoft.ServiceFabric.ContainerServiceClient.Response;

    internal class ContainerEventManager
    {
        #region Private Member

        private const string TraceType = "ContainerEventManager";

        private readonly ContainerActivatorService activatorService;
        private readonly ContainerEventsConfig eventsConfig;
        private readonly int maxContinuousFailureCount;
        private readonly TimeSpan retryDelay;

        private int continuousFailureCount;

        #endregion

        internal ContainerEventManager(ContainerActivatorService activator)
        {
            this.activatorService = activator;

            var eventTypeList = new List<string>() {"stop", "die"};

            if(HostingConfig.Config.EnableDockerHealthCheckIntegration)
            {
                eventTypeList.Add("health_status");
            }
            
            this.eventsConfig = new ContainerEventsConfig
            {
                Filters = new Dictionary<string, IList<string>>()
                {
                    { "type", new List<string> () { "container" } },
                    { "event", eventTypeList }
                }
            };

            this.maxContinuousFailureCount = HostingConfig.Config.ContainerEventManagerMaxContinuousFailure;
            this.retryDelay = TimeSpan.FromSeconds(HostingConfig.Config.ContainerEventManagerFailureBackoffInSec);

            this.continuousFailureCount = 0;
        }

        internal void StartMonitoringEvents(UInt64 sinceTime)
        {
            this.eventsConfig.Since = sinceTime.ToString();

            Task.Run(() => this.MonitorEventsAsync());
        }

        private async Task MonitorEventsAsync()
        {
            for (;;)
            {
                try
                {
                    using (var eventStream = await this.activatorService.Client.SystemOperation.MonitorEventAsync(this.eventsConfig))
                    {
                        // Reset failure count if we are able to connect and get event stream.
                        continuousFailureCount = 0;

                        using (var reader = new StreamReader(eventStream, new UTF8Encoding(false)))
                        {
                            string line;
                            while ((line = await reader.ReadLineAsync()) != null)
                            {
                                var eventResp = JsonConvert.DeserializeObject<ContainerEventResponse>(line);
                                if (eventResp != null)
                                {
                                    this.OnContainerEvent(eventResp);
                                }
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                    continuousFailureCount++;

                    HostingTrace.Source.WriteWarning(
                        TraceType,
                        "MonitorEventsAsync() encountered error. CountinuousFailureCount={0}, Exception={1}.",
                        continuousFailureCount,
                        ex.ToString());
                }

                if(continuousFailureCount >= maxContinuousFailureCount)
                {
                    break;
                }

                await Task.Delay(retryDelay);
            }

            var errMssg = string.Format(
                "Failed to monitor container events. ContinuousFailureCount={0}. Terminating process for self-repair.",
                continuousFailureCount);

            HostingTrace.Source.WriteError(TraceType, errMssg);
            ReleaseAssert.Failfast(errMssg);
        }

        private void OnContainerEvent(ContainerEventResponse eventResp)
        {
            var eventDesc = new ContainerEventDescription()
            {
                EventType = EventTypeFromString(eventResp.Status),
                ContainerId = eventResp.Actor.Id,
                ContainerName = eventResp.Actor.Attributes.Name,
                TimeStampInSeconds = eventResp.Time,
                IsHealthy = eventResp.IsHealthStatusHealthy,
                ExitCode = eventResp.Actor.Attributes.ExitCode
            };

            var notification = new ContainerEventNotification()
            {
                SinceTime = eventResp.Time,
                UntilTime = eventResp.Time,
                EventList = new List<ContainerEventDescription> { eventDesc }
            };

            this.activatorService.OnContainerEvent(notification);
            this.UpdateSinceTime(eventResp.Time);
        }

        private void UpdateSinceTime(UInt64 sinceTime)
        {
            this.eventsConfig.Since = sinceTime.ToString();
        }

        private static ContainerEventType EventTypeFromString(string eventTypeStr)
        {
            var eventTypeStrLower = eventTypeStr.ToLowerInvariant().Trim();
            var eventType = ContainerEventType.None;

            if (eventTypeStrLower == "stop")
            {
                eventType = ContainerEventType.Stop;
            }
            else if (eventTypeStrLower == "die")
            {
                eventType = ContainerEventType.Die;
            }
            else if (eventTypeStrLower.StartsWith("health_status"))
            {
                eventType = ContainerEventType.Health;
            }
            else
            {
                ReleaseAssert.Failfast("Unexpected container event received: {0}", eventTypeStr);
            }

            return eventType;
        }

        private bool IsHealthyHealthStatus(string eventStatus)
        {
            var eventStatuLower = eventStatus.ToLowerInvariant().Trim();
            return (eventStatuLower == "health_status: healthy");
        }
    }
}

/*
    
     => This is how the JSON string of docker events looks like. 
     => The set of attributes vary based on type of event.

    {
        "status":"health_status: unhealthy",
        "id":"6e328c7f65ce404b327cd31b0272724f8b7f9622ba7ff76e8748838e1218d276",
        "from":"healthcheckimage:v1",
        "Type":"container",
        "Action":"health_status: unhealthy",
        "Actor":
        {
            "ID":"6e328c7f65ce404b327cd31b0272724f8b7f9622ba7ff76e8748838e1218d276",
            "Attributes":
            {
                "Description":"Test application container health check testing",
                "Vendor":"Microsoft",
                "Version":"v1",
                "image":"healthcheckimage:v1",
                "name":"sf-0-df981d20-63fd-48cd-8d4a-b1d9e9c10521_9b1827cc-f3dd-4600-80c8-681a277f9c45"
            }
        },
        "time":1508357457,
        "timeNano":1508357457395113700
    }

    {
        "status":"die",
        "id":"6e328c7f65ce404b327cd31b0272724f8b7f9622ba7ff76e8748838e1218d276",
        "from":"healthcheckimage:v1",
        "Type":"container",
        "Action":"die",
        "Actor":
        {
            "ID":"6e328c7f65ce404b327cd31b0272724f8b7f9622ba7ff76e8748838e1218d276",
            "Attributes":
            {
                "Description":"Test application container health check testing",
                "Vendor":"Microsoft",
                "Version":"v1",
                "exitCode":"1067",
                "image":"healthcheckimage:v1",
                "name":"sf-0-df981d20-63fd-48cd-8d4a-b1d9e9c10521_9b1827cc-f3dd-4600-80c8-681a277f9c45"
            }
        },
        "time":1508357462,
        "timeNano":1508357462563238300
    }
    
    {
        "status":"stop",
        "id":"6e328c7f65ce404b327cd31b0272724f8b7f9622ba7ff76e8748838e1218d276",
        "from":"healthcheckimage:v1",
        "Type":"container",
        "Action":"stop",
        "Actor":
        {
            "ID":"6e328c7f65ce404b327cd31b0272724f8b7f9622ba7ff76e8748838e1218d276",
            "Attributes":
            {
                "Description":"Test application container health check testing",
                "Vendor":"Microsoft",
                "Version":"v1",
                "image":"healthcheckimage:v1",
                "name":"sf-0-df981d20-63fd-48cd-8d4a-b1d9e9c10521_9b1827cc-f3dd-4600-80c8-681a277f9c45"
            }
        },
        "time":1508357462,
        "timeNano":1508357462692257200
    }
    
    */