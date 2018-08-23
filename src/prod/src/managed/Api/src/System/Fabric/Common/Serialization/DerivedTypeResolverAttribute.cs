// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Serialization
{
    // This attribute will used by method(s) that resolves derived type using enum/kind value.
    // The method should be in the base class -- usually will be an abstract class.
    // The method must be static with exactly one parameter (usually an enum/kind) and return type should be "Type".
    // The method will use this enum/kind value (obtained from json string) to resolve the actual implementing type for that kind hence for the json string.
    // The base class containing the method also needs to have "KnownType" attributes specifying which are the known implementing types exists.
    // This attribute constructor takes parameter string 'resolverJsonPropertyName' whose value should be the name of the json property of the enum/kind in json string for the class.
    // The method will be called by JsonSerializer/KnownTypeJsonConverter to resolve derived type with parameter as value of matching json property.
    [global::System.AttributeUsage(AttributeTargets.Method, Inherited = false, AllowMultiple = false)]
    sealed internal class DerivedTypeResolverAttribute : Attribute
    {
        readonly string resolverPropertyName;

        // This is a positional argument
        public DerivedTypeResolverAttribute(string resolverJsonPropertyName)
        {
            this.resolverPropertyName = resolverJsonPropertyName ?? string.Empty;
        }

        public string ResolverJsonPropertyName
        {
            get { return resolverPropertyName; }
        }
    }
}