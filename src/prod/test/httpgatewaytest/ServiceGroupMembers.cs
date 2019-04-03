// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.HttpGatewayTest
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Description;
    using System.Linq;
    using System.Text;
    using System.Runtime.Serialization;

    [Serializable]
    [DataContract]
    class ServiceGroupMembersResult
    {
        public ServiceGroupMembersResult()
        {
            Name = "";
            ServiceGroupMemberDescription = new List<ServiceGroupMemberMembersResult>();
        }

        [DataMember]
        public string Name
        {
            get;
            set;
        }

        [DataMember]
        public ServicePackageActivationMode ServicePackageActivationMode
        {
            get;
            set;
        }

        [DataMember]
        public List<ServiceGroupMemberMembersResult> ServiceGroupMemberDescription
        {
            get;
            set;
        }
    }
}