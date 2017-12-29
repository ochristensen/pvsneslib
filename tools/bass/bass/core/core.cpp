#include "../architecture/architecture.h"
#include "../../nall/path.hpp"
#include "../architecture/table/table.h"
#include "core.h"

auto Bass::evaluate(const string& expression, Evaluation mode) -> int64_t {
  maybe<string> name;
  if(expression == "--") name = {"lastLabel#", lastLabelCounter - 2};
  if(expression == "-" ) name = {"lastLabel#", lastLabelCounter - 1};
  if(expression == "+" ) name = {"nextLabel#", nextLabelCounter + 0};
  if(expression == "++") name = {"nextLabel#", nextLabelCounter + 1};
  if(name) {
    if(auto constant = findConstant({name()})) return constant().value;
    if(queryPhase()) return pc();
    error("relative label not declared");
  }

  Eval::Node* node = nullptr;
  try {
    node = Eval::parse(expression);
  } catch(...) {
    error("malformed expression: ", expression);
  }
  return evaluate(node, mode);
}

auto Bass::evaluate(Eval::Node* node, Evaluation mode) -> int64_t {
  #define p(n) evaluate(node->link[n], mode)

  switch(node->type) {
  case Eval::Node::Type::Null: return 0;  //empty expressions
  case Eval::Node::Type::Function: return evaluateExpression(node, mode);
  case Eval::Node::Type::Literal: return evaluateLiteral(node, mode);
  case Eval::Node::Type::LogicalNot: return !p(0);
  case Eval::Node::Type::BitwiseNot: return ~p(0);
  case Eval::Node::Type::Positive: return +p(0);
  case Eval::Node::Type::Negative: return -p(0);
  case Eval::Node::Type::Multiply: return p(0) * p(1);
  case Eval::Node::Type::Divide: return p(0) / p(1);
  case Eval::Node::Type::Modulo: return p(0) % p(1);
  case Eval::Node::Type::Add: return p(0) + p(1);
  case Eval::Node::Type::Subtract: return p(0) - p(1);
  case Eval::Node::Type::ShiftLeft: return p(0) << p(1);
  case Eval::Node::Type::ShiftRight: return p(0) >> p(1);
  case Eval::Node::Type::BitwiseAnd: return p(0) & p(1);
  case Eval::Node::Type::BitwiseOr: return p(0) | p(1);
  case Eval::Node::Type::BitwiseXor: return p(0) ^ p(1);
  case Eval::Node::Type::Equal: return p(0) == p(1);
  case Eval::Node::Type::NotEqual: return p(0) != p(1);
  case Eval::Node::Type::LessThanEqual: return p(0) <= p(1);
  case Eval::Node::Type::GreaterThanEqual: return p(0) >= p(1);
  case Eval::Node::Type::LessThan: return p(0) < p(1);
  case Eval::Node::Type::GreaterThan: return p(0) > p(1);
  case Eval::Node::Type::LogicalAnd: return p(0) ? p(1) : 0;
  case Eval::Node::Type::LogicalOr: return !p(0) ? p(1) : 1;
  case Eval::Node::Type::Condition: return p(0) ? p(1) : p(2);
  case Eval::Node::Type::Assign: return evaluateAssign(node, mode);
  }

  #undef p
  error("unsupported operator");
}

auto Bass::evaluateParameters(Eval::Node* node, Evaluation mode) -> vector<int64_t> {
  vector<int64_t> result;
  if(node->type == Eval::Node::Type::Null) return result;
  if(node->type != Eval::Node::Type::Separator) { result.append(evaluate(node, mode)); return result; }
  for(auto& link : node->link) result.append(evaluate(link, mode));
  return result;
}

auto Bass::evaluateExpression(Eval::Node* node, Evaluation mode) -> int64_t {
  auto parameters = evaluateParameters(node->link[1], mode);
  string name = node->link[0]->literal;
  if(parameters) name.append("#", parameters.size());

  if(name == "origin") return origin;
  if(name == "base") return base;
  if(name == "pc") return pc();

  if(auto expression = findExpression(name)) {
    if(parameters) frames.append({0, true});
    for(auto n : range(parameters.size())) {
      setVariable(expression().parameters(n), evaluate(parameters(n)), Frame::Level::Inline);
    }
    auto result = evaluate(expression().value);
    if(parameters) frames.removeRight();
    return result;
  }

  error("unrecognized expression: ", name);
}

