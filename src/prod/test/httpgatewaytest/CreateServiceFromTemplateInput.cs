// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------



namespace System.Fabric.Test.HttpGatewayTest
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Description;
    using System.Linq;
    using System.Text;
    using System.Runtime.Serialization;

    [Serializable]
    [DataContract]
    class CreateServiceFromTemplateInput
    {
        [DataMember]
        public string ApplicationName
        {
            get;
            set;
        }

        [DataMember]
        public string ServiceName
        {
            get;
            set;
        }

        [DataMember]
        public string ServiceTypeName
        {
            get;
            set;
        }

        [DataMember]
        public Byte[] InitializationData
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
    }
}