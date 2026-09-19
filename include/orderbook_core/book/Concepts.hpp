#pragma once
#include <concepts>
#include <utility>
namespace rushevich {
template <typename Container, typename... Args>
concept FIFOContainer = requires(Container container, Args&&... args) {
    typename Container::value_type;
    typename Container::reference_type;

    { container.push(std::forward<Args>(args)...) };
    { container.emplace(std::forward<Args>(args)...) };
    { container.front() } -> std::convertible_to<typename Container::reference_type>;
    { container.back() } -> std::convertible_to<typename Container::reference_type>;
    { container.pop() };
    { container.empty() } -> std::convertible_to<bool>;
};
} // namespace rushevich