auto Bass::evaluateLiteral(Eval::Node* node, Evaluation mode) -> int64_t {
  string& s = node->literal;

  if(s[0] == '0' && s[1] == 'b') return toBinary(s);
  if(s[0] == '0' && s[1] == 'o') return toOctal(s);
  if(s[0] == '0' && s[1] == 'x') return toHex(s);
  if(s[0] >= '0' && s[0] <= '9') return toInteger(s);
  if(s[0] == '%') return toBinary(s);
  if(s[0] == '$') return toHex(s);
  if(s.match("'?*'")) return character(s);

  if(auto variable = findVariable(s)) return variable().value;
  if(auto constant = findConstant(s)) return constant().value;
  if(mode != Evaluation::Strict && queryPhase()) return pc();

  error("unrecognized variable: ", s);
}

auto Bass::evaluateAssign(Eval::Node* node, Evaluation mode) -> int64_t {
  string& s = node->link[0]->literal;

  if(auto variable = findVariable(s)) {
    variable().value = evaluate(node->link[1], mode);
    return variable().value;
  }

  error("unrecognized variable assignment: ", s);
}

auto Bass::analyze() -> bool {
  blocks.reset();
  ip = 0;

  while(ip < program.size()) {
    Instruction& i = program(ip++);
    if(!analyzeInstruction(i)) error("unrecognized directive: ", i.statement);
  }

  return true;
}

auto Bass::analyzeInstruction(Instruction& i) -> bool {
  string s = i.statement;

  if(s.match("}") && !blocks) error("} without matching {");

  if(s.match("{")) {
    blocks.append({ip - 1, "block"});
    i.statement = "block {";
    return true;
  }

  if(s.match("}") && blocks.right().type == "block") {
    blocks.removeRight();
    i.statement = "} endblock";
    return true;
  }

  if(s.match("namespace ?* {")) {
    blocks.append({ip - 1, "namespace"});
    return true;
  }

  if(s.match("}") && blocks.right().type == "namespace") {
    blocks.removeRight();
    i.statement = "} endnamespace";
    return true;
  }

  if(s.match("function ?* {")) {
    blocks.append({ip - 1, "function"});
    return true;
  }

  if(s.match("}") && blocks.right().type == "function") {
    blocks.removeRight();
    i.statement = "} endfunction";
    return true;
  }

  if(s.match("macro ?*(*) {")) {
    blocks.append({ip - 1, "macro"});
    return true;
  }

  if(s.match("}") && blocks.right().type == "macro") {
    uint rp = blocks.right().ip;
    program[rp].ip = ip;
    blocks.removeRight();
    i.statement = "} endmacro";
    return true;
  }

  if(s.match("inline ?*(*) {")) {
    blocks.append({ip - 1, "inline"});
    return true;
  }

  if(s.match("}") && blocks.right().type == "inline") {
    uint rp = blocks.right().ip;
    program[rp].ip = ip;
    blocks.removeRight();
    i.statement = "} endinline";
    return true;
  }

  if(s.match("?*: {") || s.match("- {") || s.match("+ {")) {
    blocks.append({ip - 1, "constant"});
    return true;
  }

  if(s.match("}") && blocks.right().type == "constant") {
    blocks.removeRight();
    i.statement = "} endconstant";
    return true;
  }

  if(s.match("if ?* {")) {
    s.trim("if ", " {", 1L);
    blocks.append({ip - 1, "if"});
    return true;
  }

  if(s.match("} else if ?* {")) {
    s.trim("} else if ", " {", 1L);
    uint rp = blocks.right().ip;
    program[rp].ip = ip - 1;
    blocks.right().ip = ip - 1;
    return true;
  }

  if(s.match("} else {")) {
    uint rp = blocks.right().ip;
    program[rp].ip = ip - 1;
    blocks.right().ip = ip - 1;
    return true;
  }

  if(s.match("}") && blocks.right().type == "if") {
    uint rp = blocks.right().ip;
    program[rp].ip = ip - 1;
    blocks.removeRight();
    i.statement = "} endif";
    return true;
  }

  if(s.match("while ?* {")) {
    s.trim("while ", " {", 1L);
    blocks.append({ip - 1, "while"});
    return true;
  }

  if(s.match("}") && blocks.right().type == "while") {
    uint rp = blocks.right().ip;
    program[rp].ip = ip;
    blocks.removeRight();
    i.statement = "} endwhile";
    i.ip = rp;
    return true;
  }

  return true;
}

auto Bass::execute() -> bool {
  frames.reset();
  conditionals.reset();
  ip = 0;
  macroInvocationCounter = 0;

  initialize();

  frames.append({0, false});
  for(auto& define : defines) {
    setDefine(define.name, {}, define.value, Frame::Level::Inline);
  }

  while(ip < program.size()) {
    Instruction& i = program(ip++);
    if(!executeInstruction(i)) error("unrecognized directive: ", i.statement);
  }

  frames.removeRight();
  return true;
}

