//----------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//----------------------------------------------------------------

#if DotNetCoreClrLinux
namespace Microsoft.ServiceFabric.ContainerServiceClient
{
    using System;
    using System.Text;
    using System.Net;
    using System.Net.Sockets;

    internal class UnixDomainSocketEndPoint : EndPoint
    {
        private const AddressFamily EndPointAddressFamily = AddressFamily.Unix;

        //
        // = offsetof(struct sockaddr_un, sun_path). It's the same on Linux and OSX
        //
        private static readonly Encoding pathEncoding = Encoding.UTF8;

        private static readonly int nativePathOffset = 2;

        //
        // sockaddr_un.sun_path at http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/sys_un.h.html, -1 for terminator
        //
        private static readonly int nativePathLength = 91;

        private static readonly int nativeAddressSize = nativePathOffset + nativePathLength;

        private readonly string path;

        private readonly byte[] encodedPath;

        public UnixDomainSocketEndPoint(string path)
        {
            this.path = path;
            this.encodedPath = pathEncoding.GetBytes(this.path);

            if (path.Length == 0 || this.encodedPath.Length > nativePathLength)
            {
                throw new ArgumentOutOfRangeException(nameof(path), path);
            }
        }

        internal UnixDomainSocketEndPoint(SocketAddress socketAddress)
        {
            if (socketAddress == null)
            {
                throw new ArgumentNullException(nameof(socketAddress));
            }

            if (socketAddress.Family != EndPointAddressFamily ||
                socketAddress.Size > nativeAddressSize)
            {
                throw new ArgumentOutOfRangeException(nameof(socketAddress));
            }

            if (socketAddress.Size > nativePathOffset)
            {
                this.encodedPath = new byte[socketAddress.Size - nativePathOffset];
                for (int i = 0; i < this.encodedPath.Length; i++)
                {
                    this.encodedPath[i] = socketAddress[nativePathOffset + i];
                }

                this.path = pathEncoding.GetString(this.encodedPath, 0, this.encodedPath.Length);
            }
            else
            {
                this.encodedPath = new byte[0];
                this.path = string.Empty;
            }
        }

        public override SocketAddress Serialize()
        {
            var result = new SocketAddress(AddressFamily.Unix, nativeAddressSize);

            for (int index = 0; index < this.encodedPath.Length; index++)
            {
                result[nativePathOffset + index] = this.encodedPath[index];
            }

            result[nativePathOffset + this.encodedPath.Length] = 0; // path must be null-terminated

            return result;
        }

        public override EndPoint Create(SocketAddress socketAddress)
        {
            return new UnixDomainSocketEndPoint(socketAddress);
        }
        
        public override AddressFamily AddressFamily
        {
            get
            {
                return EndPointAddressFamily;
            }
        }

        public override string ToString()
        {
            return this.path;
        }
    }
}
#endif
