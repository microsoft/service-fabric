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

// The following GUID is for the ID of the typelib if this project is exposed to COM
[assembly: Guid("ca453078-d477-4335-8ba9-6a288e9aa346")]

[assembly: NeutralResourcesLanguageAttribute("en-US")]

// Friend assemblies - product
[assembly: InternalsVisibleTo("System.Fabric.JsonImpl, PublicKey=0024000004800000940000000602000000240000525341310004000001000100b5fc90e7027f67871e773a8fde8938c81dd402ba65b9201d60593e96c492651e889cc13f1415ebb53fac1131ae0bd333c5ee6021672d9718ea31a8aebd0da0072f25d87dba6fc90ffd598ed4da35e44c398c454307e8e33b8426143daec9f596836f97c8f74750e5975c64e2189f45def46b2a2b1247adc3652bf5c308055da9")]
[assembly: InternalsVisibleTo("AzureCliProxy, PublicKey=0024000004800000940000000602000000240000525341310004000001000100b5fc90e7027f67871e773a8fde8938c81dd402ba65b9201d60593e96c492651e889cc13f1415ebb53fac1131ae0bd333c5ee6021672d9718ea31a8aebd0da0072f25d87dba6fc90ffd598ed4da35e44c398c454307e8e33b8426143daec9f596836f97c8f74750e5975c64e2189f45def46b2a2b1247adc3652bf5c308055da9")]
[assembly: InternalsVisibleTo("System.Fabric.WRPCommon, PublicKey=0024000004800000940000000602000000240000525341310004000001000100b5fc90e7027f67871e773a8fde8938c81dd402ba65b9201d60593e96c492651e889cc13f1415ebb53fac1131ae0bd333c5ee6021672d9718ea31a8aebd0da0072f25d87dba6fc90ffd598ed4da35e44c398c454307e8e33b8426143daec9f596836f97c8f74750e5975c64e2189f45def46b2a2b1247adc3652bf5c308055da9")]
[assembly: InternalsVisibleTo("System.Fabric.Management, PublicKey=0024000004800000940000000602000000240000525341310004000001000100b5fc90e7027f67871e773a8fde8938c81dd402ba65b9201d60593e96c492651e889cc13f1415ebb53fac1131ae0bd333c5ee6021672d9718ea31a8aebd0da0072f25d87dba6fc90ffd598ed4da35e44c398c454307e8e33b8426143daec9f596836f97c8f74750e5975c64e2189f45def46b2a2b1247adc3652bf5c308055da9")]
[assembly: InternalsVisibleTo("System.Fabric.WRPService.Common, PublicKey=0024000004800000940000000602000000240000525341310004000001000100b5fc90e7027f67871e773a8fde8938c81dd402ba65b9201d60593e96c492651e889cc13f1415ebb53fac1131ae0bd333c5ee6021672d9718ea31a8aebd0da0072f25d87dba6fc90ffd598ed4da35e44c398c454307e8e33b8426143daec9f596836f97c8f74750e5975c64e2189f45def46b2a2b1247adc3652bf5c308055da9")]
[assembly: InternalsVisibleTo("System.Fabric.Replicator, PublicKey=0024000004800000940000000602000000240000525341310004000001000100b5fc90e7027f67871e773a8fde8938c81dd402ba65b9201d60593e96c492651e889cc13f1415ebb53fac1131ae0bd333c5ee6021672d9718ea31a8aebd0da0072f25d87dba6fc90ffd598ed4da35e44c398c454307e8e33b8426143daec9f596836f97c8f74750e5975c64e2189f45def46b2a2b1247adc3652bf5c308055da9")]
[assembly: InternalsVisibleTo("System.Fabric.ReplicatedStore, PublicKey=0024000004800000940000000602000000240000525341310004000001000100b5fc90e7027f67871e773a8fde8938c81dd402ba65b9201d60593e96c492651e889cc13f1415ebb53fac1131ae0bd333c5ee6021672d9718ea31a8aebd0da0072f25d87dba6fc90ffd598ed4da35e44c398c454307e8e33b8426143daec9f596836f97c8f74750e5975c64e2189f45def46b2a2b1247adc3652bf5c308055da9")]
[assembly: InternalsVisibleTo("System.Fabric.Wave, PublicKey=0024000004800000940000000602000000240000525341310004000001000100b5fc90e7027f67871e773a8fde8938c81dd402ba65b9201d60593e96c492651e889cc13f1415ebb53fac1131ae0bd333c5ee6021672d9718ea31a8aebd0da0072f25d87dba6fc90ffd598ed4da35e44c398c454307e8e33b8426143daec9f596836f97c8f74750e5975c64e2189f45def46b2a2b1247adc3652bf5c308055da9")]
[assembly: InternalsVisibleTo("Microsoft.ServiceFabric.Powershell, PublicKey=0024000004800000940000000602000000240000525341310004000001000100b5fc90e7027f67871e773a8fde8938c81dd402ba65b9201d60593e96c492651e889cc13f1415ebb53fac1131ae0bd333c5ee6021672d9718ea31a8aebd0da0072f25d87dba6fc90ffd598ed4da35e44c398c454307e8e33b8426143daec9f596836f97c8f74750e5975c64e2189f45def46b2a2b1247adc3652bf5c308055da9")]
[assembly: InternalsVisibleTo("Microsoft.ServiceFabric.PowerTools, PublicKey=0024000004800000940000000602000000240000525341310004000001000100b5fc90e7027f67871e773a8fde8938c81dd402ba65b9201d60593e96c492651e889cc13f1415ebb53fac1131ae0bd333c5ee6021672d9718ea31a8aebd0da0072f25d87dba6fc90ffd598ed4da35e44c398c454307e8e33b8426143daec9f596836f97c8f74750e5975c64e2189f45def46b2a2b1247adc3652bf5c308055da9")]
[assembly: InternalsVisibleTo("Microsoft.ServiceFabric.Preview, PublicKey=0024000004800000940000000602000000240000525341310004000001000100b5fc90e7027f67871e773a8fde8938c81dd402ba65b9201d60593e96c492651e889cc13f1415ebb53fac1131ae0bd333c5ee6021672d9718ea31a8aebd0da0072f25d87dba6fc90ffd598ed4da35e44c398c454307e8e33b8426143daec9f596836f97c8f74750e5975c64e2189f45def46b2a2b1247adc3652bf5c308055da9")]
    