auto Bass::executeInstruction(Instruction& i) -> bool {
  activeInstruction = &i;
  string s = i.statement;
  evaluateDefines(s);

  bool global = s.beginsWith("global ");
  bool parent = s.beginsWith("parent ");
  if(global && parent) error("multiple frame specifiers are not allowed");

  Frame::Level level = Frame::Level::Active;
  if(global) s.trimLeft("global ", 1L), level = Frame::Level::Global;
  if(parent) s.trimLeft("parent ", 1L), level = Frame::Level::Parent;

  if(s.match("macro ?*(*) {")) {
    bool inlined = false;
    s.trim("macro ", ") {", 1L);
    auto p = s.split("(", 1L).strip();
    auto parameters = split(p(1));
    setMacro(p(0), parameters, ip, inlined, level);
    ip = i.ip;
    return true;
  }

  if(s.match("inline ?*(*) {")) {
    bool inlined = true;
    s.trim("inline ", ") {", 1L);
    auto p = s.split("(", 1L).strip();
    auto parameters = split(p(1));
    setMacro(p(0), parameters, ip, inlined, level);
    ip = i.ip;
    return true;
  }

  if(s.match("define ?*(*)*")) {
    auto e = s.trimLeft("define ", 1L).split("=", 1L).strip();
    auto p = e(0).trimRight(")", 1L).split("(", 1L).strip();
    auto parameters = split(p(1));
    setDefine(p(0), parameters, e(1), level);
    return true;
  }

  if(s.match("define ?*")) {
    auto p = s.trimLeft("define ", 1L).split("=", 1L).strip();
    setDefine(p(0), {}, p(1), level);
    return true;
  }

  if(s.match("evaluate ?*")) {
    auto p = s.trimLeft("evaluate ", 1L).split("=", 1L).strip();
    setDefine(p(0), {}, evaluate(p(1)), level);
    return true;
  }

  if(s.match("expression ?*(*)*")) {
    auto e = s.trimLeft("expression ", 1L).split("=", 1L).strip();
    auto p = e(0).trimRight(")", 1L).split("(", 1L).strip();
    auto parameters = split(p(1));
    setExpression(p(0), parameters, e(1), level);
    return true;
  }

  if(s.match("variable ?*")) {
    auto p = s.trimLeft("variable ", 1L).split("=", 1L).strip();
    setVariable(p(0), evaluate(p(1)), level);
    return true;
  }

  if(global || parent) error("invalid frame specifier");

  if(s.match("if ?* {")) {
    s.trim("if ", " {", 1L).strip();
    bool match = evaluate(s, Evaluation::Strict);
    conditionals.append(match);
    if(match == false) {
      ip = i.ip;
    }
    return true;
  }

  if(s.match("} else if ?* {")) {
    if(conditionals.right()) {
      ip = i.ip;
    } else {
      s.trim("} else if ", " {", 1L).strip();
      bool match = evaluate(s, Evaluation::Strict);
      conditionals.right() = match;
      if(match == false) {
        ip = i.ip;
      }
    }
    return true;
  }

  if(s.match("} else {")) {
    if(conditionals.right()) {
      ip = i.ip;
    } else {
      conditionals.right() = true;
    }
    return true;
  }

  if(s.match("} endif")) {
    conditionals.removeRight();
    return true;
  }

  if(s.match("while ?* {")) {
    s.trim("while ", " {", 1L).strip();
    bool match = evaluate(s, Evaluation::Strict);
    if(match == false) ip = i.ip;
    return true;
  }

  if(s.match("} endwhile")) {
    ip = i.ip;
    return true;
  }

  if(s.match("?*(*)")) {
    auto p = string{s}.trimRight(")", 1L).split("(", 1L).strip();
    auto name = p(0);
    auto parameters = split(p(1));
    if(parameters) name.append("#", parameters.size());
    if(auto macro = findMacro({name})) {
      frames.append({ip, macro().inlined});
      if(!frames.right().inlined) scope.append(p(0));

      setDefine("#", {}, {"_", macroInvocationCounter++, "_"}, Frame::Level::Inline);
      for(uint n : range(parameters)) {
        auto p = macro().parameters(n).split(" ", 1L).strip();
        if(p.size() == 1) p.prepend("define");

        if(0);
        else if(p[0] == "define") setDefine(p[1], {}, parameters(n), Frame::Level::Inline);
        else if(p[0] == "string") setDefine(p[1], {}, text(parameters(n)), Frame::Level::Inline);
        else if(p[0] == "evaluate") setDefine(p[1], {}, evaluate(parameters(n)), Frame::Level::Inline);
        else if(p[0] == "variable") setVariable(p[1], evaluate(parameters(n)), Frame::Level::Inline);
        else error("unsupported parameter type: ", p[0]);
      }

      ip = macro().ip;
      return true;
    }
  }

  if(s.match("} endmacro") || s.match("} endinline")) {
    ip = frames.right().ip;
    if(!frames.right().inlined) scope.removeRight();
    frames.removeRight();
    return true;
  }

  if(assemble(s)) {
    return true;
  }

  evaluate(s);
  return true;
}

