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
    public class CodePackageOnNodeResult
    {
        public CodePackageOnNodeResult()
        {
            SetupEntryPoint = new CodePackageEntryPoint();
            MainEntryPoint = new CodePackageEntryPoint();
        }

        [DataMember(IsRequired = true)]
        public string Name
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string Version
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public CodePackageEntryPoint SetupEntryPoint
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public CodePackageEntryPoint MainEntryPoint
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public bool HasSetupEntryPoint
        {
            get;
            set;
        }

        [DataMember]
        public string ServicePackageActivationId
        {
            get;
            set;
        }
    }
}