// FxCop suppressions
[assembly: SuppressMessage(FxCop.Category.Design, FxCop.Rule.AssembliesShouldHaveValidStrongNames, Justification = "These assemblies are delay-signed.")]
[assembly: SuppressMessage(FxCop.Category.Design, FxCop.Rule.AvoidNamespacesWithFewTypes, Justification = "Classes are grouped logically for user clarity.", Scope = "Namespace", Target = "System.Fabric.Strings")]
[assembly: SuppressMessage(FxCop.Category.Design, FxCop.Rule.AvoidNamespacesWithFewTypes, Justification = "Classes are grouped logically for user clarity.", Scope = "Namespace", Target = "Microsoft.Fabric.Runtime")]
[assembly: SuppressMessage(FxCop.Category.Design, FxCop.Rule.AvoidNamespacesWithFewTypes, Justification = "Classes are grouped logically for user clarity.", Scope = "Namespace", Target = "System.Fabric.Hosting")]
[assembly: SuppressMessage(FxCop.Category.Design, FxCop.Rule.AvoidNamespacesWithFewTypes, Justification = "Classes are grouped logically for user clarity.", Scope = "Namespace", Target = "Microsoft.Fabric.Cas")]
[assembly: SuppressMessage(FxCop.Category.Naming, FxCop.Rule.IdentifiersShouldBeSpelledCorrectly, Scope = "member", Target = "System.Fabric.Strings.SerializationFormat.Rss", Justification = "Rss is the right naming")]
[assembly: SuppressMessage(FxCop.Category.MSInternal, FxCop.Rule.AvoidTypesThatRequireJitCompilationInPrecompiledAssemblies, Justification = "This assembly is not precompiled", Scope = "Member",
    Target = "System.Fabric.Hosting.HostEntryPointManager.#EndActivate(System.Fabric.Interop.NativeCommon+IFabricAsyncOperationContext)")]
[assembly: SuppressMessage(FxCop.Category.MSInternal, FxCop.Rule.AvoidTypesThatRequireJitCompilationInPrecompiledAssemblies, Justification = "This assembly is not precompiled", Scope = "Member",
    Target = "System.Fabric.Hosting.HostEntryPointManager.#BeginActivate(Microsoft.Fabric.Runtime.NativeRuntime+IFabricCodePackage,Microsoft.Fabric.Runtime.NativeRuntime+IFabricCodePackageActivationContext,Microsoft.Fabric.Runtime.NativeRuntime+IFabricRuntime,System.Fabric.Interop.NativeCommon+IFabricAsyncOperationCallback)")]
[assembly: SuppressMessage(FxCop.Category.MSInternal, FxCop.Rule.AvoidTypesThatRequireJitCompilationInPrecompiledAssemblies, Justification = "This assembly is not precompiled", Scope = "Member",
    Target = "System.Fabric.Hosting.HostEntryPointManager.#BeginDeactivate(System.String,System.Fabric.Interop.NativeCommon+IFabricAsyncOperationCallback)")]
[assembly: SuppressMessage(FxCop.Category.MSInternal, FxCop.Rule.AvoidTypesThatRequireJitCompilationInPrecompiledAssemblies, Justification = "This assembly is not precompiled", Scope = "Member",
    Target = "System.Fabric.Hosting.HostEntryPointManager.#EndDeactivate(System.Fabric.Interop.NativeCommon+IFabricAsyncOperationContext)")]