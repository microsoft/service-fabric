// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System.Collections.Generic;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ImageBuilder;
    using System.IO;
    using System.Linq;
    using System.Reflection;

    public class ImageBuilderTestContext
    {
        private static readonly string ImageStoreRootPath = Path.Combine(TestUtility.TestDirectory + "ImageStoreRoot");

        private static IEnumerable<string> azureStorageTests;

        internal readonly string FabricWorkingDir = "FabricWorkingDir";

        internal ImageBuilder ImageBuilder { get; private set; }
        internal IImageStore ImageStore { get; private set; }

        static ImageBuilderTestContext()
        {
            List<string> testList = new List<string>();
            Assembly currentAssembly = Assembly.GetExecutingAssembly();
            var modules = currentAssembly.GetModules();
            foreach (var module in modules)
            {
                var types = module.GetTypes();
                foreach (var type in types)
                {
                    if (type.CustomAttributes.Any(attribute => attribute.AttributeType == (typeof(TestClassAttribute))))
                    {
                        var methods = type.GetMethods();
                        foreach (var method in methods)
                        {
                            if (!method.CustomAttributes.Any(attribute => attribute.AttributeType == typeof(TestMethodAttribute)))
                            {
                                continue;
                            }

                            CustomAttributeData matchingAttribute = method.CustomAttributes.FirstOrDefault(attribute => attribute.AttributeType == typeof(ImageStoreTypeAttribute));
                            if (matchingAttribute != null)
                            {
                                var matchingArgument = matchingAttribute.ConstructorArguments.FirstOrDefault(argument => argument.ArgumentType == typeof(ImageStoreEnum));
                                if ((ImageStoreEnum)matchingArgument.Value == ImageStoreEnum.XStore)
                                {
                                    testList.Add(method.DeclaringType.FullName + "." + method.Name);
                                }
                            }
                        }
                    }
                }
            }

            azureStorageTests = testList;
        }

        private ImageBuilderTestContext(string currentTestName)
        {
            bool useAzureStorage = azureStorageTests.Any(testName => currentTestName.Equals(testName, StringComparison.OrdinalIgnoreCase));

            if (useAzureStorage)
            {
                this.ImageStore = TestUtility.GetAzureImageStore(
                    TestUtility.XStoreAccountName,
                    TestUtility.XStoreContainerName);
            }
            else
            {
                this.ImageStore = TestUtility.GetFileImageStore(ImageStoreRootPath);
            }

            this.ImageBuilder = new ImageBuilder(
                this.ImageStore,
                Path.Combine(TestUtility.TestDirectory, "ServiceFabricServiceModel.xsd"),
                FabricWorkingDir);

            this.ImageStore.DeleteContent("Store", TimeSpan.MaxValue);
            this.ImageStore.DeleteContent("WindowsFabricStore", TimeSpan.MaxValue);
            this.ImageStore.DeleteContent("ImageStoreTest", TimeSpan.MaxValue);
        }

        public void TestCleanup()
        {
            if (Directory.Exists(FabricWorkingDir))
            {
                Directory.Delete(FabricWorkingDir, true);
            }

            this.ImageStore.DeleteContent("Store", TimeSpan.MaxValue);
            this.ImageStore.DeleteContent("WindowsFabricStore", TimeSpan.MaxValue);
            this.ImageStore.DeleteContent("ImageStoreTest", TimeSpan.MaxValue);
        }

        public static ImageBuilderTestContext Create(string currentTestName)
        {
            return new ImageBuilderTestContext(currentTestName);
        }
    }
}