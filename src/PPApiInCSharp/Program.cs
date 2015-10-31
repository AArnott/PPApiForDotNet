using System;
using System.Collections.Generic;
using System.Text;

public class Program
{
    public static int Main(string arg)
    {
        return arg?.Length ?? -1;
    }
}
