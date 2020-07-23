// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.Serialization;
using System.Runtime.InteropServices.ComTypes;

namespace System.Fabric.Test.HttpGatewayTest
{
    [Serializable]
    [DataContract]   
    public class CodePackageEntryPointStatistics
    {
        [DataMember(IsRequired=true)]
        public UInt32 LastExitCode
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string LastActivationTime
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string LastExitTime
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string LastSuccessfulActivationTime
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string LastSuccessfulExitTime
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public ulong ActivationFailureCount
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public ulong ContinuousActivationFailureCount
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public ulong ExitFailureCount
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public ulong ContinuousExitFailureCount
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public ulong ActivationCount
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public ulong ExitCount
        {
            get;
            set;
        }
    }
}