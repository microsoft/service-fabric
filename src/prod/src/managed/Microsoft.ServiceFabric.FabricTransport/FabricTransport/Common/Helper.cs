// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal sealed class Helper
    {
        internal const string Secure = "Secure";

        public static void ThrowIfValueOutOfBounds(long value, string argumentName)
        {
            if (value > int.MaxValue || value < 0)
            {
                throw new ArgumentOutOfRangeException(argumentName);
            }
        }

        public static unsafe byte[] Get_Byte(IntPtr message)
        {
            var msgBufferValue = (NativeTypes.FABRIC_MESSAGE_BUFFER*) message;
            // TODO: Copy shld be removed
            return NativeTypes.FromNativeBytes(msgBufferValue->Buffer, msgBufferValue->BufferSize);
        }

        internal static int GetEndpointPort(
            ICodePackageActivationContext codePackageActivationContext,
            string endpointResourceName)
        {
            var port = 0;

            var endpointName = endpointResourceName;
            var endpoints = codePackageActivationContext.GetEndpoints();
            foreach (var endpoint in endpoints)
            {
                if (string.Compare(endpoint.Name, endpointName, StringComparison.InvariantCultureIgnoreCase) == 0)
                {
                    port = endpoint.Port;
                    break;
                }
            }
            AppTrace.TraceSource.WriteInfo("FabricTransportHelper", "Port for EndpointResouceName: {0} is {1}",
                endpointResourceName, port);
            return port;
        }
    }
}