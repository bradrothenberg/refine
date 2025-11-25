/// Example of querying the bounding box from an implicit
#include <iostream>

#include <ntop_core/ntop_core.h>

int main() {
  // Select a .implicit file and load it
  std::cout << "Enter a path to an implicit (examples in the examples/assets "
               "folder): ";
  std::string path;
  std::cin >> path;
  void *implicit_handle{};

  uint32_t res = ntop_core_import_from_file(path.c_str(), &implicit_handle);

  // Do some error handling for bad paths or corrupt implicits
  switch (res) {
    case ntop_core_import_result::SUCCESS:
      break;
    case ntop_core_import_result::FILE_DOESNT_EXIST:
      std::cout << "File not found" << std::endl;
      exit(1);
      break;
    case ntop_core_import_result::FILE_OPEN_ERROR:
      std::cout << "Unable to open file" << std::endl;
      exit(2);
      break;
    case ntop_core_import_result::CORRUPT_FILE:
      std::cout << "Corrupted file" << std::endl;
      exit(3);
      break;
    case ntop_core_import_result::UNSUPPORTED_VERSION:
      int major, minor, patch;
      ntop_core_library_version(&major, &minor, &patch);
      std::cout << "Implicit file is unsupported by nTop Core version " << major << "." << minor << "." << patch << std::endl;
      exit(4);
      break;
    default: {
      std::cout << "Unspecified error loading" << std::endl;
      exit(5);
    }
  }

  // Initialize a bounding box to fill with data
  ntop_core_bounding_box bb{};

  // Query the bounding box, filling bb with data
  ntop_core_query_bounding_box(implicit_handle, &bb);

  // Region to voxelize, obtained from body's bounding box.

  int n = 200; // Number of voxels used = n*n*n. Increase this to improve volume
               // accuracy.

  double xmin = bb.min.x;
  double xmax = bb.max.x;
  double dx = (xmax - xmin) / n;
  double ymin = bb.min.y;
  double ymax = bb.max.y;
  double dy = (ymax - ymin) / n;
  double zmin = bb.min.z;
  double zmax = bb.max.z;
  double dz = (zmax - zmin) / n;

  int count = 0;

  // Loop to count number of voxels inside the body
  for (int i = 0; i <= n; i++) {
    for (int j = 0; j <= n; j++) {
      for (int k = 0; k <= n; k++) {
        double x = xmin + i * dx + 0.5 * dx;
        double y = ymin + j * dy + 0.5 * dy;
        double z = zmin + k * dz + 0.5 * dz;
        double fxyz = ntop_core_query_field(implicit_handle, {x, y, z});
        if (fxyz < 0)
          count++; // voxel center is inside body, so add to count
      }
    }
  }

  // Free the memory used by the implicit as no more operations are performed
  // using nTop Core
  ntop_core_release(implicit_handle);

  // Compute volume = number of inside voxels * voxel volume
  double volume = count * (dx * dy * dz);

  std::cout << "Computed volume  = " << volume << std::endl;
}
