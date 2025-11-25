using System.Runtime.InteropServices;

namespace nTop.Core.Wrapper
{

    [StructLayout(LayoutKind.Sequential)]
    struct Vec2
    {
        private double x;
        private double y;

        public double X => (double)x;
        public double Y => (double)y;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Vec3
    {
        private double x;
        private double y;
        private double z;

        public Vec3(double x, double y, double z)
        {
            this.x = x;
            this.y = y;
            this.z = z;
        }

        public double X => (double)x;
        public double Y => (double)y;
        public double Z => (double)z;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Frame
    {
        private Vec3 origin;
        private Vec3 x_axis;
        private Vec3 y_axis;

        public Vec3 Origin => origin;
        public Vec3 XAxis => x_axis;
        public Vec3 YAxis => y_axis;

        public Frame(Vec3 origin, Vec3 xAxis, Vec3 yAxis)
        {
            this.origin = origin;
            this.x_axis = xAxis;
            this.y_axis = yAxis;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct BoundingBox
    {
        private Vec3 min;
        private Vec3 max;

        public Vec3 Min => min;
        public Vec3 Max => max;
    }
}
