// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Structures
{
    using System;
    using System.Runtime.Serialization;

    [Serializable]
    [DataContract]
    internal class WinFabricManifestResult
    {
        [DataMember]
        public string Manifest
        {
            get;
            set;
        }
    }
}


#pragma warning restore 1591