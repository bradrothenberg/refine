using System;
using System.IO;

namespace nTop.Core.Examples.Utils;

public class CLI
{
    public static string LoadPrompt()
    {
        {
            Console.WriteLine("Please choose a .implicit file as input");
            Console.WriteLine("Enter 1 for gyroid sphere, 2 for heat sink or type a file pathname");
            string? input = Console.ReadLine();
            string path;
            if (input != null)
            {
                path = input;
            }
            else
            {
                throw new IOException("Error with path.");
            }

            if (input == "1")
            {
                path = @"..\assets\gyroid_sphere_5mm_radius.implicit";
            }
            else if (input == "2")
            {
                path = @"..\assets\heat-sink.implicit";
            }
            
            return path;
        }
    }
}
