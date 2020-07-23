// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Service
{
    using System;
    using Newtonsoft.Json;

    [JsonObject("ServiceEvent")]
    public class ServiceEvent : FabricEvent
    {
        /// <summary>
        /// Initializes a new instance of the ServiceEvent class.
        /// </summary>
        public ServiceEvent()
        {
        }

        /// <summary>
        /// Initializes a new instance of the ServiceEvent class.
        /// </summary>
        /// <param name="eventInstanceId">The identifier for the FabricEvent
        /// instance.</param>       
        /// <param name="timeStamp">The time event was logged</param>
        /// <param name="category">The Category of the Event.</param>
        /// <param name="serviceId">The identity of the service. This is an
        /// encoded representation of the service name. This is used in the
        /// REST APIs to identify the service resource.
        /// Starting in version 6.0, hierarchical names are delimited with the
        /// "\~" character. For example, if the service name is
        /// "fabric:/myapp/app1/svc1",
        /// the service identity would be "myapp~app1\~svc1" in 6.0+ and
        /// "myapp/app1/svc1" in previous versions.
        /// </param>
        /// <param name="hasCorrelatedEvents">Shows that there is existing
        /// related events available.</param>
        public ServiceEvent(Guid eventInstanceId, DateTime timeStamp, string category, string serviceId, bool? hasCorrelatedEvents = false)
            : base(eventInstanceId, timeStamp, category, hasCorrelatedEvents)
        {
            ServiceId = TransformServiceName(serviceId);
        }

        /// <summary>
        /// Gets or sets the identity of the service. This is an encoded
        /// representation of the service name. This is used in the REST APIs
        /// to identify the service resource.
        /// Starting in version 6.0, hierarchical names are delimited with the
        /// "\~" character. For example, if the service name is
        /// "fabric:/myapp/app1/svc1",
        /// the service identity would be "myapp~app1\~svc1" in 6.0+ and
        /// "myapp/app1/svc1" in previous versions.
        ///
        /// </summary>
        [JsonProperty(PropertyName = "ServiceId")]
        public string ServiceId { get; set; }

        public static string TransformServiceName(string serviceId)
        {
            if (serviceId.StartsWith("fabric:/", StringComparison.InvariantCultureIgnoreCase))
            {
                serviceId = serviceId.Substring(8);
            }
            else
            {
                serviceId = string.Format("system/{0}", serviceId);
            }

            if (serviceId.Contains("/"))
            {
                var segments = serviceId.Split(
                    new[] { '/' },
                    StringSplitOptions.RemoveEmptyEntries);

                serviceId = string.Join("~", segments);
            }

            return serviceId;
        }
    }
}
