// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.Serialization;

namespace System.Fabric.Test.HttpGatewayTest
{
    [Serializable]
    [DataContract]
    class ApplicationsResult
    {
        [DataMember]
        public string Id
        {
            get;
            set;
        }

        [DataMember]
        public string Name
        {
            get;
            set;
        }

        [DataMember]
        public string TypeName
        {
            get;
            set;
        }

        [DataMember]
        public string TypeVersion
        {
            get;
            set;
        }

        [DataMember]
        public Dictionary<string, string> Parameters
        {
            get;
            set;
        }

        [DataMember(Name = "Status")]
        private string StatusString
        {
            get;
            set;
        }

        [DataMember(Name = "HealthState")]
        private string HealthStateString
        {
            get;
            set;
        }

        public System.Fabric.Query.ApplicationStatus Status
        {
            get
            {
                return (Query.ApplicationStatus)Enum.Parse(typeof(Query.ApplicationStatus), StatusString, true);
            }
        }

        public System.Fabric.Health.HealthState HealthState
        {
            get
            {
                return (Health.HealthState)Enum.Parse(typeof(Health.HealthState), HealthStateString, true);
            }
        }
    }
}