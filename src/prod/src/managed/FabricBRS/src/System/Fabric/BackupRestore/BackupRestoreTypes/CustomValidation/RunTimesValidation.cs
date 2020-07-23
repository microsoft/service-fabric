// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;

    [AttributeUsage(AttributeTargets.Property |
        AttributeTargets.Field, AllowMultiple = true)]
    public sealed class RunTimesValidation : ValidationAttribute
    {
        public const double TotalMilliSecondsInADay = 86400000;
        public override bool IsValid(object value)
        {
            List<TimeSpan> timeSpans = value as List<TimeSpan>;
            if (timeSpans == null)
            {
                return false;
            }
            
            foreach (var timeSpan in timeSpans)
            {
                if (! this.LessThan24Hrs(timeSpan))
                {
                    return false;
                }
            }
            return true;
        }

        private bool  LessThan24Hrs(TimeSpan timeSpan)
        {
            if (timeSpan.TotalMilliseconds < TotalMilliSecondsInADay )
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }
}