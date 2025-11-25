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
      std::cout <<  "File not found" << std::endl;
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

  // Free the memory used by the implicit as no more operations are performed
  // using nTop Core
  ntop_core_release(implicit_handle);

  // Print out some useful information about the bounding box
  std::cout << "Minimum: (" << bb.min.x << ", " << bb.min.y << ", " << bb.min.x
            << ")" << std::endl;
  std::cout << "Maximum: (" << bb.max.x << ", " << bb.max.y << ", " << bb.max.x
            << ")" << std::endl;

  std::cout << "Corners:" << std::endl;
  std::cout << "   G-----H  Z ^   ^ Y" << std::endl;
  std::cout << "  /|    /|    |  /" << std::endl;
  std::cout << " / |   / |    | /" << std::endl;
  std::cout << "E-----F  |    |/" << std::endl;
  std::cout << "|  C--|--D    *------> X" << std::endl;
  std::cout << "| /   | /" << std::endl;
  std::cout << "|/    |/" << std::endl;
  std::cout << "A-----B" << std::endl;
  std::cout << "\tA: (" << bb.min.x << ", " << bb.min.y << ", " << bb.min.x
            << ")" << std::endl;
  std::cout << "\tB: (" << bb.max.x << ", " << bb.min.y << ", " << bb.min.z
            << ")" << std::endl;
  std::cout << "\tC: (" << bb.min.x << ", " << bb.max.y << ", " << bb.min.z
            << ")" << std::endl;
  std::cout << "\tD: (" << bb.max.x << ", " << bb.max.y << ", " << bb.min.z
            << ")" << std::endl;
  std::cout << "\tE: (" << bb.min.x << ", " << bb.min.y << ", " << bb.max.z
            << ")" << std::endl;
  std::cout << "\tF: (" << bb.max.x << ", " << bb.min.y << ", " << bb.max.z
            << ")" << std::endl;
  std::cout << "\tG: (" << bb.min.x << ", " << bb.max.y << ", " << bb.max.z
            << ")" << std::endl;
  std::cout << "\tH: (" << bb.max.x << ", " << bb.max.y << ", " << bb.max.x
            << ")" << std::endl;
}
