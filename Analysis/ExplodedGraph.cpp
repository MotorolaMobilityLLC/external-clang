//=-- ExplodedGraph.cpp - Local, Path-Sens. "Exploded Graph" -*- C++ -*------=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the template classes ExplodedNode and ExplodedGraph,
//  which represent a path-sensitive, intra-procedural "exploded graph."
//
//===----------------------------------------------------------------------===//

#include "clang/Analysis/PathSensitive/ExplodedGraph.h"
#include <vector>

using namespace clang;


static inline std::vector<ExplodedNodeImpl*>& getVector(void* P) {
  return *reinterpret_cast<std::vector<ExplodedNodeImpl*>*>(P);
}

void ExplodedNodeImpl::NodeGroup::addNode(ExplodedNodeImpl* N) {
  
  assert ((reinterpret_cast<uintptr_t>(N) & Mask) == 0x0);
  
  if (getKind() == Size1) {
    if (ExplodedNodeImpl* NOld = getNode()) {
      std::vector<ExplodedNodeImpl*>* V = new std::vector<ExplodedNodeImpl*>();
      assert ((reinterpret_cast<uintptr_t>(V) & Mask) == 0x0);
      V->push_back(NOld);
      V->push_back(N);
      P = reinterpret_cast<uintptr_t>(V) | SizeOther;
      assert (getPtr() == (void*) V);
      assert (getKind() == SizeOther);
    }
    else {
      P = reinterpret_cast<uintptr_t>(N);
      assert (getKind() == Size1);
    }
  }
  else {
    assert (getKind() == SizeOther);
    getVector(getPtr()).push_back(N);
  }
}

bool ExplodedNodeImpl::NodeGroup::empty() const {
  if (getKind() == Size1)
    return getNode() ? false : true;
  else
    return getVector(getPtr()).empty();
}

unsigned ExplodedNodeImpl::NodeGroup::size() const {
  if (getKind() == Size1)
    return getNode() ? 1 : 0;
  else
    return getVector(getPtr()).size();
}

ExplodedNodeImpl** ExplodedNodeImpl::NodeGroup::begin() const {
  if (getKind() == Size1)
    return (ExplodedNodeImpl**) &P;
  else
    return const_cast<ExplodedNodeImpl**>(&*(getVector(getPtr()).begin()));
}

ExplodedNodeImpl** ExplodedNodeImpl::NodeGroup::end() const {
  if (getKind() == Size1)
    return (ExplodedNodeImpl**) (P ? &P+1 : &P);
  else
    return const_cast<ExplodedNodeImpl**>(&*(getVector(getPtr()).end()));
}

ExplodedNodeImpl::NodeGroup::~NodeGroup() {
  if (getKind() == SizeOther) delete &getVector(getPtr());
}
