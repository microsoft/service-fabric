// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Diagnostics;
using Mono.Reflection;

namespace DotNetCoreCompatVerifier
{
    internal class Program
    {
        private static string BinsFolderPath = Path.GetDirectoryName(System.Reflection.Assembly.GetEntryAssembly().Location);

        private const string imageStoreTypeName = "System.Fabric.ImageStore.IImageStore, System.Fabric";

        /// <summary>
        /// List of dlls which are loaded in user process and are part of runtime and depend on system.fabric.dll.
        /// Every dll in this list should have <PackageReference> tag in DotNetCoreCompatVerifier's csproj.
        /// </summary>
        private static readonly Dictionary<string, string[]> Dependencies = new Dictionary<string, string[]>()
            {
                {
                    "System.Fabric.dll", new string[] { "Microsoft.ServiceFabric.Data.Impl.dll", "System.Fabric.BackupRestore.dll"}
                },
                {
                    "Microsoft.ServiceFabric.Data.Interfaces.dll", new string[]{ "Microsoft.ServiceFabric.Data.Impl.dll" }
                }
            };

        /// <summary>
        /// List of methods currently invoked via ImageStoreProxy in System.Fabric.dll
        /// </summary>
        private static readonly List<string> ExpectedMethodList = new List<string>(new[]
        {
            "Void UploadContent(System.String, System.String, System.TimeSpan, System.Fabric.CopyFlag, Boolean)",
            "Void UploadContent(System.String, System.String, System.Fabric.IImageStoreProgressHandler, System.TimeSpan, System.Fabric.CopyFlag, Boolean)",
            "Void DeleteContent(System.String, System.TimeSpan)"
        });

        private static void Main(string[] args)
        {
            if (args.Length == 0)
            {
                PrintUsageFormat();
                Environment.Exit(-1);
            }

            switch (args[0].ToLower())
            {
                case "/generatemetadata":
                    WriteMetadata(args);
                    break;
                case "/detect":
                    DetectBreakingChange(args);
                    Console.WriteLine("No breaking change detected");
                    break;
                default:
                    PrintUsageFormat();
                    break;
            }
        }

        private static void WriteMetadata(string[] args)
        {
            if (args.Length < 2)
            {
                PrintUsageFormat();
                Environment.Exit(-1);
            }

            var dllName = args[1];
            var systemFabric = Path.Combine(BinsFolderPath, dllName);
            var version = FileVersionInfo.GetVersionInfo(systemFabric);
            var outputFileName = String.Format("{0}.{1}.metadata", dllName, version.ProductVersion);

            List<string> memberList = new List<string>();
            var types = GetAllTypes(systemFabric);
            foreach (var type in types)
            {
                memberList.AddRange(GetAllNonPrivateMembers(type));
            }

            memberList.AddRange(types.Select(t => GetMemberString(t)));

            File.WriteAllLines(outputFileName, memberList);
            Console.WriteLine("Written metadata successfully at " + outputFileName);
        }

        private static string GetDependencyNameLookup(string dependencyDll)
        {
            var pattern = ".dll";
            if (!dependencyDll.EndsWith(pattern))
            {
                throw new Exception("Dll name should end with " + pattern);
            }
            return dependencyDll.Substring(0, dependencyDll.Length - pattern.Length) + ",";
        }

        private static void DetectBreakingChange(string[] args)
        {
            if (args.Length < 1)
            {
                PrintUsageFormat();
                Environment.Exit(-1);
            }

            foreach (string dependencyDllName in Dependencies.Keys)
            {
                var dependencyNewList = new List<string>();
                var dependencyDllPath = Path.Combine(BinsFolderPath, dependencyDllName);
                var dependencyNewTypes = GetAllTypes(dependencyDllPath);
                var dependencyNameLookup = GetDependencyNameLookup(dependencyDllName);

                foreach (var type in dependencyNewTypes)
                {
                    dependencyNewList.AddRange(GetAllNonPrivateMembers(type));
                }

                dependencyNewList.AddRange(dependencyNewTypes.Select(t => GetMemberString(t)));

                foreach (var runtimeAssemblyName in Dependencies[dependencyDllName])
                {
                    var assemblyPath = Path.Combine(BinsFolderPath, runtimeAssemblyName);
                    var runtimeAssemblyDependencies = new List<string>();

                    if (!File.Exists(assemblyPath))
                    {
                        throw new FileNotFoundException("Couldn't locate the runtime runtimeAssemblyName " + assemblyPath);
                    }

                    var runtimeAssemblyTypes = GetAllTypes(assemblyPath);
                    foreach (var type in runtimeAssemblyTypes)
                    {
                        foreach (var methodInfo in GetAllMethods(type))
                        {
                            runtimeAssemblyDependencies.AddRange(FindMethodDependencies(dependencyNameLookup, methodInfo));
                        }

                        runtimeAssemblyDependencies.AddRange(FindTypeDependencies(dependencyDllName, type));
                    }

                    CheckBreakingChangeWithPreviousVersions(dependencyDllName, dependencyNewList, runtimeAssemblyName, runtimeAssemblyDependencies);
                }
            }

            Console.WriteLine("Verifying Image Store Proxy interface");
            DetectBreakingChangeInXStoreProxy();

            Console.WriteLine("All tests passed.");
        }

