using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PPApiInCSharp
{
    public class Program
    {
        public static int Main(string arg)
        {
            return arg?.Length ?? -1;
        }
    }
}
