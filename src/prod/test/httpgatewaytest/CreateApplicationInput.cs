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
    class CreateApplicationInput
    {
        public CreateApplicationInput(string appName, string appTypeName, string appVersion, Dictionary<string, string> param)
        {
            Name = appName;
            TypeName = appTypeName;
            TypeVersion = appVersion;
            ParameterList = param;
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
        public Dictionary<string, string> ParameterList
        {
            get;
            set;
        }
    }
}