// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition
{
    using System;
    using System.Runtime.Serialization;

    /// <summary>
    /// Captures the Instance Identify of a Trace
    /// </summary>
    [DataContract]
    public class InstanceIdentity
    {
        public InstanceIdentity(Guid id)
        {
            Assert.AssertIsFalse(id == Guid.Empty, "Id can't be empty");
            this.Id = id;
        }

        /// <summary>
        /// Instance Id
        /// </summary>
        [DataMember]
        public Guid Id { get; }
    }
}