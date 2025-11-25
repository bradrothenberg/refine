/// Example of slicing a single layer from an implicit by transforming the
/// implicit to a build orientation and z-slice height.

#include <array>
#include <filesystem>
#include <iostream>
#include <vector>

#include <matplot/matplot.h>
#include <ntop_core/ntop_core.h>

int main() {
  // Select a .implicit file and load it
  std::cout << "Enter a path to an implicit (examples in the examples/assets "
               "folder): ";
  std::string path;
  std::cin >> path;
  ntop_core_handle implicit_handle{};

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

  // Transform the part for (build volume) orientation prior to slicing

  // Slice along the YZ plane of the part
  ntop_core_bounding_box bb{};
  ntop_core_query_bounding_box(implicit_handle, &bb);
  ntop_core_vec3 centroid{(bb.min.x + bb.max.x) / 2.0,
                          (bb.min.y + bb.max.y) / 2.0,
                          (bb.min.z + bb.max.z) / 2.0};
  ntop_core_frame const frame{centroid, {0, 1, 0}, {0, 0, 1}};

  // Create a new implicit to slice
  ntop_core_handle transformed_handle{};
  ntop_core_transformed(implicit_handle, &frame, &transformed_handle);

  // Free the memory of the original implicit
  ntop_core_release(implicit_handle);

  // Initialize the vector to hold the slice data
  std::vector<std::vector<std::array<double, 2>>> slice;

  // This lambda will be called for every independent polyline
  // We place a reference to slice in the capture clause so we can deposit
  // the planar points (x and y in the length 2 double arrays) into it.
  auto deposit = [&slice](ntop_core_vec2 const *points, uint32_t sz,
                          bool is_closed) {
    // Initialize a vector to hold this polyline curve
    std::vector<std::array<double, 2>> poly;
    poly.reserve(sz);

    // Iterate over the points in the polyline and store them
    for (uint32_t i = 0; i < sz; ++i) {
      poly.push_back({points[i].x, points[i].y});
    }

    // For our example when the curve is closed we want to draw the connection
    // so the first point is added again
    if (is_closed) {
      poly.push_back({points[0].x, points[0].y});
    }

    // Store the polyline curve to the slice
    slice.push_back(std::move(poly));
  };

  // Define the feature size of the slice
  double feature_size = 0.0001;

  // Slice the body at Z height
  double slice_z_height = 0.0;

  // Slice using the marching squares algorithm.
  // The function signature is the same as ntop_core_generate_slice_by_DC and
  // can be substituted depending on the specifics of the implicit being sliced
  // and desired slice characteristics: sharp corners should use dual contouring
  // while avoidance of possible slicing artifacts should use marching squares.
  ntop_core_generate_slice_by_MS(
      transformed_handle, slice_z_height, feature_size, &deposit,
      [](void *ctx, auto... ps) {
        return (*reinterpret_cast<decltype(deposit) *>(
            ctx))(std::forward<decltype(ps)>(ps)...);
      });

  // Free the memory used by the implicit as no more operations are performed
  // using nTop Core
  ntop_core_release(transformed_handle);

  // Provide a little debugging output
  std::cout << "Slices contours: " << slice.size() << std::endl;

  using namespace matplot;

  // plot all the polylines on this one figure
  matplot::hold(on);
  // Iterate over the contours in the slice and structure the data for the
  // graphing lib
  for (auto &contour : slice) {
    std::vector<double> x;
    std::vector<double> y;
    for (auto &point : contour) {
      x.push_back(point[0]);
      y.push_back(point[1]);
    }
    matplot::plot(x, y);
  }

  matplot::xlabel("meters");
  matplot::ylabel("meters");
  matplot::show();

  // Save the slice as a SVG
  matplot::save("sliceMS.svg");
  std::cout << "Slice image written to "
            << std::filesystem::current_path().append("sliceMS.svg")
            << std::endl;
}