auto Bass::initialize() -> void {
  queue.reset();
  scope.reset();
  for(uint n : range(256)) stringTable[n] = n;
  endian = Endian::LSB;
  origin = 0;
  base = 0;
  lastLabelCounter = 1;
  nextLabelCounter = 1;
}

auto Bass::assemble(const string& statement) -> bool {
  string s = statement;

  if(s.match("block {")) return true;
  if(s.match("} endblock")) return true;

  //namespace name {
  if(s.match("namespace ?* {")) {
    s.trim("namespace ", "{", 1L).strip();
    if(!validate(s)) error("invalid namespace specifier: ", s);
    scope.append(s);
    return true;
  }

  //}
  if(s.match("} endnamespace")) {
    scope.removeRight();
    return true;
  }

  //function name {
  if(s.match("function ?* {")) {
    s.trim("function ", "{", 1L).strip();
    setConstant(s, pc());
    scope.append(s);
    return true;
  }

  //}
  if(s.match("} endfunction")) {
    scope.removeRight();
    return true;
  }

  //constant name(value)
  if(s.match("constant ?*")) {
    auto p = s.trimLeft("constant ", 1L).split("=", 1L).strip();
    setConstant(p(0), evaluate(p(1)));
    return true;
  }

  //label: or label: {
  if(s.match("?*:") || s.match("?*: {")) {
    s.trimRight(" {", 1L);
    s.trimRight(":", 1L);
    setConstant(s, pc());
    return true;
  }

  //- or - {
  if(s.match("-") || s.match("- {")) {
    setConstant({"lastLabel#", lastLabelCounter++}, pc());
    return true;
  }

  //+ or + {
  if(s.match("+") || s.match("+ {")) {
    setConstant({"nextLabel#", nextLabelCounter++}, pc());
    return true;
  }

  //}
  if(s.match("} endconstant")) {
    return true;
  }

  //output "filename" [, create]
  if(s.match("output ?*")) {
    auto p = split(s.trimLeft("output ", 1L));
    string filename = {filepath(), p.take(0).trim("\"", "\"", 1L)};
    bool create = (p.size() && p(0) == "create");
    target(filename, create);
    return true;
  }

  //architecture name
  if(s.match("architecture ?*")) {
    s.trimLeft("architecture ", 1L);
    if(s == "none") architecture = new Architecture{*this};
    else {
      string location{Path::local(), "bass/architectures/", s, ".arch"};
      if(!file::exists(location)) location = {Path::program(), "architectures/", s, ".arch"};
      if(!file::exists(location)) error("unknown architecture: ", s);
      architecture = new Table{*this, string::read(location)};
    }
    return true;
  }

  //endian (lsb|msb)
  if(s.match("endian ?*")) {
    s.trimLeft("endian ", 1L);
    if(s == "lsb") { endian = Endian::LSB; return true; }
    if(s == "msb") { endian = Endian::MSB; return true; }
    error("invalid endian mode");
  }

  //origin offset
  if(s.match("origin ?*")) {
    s.trimLeft("origin ", 1L);
    origin = evaluate(s);
    seek(origin);
    return true;
  }

  //base offset
  if(s.match("base ?*")) {
    s.trimLeft("base ", 1L);
    base = evaluate(s) - origin;
    return true;
  }

  //enqueue variable [, ...]
  if(s.match("enqueue ?*")) {
    auto p = split(s.trimLeft("enqueue ", 1L));
    for(auto& t : p) {
      if(t == "origin") {
        queue.append(origin);
      } else if(t == "base") {
        queue.append(base);
      } else if(t == "pc") {
        queue.append(origin);
        queue.append(base);
      } else {
        error("unrecognized enqueue variable: ", t);
      }
    }
    return true;
  }

  //dequeue variable [, ...]
  if(s.match("dequeue ?*")) {
    auto p = split(s.trimLeft("dequeue ", 1L));
    for(auto& t : p) {
      if(t == "origin") {
        origin = queue.takeRight().natural();
        seek(origin);
      } else if(t == "base") {
        base = queue.takeRight().integer();
      } else if(t == "pc") {
        base = queue.takeRight().integer();
        origin = queue.takeRight().natural();
        seek(origin);
      } else {
        error("unrecognized dequeue variable: ", t);
      }
    }
    return true;
  }

  //insert [name, ] filename [, offset] [, length]
  if(s.match("insert ?*")) {
    auto p = split(s.trimLeft("insert ", 1L));
    string name;
    if(!p(0).match("\"*\"")) name = p.take(0);
    if(!p(0).match("\"*\"")) error("missing filename");
    string filename = {filepath(), p.take(0).trim("\"", "\"", 1L)};
    file fp;
    if(!fp.open(filename, file::mode::read)) error("file not found: ", filename);
    uint offset = p.size() ? evaluate(p.take(0)) : 0;
    if(offset > fp.size()) offset = fp.size();
    uint length = p.size() ? evaluate(p.take(0)) : 0;
    if(length == 0) length = fp.size() - offset;
    if(name) {
      setConstant({name}, pc());
      setConstant({name, ".size"}, length);
    }
    fp.seek(offset);
    while(!fp.end() && length--) write(fp.read());
    return true;
  }

  //fill length [, with]
  if(s.match("fill ?*")) {
    auto p = split(s.trimLeft("fill ", 1L));
    uint length = evaluate(p(0));
    uint byte = evaluate(p(1, "0"));
    while(length--) write(byte);
    return true;
  }

  //map 'char' [, value] [, length]
  if(s.match("map ?*")) {
    auto p = split(s.trimLeft("map ", 1L));
    uint8_t index = evaluate(p(0));
    int64_t value = evaluate(p(1, "0"));
    int64_t length = evaluate(p(2, "1"));
    for(int n : range(length)) {
      stringTable[index + n] = value + n;
    }
    return true;
  }

  //d[bwldq] ("string"|variable) [, ...]
  uint dataLength = 0;
  if(s.beginsWith("db ")) dataLength = 1;
  if(s.beginsWith("dw ")) dataLength = 2;
  if(s.beginsWith("dl ")) dataLength = 3;
  if(s.beginsWith("dd ")) dataLength = 4;
  if(s.beginsWith("dq ")) dataLength = 8;
  if(dataLength) {
    s = slice(s, 3);  //remove prefix
    auto p = split(s);
    for(auto& t : p) {
      if(t.match("\"*\"")) {
        t = text(t);
        for(auto& b : t) write(stringTable[b], dataLength);
      } else {
        write(evaluate(t), dataLength);
      }
    }
    return true;
  }

  //print ("string"|[cast:]variable) [, ...]
  if(s.match("print ?*")) {
    s.trimLeft("print ", 1L).strip();
    if(writePhase()) {
      auto p = split(s);
      for(auto& t : p) {
        if(t.match("\"*\"")) {
          print(stderr, text(t));
        } else if(t.match("binary:?*")) {
          t.trimLeft("binary:", 1L);
          print(stderr, binary(evaluate(t)));
        } else if(t.match("hex:?*")) {
          t.trimLeft("hex:", 1L);
          print(stderr, hex(evaluate(t)));
        } else if(t.match("char:?*")) {
          t.trimLeft("char:", 1L);
          print(stderr, (char)evaluate(t));
        } else {
          print(stderr, evaluate(t));
        }
      }
    }
    return true;
  }

  //notice "string"
  if(s.match("notice \"*\"")) {
    if(writePhase()) {
      s.trimLeft("notice ", 1L).strip();
      notice(text(s));
    }
    return true;
  }

  //warning "string"
  if(s.match("warning \"*\"")) {
    if(writePhase()) {
      s.trimLeft("warning ", 1L).strip();
      warning(text(s));
    }
    return true;
  }

  //error "string"
  if(s.match("error \"*\"")) {
    if(writePhase()) {
      s.trimLeft("error ", 1L).strip();
      error(text(s));
    }
    return true;
  }

  return architecture->assemble(statement);
}
auto Bass::setMacro(const string& name, const string_vector& parameters, uint ip, bool inlined, Frame::Level level) -> void {
  if(!validate(name)) error("invalid macro identifier: ", name);
  string scopedName = {scope.merge("."), scope ? "." : "", name};
  if(parameters) scopedName.append("#", parameters.size());

  for(int n : rrange(frames.size())) {
    if(level != Frame::Level::Inline) {
      if(frames[n].inlined) continue;
      if(level == Frame::Level::Global && n) { continue; }
      if(level == Frame::Level::Parent && n) { level = Frame::Level::Active; continue; }
    }

    auto& macros = frames[n].macros;
    if(auto macro = macros.find({scopedName})) {
      macro().parameters = parameters;
      macro().ip = ip;
      macro().inlined = inlined;
    } else {
      macros.insert({scopedName, parameters, ip, inlined});
    }

    return;
  }
}

