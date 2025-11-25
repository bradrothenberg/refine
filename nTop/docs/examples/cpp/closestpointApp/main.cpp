/// Example of querying the closestpoint on an implicit
#include <iostream>
#include <vector>

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
    std::cout << "Unsupported implicit version" << std::endl;
    exit(4);
    break;
  default: {
    std::cout << "Unspecified error loading" << std::endl;
    exit(5);
  }
  }

  // Select a points to query
  ntop_core_vec3 query_pts[] = {{0, 1, 2}, {3, 4, 5}, {2.718, 3.141, 1.618}};
  auto n_qpts = 3;

  // Pick a tolerance (in meters)
  float tolerance = 0.001;

  // Set up vectors to receive the query answers.
  std::vector<ntop_core_vec3> closest_pts{};
  std::vector<uint32_t> bad_pt_indices{};

  // This lambda will be called with all the closest points and their validity.
  // It has the same signature as ntop_core_query_closest_point_array_cb
  // skipping the context.
  auto closest_point_callback =
      [&](ntop_core_vec3 const *cpoints, uint32_t n_cpts,
          uint32_t const *bad_indices, uint32_t n_indices) {
        // Note: the number of callback points, n_cpts, always equals the number
        // of query points, n_qpts;
        for (uint32_t idx = 0; idx < n_cpts; idx++) {
          closest_pts.push_back(cpoints[idx]);
        }
        for (uint32_t idx = 0; idx < n_indices; idx++) {
          bad_pt_indices.push_back(bad_indices[idx]);
        }
      };

  // Query for the closest points
  ntop_core_query_closest_point_array(
      implicit_handle, query_pts, n_qpts, tolerance, &closest_point_callback,
      [](void *ctx, ntop_core_vec3 const *pts, uint32_t n_pts,
         uint32_t const *bad_indices, uint32_t n_bad_indices) {
        auto const &my_lambda =
            *reinterpret_cast<decltype(closest_point_callback) *>(ctx);
        my_lambda(pts, n_pts, bad_indices, n_bad_indices);
      });

  // Free the memory used by the implicit as no more operations are performed
  // using nTop Core
  ntop_core_release(implicit_handle);

  // Print out the closest point on the implicit to the query point.
  // (And whether the field value at the found point is within tolerance.)
  for (size_t idx = 0, bad_pt_idx = 0; idx < n_qpts; idx++) {
    auto qx = query_pts[idx].x;
    auto qy = query_pts[idx].y;
    auto qz = query_pts[idx].z;
    auto cx = closest_pts[idx].x;
    auto cy = closest_pts[idx].y;
    auto cz = closest_pts[idx].z;
    bool converged = true;
    if (bad_pt_idx < bad_pt_indices.size() &&
        bad_pt_indices[bad_pt_idx] == idx) {
      converged = false;
      bad_pt_idx++;
    }

    std::cout << "Closest point to (" << qx << ", " << qy << ", " << qz
              << ") = "
              << "(" << cx << ", " << cy << ", " << cz << ") (" << converged
              << ")" << std::endl;
  }
}
