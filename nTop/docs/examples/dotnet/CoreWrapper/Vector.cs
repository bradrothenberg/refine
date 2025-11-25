using System;
using static System.Math;
using System.Linq;

/// <summary>Represents a vector or position in 3D space</summary>
/// <remarks>
/// Provides functions for creating vectors, and for doing operations
/// like sum, difference, and scalar multiplication using the 
/// normal arithmetic (in-fix) notation. Also provides dot and cross 
/// products, unitizing, and measuring angles. These vector objects
/// are also used to represent the position vectors of points.
/// Points and vectors are not really the same thing, but ...
/// </remarks>
public struct Vector
{
    #region #### Properties ############################

    /// <summary>The x-component (or coordinate) of the vector</summary>
    public double X;

    /// <summary>The y-component (or coordinate) of the vector</summary>
    public double Y;

    /// <summary>The z-component (or coordinate) of the vector</summary>
    public double Z;

    /// <summary>Array of the components of the vector</summary>
    public double[] ToArray
    {
        get
        {
            double[] a = { this.X, this.Y, this.Z };
            return a;
        }
    }

    /// <summary>Returns the norm squared (length squared) of a vector</summary>
    /// <returns>Norm (length) squared of vector</returns>      
    public double Norm2
    {
        get
        {
            return this.X * this.X + this.Y * this.Y + this.Z * this.Z;
        }
    }

    /// <summary>Calculates the norm (length) of a vector</summary>
    /// <returns>Norm (length) of vector</returns>      
    public double Norm
    {
        get
        {
            return System.Math.Sqrt(this.X * this.X + this.Y * this.Y + this.Z * this.Z);
        }
    }

    /// <summary>Unitizes a given vector</summary>
    /// <param name="u">Vector to be unitized</param>
    /// <returns>Unit vector in same direction</returns>
    /// <remarks>
    /// If the input is the zero vector, then each component
    /// of the returned vector will be NaN (not a number).
    /// </remarks>
    public Vector Unit
    {
        get
        {
            double r = 1.0 / this.Norm;
            return new Vector(r * this.X, r * this.Y, r * this.Z);
        }
    }

    #endregion ###############################

    #region #### Operator overloads ##########################

    /// <summary>Adds two vectors using the "+" notation</summary>
    /// <param name="u">First vector</param>
    /// <param name="v">Second vector</param>
    /// <returns>Sum of two vectors: u + v</returns> 
    public static Vector operator +(Vector u, Vector v)
    {
        return new Vector(u.X + v.X, u.Y + v.Y, u.Z + v.Z);
    }

    /// <summary>Subtracts two vectors using the "-" notation</summary>
    /// <param name="u">First vector</param>
    /// <param name="v">Second vector</param>
    /// <returns>Difference of two vectors: u - v</returns> 
    public static Vector operator -(Vector u, Vector v)
    {
        return new Vector(u.X - v.X, u.Y - v.Y, u.Z - v.Z);
    }

    /// <summary>Negates (reverses) a vector using the "-" notation</summary>
    /// <param name="u">The vector</param>
    /// <returns>Negative of the given vector</returns>
    public static Vector operator -(Vector u)
    {
        return new Vector(-u.X, -u.Y, -u.Z);
    }

    /// <summary>Multiplies a vector by a scalar using "*" notation</summary>
    /// <param name="s">Scalar (double)</param>
    /// <param name="u">Vector</param>
    /// <returns>Scalar multiple: s*u</returns>
    public static Vector operator *(double s, Vector u)
    {
        return new Vector(s * u.X, s * u.Y, s * u.Z);
    }

    /// <summary>Multiplies a vector by a scalar using "*" notation</summary>
    /// <param name="s">Scalar (int)</param>
    /// <param name="u">Vector</param>
    /// <returns>Scalar multiple: s*u</returns>
    public static Vector operator *(int s, Vector u)
    {
        return new Vector(s * u.X, s * u.Y, s * u.Z);
    }

    /// <summary>Divides a vector by a scalar using "/" notation</summary>
    /// <param name="u">Vector</param>
    /// <param name="s">Scalar (double)</param>
    /// <returns>Scalar multiple: u/s</returns>
    /// <remarks>
    /// If s = 0, then each component of the returned vector will  
    /// be either Infinity, -Infinity, or NaN (not a number).
    /// </remarks>
    public static Vector operator /(Vector u, double s)
    {
        return new Vector(u.X / s, u.Y / s, u.Z / s);
    }

