// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common.Model
{
    using Newtonsoft.Json;
    using System.Fabric.BackupRestore.Enums;
    using System.Runtime.Serialization;
    using BackupRestoreView = System.Fabric.BackupRestore.BackupRestoreTypes;

    [DataContract]
    [KnownType(typeof(BasicRetentionPolicy))]
    internal abstract class RetentionPolicy
    {
        internal RetentionPolicy(RetentionPolicyType retentionPolicyType)
        {
            this.RetentionPolicyType = retentionPolicyType;
        }

        [DataMember]
        internal RetentionPolicyType RetentionPolicyType { set; get; }

        internal static BackupRestoreView.RetentionPolicy ToRetentionPolicyView(RetentionPolicy retentionPolicy)
        {
            if(retentionPolicy == null)
            {
                return null;
            }
            BackupRestoreView.RetentionPolicy retentionPolicyView = null;
            if (retentionPolicy.RetentionPolicyType == RetentionPolicyType.Basic)
            {
                retentionPolicyView = BasicRetentionPolicy.ToBasicRetentionPolicyView((BasicRetentionPolicy)retentionPolicy);
                retentionPolicyView.RetentionPolicyType = RetentionPolicyType.Basic;
            }
            return retentionPolicyView;
        }

        internal static RetentionPolicy FromRetentionPolicyView(BackupRestoreView.RetentionPolicy retentionPolicyView)
        {
            if(retentionPolicyView ==null)
            {
                return null;
            }
            RetentionPolicy retentionPolicy = null;
            if (retentionPolicyView.RetentionPolicyType == RetentionPolicyType.Basic)
            {
                retentionPolicy =
                    BasicRetentionPolicy.FromBasicRetentionPolicyView(
                        (BackupRestoreView.BasicRetentionPolicy) retentionPolicyView);
            }
            return retentionPolicy;
            
        }
    }

}