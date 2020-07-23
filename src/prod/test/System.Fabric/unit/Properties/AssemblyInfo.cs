// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Reflection;
using System.Runtime.InteropServices;
using TestHost = Microsoft.Infrastructure.Test;
using System.Runtime.CompilerServices;

// General Information about an assembly is controlled through the following 
// set of attributes. Change these attribute values to modify the information
// associated with an assembly.
[assembly: AssemblyCulture("")]

// The following GUID is for the ID of the typelib if this project is exposed to COM
[assembly: Guid("ea7b774d-0995-4e4b-8c43-19e8d37a9c95")]

[assembly: TestHost.TestAssembly(
    Startup = "AssemblyInitialize", Cleanup = "AssemblyCleanup",
    Type = "Microsoft.Distributed.UnitTests.AssemblyInitializeCleanup")]
[assembly: InternalsVisibleTo("HostEntryPointWorkerProcess, PublicKey=0024000004800000940000000602000000240000525341310004000001000100197C25D0A04F73CB271E8181DBA1C0C713DF8DEEBB25864541A66670500F34896D280484B45FE1FF6C29F2EE7AA175D8BCBD0C83CC23901A894A86996030F6292CE6EDA6E6F3E6C74B3C5A3DED4903C951E6747E6102969503360F7781BF8BF015058EB89B7621798CCC85AACA036FF1BC1556BB7F62DE15908484886AA8BBAE")]
[assembly: InternalsVisibleTo("Microsoft.ApplicationServer.CompAppTestHost, PublicKey=0024000004800000940000000602000000240000525341310004000001000100197C25D0A04F73CB271E8181DBA1C0C713DF8DEEBB25864541A66670500F34896D280484B45FE1FF6C29F2EE7AA175D8BCBD0C83CC23901A894A86996030F6292CE6EDA6E6F3E6C74B3C5A3DED4903C951E6747E6102969503360F7781BF8BF015058EB89B7621798CCC85AACA036FF1BC1556BB7F62DE15908484886AA8BBAE")]

// Version information for an assembly consists of the following four values:
//
//      Major Version
//      Minor Version 
//      Build Number
//      Revision
//
// You can specify all the values or you can default the Build and Revision Numbers 
// by using the '*' as shown below:
// [assembly: AssemblyFileVersion("1.0.0.0")]