auto Bass::findMacro(const string& name) -> maybe<Macro&> {
  for(int n : rrange(frames.size())) {
    auto& macros = frames[n].macros;
    auto s = scope;
    while(true) {
      string scopedName = {s.merge("."), s ? "." : "", name};
      if(auto macro = macros.find({scopedName})) {
        return macro();
      }
      if(!s) break;
      s.removeRight();
    }
  }

  return nothing;
}

auto Bass::setDefine(const string& name, const string_vector& parameters, const string& value, Frame::Level level) -> void {
  if(!validate(name)) error("invalid define identifier: ", name);
  string scopedName = {scope.merge("."), scope ? "." : "", name};
  if(parameters) scopedName.append("#", parameters.size());

  for(int n : rrange(frames.size())) {
    if(level != Frame::Level::Inline) {
      if(frames[n].inlined) continue;
      if(level == Frame::Level::Global && n) { continue; }
      if(level == Frame::Level::Parent && n) { level = Frame::Level::Active; continue; }
    }

    auto& defines = frames[n].defines;
    if(auto define = defines.find({scopedName})) {
      define().parameters = parameters;
      define().value = value;
    } else {
      defines.insert({scopedName, parameters, value});
    }

    return;
  }
}

auto Bass::findDefine(const string& name) -> maybe<Define&> {
  for(int n : rrange(frames.size())) {
    auto& defines = frames[n].defines;
    auto s = scope;
    while(true) {
      string scopedName = {s.merge("."), s ? "." : "", name};
      if(auto define = defines.find({scopedName})) {
        return define();
      }
      if(!s) break;
      s.removeRight();
    }
  }

  return nothing;
}

