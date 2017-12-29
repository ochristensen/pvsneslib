#include "architecture.h"

Architecture::Architecture(Bass& self) : self(self) {
}

auto Architecture::assemble(const string& statement) -> bool {
    return false;
}

auto Architecture::pc() const -> uint {
    return self.pc();
}

auto Architecture::endian() const -> Bass::Endian {
    return self.endian;
}

auto Architecture::setEndian(Bass::Endian endian) -> void {
    self.endian = endian;
}

auto Architecture::evaluate(const string& expression, Bass::Evaluation mode) -> int64_t {
    return self.evaluate(expression, mode);
}

auto Architecture::write(uint64_t data, uint length) -> void {
    return self.write(data, length);
}
