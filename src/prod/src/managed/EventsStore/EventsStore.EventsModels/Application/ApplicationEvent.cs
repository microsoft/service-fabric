// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Application
{
    using System;
    using Newtonsoft.Json;

    [JsonObject("ApplicationEvent")]
    public class ApplicationEvent : FabricEvent
    {
        /// <summary>
        /// Initializes a new instance of the ApplicationEvent class.
        /// </summary>
        public ApplicationEvent()
        {
        }

        /// <summary>
        /// Initializes a new instance of the ApplicationEvent class.
        /// </summary>
        /// <param name="eventInstanceId">The identifier for the FabricEvent
        /// instance.</param>       
        /// <param name="timeStamp">The time event was logged</param>
        /// <param name="category">The Category of the Event.</param>
        /// <param name="applicationId">The identity of the application. This
        /// is an encoded representation of the application name. This is used
        /// in the REST APIs to identify the application resource.
        /// Starting in version 6.0, hierarchical names are delimited with the
        /// "\~" character. For example, if the application name is
        /// "fabric:/myapp/app1",
        /// the application identity would be "myapp\~app1" in 6.0+ and
        /// "myapp/app1" in previous versions.
        /// </param>
        /// <param name="hasCorrelatedEvents">Shows that there is existing
        /// related events available.</param>
        public ApplicationEvent(Guid eventInstanceId, DateTime timeStamp, string category, string applicationId, bool? hasCorrelatedEvents = false)
            : base(eventInstanceId, timeStamp, category, hasCorrelatedEvents)
        {
            ApplicationId = TransformAppName(applicationId);
        }

        /// <summary>
        /// Gets or sets the identity of the application. This is an encoded
        /// representation of the application name. This is used in the REST
        /// APIs to identify the application resource.
        /// Starting in version 6.0, hierarchical names are delimited with the
        /// "\~" character. For example, if the application name is
        /// "fabric:/myapp/app1",
        /// the application identity would be "myapp\~app1" in 6.0+ and
        /// "myapp/app1" in previous versions.
        ///
        /// </summary>
        [JsonProperty(PropertyName = "ApplicationId")]
        public string ApplicationId { get; set; }

        public static string TransformAppName(string applicationId)
        {
            if (applicationId.StartsWith("fabric:/", StringComparison.InvariantCultureIgnoreCase))
            {
                applicationId = applicationId.Substring(8);
            }

            if (applicationId.Contains("/"))
            {
                var segments = applicationId.Split(
                    new[] { '/' },
                    StringSplitOptions.RemoveEmptyEntries);

                applicationId = string.Join("~", segments);
            }

            return applicationId;
        }
    }
}
