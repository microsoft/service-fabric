// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.DockerCompose
{
    using System.Runtime.Serialization;

#if !DotNetCoreClr
    [Serializable]
#endif
    internal class FabricComposeException :
#if !StandaloneTool
        FabricImageBuilderValidationException
#else
        Exception
#endif
    {
        public FabricComposeException()
        {
        }

#if !StandaloneTool
        public FabricComposeException(string message)
            : base(message)
        {
        }

        public FabricComposeException(string message, Exception inner)
            : base(message, inner)
        {
        }

#else
        public FabricComposeException(string message)
            : base(message)
        {
        }

        public FabricComposeException(string message, Exception inner)
            : base(message, inner)
        {
        }
#endif


#if !DotNetCoreClr
        protected FabricComposeException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }
#endif
    }
}