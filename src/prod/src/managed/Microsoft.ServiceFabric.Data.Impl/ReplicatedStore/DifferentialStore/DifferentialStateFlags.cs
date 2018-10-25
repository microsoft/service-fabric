using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace System.Fabric.Store
{
    [Flags]
    enum DifferentialStateFlags : byte
    {
        None = 0b0,
        NewKey = 0b1
    }
}
