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
    class ServiceResult
    {
        [DataMember(IsRequired = true)]
        public string Id
        {
            get;
            set;
        }

        [DataMember(Name="ServiceKind", IsRequired = true)]
        private string ServiceKindString
        {
            get;
            set;
        }

        public System.Fabric.Query.ServiceKind ServiceKind
        {
            get
            {
                return (Query.ServiceKind)Enum.Parse(typeof(Query.ServiceKind), ServiceKindString, true);
            }
        }

        [DataMember(IsRequired = true)]
        public string Name
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string TypeName
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string ManifestVersion
        {
            get;
            set;
        }

        [DataMember]
        public bool HasPersistedState
        {
            get;
            set;
        }

        [DataMember(Name="ServiceStatus", IsRequired = true)]
        private string ServiceStatusString
        {
            get;
            set;
        }

        [DataMember(Name= "HealthState", IsRequired = true)]
        private string HealthStateString
        {
            get;
            set;
        }

        public System.Fabric.Query.ServiceStatus ServiceStatus
        {
            get
            {
                return (Query.ServiceStatus)Enum.Parse(typeof(Query.ServiceStatus), ServiceStatusString, true);
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