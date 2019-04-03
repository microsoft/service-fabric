// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ImageBuilderExe.Test
{

    internal static class TestStringConstants
    {        
        public static readonly string DefaultSchemaPath = "ServiceFabricServiceModel.xsd";

        //Command Line Arguments Key
        public static readonly string SchemaPath = "/schemaPath";
        public static readonly string WorkingDir = "/workingDir";
        public static readonly string Operation = "/operation";
        public static readonly string StoreRoot = "/storeRoot";
        public static readonly string AppTypeName = "/appTypeName";
        public static readonly string AppTypeVersion = "/appTypeVersion";
        public static readonly string AppId = "/appId";
        public static readonly string NameUri = "/nameUri";
        public static readonly string AppParam = "/appParam";
        public static readonly string BuildPath = "/buildPath";
        public static readonly string Output = "/output";
        public static readonly string CurrentAppInstanceVersion = "/currentAppInstanceVersion";

        //Command Line Arguments Value
        public static readonly string OperationBuildApplicationTypeInfo = "BuildApplicationTypeInfo";
        public static readonly string OperationBuildApplicationType = "BuildApplicationType";
        public static readonly string OperationBuildApplication = "BuildApplication";
        public static readonly string OperationUpgradeApplication = "UpgradeApplication";
    }
}