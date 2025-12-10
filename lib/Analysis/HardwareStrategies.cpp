//===-- lib/Analysis/HardwareStrategies.cpp - Hardware Strategy Impl -*- C++
//-*-===//
//
// Part of the LLTA Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file implements concrete hardware strategy classes for cache and
/// branch prediction modeling in WCET analysis.
///
//===----------------------------------------------------------------------===//

#include "Analysis/HardwareStrategies.h"

#include <list>
#include <unordered_map>
#include <vector>

namespace llta {

//===----------------------------------------------------------------------===//
// AlwaysMissCache Implementation
//===----------------------------------------------------------------------===//

/// A conservative cache model that always returns Miss.
/// This provides a safe upper bound for WCET analysis.
class AlwaysMissCache : public CacheStrategy {
public:
  AccessResult access(uint64_t AbstractAddress, bool IsWrite) override {
    return Miss;
  }
};

//===----------------------------------------------------------------------===//
// LRUCache Implementation
//===----------------------------------------------------------------------===//

/// A set-associative LRU cache model.
/// Models cache behavior for more precise WCET analysis.
class LRUCache : public CacheStrategy {
  unsigned NumSets;
  unsigned Associativity;
  unsigned LineSize;

  // Each set contains a list of tags in LRU order (front = MRU, back = LRU)
  std::vector<std::list<uint64_t>> Sets;

public:
  /// Constructor for LRUCache.
  /// \param NumSets Number of cache sets.
  /// \param Associativity Number of ways per set.
  /// \param LineSize Size of each cache line in bytes.
  LRUCache(unsigned NumSets, unsigned Associativity, unsigned LineSize)
      : NumSets(NumSets), Associativity(Associativity), LineSize(LineSize),
        Sets(NumSets) {}

  AccessResult access(uint64_t AbstractAddress, bool IsWrite) override {
    // Calculate cache line address and set index
    uint64_t LineAddress = AbstractAddress / LineSize;
    uint64_t SetIndex = LineAddress % NumSets;
    uint64_t Tag = LineAddress / NumSets;

    std::list<uint64_t> &Set = Sets[SetIndex];

    // Search for the tag in the set
    for (auto It = Set.begin(); It != Set.end(); ++It) {
      if (*It == Tag) {
        // Hit: Move to front (MRU position)
        Set.erase(It);
        Set.push_front(Tag);
        return Hit;
      }
    }

    // Miss: Add to front and evict if necessary
    Set.push_front(Tag);
    if (Set.size() > Associativity) {
      Set.pop_back(); // Evict LRU entry
    }

    return Miss;
  }
};

//===----------------------------------------------------------------------===//
// AlwaysTakenPredictor Implementation
//===----------------------------------------------------------------------===//

/// A simple branch predictor that always predicts taken.
/// Conservative for loops but may overestimate for conditional branches.
class AlwaysTakenPredictor : public BranchPredictorStrategy {
public:
  PredictionResult predict(const llvm::MachineInstr &MI) override {
    // For WCET analysis, returning Unknown is conservative
    return Unknown;
  }

  void update(const llvm::MachineInstr &MI, bool Taken) override {
    // No state to update for static predictor
  }
};

//===----------------------------------------------------------------------===//
// Factory Functions
//===----------------------------------------------------------------------===//

std::unique_ptr<CacheStrategy> createAlwaysMissCache() {
  return std::make_unique<AlwaysMissCache>();
}

std::unique_ptr<CacheStrategy> createLRUCache(unsigned NumSets,
                                              unsigned Associativity,
                                              unsigned LineSize) {
  return std::make_unique<LRUCache>(NumSets, Associativity, LineSize);
}

std::unique_ptr<BranchPredictorStrategy> createAlwaysTakenPredictor() {
  return std::make_unique<AlwaysTakenPredictor>();
}

} // namespace llta
