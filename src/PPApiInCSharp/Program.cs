using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace PPApiInCSharp
{
    public class Program
    {
        public static int Main(string arg)
        {
            MessageBox.Show("Welcome to C# running on Electron!");
            return arg?.Length ?? -1;
        }
    }
}
