#pragma once
#include "../core/core.h"
#include "../../nall/string.hpp"

struct Architecture {
  Architecture(Bass& self);
  virtual ~Architecture() = default;

  virtual auto assemble(const string& statement) -> bool;
  auto pc() const -> uint;
  auto endian() const -> Bass::Endian;
  auto setEndian(Bass::Endian endian) -> void;
  auto evaluate(const string& expression, Bass::Evaluation mode = Bass::Evaluation::Default) -> int64_t;
  auto write(uint64_t data, uint length = 1) -> void;

  template<typename... P> auto notice(P&&... p) -> void {
    return self.notice(forward<P>(p)...);
  }

  template<typename... P> auto warning(P&&... p) -> void {
    return self.warning(forward<P>(p)...);
  }

  template<typename... P> auto error(P&&... p) -> void {
    return self.error(forward<P>(p)...);
  }

  Bass& self;
};
