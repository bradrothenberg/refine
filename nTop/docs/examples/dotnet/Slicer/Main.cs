using System;
using System.IO;

using nTop.Core.Wrapper;
using nTop.Core.Examples.Utils;

using ScottPlot;

namespace nTop.Core.Examples.Slicer
{
	public partial class Slicer
	{
		/// <summary>
		/// Draw the slice and write it to disk
		/// </summary>
		/// <param name="s">The slice to render</param>
		/// <param name="filepath">Location to write the PNG</param>
		private static void DrawSlice(Slice s, string filepath)
		{
			var plot = new ScottPlot.Plot(800, 800);

			// Draw each curve in the slice
			foreach (List<Tuple<double, double>> curve in s.Contours)
			{
				double[] xs = (from point in curve 
					select point.Item1).ToArray<double>();
				double[] ys = (from point in curve 
					select point.Item2).ToArray<double>();
				var contour = plot.AddScatter(xs, ys);
				contour.MarkerShape = MarkerShape.none;
			}
			plot.XAxis.Label("meters");
			plot.YAxis.Label("meters");
			plot.SaveFig(filepath);
		}
		
		public static void Main()
		{
			// Get .implicit file path from user
			string path = CLI.LoadPrompt();

			// Create an ImplicitBody object
			var body = new ImplicitBody(path);

			// Use mid-z value as slicing height
			double midZ = 0.5 * (body.Box.MinZ + body.Box.MaxZ);
			Frame sliceFrame = new Frame(
				new Vec3(0, 0, midZ),
				new Vec3(1, 0, 0),
				new Vec3(0, 1, 0)
			);

			// Slice the body on the frame
			Slice s = body.Slice(sliceFrame, 0.0001);
			
            // Free the memory used by the implicit in the library
			body.Release();

			// Draw a graph of the slice
			DrawSlice(s, "./slice.png");

			Console.WriteLine($"Slice written to {Directory.GetCurrentDirectory()}{Path.DirectorySeparatorChar}slice.png");
		}
	}
}
