#include <print>
#include <optional>

using return_type = std::optional<int *>;

auto goofy_function() -> return_type {
  int *heap_allocated_int{new int{69}};
  if (heap_allocated_int != nullptr) {
      return heap_allocated_int;
  }
  return {}; // nullopt
}

int main() {
  std::println("Hello world!");
  auto heap_ptr{goofy_function()};
  if (heap_ptr.has_value()) {
    delete *heap_ptr;
    std::println("Memory has been cleaned up!");
  }
  return 0;
}