auto Bass::setExpression(const string& name, const string_vector& parameters, const string& value, Frame::Level level) -> void {
  if(!validate(name)) error("invalid expression identifier: ", name);
  string scopedName = {scope.merge("."), scope ? "." : "", name};
  if(parameters) scopedName.append("#", parameters.size());

  for(int n : rrange(frames.size())) {
    if(level != Frame::Level::Inline) {
      if(frames[n].inlined) continue;
      if(level == Frame::Level::Global && n) { continue; }
      if(level == Frame::Level::Parent && n) { level = Frame::Level::Active; continue; }
    }

    auto& expressions = frames[n].expressions;
    if(auto expression = expressions.find({scopedName})) {
      expression().parameters = parameters;
      expression().value = value;
    } else {
      expressions.insert({scopedName, parameters, value});
    }

    return;
  }
}

auto Bass::findExpression(const string& name) -> maybe<Expression&> {
  for(int n : rrange(frames.size())) {
    auto& expressions = frames[n].expressions;
    auto s = scope;
    while(true) {
      string scopedName = {s.merge("."), s ? "." : "", name};
      if(auto expression = expressions.find({scopedName})) {
        return expression();
      }
      if(!s) break;
      s.removeRight();
    }
  }

  return nothing;
}

auto Bass::setVariable(const string& name, int64_t value, Frame::Level level) -> void {
  if(!validate(name)) error("invalid variable identifier: ", name);
  string scopedName = {scope.merge("."), scope ? "." : "", name};

  for(int n : rrange(frames.size())) {
    if(level != Frame::Level::Inline) {
      if(frames[n].inlined) continue;
      if(level == Frame::Level::Global && n) { continue; }
      if(level == Frame::Level::Parent && n) { level = Frame::Level::Active; continue; }
    }

    auto& variables = frames[n].variables;
    if(auto variable = variables.find({scopedName})) {
      variable().value = value;
    } else {
      variables.insert({scopedName, value});
    }

    return;
  }
}

auto Bass::findVariable(const string& name) -> maybe<Variable&> {
  for(int n : rrange(frames.size())) {
    auto& variables = frames[n].variables;
    auto s = scope;
    while(true) {
      string scopedName = {s.merge("."), s ? "." : "", name};
      if(auto variable = variables.find({scopedName})) {
        return variable();
      }
      if(!s) break;
      s.removeRight();
    }
  }

  return nothing;
}