    /// <summary>Calculates the dot product (scalar product) of two vectors</summary>
    /// <param name="u">First vector</param>
    /// <param name="v">Second vector</param>
    /// <returns>Dot product: u*v</returns>
    public static double operator *(Vector u, Vector v)
    {
        return u.X * v.X + u.Y * v.Y + u.Z * v.Z;
    }

    #endregion ###############################################

    #region #### Constructors ##############################

    /// <summary>Creates a vector from three rectangular coordinates</summary>
    /// <param name="x">x-coordinate</param>
    /// <param name="y">y-coordinate</param>
    /// <param name="z">z-coordinate</param>
    public Vector(double x, double y, double z)
    {
        this.X = x; this.Y = y; this.Z = z;
    }

    /// <summary>Creates a vector from an array of three coordinates</summary>
    /// <param name="coords">Array of three coordinates</param>
    public Vector(double[] coords)
    {
        this.X = coords[0];
        this.Y = coords[1];
        this.Z = coords[2];
    }

    #endregion ###########################################

    #region #### Public Operation Functions ########################

    /// <summary>Produces a string representation of a Vector object using a given format</summary>
    /// <param name="format">A numeric format specifier</param>
    /// <returns>String in the form (X, Y, Z)</returns> 
    /// <remarks>
    /// The X, Y, Z components are converted to strings using the given format
    /// and the standard .NET Double.ToString function. This means that the string
    /// depends on your Windows "culture" settings. In particular, the decimal point might be represented
    /// by either a period or a comma, depending on your settings.
    /// </remarks>
    public string ToString(string format)
    {
        return "( " + X.ToString(format) + " , " + Y.ToString(format) + " , " + Z.ToString(format) + " )";
    }

    /// <summary>Calculates the cross product (vector product) of two vectors</summary>
    /// <param name="u">First vector</param>
    /// <param name="v">Second vector</param>
    /// <returns>Cross product</returns>
    /// <remarks>
    /// As is well known, order matters: Cross(u,v) = - Cross(v,u)
    /// </remarks>
    public static Vector Cross(Vector u, Vector v)
    {
        return new Vector(u.Y * v.Z - u.Z * v.Y, u.Z * v.X - u.X * v.Z, u.X * v.Y - u.Y * v.X);
    }

    /// <summary>Calculates the unitized cross product (vector product) of two vectors</summary>
    /// <param name="u">First vector</param>
    /// <param name="v">Second vector</param>
    /// <returns>Unitized cross product</returns>
    /// <remarks>
    /// If the cross product is the zero vector, then each component
    /// of the returned vector will be NaN (not a number).
    /// </remarks>
    public static Vector UnitCross(Vector u, Vector v)
    {
        return Cross(u, v).Unit;
    }

    /// <summary>Calculates the angle in radians between two vectors</summary>
    /// <param name="u">First vector</param>
    /// <param name="v">Second vector</param>
    /// <returns>The angle, theta, in radians, where 0 &#8804; theta &#8804; Pi</returns>      
    public static double Angle(Vector u, Vector v)
    {
        double numer = (u - v).Norm;  // this is better than the acos(u*v) formula
        double denom = (u + v).Norm;
        return 2 * System.Math.Atan(numer / denom);
    }

    /// <summary>A unit vector in the direction of the X-axis -- (1,0,0)</summary>
    public static readonly Vector AxisX = new Vector(1, 0, 0);

    /// <summary>A unit vector in the direction of the Y-axis -- (0,1,0)</summary>
    public static readonly Vector AxisY = new Vector(0, 1, 0);

    /// <summary>A unit vector in the direction of the Z-axis -- (0,0,1)</summary>
    public static readonly Vector AxisZ = new Vector(0, 0, 1);

    /// <summary>The polar angle "theta" -- the angle of rotation in the XY-plane, in radians</summary>
    /// <remarks>
    /// The function returns System.Math.Atan2(y, x). 
    /// So, the returned angle is always between -180 and 180.
    /// See the .NET documentation for Math.Atan2 for further details.            
    /// </remarks>

    public double PolarTheta
    {
        get
        {
            return System.Math.Atan2(this.Y, this.X);
        }
    }

    /// <summary>The polar angle "phi" -- angle between the vector and the XY-plane, in radians</summary>
    /// <remarks>
    /// The function returns <c>System.Math.Atan2(Sqrt(x*x + y*y), z)</c>. 
    /// So, the returned angle is always between -180 and 180.
    /// See the .NET documentation for Math.Atan2 for further details.
    /// </remarks>
    public double PolarPhi
    {
        get
        {
            double x = this.X; double y = this.Y;
            return System.Math.Atan2(this.Z, System.Math.Sqrt(x * x + y * y));
        }
    }

    #endregion ##########################################################
}
