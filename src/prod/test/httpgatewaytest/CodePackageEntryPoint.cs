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
    public class CodePackageEntryPoint
    {
        public CodePackageEntryPoint()
        {
            CodePackageEntryPointStatistics = new CodePackageEntryPointStatistics();
        }

        [DataMember(IsRequired=true)]
        public string EntryPointLocation
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public Int32 ProcessId
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string RunAsUserName
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public CodePackageEntryPointStatistics CodePackageEntryPointStatistics
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public long InstanceId
        {
            get;
            set;
        }
    }
}