        private static void CheckBreakingChangeWithPreviousVersions(string dependencyDllName, List<string> dependencyNewList,
            string runtimeAssemblyName, List<string> runtimeAssemblyDependencies)
        {
            bool hasBreakingChange = false;
            uint numTestRuns = 0;
            foreach (var fileName in Directory.EnumerateFiles("metadata"))
            {
                if (!Path.GetFileName(fileName).EndsWith(".metadata"))
                {
                    Console.WriteLine("Skipping {0} as it does not end with .metadata", fileName);
                    continue;
                }

                if (!fileName.Contains(dependencyDllName))
                {
                    Console.WriteLine("Skipping test run with {0} for {1}'s dependency : {2}", fileName, runtimeAssemblyName, dependencyDllName);
                    continue;
                }

                Console.WriteLine("Running test between metadata version: {0}, Runtime Dll: {1}", fileName, runtimeAssemblyName);

                numTestRuns = numTestRuns + 1;

                var lines = File.ReadAllLines(fileName);
                var dependencyOldList = new List<string>(lines);
                var dependencyDiff = dependencyNewList.Except(dependencyOldList).ToList();

                var conflicts = dependencyDiff.Intersect(runtimeAssemblyDependencies).ToList();
                if (conflicts.Any())
                {
                    hasBreakingChange = true;
                    Console.WriteLine("!!!!! BREAKING CHANGE DETECTED !!!!!!");
                    Console.WriteLine("Metadata version: {0}, Runtime Dll: {1}", fileName, runtimeAssemblyName);
                    foreach (var conflict in conflicts)
                    {
                        Console.WriteLine(conflict);
                    }
                }
            }

            if (hasBreakingChange)
            {
                throw new Exception("!!!!! BREAKING CHANGE DETECTED !!!!!!");
            }

            if (numTestRuns == 0)
            {
                throw new Exception("No valid metadata file found for " + dependencyDllName);
            }
        }

        private static IEnumerable<string> FindTypeDependencies(string assemblyName, Type type)
        {
            var list = new List<string>();

            if (IsTypeInAssembly(assemblyName, type.BaseType))
            {
                list.Add(GetMemberString(type.BaseType));
            }

            foreach (var iFace in type.GetInterfaces())
            {
                if (IsTypeInAssembly(assemblyName, iFace))
                {
                    list.Add(GetMemberString(iFace));
                }
            }

            return list;
        }

        private static bool IsTypeInAssembly(string assemblyName, Type checkType)
        {
            if (checkType == null) return false;
            return checkType.Module.Name == assemblyName;
        }

        private static void DetectBreakingChangeInXStoreProxy()
        {
            var sfmPath = Path.Combine(BinsFolderPath, "System.Fabric.dll");

            var methodInfoList = GetIImageStoreMethodInfo(sfmPath);
            var methodInfoStrList = methodInfoList.Select(m => m.ToString());

            // Check if the expected method list is present
            if (ExpectedMethodList.Intersect(methodInfoStrList).Count() != ExpectedMethodList.Count())
            {
                Console.WriteLine("!!!!! BREAKING CHANGE DETECTED !!!!!!");
                Console.WriteLine("\r\n");
                Console.WriteLine("Expected Methods List:");
                foreach (var method in ExpectedMethodList)
                {
                    Console.WriteLine(method);
                }

                Console.WriteLine("\r\n");
                Console.WriteLine("Current Method List:");
                foreach (var method in methodInfoStrList)
                {
                    Console.WriteLine(method);
                }

                throw new Exception("!!!!! BREAKING CHANGE DETECTED !!!!!!");
            }
        }

        private static MethodInfo[] GetIImageStoreMethodInfo(string filePath)
        {
            var assembly = Assembly.LoadFile(filePath);
            var imageStoreType = Type.GetType(imageStoreTypeName, true);
            return imageStoreType.GetMethods();
        }

        private static Type[] GetAllTypes(string fileName)
        {
            var assembly = Assembly.LoadFile(fileName);
            return assembly.GetTypes();
        }

        private static MethodInfo[] GetAllMethods(Type type)
        {
            return type.GetMethods(BindingFlags.Instance | BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public);
        }

        private static List<string> FindMethodDependencies(string assemblyNameLookup, MethodBase methodBase)
        {
            var members = new List<string>();
            IList<Instruction> instructions;
            try
            {
                instructions = methodBase.GetInstructions();
            }
            catch (Exception)
            {
                return members;
            }


            foreach (var instruction in instructions)
            {
                var info = instruction.Operand as MemberInfo;
                if (null != info && info.DeclaringType != null && info.DeclaringType.Assembly.FullName.Contains(assemblyNameLookup))
                {
                    members.Add(GetMemberString(info));
                }
            }

            return members;
        }

        private static string GetMemberString(MemberInfo memberInfo)
        {
            return memberInfo.MemberType + " " + memberInfo.ReflectedType + " " + memberInfo;
        }

        private static List<string> GetAllNonPrivateMembers(Type type)
        {
            var allMembers = new List<string>();

            foreach (var member in type.GetMembers(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static | BindingFlags.Instance))
            {
                if (member.MemberType == MemberTypes.Method)
                {
                    var methInfo = (MethodInfo)member;
                    if (!methInfo.IsPrivate)
                    {
                        allMembers.Add(GetMemberString(member));
                    }
                }
                else if (member.MemberType == MemberTypes.Constructor)
                {
                    var constructorInfo = (ConstructorInfo)member;
                    if (!constructorInfo.IsPrivate)
                    {
                        allMembers.Add(GetMemberString(member));
                    }
                }
            }

            return allMembers;
        }

        private static void PrintUsageFormat()
        {
            Console.WriteLine("!!!! USAGE !!!!");
            Console.WriteLine("DotNetCoreCompatVerifier.exe /generatemetadata <dll_name>");
            Console.WriteLine("DotNetCoreCompatVerifier.exe /detect");
            throw new InvalidOperationException();
        }
    }
}