auto Bass::setConstant(const string& name, int64_t value) -> void {
  if(!validate(name)) error("invalid constant identifier: ", name);
  string scopedName = {scope.merge("."), scope ? "." : "", name};

  if(auto constant = constants.find({scopedName})) {
    if(queryPhase()) error("constant cannot be modified: ", scopedName);
    constant().value = value;
  } else {
    constants.insert({scopedName, value});
  }
}

auto Bass::findConstant(const string& name) -> maybe<Constant&> {
  auto s = scope;
  while(true) {
    string scopedName = {s.merge("."), s ? "." : "", name};
    if(auto constant = constants.find({scopedName})) {
      return constant();
    }
    if(!s) break;
    s.removeRight();
  }

  return nothing;
}

auto Bass::evaluateDefines(string& s) -> void {
  for(int x = s.size() - 1, y = -1; x >= 0; x--) {
    if(s[x] == '}') y = x;
    if(s[x] == '{' && y > x) {
      string name = slice(s, x + 1, y - x - 1);

      if(name.match("defined ?*")) {
        name.trimLeft("defined ", 1L).strip();
        s = {slice(s, 0, x), findDefine(name) ? 1 : 0, slice(s, y + 1)};
        return evaluateDefines(s);
      }

      string_vector parameters;
      if(name.match("?*(*)")) {
        auto p = name.trimRight(")", 1L).split("(", 1L).strip();
        name = p(0);
        parameters = split(p(1));
      }
      if(parameters) name.append("#", parameters.size());

      if(auto define = findDefine(name)) {
        if(parameters) frames.append({0, true});
        for(auto n : range(parameters.size())) {
          auto p = define().parameters(n).split(" ", 1L).strip();
          if(p.size() == 1) p.prepend("define");

          if(0);
          else if(p[0] == "define") setDefine(p[1], {}, parameters(n), Frame::Level::Inline);
          else if(p[0] == "string") setDefine(p[1], {}, text(parameters(n)), Frame::Level::Inline);
          else if(p[0] == "evaluate") setDefine(p[1], {}, evaluate(parameters(n)), Frame::Level::Inline);
          else error("unsupported parameter type: ", p[0]);
        }
        auto value = define().value;
        evaluateDefines(value);
        s = {slice(s, 0, x), value, slice(s, y + 1)};
        if(parameters) frames.removeRight();
        return evaluateDefines(s);
      }
    }
  }
}

auto Bass::filepath() -> string {
  return Location::path(sourceFilenames[activeInstruction->fileNumber]);
}

//split argument list by commas, being aware of parenthesis depth and quotes
auto Bass::split(const string& s) -> string_vector {
  string_vector result;
  uint offset = 0;
  char quoted = 0;
  uint depth = 0;
  for(uint n : range(s.size())) {
    if(!quoted) {
      if(s[n] == '"' || s[n] == '\'') quoted = s[n];
    } else if(quoted == s[n]) {
      quoted = 0;
    }
    if(s[n] == '(' && !quoted) depth++;
    if(s[n] == ')' && !quoted) depth--;
    if(s[n] == ',' && !quoted && !depth) {
      result.append(slice(s, offset, n - offset));
      offset = n + 1;
    }
  }
  if(offset < s.size()) result.append(slice(s, offset, s.size() - offset));
  if(quoted) error("mismatched quotes in expression");
  if(depth) error("mismatched parentheses in expression");
  return result.strip();
}

//reduce all duplicate whitespace segments (eg "  ") to single whitespace (" ")
auto Bass::strip(string& s) -> void {
  uint offset = 0;
  char quoted = 0;
  for(uint n : range(s.size())) {
    if(!quoted) {
      if(s[n] == '"' || s[n] == '\'') quoted = s[n];
    } else if(quoted == s[n]) {
      quoted = 0;
    }
    if(!quoted && s[n] == ' ' && s[n + 1] == ' ') continue;
    s.get()[offset++] = s[n];
  }
  s.resize(offset);
}

//returns true for valid name identifiers
auto Bass::validate(const string& s) -> bool {
  for(uint n : range(s.size())) {
    char c = s[n];
    if(c == '_' || c == '#') continue;
    if(c >= 'A' && c <= 'Z') continue;
    if(c >= 'a' && c <= 'z') continue;
    if(c >= '0' && c <= '9' && n) continue;
    if(c == '.' && n) continue;
    return false;
  }
  return true;
}

auto Bass::text(string s) -> string {
  if(!s.match("\"*\"")) warning("string value is unquoted: ", s);
  s.trim("\"", "\"", 1L);
  s.replace("\\s", "\'");
  s.replace("\\d", "\"");
  s.replace("\\c", ",");
  s.replace("\\b", ";");
  s.replace("\\n", "\n");
  s.replace("\\\\", "\\");
  return s;
}

