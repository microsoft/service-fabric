// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using System.Runtime.Serialization;

#if !DotNetCoreClr
    [Serializable]
#endif
    internal class FabricApplicationException :
#if !StandaloneTool
        FabricImageBuilderValidationException
#else
        Exception
#endif
    {
        public FabricApplicationException()
        {
        }

#if !StandaloneTool
        public FabricApplicationException(string message)
            : base(message)
        {
        }

        public FabricApplicationException(string message, Exception inner)
            : base(message, inner)
        {
        }

#else
        public FabricApplicationException(string message)
            : base(message)
        {
        }

        public FabricApplicationException(string message, Exception inner)
            : base(message, inner)
        {
        }
#endif


#if !DotNetCoreClr
        protected FabricApplicationException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }
#endif
    }
}

