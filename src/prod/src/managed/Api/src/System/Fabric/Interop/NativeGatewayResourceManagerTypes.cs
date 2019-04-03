namespace System.Fabric.Interop
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Strings;
    using System.Numerics;
    using System.Runtime.InteropServices;
    using SecureString = System.Security.SecureString;

    //// ----------------------------------------------------------------------------
    //// typedefs

    using BOOLEAN = System.SByte;
    using FABRIC_ATOMIC_GROUP_ID = System.Int64;
    using FABRIC_INSTANCE_ID = System.Int64;
    using FABRIC_NODE_INSTANCE_ID = System.UInt64;
    using FABRIC_PARTITION_ID = System.Guid;
    using FABRIC_REPLICA_ID = System.Int64;
    using FABRIC_SEQUENCE_NUMBER = System.Int64;
    using FABRIC_TESTABILITY_OPERATION_ID = System.Guid;
    using FABRIC_HOST_PROCESS_ID = System.Int64;

    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder", Justification = "Matches order in IDL.")]
    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121:UseBuiltInTypeAlias", Justification = "It is important here to explicitly state the size of integer parameters.")]
    [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1310:FieldNamesMustNotContainUnderscore", Justification = "Matches names in IDL.")]
    internal static class NativeGatewayResourceManagerTypes
    {
        [Guid("4e9e4fc0-39dd-4ac6-9a46-9b5ad0c47b6c")]
        internal enum FABRIC_GATEWAY_RESOURCE_STATUS : uint
        {
            FABRIC_GATEWAY_RESOURCE_STATUS_INVALID = 0,
            FABRIC_GATEWAY_RESOURCE_STATUS_CREATING = 1,
            FABRIC_GATEWAY_RESOURCE_STATUS_READY = 2,
            FABRIC_GATEWAY_RESOURCE_STATUS_DELETING = 3,
            FABRIC_GATEWAY_RESOURCE_STATUS_FAILED = 4,
            FABRIC_GATEWAY_RESOURCE_STATUS_UPGRADING = 5
        }        

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct NATIVE_LIST
        {
            public UInt32 Count;
            public IntPtr Items;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct NETWORK_REF
        {
            public IntPtr Name;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct ROUTING_DESTINATION
        {
            public IntPtr ApplicationName;
            public IntPtr ServiceName;
            public IntPtr EndpointName;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_GATEWAY_RESOURCE_TCP_CONFIG
        {
            public IntPtr Name;
            public Int32 ListenPort;
            public ROUTING_DESTINATION Destination;
            public IntPtr Reserved;
        }

        // [StructLayout(LayoutKind.Sequential, Pack = 8)]
        // internal struct FABRIC_GATEWAY_RESOURCE_TCP_CONFIG_LIST
        // {
        //     public ULONG Count;
        //     public [size_is(Count)] FABRIC_GATEWAY_RESOURCE_TCP_CONFIG * Items;
        // }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_GATEWAY_RESOURCE_DESCRIPTION
        {
            public IntPtr Name;
            public IntPtr Description;
            public NETWORK_REF SourceNetwork;
            public NETWORK_REF DestinationNetwork;
            public FABRIC_GATEWAY_RESOURCE_STATUS Status;
            public IntPtr StatusDescription;
            public NATIVE_LIST Tcp;
            public NATIVE_LIST Http;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_GATEWAY_RESOURCE_QUERY_DESCRIPTION
        {
            public IntPtr Name;
            public IntPtr PagingDescription;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_GATEWAY_RESOURCE_QUERY_RESULT
        {
            public NATIVE_LIST Items;
            public IntPtr ContinuationToken;
            public IntPtr Reserved;
        }

        public abstract class NativeConvertorBase<ManagedType, NativeType>
            where NativeType : struct
            where ManagedType : NativeConvertorBase<ManagedType, NativeType>, new()
        {
            public abstract NativeType ToNative(PinCollection pin);
            public abstract ManagedType InitFromNative(NativeType nativeType);

            static public ManagedType FromNative(NativeType nativeType)
            {
                var ret = new ManagedType();
                return ret.InitFromNative(nativeType);
            }
        }
    }
}