auto Bass::character(const string& s) -> int64_t {
  if(s[0] != '\'') goto unknown;
  if(s[2] == '\'') return s[1];
  if(s[3] != '\'') goto unknown;
  if(s[1] != '\\') goto unknown;
  if(s[2] == 's') return '\'';
  if(s[2] == 'd') return '\"';
  if(s[2] == 'c') return ',';
  if(s[2] == 'b') return ';';
  if(s[2] == 'n') return '\n';
  if(s[2] == '\\') return '\\';

unknown:
  warning("unrecognized character constant: ", s);
  return 0;
}

auto Bass::target(const string& filename, bool create) -> bool {
  if(targetFile.open()) targetFile.close();
  if(!filename) return true;

  //cannot modify a file unless it exists
  if(!file::exists(filename)) create = true;

  if(!targetFile.open(filename, create ? file::mode::write : file::mode::modify)) {
    print(stderr, "warning: unable to open target file: ", filename, "\n");
    return false;
  }

  return true;
}

auto Bass::source(const string& filename) -> bool {
  if(!file::exists(filename)) {
    print(stderr, "warning: source file not found: ", filename, "\n");
    return false;
  }

  uint fileNumber = sourceFilenames.size();
  sourceFilenames.append(filename);

  string data = file::read(filename);
  data.transform("\t\r", "  ");

  auto lines = data.split("\n");
  for(uint lineNumber : range(lines)) {
    if(auto position = lines[lineNumber].qfind("//")) {
      lines[lineNumber].resize(position());  //remove comments
    }

    auto blocks = lines[lineNumber].qsplit(";").strip();
    for(uint blockNumber : range(blocks)) {
      string statement = blocks[blockNumber];
      strip(statement);
      if(!statement) continue;

      if(statement.match("include \"?*\"")) {
        statement.trim("include \"", "\"", 1L);
        source({Location::path(filename), statement});
      } else {
        Instruction instruction;
        instruction.statement = statement;
        instruction.fileNumber = fileNumber;
        instruction.lineNumber = 1 + lineNumber;
        instruction.blockNumber = 1 + blockNumber;
        program.append(instruction);
      }
    }
  }

  return true;
}

auto Bass::define(const string& name, const string& value) -> void {
  defines.insert({name, {}, value});
}

auto Bass::constant(const string& name, const string& value) -> void {
  try {
    constants.insert({name, evaluate(value, Evaluation::Strict)});
  } catch(...) {
  }
}

auto Bass::assemble(bool strict) -> bool {
  this->strict = strict;

  try {
    phase = Phase::Analyze;
    analyze();

    phase = Phase::Query;
    architecture = new Architecture{*this};
    execute();

    phase = Phase::Write;
    architecture = new Architecture{*this};
    execute();
  } catch(...) {
    return false;
  }

  return true;
}

//internal

auto Bass::pc() const -> uint {
  return origin + base;
}

auto Bass::seek(uint offset) -> void {
  if(!targetFile.open()) return;
  if(writePhase()) targetFile.seek(offset);
}

auto Bass::write(uint64_t data, uint length) -> void {
  if(writePhase()) {
    if(targetFile.open()) {
      if(endian == Endian::LSB) targetFile.writel(data, length);
      if(endian == Endian::MSB) targetFile.writem(data, length);
    } else if(!isatty(fileno(stdout))) {
      if(endian == Endian::LSB) for(auto n :  range(length)) fputc(data >> n * 8, stdout);
      if(endian == Endian::MSB) for(auto n : rrange(length)) fputc(data >> n * 8, stdout);
    }
  }
  origin += length;
}

auto Bass::printInstruction() -> void {
  if(activeInstruction) {
    auto& i = *activeInstruction;
    print(stderr, sourceFilenames[i.fileNumber], ":", i.lineNumber, ":", i.blockNumber, ": ", i.statement, "\n");
  }
}

template<typename... P> auto Bass::notice(P&&... p) -> void {
  string s{forward<P>(p)...};
  print(stderr, "notice: ", s, "\n");
  printInstruction();
}

template<typename... P> auto Bass::warning(P&&... p) -> void {
  string s{forward<P>(p)...};
  print(stderr, "warning: ", s, "\n");
  printInstruction();

  if(!strict) return;
  struct BassWarning {};
  throw BassWarning();
}

template<typename... P> auto Bass::error(P&&... p) -> void {
  string s{forward<P>(p)...};
  print(stderr, "error: ", s, "\n");
  printInstruction();

  struct BassError {};
  throw BassError();
}
