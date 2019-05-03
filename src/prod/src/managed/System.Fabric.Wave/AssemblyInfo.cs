// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Diagnostics.CodeAnalysis;
using System.Fabric.Common;
using System.Reflection;
using System.Resources;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

// General Information about an assembly is controlled through the following 
// set of attributes. Change these attribute values to modify the information
// associated with an assembly.
[assembly: AssemblyCulture("")]

[assembly: NeutralResourcesLanguageAttribute("en-US")]

// FxCop suppressions
[assembly: SuppressMessage(FxCop.Category.Design, FxCop.Rule.AssembliesShouldHaveValidStrongNames, Justification = "These assemblies are delay-signed.")]