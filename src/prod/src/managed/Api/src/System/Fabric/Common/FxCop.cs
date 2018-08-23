// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    internal static class FxCop
    {
        public static class Category
        {
            public const string Design = "Microsoft.Design";
            public const string Globalization = "Microsoft.Globalization";
            public const string Maintainability = "Microsoft.Maintainability";
            public const string MSInternal = "Microsoft.MSInternal";
            public const string Naming = "Microsoft.Naming";
            public const string Performance = "Microsoft.Performance";
            public const string Reliability = "Microsoft.Reliability";
            public const string Security = "Microsoft.Security";
            public const string Usage = "Microsoft.Usage";
            public const string Configuration = "Configuration";
            public const string Xaml = "XAML";
            public const string Interoperability = "Microsoft.Interoperability";
            public const string ReliabilityBasic = "Reliability";
            public const string StyleCopMaintainability = "StyleCop.CSharp.MaintainabilityRules";
        }

        public static class Rule
        {
            public const string AptcaMethodsShouldOnlyCallAptcaMethods = "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods";
            public const string AssembliesShouldHaveValidStrongNames = "CA2210:AssembliesShouldHaveValidStrongNames";
            public const string AttributeStringLiteralsShouldParseCorrectly = "CA2243:AttributeStringLiteralsShouldParseCorrectly";
            public const string AvoidCallingProblematicMethods = "CA2001:AvoidCallingProblematicMethods";
            public const string AvoidExcessiveClassCoupling = "CA1506:AvoidExcessiveClassCoupling";
            public const string AvoidExcessiveComplexity = "CA1502:AvoidExcessiveComplexity";
            public const string AvoidExcessiveParametersOnGenericTypes = "CA1005:AvoidExcessiveParametersOnGenericTypes";
            public const string AvoidNamespacesWithFewTypes = "CA1020:AvoidNamespacesWithFewTypes";
            public const string AvoidOutParameters = "CA1021:AvoidOutParameters";
            public const string AvoidTypesThatRequireJitCompilationInPrecompiledAssemblies = "CA908:AvoidTypesThatRequireJitCompilationInPrecompiledAssemblies";
            public const string AvoidUncalledPrivateCode = "CA1811:AvoidUncalledPrivateCode";
            public const string AvoidUninstantiatedInternalClasses = "CA1812:AvoidUninstantiatedInternalClasses";
            public const string AvoidUnmaintainableCode = "CA1505:AvoidUnmaintainableCode";
            public const string AvoidUnusedPrivateFields = "CA1823:AvoidUnusedPrivateFields";
            public const string CallGCSuppressFinalizeCorrectly = "CA1816:CallGCSuppressFinalizeCorrectly";
            public const string CollectionPropertiesShouldBeReadOnly = "CA2227:CollectionPropertiesShouldBeReadOnly";
            public const string CollectionsShouldImplementGenericInterface = "CA1010:CollectionsShouldImplementGenericInterface";
            public const string ResourceStringCompoundWordsShouldBeCasedCorrectly = "CA1701:ResourceStringCompoundWordsShouldBeCasedCorrectly";
            public const string CompoundWordsShouldBeCasedCorrectly = "CA1702:CompoundWordsShouldBeCasedCorrectly";
            public const string ConfigurationPropertyAttributeRule = "Configuration102:ConfigurationPropertyAttributeRule";
            public const string ConfigurationValidatorAttributeRule = "Configuration104:ConfigurationValidatorAttributeRule";
            public const string ConsiderPassingBaseTypesAsParameters = "CA1011:ConsiderPassingBaseTypesAsParameters";
            public const string DefaultParametersShouldNotBeUsed = "CA1026:DefaultParametersShouldNotBeUsed";
            public const string DefineAccessorsForAttributeArguments = "CA1019:DefineAccessorsForAttributeArguments";
            public const string DisposableFieldsShouldBeDisposed = "CA2213:DisposableFieldsShouldBeDisposed";
            public const string DoNotCallOverridableMethodsInConstructors = "CA2214:DoNotCallOverridableMethodsInConstructors";
            public const string DoNotCastUnnecessarily = "CA1800:DoNotCastUnnecessarily";
            public const string DoNotCatchGeneralExceptionTypes = "CA1031:DoNotCatchGeneralExceptionTypes";
            public const string DoNotDeclareReadOnlyMutableReferenceTypes = "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes";
            public const string DoNotExposeGenericLists = "CA1002:DoNotExposeGenericLists";
            public const string DoNotHideBaseClassMethods = "CA1061:DoNotHideBaseClassMethods";
            public const string DoNotLockOnObjectsWithWeakIdentity = "CA2002:DoNotLockOnObjectsWithWeakIdentity";
            public const string DoNotNestGenericTypesInMemberSignatures = "CA1006:DoNotNestGenericTypesInMemberSignatures";
            public const string DoNotIndirectlyExposeMethodsWithLinkDemands = "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands";
            public const string DoNotInitializeUnnecessarily = "CA1805:DoNotInitializeUnnecessarily";
            public const string DoNotIgnoreMethodResults = "CA1806:DoNotIgnoreMethodResults";
            public const string DoNotPassLiteralsAsLocalizedParameters = "CA1303:DoNotPassLiteralsAsLocalizedParameters";
            public const string DoNotRaiseExceptionsInUnexpectedLocations = "CA1065:DoNotRaiseExceptionsInUnexpectedLocations";
            public const string DoNotRaiseExceptionsInExceptionClauses = "CA2219:DoNotRaiseExceptionsInExceptionClauses";
            public const string DoNotRaiseReservedExceptionTypes = "CA2201:DoNotRaiseReservedExceptionTypes";
            public const string ExceptionsShouldBePublic = "CA1064:ExceptionsShouldBePublic";
            public const string GenericMethodsShouldProvideTypeParameter = "CA1004:GenericMethodsShouldProvideTypeParameter";
            public const string IdentifiersShouldBeSpelledCorrectly = "CA1704:IdentifiersShouldBeSpelledCorrectly";
            public const string IdentifiersShouldHaveCorrectPrefix = "CA1715:IdentifiersShouldHaveCorrectPrefix";
            public const string IdentifiersShouldHaveCorrectSuffix = "CA1710:IdentifiersShouldHaveCorrectSuffix";
            public const string IdentifiersShouldNotContainTypeNames = "CA1720:IdentifiersShouldNotContainTypeNames";
            public const string IdentifiersShouldNotMatchKeywords = "CA1716:IdentifiersShouldNotMatchKeywords";
            public const string RemoveUnusedLocals = "CA1804:RemoveUnusedLocals";
            public const string IdentifiersShouldBeCasedCorrectly = "CA1709:IdentifiersShouldBeCasedCorrectly";
            public const string ImplementSerializationMethodsCorrectly = "CA2238:ImplementSerializationMethodsCorrectly";
            public const string ImplementIDisposableCorrectly = "CA1063:ImplementIDisposableCorrectly";
            public const string InitializeReferenceTypeStaticFieldsInline = "CA1810:InitializeReferenceTypeStaticFieldsInline";
            public const string InstantiateArgumentExceptionsCorrectly = "CA2208:InstantiateArgumentExceptionsCorrectly";
            public const string InterfaceMethodsShouldBeCallableByChildTypes = "CA1033:InterfaceMethodsShouldBeCallableByChildTypes";
            public const string IsFatalRule = "Reliability108:IsFatalRule";
            public const string MarkBooleanPInvokeArgumentsWithMarshalAs = "CA1414:MarkBooleanPInvokeArgumentsWithMarshalAs";
            public const string MarkMembersAsStatic = "CA1822:MarkMembersAsStatic";
            public const string MovePInvokesToNativeMethodsClass = "CA1060:MovePInvokesToNativeMethodsClass";
            public const string NestedTypesShouldNotBeVisible = "CA1034:NestedTypesShouldNotBeVisible";
            public const string SpecifyIFormatProvider = "CA1305:SpecifyIFormatProvider";
            public const string NormalizeStringsToUppercase = "CA1308:NormalizeStringsToUppercase";
            public const string OverrideLinkDemandsShouldBeIdenticalToBase = "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase";
            public const string OperatorOverloadsHaveNamedAlternates = "CA2225:OperatorOverloadsHaveNamedAlternates";
            public const string ParameterNamesShouldMatchBaseDeclaration = "CA1725:ParameterNamesShouldMatchBaseDeclaration";
            public const string PInvokeEntryPointsShouldExist = "CA1400:PInvokeEntryPointsShouldExist";
            public const string PreferJaggedArraysOverMultidimensional = "CA1814:PreferJaggedArraysOverMultidimensional";
            public const string PropertiesShouldNotReturnArrays = "CA1819:PropertiesShouldNotReturnArrays";
            public const string PropertyNamesShouldNotMatchGetMethods = "CA1721:PropertyNamesShouldNotMatchGetMethods";
            public const string ReplaceRepetitiveArgumentsWithParamsArray = "CA1025:ReplaceRepetitiveArgumentsWithParamsArray";
            public const string ResourceStringsShouldBeSpelledCorrectly = "CA1703:ResourceStringsShouldBeSpelledCorrectly";
            public const string ReviewUnusedParameters = "CA1801:ReviewUnusedParameters";
            public const string ReviewSuppressUnmanagedCodeSecurityUsage = "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage";
            public const string SecureAsserts = "CA2106:SecureAsserts";
            public const string SpecifyMarshalingForPInvokeStringArguments = "CA2101:SpecifyMarshalingForPInvokeStringArguments";
            public const string SystemNamespacesRequireApproval = "CA905:SystemNamespacesRequireApproval";
            public const string TypeConvertersMustBePublic = "XAML1004:TypeConvertersMustBePublic";
            public const string TypeNamesShouldNotMatchNamespaces = "CA1724:TypeNamesShouldNotMatchNamespaces";
            public const string TypesMustHaveXamlCallableConstructors = "XAML1007:TypesMustHaveXamlCallableConstructors";
            public const string TypesThatOwnDisposableFieldsShouldBeDisposable = "CA1001:TypesThatOwnDisposableFieldsShouldBeDisposable";
            public const string UseApprovedGenericsForPrecompiledAssemblies = "CA908:UseApprovedGenericsForPrecompiledAssemblies";
            public const string UseManagedEquivalentsOfWin32Api = "CA2205:UseManagedEquivalentsOfWin32Api";
            public const string UseGenericsWhereAppropriate = "CA1007:UseGenericsWhereAppropriate";
            public const string UsePropertiesWhereAppropriate = "CA1024:UsePropertiesWhereAppropriate";
            public const string UriParametersShouldNotBeStrings = "CA1054:UriParametersShouldNotBeStrings";
            public const string UriPropertiesShouldNotBeStrings = "CA1056:UriPropertiesShouldNotBeStrings";
            public const string UseNewGuidHelperRule = "Reliability113:UseNewGuidHelperRule";
            public const string VariableNamesShouldNotMatchFieldNames = "CA1500:VariableNamesShouldNotMatchFieldNames";           
            public const string WrapExceptionsRule = "Reliability102:WrapExceptionsRule";
            public const string RemoveCallsToGCKeepAlive = "CA2004:RemoveCallsToGCKeepAlive";
            public const string TestForEmptyStringsUsingStringLength = "CA1820:TestForEmptyStringsUsingStringLength";
            public const string FileMayOnlyContainASingleClass = "SA1402:FileMayOnlyContainASingleClass";
        }

        public static class Scope
        { 
            public const string Member = "member";
            public const string Method = "method";
            public const string Module = "module";
            public const string Namespace = "namespace";
            public const string Resource = "resource";
            public const string Type = "type";
        }

        public static class Justification
        {
            public const string EnsureNativeBuffersLifetime = "Ensure that the native buffers whose lifetime is bound to the interface, do not get freed prematurely.";
            public const string UncallableCodeDueToSourceInclude = "This source file is included in building more than one assembly. The function is not called in one of the assembly, but used in the other ones.";
        }
    }
}