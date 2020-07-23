// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FileLogUtility
{
    using System;
    using System.Collections.Generic;
    using System.Linq;

    public static class StatUtility
    {
        public static double GetPercent(double operand, double dividend)
        {
            return Math.Round((operand / dividend) * 100.0, digits: 2);
        }

        public static double GetAverage(double operand, double dividend)
        {
            return Math.Round(operand / dividend, digits: 2);
        }

        public static string GetBytesString(double byteCount)
        {
            if (byteCount < 1024)
            {
                return string.Format("{0} bytes", Math.Round(byteCount, digits: 2));
            }
            else if (byteCount < 1024 * 1024)
            {
                return string.Format("{0} bytes [{1} KB]", Math.Round(byteCount, digits: 2), Math.Round(byteCount / 1024.0, digits: 2));
            }
            else
            {
                return string.Format("{0} bytes [{1} MB]", Math.Round(byteCount, digits: 2), Math.Round(byteCount / (1024.0 * 1024.0), digits: 2));
            }
        }
    }
}