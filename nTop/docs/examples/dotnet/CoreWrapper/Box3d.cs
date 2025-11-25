/// <summary>Represents a 3D axis-aligned box</summary>
/// <remarks>
/// Usually used to represent a region of space that encloses some object.
/// The box corners are at the points (MinX, MinY, MinZ) and (MaxX, MaxY), MaxZ.
/// </remarks>
public class Box3d
{
    /// <summary>The lower x-value</summary>
    public double MinX { get; set; }

    /// <summary>The lower y-value</summary>
    public double MinY { get; set; }

    /// <summary>The lower z-value</summary>
    public double MinZ { get; set; }

    /// <summary>The upper x-value</summary>
    public double MaxX { get; set; }

    /// <summary>The upper y-value</summary>
    public double MaxY { get; set; }

    /// <summary>The upper z-value</summary>
    public double MaxZ { get; set; }

    /// <summary>The lower corner of the box (min X, Y, Z values)</summary>
    public Vector MinPoint
    {
        get { return new Vector(this.MinX, this.MinY, this.MinZ); }
        set { this.MinX = value.X; this.MinY = value.Y; this.MinZ = value.Z; }
    }

    /// <summary>The upper corner of the box (max X, Y, Z values)</summary>
    public Vector MaxPoint
    {
        get { return new Vector(this.MaxX, this.MaxY, this.MaxZ); }
        set { this.MaxX = value.X; this.MaxY = value.Y; this.MaxZ = value.Z; }
    }

    public Box3d(double minX, double minY, double minZ, double maxX, double maxY, double maxZ)
    {
        this.MinX = minX; this.MinY = minY; this.MinZ = minZ;
        this.MaxX = maxX; this.MaxY = maxY; this.MaxZ = maxZ;
    }

    public Box3d(Vector minPoint, Vector maxPoint)
    {
        this.MinX = minPoint.X; this.MinY = minPoint.Y; this.MinZ = minPoint.Z;
        this.MaxX = maxPoint.X; this.MaxY = maxPoint.Y; this.MaxZ = maxPoint.Z;
    }

    /// <summary>Enlarges a box</summary>
    /// <param name="factor">Enlargement factor: use factor = 1.5 to make 50% bigger</param>
    /// <returns>Enlarged box (with same center as original)</returns>
    public Box3d Enlarge(double factor)
    {
        Vector center = 0.5 * (this.MinPoint + this.MaxPoint);
        Vector diag = this.MaxPoint - this.MinPoint;
        Vector minPt = center - 0.5 * factor * diag;
        Vector maxPt = center + 0.5 * factor * diag;
        return new Box3d(minPt, maxPt);
    }
}
