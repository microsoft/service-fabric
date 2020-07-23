// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using System.Reflection;
    using System.Threading;

    [TestClass]
    public class ManagedCodeCompatibilityChecker
    {
        private static string[] assemblyNames = { "System.Fabric.dll" };
        private static string previousVersionLocation = @"WinfabBinariesFromPreviousRelease\";
        private static string fileRoot = @"%_NTTREE%\FabricUnitTests";

        //
        // This is to be used incase there are types or methods that are intentionally modified/removed.
        //
        static string[] ExcludedTypes = { /* <TypeName> */ };
        static Tuple<string, string>[] ExcludedMethods = {
                                                             //, new Tuple<string, string>(<TypeName>, <MethodName>)
                                                         };

        [TestMethod]
        public void ValidateManagedCode()
        {
            foreach (string assemblyName in assemblyNames)
            {
                string previousVersionAssembly = Path.Combine(Environment.ExpandEnvironmentVariables(fileRoot), previousVersionLocation, assemblyName);
                string currentVersionAssembly = Path.Combine(Environment.ExpandEnvironmentVariables(fileRoot), assemblyName);
                Console.WriteLine("Validating {0} against {1}", currentVersionAssembly, previousVersionAssembly);

                var typesInReleasedAssembly = GetAllPublicTypes(previousVersionAssembly);
                var typesInCurrentAssembly = GetAllPublicTypes(currentVersionAssembly);

                foreach (var releasedType in typesInReleasedAssembly)
                {
                    Type currentType;
                    if (!typesInCurrentAssembly.TryGetValue(releasedType.Key, out currentType))
                    {
                        string message = string.Format("{0} present in previous release but removed in current", releasedType.Key);
                        Console.WriteLine(message);
                        throw new Exception(message);
                    }

                    if (releasedType.Value != currentType && !IsExcludedType(releasedType.Key))
                    {
                        CompareMembers(releasedType.Key, releasedType.Value.GetMembers(), currentType.GetMembers(), releasedType.Value.IsInterface);
                    }
                }
            }

            Console.WriteLine("Success..");
        }

        private Dictionary<string, Type> GetAllPublicTypes(string assemblyName)
        {
            Dictionary<string, Type> publicTypes = new Dictionary<string, Type>();
            Console.WriteLine("Trying to load {0}", assemblyName);
            try
            {
                Assembly assembly = Assembly.LoadFile(assemblyName);
                Console.WriteLine("Loaded {0}", assembly.CodeBase);
                foreach (Type type in assembly.GetTypes())
                {
                    if (type.IsPublic)
                    {
                        publicTypes.Add(type.AssemblyQualifiedName, type);
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine("Exception : {0}", e.Message);
                throw;
            }

            return publicTypes;
        }

        private void CompareMembers(string typeName, MemberInfo[] releasedMemberInfos, MemberInfo[] currentMemberInfos, bool mustBeSame = false)
        {
            if (mustBeSame && releasedMemberInfos.Length != currentMemberInfos.Length)
            {
                string message = string.Format("Number of members in {0} in previous release doesnt match the type in current", typeName);
                Console.WriteLine(message);
                throw new Exception(message);
            }

            Dictionary<string, MemberInfo> currentMemberInfosTable = new Dictionary<string, MemberInfo>();
            
            foreach (var member in currentMemberInfos)
            {
                if (member.MemberType == MemberTypes.Method)
                {
                    currentMemberInfosTable.Add(GetMethodId((MethodInfo)member), member);
                }
                else if (member.MemberType == MemberTypes.Constructor)
                {
                    currentMemberInfosTable.Add(GetConstructorId((MethodBase)member), member);
                }
                else if (member.MemberType == MemberTypes.Field)
                {
                    currentMemberInfosTable.Add(GetFieldId((FieldInfo)member), member);
                }
                else if (member.MemberType == MemberTypes.Property)
                {
                    currentMemberInfosTable.Add(GetPropertyId((PropertyInfo)member), member);
                }
                else
                {
                    currentMemberInfosTable.Add(member.Name, member);
                }

            }

            foreach(var releasedMemberInfo in releasedMemberInfos)
            {
                MemberInfo currentMemberInfo;
                if (releasedMemberInfo.MemberType == MemberTypes.Field)
                {
                    if (!currentMemberInfosTable.TryGetValue(GetFieldId((FieldInfo)releasedMemberInfo), out currentMemberInfo))
                    {
                        string message = string.Format("Field {0} in {1} in previous release is not in current", releasedMemberInfo.Name, typeName);
                        Console.WriteLine(message);
                        throw new Exception(message);
                    }
                }
                else if (releasedMemberInfo.MemberType == MemberTypes.Property)
                {
                    if (!currentMemberInfosTable.TryGetValue(GetPropertyId((PropertyInfo)releasedMemberInfo), out currentMemberInfo))
                    {
                        string message = string.Format("Property {0} in {1} in previous release is not in current", releasedMemberInfo.Name, typeName);
                        Console.WriteLine(message);
                        throw new Exception(message);
                    }
                }
                else if (releasedMemberInfo.MemberType == MemberTypes.Method)
                {
                    MethodInfo releasedMethodInfo = (MethodInfo)releasedMemberInfo;
                    if (!currentMemberInfosTable.TryGetValue(GetMethodId(releasedMethodInfo), out currentMemberInfo))
                    {
                        if (IsExcludedMethod(typeName, releasedMethodInfo.Name))
                        {
                            Console.WriteLine("Found method {0} in Type {1} mentioned in excluded list", releasedMethodInfo.Name, typeName);
                        }
                        else
                        {
                            string message = string.Format("{0} : Method {1} declared in {2} in previous release is not in current", typeName, releasedMemberInfo.Name, releasedMemberInfo.DeclaringType.FullName);
                            Console.WriteLine(message);
                            throw new Exception(message);
                        }
                    }
                }
                else if (releasedMemberInfo.MemberType == MemberTypes.Constructor)
                {
                    if (!currentMemberInfosTable.TryGetValue(GetConstructorId((MethodBase)releasedMemberInfo), out currentMemberInfo))
                    {
                        if (IsExcludedMethod(typeName, releasedMemberInfo.Name))
                        {
                            Console.WriteLine("Found method {0} in Type {1} mentioned in excluded list", releasedMemberInfo.Name, typeName);
                        }
                        else
                        {
                            string message = string.Format("Method {0} in {1} in previous release is not in current", releasedMemberInfo.Name, typeName);
                            Console.WriteLine(message);
                            throw new Exception(message);
                        }
                    }
                }
                else
                {
                    if (!currentMemberInfosTable.TryGetValue(releasedMemberInfo.Name, out currentMemberInfo))
                    {
                        string message = string.Format("Member {0} in {1} in previous release is not in current", releasedMemberInfo.Name, typeName);
                        Console.WriteLine(message);
                        throw new Exception(message);
                    }
                }
            }
        }

        private string GetMethodId(MethodInfo methodInfo)
        {
            string methodId = GetMethodIdFromMethodBase(methodInfo);
            methodId = methodId + "-" + methodInfo.ReturnType.FullName;

            //
            // Virtual methods can be overridden, so dont qualify with type.
            //
            if (!methodInfo.Attributes.HasFlag(MethodAttributes.Virtual))
            {
                methodId = methodInfo.DeclaringType.FullName + "-" + methodId;
            }

            //
            // Note: Attribute changes between methods in 2 versions of the binary are not tracked.
            //
            return methodId;
        }

        private string GetConstructorId(MethodBase methodBase)
        {
            return GetMethodIdFromMethodBase(methodBase);
        }

        private string GetMethodIdFromMethodBase(MethodBase methodBase)
        {
            string methodName = methodBase.Name;
            foreach (var param in methodBase.GetParameters())
            {
                methodName = methodName + "-" + param.ParameterType.FullName;
            }
            return methodName;
        }

        private string GetFieldId(FieldInfo fieldInfo)
        {
            return fieldInfo.Name + "-" + fieldInfo.FieldType.FullName;
        }

        private string GetPropertyId(PropertyInfo propertyInfo)
        {
            string propertyId = propertyInfo.Name + "-" + propertyInfo.PropertyType.FullName;
            if (propertyInfo.SetMethod != null)
            {
                propertyId = propertyId + "-" + GetMethodId(propertyInfo.SetMethod);
            }

            if (propertyInfo.GetMethod != null)
            {
                propertyId = propertyId + "-" + GetMethodId(propertyInfo.GetMethod);
            }

            return propertyId;
        }

        private bool IsExcludedType(string assemblyQualifiedTypeName)
        {
            foreach (string excludedType in ExcludedTypes)
            {
                if (assemblyQualifiedTypeName.IndexOf(excludedType) != -1)
                {
                    Console.WriteLine("Excluded type - {0}", excludedType);
                    return true;
                }
            }

            return false;
        }

        private bool IsExcludedMethod(string assemblyQualifiedTypeName, string assemblyQualifiedMethodName)
        {
            foreach (var excludedMethod in ExcludedMethods)
            {
                if (assemblyQualifiedTypeName.IndexOf(excludedMethod.Item1) != -1)
                {
                    if (assemblyQualifiedMethodName.IndexOf(excludedMethod.Item2) != -1)
                    {
                        return true;
                    }
                }
            }

            return false;
        }
    }
}