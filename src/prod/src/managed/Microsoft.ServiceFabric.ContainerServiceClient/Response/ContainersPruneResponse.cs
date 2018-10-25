//----------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//----------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient.Response
{
    using System.Collections.Generic;
    using System.Runtime.Serialization;

    [DataContract]
    internal class ContainersPruneResponse
    {
        [DataMember(Name = "ContainersDeleted", EmitDefaultValue = false)]
        public IList<string> ContainersDeleted { get; set; }

        [DataMember(Name = "SpaceReclaimed", EmitDefaultValue = false)]
        public ulong SpaceReclaimed { get; set; }
    }
}
