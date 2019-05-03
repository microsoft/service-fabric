// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Setup
{
    using ComponentModel;
    using System.Fabric.Common.Tracing;
    using System.Net;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography.X509Certificates;
    using System.Threading;

    internal class PortAclUtility
    {
        #region DllImport

        [DllImport("httpapi.dll", SetLastError = true)]
        public static extern uint HttpInitialize(
            HTTPAPI_VERSION Version,
            uint Flags,
            IntPtr pReserved);

        [DllImport("httpapi.dll", SetLastError = true)]
        static extern uint HttpSetServiceConfiguration(
             IntPtr ServiceIntPtr,
             HTTP_SERVICE_CONFIG_ID ConfigId,
             IntPtr pConfigInformation,
             int ConfigInformationLength,
             IntPtr pOverlapped);

        [DllImport("httpapi.dll", SetLastError = true)]
        static extern uint HttpDeleteServiceConfiguration(
            IntPtr ServiceIntPtr,
            HTTP_SERVICE_CONFIG_ID ConfigId,
            IntPtr pConfigInformation,
            int ConfigInformationLength,
            IntPtr pOverlapped);

        [DllImport("httpapi.dll", SetLastError = true)]
        public static extern uint HttpTerminate(
            uint Flags,
            IntPtr pReserved);

        enum HTTP_SERVICE_CONFIG_ID
        {
            HttpServiceConfigIPListenList = 0,
            HttpServiceConfigSSLCertInfo,
            HttpServiceConfigUrlAclInfo,
            HttpServiceConfigMax
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct HTTP_SERVICE_CONFIG_IP_LISTEN_PARAM
        {
            public ushort AddrLength;
            public IntPtr pAddress;
        }

        [StructLayout(LayoutKind.Sequential)]
        struct HTTP_SERVICE_CONFIG_SSL_SET
        {
            public HTTP_SERVICE_CONFIG_SSL_KEY KeyDesc;
            public HTTP_SERVICE_CONFIG_SSL_PARAM ParamDesc;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct HTTP_SERVICE_CONFIG_SSL_KEY
        {
            public IntPtr pIpPort;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        public struct HTTP_SERVICE_CONFIG_URLACL_KEY
        {
            [MarshalAs(UnmanagedType.LPWStr)]
            public string pUrlPrefix;

            public HTTP_SERVICE_CONFIG_URLACL_KEY(string urlPrefix)
            {
                pUrlPrefix = urlPrefix;
            }
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        struct HTTP_SERVICE_CONFIG_URLACL_PARAM
        {
            [MarshalAs(UnmanagedType.LPWStr)]
            public string pStringSecurityDescriptor;

            public HTTP_SERVICE_CONFIG_URLACL_PARAM(string securityDescriptor)
            {
                pStringSecurityDescriptor = securityDescriptor;
            }
        }

        [StructLayout(LayoutKind.Sequential)]
        struct HTTP_SERVICE_CONFIG_URLACL_SET
        {
            public HTTP_SERVICE_CONFIG_URLACL_KEY KeyDesc;
            public HTTP_SERVICE_CONFIG_URLACL_PARAM ParamDesc;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        struct HTTP_SERVICE_CONFIG_SSL_PARAM
        {
            public int SslHashLength;
            public IntPtr pSslHash;
            public Guid AppId;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string pSslCertStoreName;
            public uint DefaultCertCheckMode;
            public int DefaultRevocationFreshnessTime;
            public int DefaultRevocationUrlRetrievalTimeout;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string pDefaultSslCtlIdentifier;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string pDefaultSslCtlStoreName;
            public uint DefaultFlags;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 2)]
        public struct HTTPAPI_VERSION
        {
            public ushort HttpApiMajorVersion;
            public ushort HttpApiMinorVersion;

            public HTTPAPI_VERSION(ushort majorVersion, ushort minorVersion)
            {
                HttpApiMajorVersion = majorVersion;
                HttpApiMinorVersion = minorVersion;
            }
        }

        #endregion

        #region Constants

        private const string TraceType = "FabricBRSSetup_PortAcl";
        private static readonly FabricEvents.ExtensionsEvents TraceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.BackupRestoreService);

        public const uint HTTP_INITIALIZE_CONFIG = 0x00000002;
        public const uint HTTP_SERVICE_CONFIG_SSL_FLAG_USE_DS_MAPPER = 0x00000001;
        public const uint HTTP_SERVICE_CONFIG_SSL_FLAG_NEGOTIATE_CLIENT_CERT = 0x00000002;
        public const uint HTTP_SERVICE_CONFIG_SSL_FLAG_NO_RAW_FILTER = 0x00000004;
        private static int NOERROR = 0;
        private static int ERROR_FILE_NOT_FOUND = 2;
        private static int ERROR_ALREADY_EXISTS = 183;
        
        #endregion

        #region Public methods

        public static void ReserveUrl(string networkUrl, string securityDescriptor)
        {
            TraceSource.WriteInfo(
                PortAclUtility.TraceType,
                "Started to Reserve Url. networkUrl: {0}, securityDescriptor: {1}.",
                networkUrl,
                securityDescriptor);

            uint retVal = (uint)NOERROR; // NOERROR = 0

            HTTPAPI_VERSION httpApiVersion = new HTTPAPI_VERSION(1, 0);

            retVal = HttpInitialize(httpApiVersion, HTTP_INITIALIZE_CONFIG, IntPtr.Zero);

            TraceSource.WriteInfo(
                PortAclUtility.TraceType,
                "HttpInitialize completed with return value: {0}.",
                retVal);

            if ((uint)NOERROR == retVal)
            {
                HTTP_SERVICE_CONFIG_URLACL_KEY keyDesc = new HTTP_SERVICE_CONFIG_URLACL_KEY(networkUrl);
                HTTP_SERVICE_CONFIG_URLACL_PARAM paramDesc = new HTTP_SERVICE_CONFIG_URLACL_PARAM(securityDescriptor);

                HTTP_SERVICE_CONFIG_URLACL_SET inputConfigInfoSet = new HTTP_SERVICE_CONFIG_URLACL_SET
                {
                    KeyDesc = keyDesc,
                    ParamDesc = paramDesc
                };

                IntPtr pInputConfigInfo = Marshal.AllocCoTaskMem(Marshal.SizeOf(typeof(HTTP_SERVICE_CONFIG_URLACL_SET)));
                Marshal.StructureToPtr(inputConfigInfoSet, pInputConfigInfo, false);

                retVal = HttpSetServiceConfiguration(IntPtr.Zero,
                    HTTP_SERVICE_CONFIG_ID.HttpServiceConfigUrlAclInfo,
                    pInputConfigInfo,
                    Marshal.SizeOf(inputConfigInfoSet),
                    IntPtr.Zero);

                TraceSource.WriteInfo(
                    PortAclUtility.TraceType,
                    "HttpSetServiceConfiguration completed with return value: {0}.",
                    retVal);

                if ((uint)ERROR_ALREADY_EXISTS == retVal)  // ERROR_ALREADY_EXISTS = 183
                {
                    retVal = HttpDeleteServiceConfiguration(IntPtr.Zero,
                        HTTP_SERVICE_CONFIG_ID.HttpServiceConfigUrlAclInfo,
                        pInputConfigInfo,
                        Marshal.SizeOf(inputConfigInfoSet),
                        IntPtr.Zero);

                    TraceSource.WriteInfo(
                        PortAclUtility.TraceType,
                        "HttpDeleteServiceConfiguration(outer) completed with return value: {0}.",
                        retVal);

                    if ((uint)ERROR_FILE_NOT_FOUND == retVal)
                    {
                        // This means its possible that the URL is ACLed for the same port but different protocol (http/https)
                        // Try deleting ACL using the other protocol
                        var networkUrlInner = networkUrl.ToLower();
                        if (networkUrlInner.Contains("https"))
                        {
                            networkUrlInner = networkUrlInner.Replace("https", "http");
                        }
                        else
                        {
                            networkUrlInner = networkUrlInner.Replace("http", "https");
                        }

                        HTTP_SERVICE_CONFIG_URLACL_KEY keyDescInner = new HTTP_SERVICE_CONFIG_URLACL_KEY(networkUrlInner);
                        HTTP_SERVICE_CONFIG_URLACL_SET inputConfigInfoSetInner = new HTTP_SERVICE_CONFIG_URLACL_SET
                        {
                            KeyDesc = keyDescInner
                        };

                        IntPtr pInputConfigInfoInner = IntPtr.Zero;
                        try
                        {
                            pInputConfigInfoInner = Marshal.AllocCoTaskMem(Marshal.SizeOf(typeof(HTTP_SERVICE_CONFIG_URLACL_SET)));
                            Marshal.StructureToPtr(inputConfigInfoSetInner, pInputConfigInfoInner, false);

                            retVal = HttpDeleteServiceConfiguration(IntPtr.Zero,
                                HTTP_SERVICE_CONFIG_ID.HttpServiceConfigUrlAclInfo,
                                pInputConfigInfoInner,
                                Marshal.SizeOf(inputConfigInfoSetInner),
                                IntPtr.Zero);

                            TraceSource.WriteWarning(
                                PortAclUtility.TraceType,
                                "HttpDeleteServiceConfiguration(Inner) completed with return value: {0}.",
                                retVal);
                        }
                        finally
                        {
                            if (IntPtr.Zero != pInputConfigInfoInner)
                            {
                                Marshal.FreeCoTaskMem(pInputConfigInfoInner);
                            }
                        }
                    }

                    if ((uint)NOERROR == retVal)
                    {
                        retVal = HttpSetServiceConfiguration(IntPtr.Zero,
                            HTTP_SERVICE_CONFIG_ID.HttpServiceConfigUrlAclInfo,
                            pInputConfigInfo,
                            Marshal.SizeOf(inputConfigInfoSet),
                            IntPtr.Zero);

                        TraceSource.WriteInfo(
                            PortAclUtility.TraceType,
                            "HttpSetServiceConfiguration completed with return value: {0}.",
                            retVal);
                    }
                }

                Marshal.FreeCoTaskMem(pInputConfigInfo);
                HttpTerminate(HTTP_INITIALIZE_CONFIG, IntPtr.Zero);
            }

            if ((uint)NOERROR != retVal)
            {
                TraceSource.WriteError(
                    PortAclUtility.TraceType,
                    "ReserveUrl failed with error: {0}.",
                    retVal);
                throw new Win32Exception(Convert.ToInt32(retVal));
            }
        }


        public static void BindCertificate(string ipAddress, int port, byte[] hash)
        {
            TraceSource.WriteInfo(
                PortAclUtility.TraceType,
                "Started to Bind certificate to port. ipAddress: {0}, port: {1}, hash: {2}. Current thread identity: {3}",
                ipAddress,
                port,
                BitConverter.ToString(hash),
                Thread.CurrentPrincipal.Identity.Name);

            uint retVal = (uint)NOERROR; // NOERROR = 0

            HTTPAPI_VERSION httpApiVersion = new HTTPAPI_VERSION(1, 0);
            retVal = HttpInitialize(httpApiVersion, HTTP_INITIALIZE_CONFIG, IntPtr.Zero);

            TraceSource.WriteInfo(
                PortAclUtility.TraceType,
                "HttpInitialize completed with return value: {0}.",
                retVal);

            if ((uint)NOERROR == retVal)
            {
                HTTP_SERVICE_CONFIG_SSL_SET configSslSet = new HTTP_SERVICE_CONFIG_SSL_SET();
                HTTP_SERVICE_CONFIG_SSL_KEY httpServiceConfigSslKey = new HTTP_SERVICE_CONFIG_SSL_KEY();
                HTTP_SERVICE_CONFIG_SSL_PARAM configSslParam = new HTTP_SERVICE_CONFIG_SSL_PARAM();

                IPAddress ip = IPAddress.Any; //IPAddress.Parse("0.0.0.0");

                IPEndPoint ipEndPoint = new IPEndPoint(ip, port);
                // serialize the endpoint to a SocketAddress and create an array to hold the values.  Pin the array.
                SocketAddress socketAddress = ipEndPoint.Serialize();
                byte[] socketBytes = new byte[socketAddress.Size];
                GCHandle handleSocketAddress = GCHandle.Alloc(socketBytes, GCHandleType.Pinned);
                // Should copy the first 16 bytes (the SocketAddress has a 32 byte buffer, the size will only be 16,
                //which is what the SOCKADDR accepts
                for (int i = 0; i < socketAddress.Size; ++i)
                {
                    socketBytes[i] = socketAddress[i];
                }

                httpServiceConfigSslKey.pIpPort = handleSocketAddress.AddrOfPinnedObject();

                GCHandle handleHash = GCHandle.Alloc(hash, GCHandleType.Pinned);
                configSslParam.AppId = Guid.NewGuid();
                configSslParam.DefaultCertCheckMode = 0;
                configSslParam.DefaultFlags = HTTP_SERVICE_CONFIG_SSL_FLAG_NEGOTIATE_CLIENT_CERT;
                configSslParam.DefaultRevocationFreshnessTime = 0;
                configSslParam.DefaultRevocationUrlRetrievalTimeout = 0;
                configSslParam.pSslCertStoreName = StoreName.My.ToString();
                configSslParam.pSslHash = handleHash.AddrOfPinnedObject();
                configSslParam.SslHashLength = hash.Length;
                configSslSet.ParamDesc = configSslParam;
                configSslSet.KeyDesc = httpServiceConfigSslKey;

                IntPtr pInputConfigInfo = Marshal.AllocCoTaskMem(Marshal.SizeOf(typeof(HTTP_SERVICE_CONFIG_SSL_SET)));
                Marshal.StructureToPtr(configSslSet, pInputConfigInfo, false);

                retVal = HttpSetServiceConfiguration(IntPtr.Zero,
                    HTTP_SERVICE_CONFIG_ID.HttpServiceConfigSSLCertInfo,
                    pInputConfigInfo,
                    Marshal.SizeOf(configSslSet),
                    IntPtr.Zero);

                TraceSource.WriteInfo(
                    PortAclUtility.TraceType,
                    "HttpSetServiceConfiguration completed with return value: {0}.",
                    retVal);

                if ((uint)ERROR_ALREADY_EXISTS == retVal)  // ERROR_ALREADY_EXISTS = 183
                {
                    retVal = HttpDeleteServiceConfiguration(IntPtr.Zero,
                    HTTP_SERVICE_CONFIG_ID.HttpServiceConfigSSLCertInfo,
                    pInputConfigInfo,
                    Marshal.SizeOf(configSslSet),
                    IntPtr.Zero);

                    TraceSource.WriteInfo(
                        PortAclUtility.TraceType,
                        "HttpDeleteServiceConfiguration completed with return value: {0}.",
                        retVal);

                    if ((uint)NOERROR == retVal)
                    {
                        retVal = HttpSetServiceConfiguration(IntPtr.Zero,
                            HTTP_SERVICE_CONFIG_ID.HttpServiceConfigSSLCertInfo,
                            pInputConfigInfo,
                            Marshal.SizeOf(configSslSet),
                            IntPtr.Zero);

                        TraceSource.WriteInfo(
                            PortAclUtility.TraceType,
                            "HttpSetServiceConfiguration completed with return value: {0}.",
                            retVal);
                    }
                }

                handleSocketAddress.Free();
                handleHash.Free();
                Marshal.FreeCoTaskMem(pInputConfigInfo);
                HttpTerminate(HTTP_INITIALIZE_CONFIG, IntPtr.Zero);
            }

            if ((uint)NOERROR != retVal)
            {
                TraceSource.WriteError(
                    PortAclUtility.TraceType,
                    "BindCertificate failed with error: {0}.",
                    retVal);

                throw new Win32Exception(Convert.ToInt32(retVal));
            }
        }

        #endregion
    }
}