#ifndef WEIGHT_PROVIDER_H
#define WEIGHT_PROVIDER_H

#include <type_traits>

namespace llvm {

template <typename T> class WeightProvider {
public:
  virtual ~WeightProvider() = default;

  virtual T getWeight(unsigned nodeId) const = 0;
};

} // namespace llvm

#endif // WEIGHT_PROVIDER_H
