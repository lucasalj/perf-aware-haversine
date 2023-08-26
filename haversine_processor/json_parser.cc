#include "json_parser.h"

#include <charconv>

namespace json_parser {

namespace {
void skipWhiteSpace(Context &ctx) {
  while (ctx.mCurrentPos < ctx.mInput.size()) {
    auto currChar = ctx.mInput[ctx.mCurrentPos];
    if (currChar == '\n') {
      ctx.mCurrentLine++;
      ctx.mCurrentColumn = 0;
      ctx.mCurrentPos++;
    } else if (currChar == '\r') {
      ctx.mCurrentColumn = 0;
      ctx.mCurrentPos++;
    } else if (currChar == ' ' || currChar == '\t') {
      ctx.mCurrentPos++;
      ctx.mCurrentColumn++;
    } else {
      break;
    }
  }
}

void parseFraction(Context &ctx, std::string_view &fraction) {
  if (ctx.mCurrentPos >= ctx.mInput.size() ||
      ctx.mInput[ctx.mCurrentPos] != '.') {
    return;
  }
  const auto beginFraction = ctx.mCurrentPos++;
  auto endFraction = ctx.mCurrentPos;
  ctx.mCurrentColumn++;
  char currChar = ctx.mInput[ctx.mCurrentPos];
  if (ctx.mCurrentPos >= ctx.mInput.size() ||
      (currChar < '0' || currChar > '9')) {
    ctx.mAbort = true;
    ctx.mErrorMessage = "Unexpected end of input while parsing a number";
    return;
  }
  do {
    endFraction++;
    ctx.mCurrentPos++;
    ctx.mCurrentColumn++;
    currChar = ctx.mInput[ctx.mCurrentPos];
  } while (ctx.mCurrentPos < ctx.mInput.size() &&
           (currChar >= '0' && currChar <= '9'));
  fraction =
      std::string_view(&ctx.mInput[beginFraction], endFraction - beginFraction);
}

void parseExponent(Context &ctx, std::string_view &exponent) {
  if (ctx.mCurrentPos >= ctx.mInput.size() ||
      ctx.mInput[ctx.mCurrentPos] != 'e' ||
      ctx.mInput[ctx.mCurrentPos] != 'E') {
    return;
  }
  const auto beginExponent = ctx.mCurrentPos++;
  auto endExponent = ctx.mCurrentPos;
  ctx.mCurrentColumn++;
  char currChar = ctx.mInput[ctx.mCurrentPos];
  if (currChar == '+' || currChar == '-') {
    ctx.mCurrentPos++;
    ctx.mCurrentColumn++;
    endExponent++;
    currChar = ctx.mInput[ctx.mCurrentPos];
  }
  if (ctx.mCurrentPos >= ctx.mInput.size() ||
      (currChar < '0' || currChar > '9')) {
    ctx.mAbort = true;
    ctx.mErrorMessage = "Unexpected end of input while parsing a number";
    return;
  }
  do {
    endExponent++;
    currChar = ctx.mInput[ctx.mCurrentPos];
  } while (ctx.mCurrentPos < ctx.mInput.size() &&
           (currChar >= '0' && currChar <= '9'));
  exponent =
      std::string_view(&ctx.mInput[beginExponent], endExponent - beginExponent);
}

void parseInteger(Context &ctx, std::string_view &integer) {
  if (ctx.mCurrentPos >= ctx.mInput.size()) {
    ctx.mAbort = true;
    ctx.mErrorMessage = "Unexpected end of input while parsing a number";
    return;
  }

  bool isNegative = false;
  auto currChar = ctx.mInput[ctx.mCurrentPos];

  if (currChar == '-') {
    isNegative = true;
    ctx.mCurrentPos++;
    ctx.mCurrentColumn++;
    if (ctx.mInput.size() <= ctx.mCurrentPos) {
      ctx.mAbort = true;
      ctx.mErrorMessage = "Unexpected end of input while parsing a number";
      return;
    }
    currChar = ctx.mInput[ctx.mCurrentPos];
  }

  if (currChar == '0') {
    const auto beginNumber = (ctx.mCurrentPos++) - unsigned(isNegative);
    auto endNumber = ctx.mCurrentPos;
    ctx.mCurrentColumn++;
    integer =
        std::string_view(&ctx.mInput[beginNumber], endNumber - beginNumber);
    return;
  }

  if (currChar >= '1' && currChar <= '9') {
    const auto beginNumber = (ctx.mCurrentPos++) - unsigned(isNegative);
    auto endNumber = ctx.mCurrentPos;
    ctx.mCurrentColumn++;
    while (ctx.mCurrentPos < ctx.mInput.size()) {
      auto c = ctx.mInput[ctx.mCurrentPos];
      if (c < '0' || c > '9') {
        break;
      }
      endNumber = ++ctx.mCurrentPos;
      ctx.mCurrentColumn++;
    }
    integer =
        std::string_view(&ctx.mInput[beginNumber], endNumber - beginNumber);
  }
}

void printIndent(std::string &out, json_parser::PrintContext &ctx) {
  const auto newSize = out.size() + ctx.mCurrentIndentation;
  out.resize(newSize, ' ');
}

} // namespace

Json Json::parse(std::string_view input) {
  Json json;
  Context ctx{.mInput = input,
              .mCurrentPos = 0,
              .mCurrentLine = 1,
              .mCurrentColumn = 0,
              .mAbort = false,
              .mErrorMessage = ""};
  Element::parse(ctx, json.mElement);
  if (ctx.mAbort) {
    ctx.mErrorMessage += " at " + std::to_string(ctx.mCurrentLine) + ":" +
                         std::to_string(ctx.mCurrentColumn);
    throw std::runtime_error(ctx.mErrorMessage);
  }
  return json;
}

void Element::parse(Context &ctx, Element &out) {
  if (!out.mValue) {
    out.mValue = std::make_unique<Value>();
  }
  skipWhiteSpace(ctx);
  Value::parse(ctx, *out.mValue);
  if (ctx.mAbort) {
    return;
  }
  skipWhiteSpace(ctx);
}

void Value::parse(Context &ctx, Value &out) {
  if (ctx.mCurrentPos >= ctx.mInput.size()) {
    ctx.mAbort = true;
    ctx.mErrorMessage = "Unexpected end of input while parsing json value";
    return;
  }

  auto currChar = ctx.mInput[ctx.mCurrentPos];

  if (currChar == '{') {
    out.mValueType = ValueType::OBJECT;
    new (&out.mInternalValue.mObject) Object();
    Object::parse(ctx, out.mInternalValue.mObject);
  } else if (currChar == '[') {
    out.mValueType = ValueType::ARRAY;
    new (&out.mInternalValue.mArray) Array();
    Array::parse(ctx, out.mInternalValue.mArray);
  } else if (currChar == '"') {
    out.mValueType = ValueType::STRING;
    new (&out.mInternalValue.mString) String();
    String::parse(ctx, out.mInternalValue.mString);
  } else if ((currChar >= '0' && currChar <= '9') || (currChar == '-')) {
    out.mValueType = ValueType::NUMBER;
    new (&out.mInternalValue.mNumber) Number();
    Number::parse(ctx, out.mInternalValue.mNumber);
  } else if (currChar == 't') {
    out.mValueType = ValueType::TRUE;
    new (&out.mInternalValue.mTrue) True();
    True::parse(ctx, out.mInternalValue.mTrue);
  } else if (currChar == 'f') {
    out.mValueType = ValueType::FALSE;
    new (&out.mInternalValue.mFalse) False();
    False::parse(ctx, out.mInternalValue.mFalse);
  } else if (currChar == 'n') {
    out.mValueType = ValueType::JSON_NULL;
    new (&out.mInternalValue.mNull) Null();
    Null::parse(ctx, out.mInternalValue.mNull);
  } else {
    ctx.mAbort = true;
    ctx.mErrorMessage = "Unexpected character while parsing json value";
  }
}

void Null::parse(Context &ctx, Null &out) {
  if ((ctx.mInput.size() - ctx.mCurrentPos) < 4) {
    ctx.mAbort = true;
    ctx.mErrorMessage = "Unexpected end of input while parsing a null value";
    return;
  }
  if (ctx.mInput[ctx.mCurrentPos + 0] != 'n' ||
      ctx.mInput[ctx.mCurrentPos + 1] != 'u' ||
      ctx.mInput[ctx.mCurrentPos + 2] != 'l' ||
      ctx.mInput[ctx.mCurrentPos + 3] != 'l') {
    ctx.mAbort = true;
    ctx.mErrorMessage = "Failed to parse a null value";
    return;
  }
  ctx.mCurrentPos += 4;
  ctx.mCurrentColumn += 4;
}

void True::parse(Context &ctx, True &out) {
  if ((ctx.mInput.size() - ctx.mCurrentPos) < 4) {
    ctx.mAbort = true;
    ctx.mErrorMessage = "Unexpected end of input while parsing a true value";
    return;
  }
  if (ctx.mInput[ctx.mCurrentPos + 0] != 't' ||
      ctx.mInput[ctx.mCurrentPos + 1] != 'r' ||
      ctx.mInput[ctx.mCurrentPos + 2] != 'u' ||
      ctx.mInput[ctx.mCurrentPos + 3] != 'e') {
    ctx.mAbort = true;
    ctx.mErrorMessage = "Failed to parse a true value";
    return;
  }
  ctx.mCurrentPos += 4;
  ctx.mCurrentColumn += 4;
}

void False::parse(Context &ctx, False &out) {
  if ((ctx.mInput.size() - ctx.mCurrentPos) < 5) {
    ctx.mAbort = true;
    ctx.mErrorMessage = "Unexpected end of input while parsing a false value";
    return;
  }
  if (ctx.mInput[ctx.mCurrentPos + 0] != 'f' ||
      ctx.mInput[ctx.mCurrentPos + 1] != 'a' ||
      ctx.mInput[ctx.mCurrentPos + 2] != 'l' ||
      ctx.mInput[ctx.mCurrentPos + 3] != 's' ||
      ctx.mInput[ctx.mCurrentPos + 4] != 'e') {
    ctx.mAbort = true;
    ctx.mErrorMessage = "Failed to parse a false value";
    return;
  }
  ctx.mCurrentPos += 5;
  ctx.mCurrentColumn += 5;
}

void Number::parse(Context &ctx, Number &out) {
  std::string_view integer;
  parseInteger(ctx, integer);
  if (ctx.mAbort) {
    return;
  }
  std::string_view fraction;
  parseFraction(ctx, fraction);
  if (ctx.mAbort) {
    return;
  }
  std::string_view exponent;
  parseExponent(ctx, exponent);
  if (ctx.mAbort) {
    return;
  }
  auto fullNumber = std::string_view(
      integer.data(), integer.size() + fraction.size() + exponent.size());
  if (fullNumber.empty()) {
    ctx.mAbort = true;
    ctx.mErrorMessage = "Unexpected error while parsing a number";
    return;
  }
  if (!fraction.empty() || !exponent.empty()) {
    out.mNumberType = NumberType::FLOATING_POINT;
    auto [ptr, errc] = std::from_chars(fullNumber.data(),
                                       fullNumber.data() + fullNumber.size(),
                                       out.mInternalNumber.mFloat);
    if (errc != std::errc()) {
      ctx.mAbort = true;
      ctx.mErrorMessage = "Unexpected error while parsing a number";
    }
    return;
  }
  if (fullNumber[0] == '-') {
    out.mNumberType = NumberType::SIGNED;
    auto [ptr, errc] = std::from_chars(fullNumber.data(),
                                       fullNumber.data() + fullNumber.size(),
                                       out.mInternalNumber.mSigned);
    if (errc != std::errc()) {
      ctx.mAbort = true;
      ctx.mErrorMessage = "Unexpected error while parsing a number";
    }
    return;
  }
  out.mNumberType = NumberType::UNSIGNED;
  auto [ptr, errc] =
      std::from_chars(fullNumber.data(), fullNumber.data() + fullNumber.size(),
                      out.mInternalNumber.mUnsigned);
  if (errc != std::errc()) {
    ctx.mAbort = true;
    ctx.mErrorMessage = "Unexpected error while parsing a number";
  }
}

void String::parse(Context &ctx, String &out) {
  if (ctx.mCurrentPos >= ctx.mInput.size() ||
      ctx.mInput[ctx.mCurrentPos] != '"') {
    ctx.mAbort = true;
    ctx.mErrorMessage = "Unexpected character while parsing a string";
    return;
  }

  bool escapeSequenceInitiated = false;
  bool hexInitiated = false;
  ctx.mCurrentPos++;
  ctx.mCurrentColumn++;

  const auto beginString = ctx.mCurrentPos;
  auto endString = ctx.mCurrentPos;
  do {
    auto currChar = ctx.mInput[ctx.mCurrentPos];

    if (hexInitiated) {
      if (currChar == '"') {
        ctx.mCurrentPos++;
        ctx.mCurrentColumn++;
        break;
      }
      if ((currChar < '0' || currChar > '9') &&
          (currChar < 'a' || currChar > 'f') &&
          (currChar < 'A' || currChar > 'F')) {
        ctx.mAbort = true;
        ctx.mErrorMessage = "Unexpected character while parsing a string";
        return;
      }
    } else if (escapeSequenceInitiated) {
      if (currChar != '"' && currChar != '\\' && currChar != '/' &&
          currChar != 'b' && currChar != 'f' && currChar != 'n' &&
          currChar != 'r' && currChar != 't') {
        if (currChar == 'u') {
          hexInitiated = true;
        } else {
          ctx.mAbort = true;
          ctx.mErrorMessage = "Unexpected character while parsing a string";
          return;
        }
      }
      escapeSequenceInitiated = false;
    } else if (currChar == '"') {
      ctx.mCurrentPos++;
      ctx.mCurrentColumn++;
      break;
    } else if (currChar == '\\') {
      escapeSequenceInitiated = true;
    } else if (currChar < ' ') {
      ctx.mAbort = true;
      ctx.mErrorMessage = "Unexpected character while parsing a string";
      return;
    }
    ctx.mCurrentPos++;
    ctx.mCurrentColumn++;
    endString++;
  } while (ctx.mCurrentPos < ctx.mInput.size());

  out.mValue = std::string(ctx.mInput.begin() + beginString,
                           ctx.mInput.begin() + endString);
}

void Array::parse(Context &ctx, Array &out) {
  if (ctx.mCurrentPos >= ctx.mInput.size() ||
      ctx.mInput[ctx.mCurrentPos] != '[') {
    ctx.mAbort = true;
    ctx.mErrorMessage = "Unexpected end of input while parsing an array";
    return;
  }
  ctx.mCurrentPos++;
  ctx.mCurrentColumn++;
  skipWhiteSpace(ctx);
  if (ctx.mCurrentPos >= ctx.mInput.size()) {
    ctx.mAbort = true;
    ctx.mErrorMessage = "Unexpected end of input while parsing an array";
    return;
  }
  auto currChar = ctx.mInput[ctx.mCurrentPos];
  while (currChar != ']') {
    if (!out.mElements.empty()) {
      if (currChar != ',') {
        ctx.mAbort = true;
        ctx.mErrorMessage = "Unexpected end of input while parsing an array";
        return;
      }
      ctx.mCurrentPos++;
      ctx.mCurrentColumn++;
      skipWhiteSpace(ctx);
      if (ctx.mCurrentPos >= ctx.mInput.size()) {
        ctx.mAbort = true;
        ctx.mErrorMessage = "Unexpected end of input while parsing an array";
        return;
      }
      currChar = ctx.mInput[ctx.mCurrentPos];
    }
    auto &newElem = out.mElements.emplace_back(std::make_unique<Element>());
    Element::parse(ctx, *newElem);
    if (ctx.mAbort)
      return;
    skipWhiteSpace(ctx);
    if (ctx.mCurrentPos >= ctx.mInput.size()) {
      ctx.mAbort = true;
      ctx.mErrorMessage = "Unexpected end of input while parsing an array";
      return;
    }
    currChar = ctx.mInput[ctx.mCurrentPos];
  }
  ctx.mCurrentPos++;
  ctx.mCurrentColumn++;
}

void Object::parse(Context &ctx, Object &out) {
  if (ctx.mCurrentPos >= ctx.mInput.size() ||
      ctx.mInput[ctx.mCurrentPos] != '{') {
    ctx.mAbort = true;
    ctx.mErrorMessage = "Unexpected end of input while parsing an object";
    return;
  }
  ctx.mCurrentPos++;
  ctx.mCurrentColumn++;
  skipWhiteSpace(ctx);
  if (ctx.mCurrentPos >= ctx.mInput.size()) {
    ctx.mAbort = true;
    ctx.mErrorMessage = "Unexpected end of input while parsing an object";
    return;
  }
  auto currChar = ctx.mInput[ctx.mCurrentPos];
  while (currChar != '}') {
    if (!out.mMembers.empty()) {
      if (currChar != ',') {
        ctx.mAbort = true;
        ctx.mErrorMessage = "Unexpected end of input while parsing an object";
        return;
      }
      ctx.mCurrentPos++;
      ctx.mCurrentColumn++;
      skipWhiteSpace(ctx);
      if (ctx.mCurrentPos >= ctx.mInput.size()) {
        ctx.mAbort = true;
        ctx.mErrorMessage = "Unexpected end of input while parsing an object";
        return;
      }
      currChar = ctx.mInput[ctx.mCurrentPos];
    }
    if (currChar != '"') {
      ctx.mAbort = true;
      ctx.mErrorMessage = "Unexpected character while parsing an object";
      return;
    }
    Member newMember;
    String::parse(ctx, newMember.mName);
    if (ctx.mAbort)
      return;
    skipWhiteSpace(ctx);
    if (ctx.mCurrentPos >= ctx.mInput.size()) {
      ctx.mAbort = true;
      ctx.mErrorMessage = "Unexpected end of input while parsing an object";
      return;
    }
    currChar = ctx.mInput[ctx.mCurrentPos];
    if (currChar != ':') {
      ctx.mAbort = true;
      ctx.mErrorMessage = "Unexpected character while parsing an object";
      return;
    }
    ctx.mCurrentPos++;
    ctx.mCurrentColumn++;
    skipWhiteSpace(ctx);
    if (ctx.mCurrentPos >= ctx.mInput.size()) {
      ctx.mAbort = true;
      ctx.mErrorMessage = "Unexpected end of input while parsing an object";
      return;
    }
    newMember.mElement = std::make_unique<Element>();
    Element::parse(ctx, *newMember.mElement);
    if (ctx.mAbort)
      return;
    skipWhiteSpace(ctx);
    if (ctx.mCurrentPos >= ctx.mInput.size()) {
      ctx.mAbort = true;
      ctx.mErrorMessage = "Unexpected end of input while parsing an array";
      return;
    }
    out.mMembers.push_back(std::move(newMember));
    currChar = ctx.mInput[ctx.mCurrentPos];
  }
  ctx.mCurrentPos++;
  ctx.mCurrentColumn++;
}

Value::InternalValue::InternalValue() {}
Value::InternalValue::~InternalValue() {}

Value::~Value() {
  switch (mValueType) {
  case OBJECT: {
    mInternalValue.mObject.~Object();
  } break;
  case ARRAY: {
    mInternalValue.mArray.~Array();
  } break;
  case STRING: {
    mInternalValue.mString.~String();
  } break;
  case NUMBER: {
    mInternalValue.mNumber.~Number();
  } break;
  case TRUE: {
    mInternalValue.mTrue.~True();
  } break;
  case FALSE: {
    mInternalValue.mFalse.~False();
  } break;
  case JSON_NULL: {
    mInternalValue.mNull.~Null();
  } break;
  case UNINITIALIZED:
  default:
    break;
  }
}

Number::InternalNumber::InternalNumber() {}
Number::InternalNumber::~InternalNumber() {}

Number::~Number() {}

void Json::print(std::string &out) const {
  PrintContext ctx{};
  mElement.print(out, ctx);
}

void Element::print(std::string &out, PrintContext &ctx) const {
  if (mValue) {
    mValue->print(out, ctx);
  }
}

void Value::print(std::string &out, PrintContext &ctx) const {
  switch (mValueType) {
  case OBJECT: {
    mInternalValue.mObject.print(out, ctx);
  } break;
  case ARRAY: {
    mInternalValue.mArray.print(out, ctx);
  } break;
  case STRING: {
    mInternalValue.mString.print(out, ctx);
  } break;
  case NUMBER: {
    mInternalValue.mNumber.print(out, ctx);
  } break;
  case TRUE: {
    mInternalValue.mTrue.print(out, ctx);
  } break;
  case FALSE: {
    mInternalValue.mFalse.print(out, ctx);
  } break;
  case JSON_NULL: {
    mInternalValue.mNull.print(out, ctx);
  } break;
  case UNINITIALIZED:
  default: {
    out += "uninitialized";
    return;
  } break;
  }
}

void Object::print(std::string &out, PrintContext &ctx) const {
  out += "{\n";
  ctx.mCurrentIndentation += ctx.mIndentationSpaces;
  for (auto i = 0ULL; i < mMembers.size(); ++i) {
    const auto &member = mMembers[i];
    printIndent(out, ctx);
    member.mName.print(out, ctx);
    out += ": ";
    if (member.mElement)
      member.mElement->print(out, ctx);
    else
      out += "uninitialized";
    if (i < (mMembers.size() - 1))
      out += ",\n";
    else
      out += "\n";
  }
  ctx.mCurrentIndentation -= ctx.mIndentationSpaces;
  printIndent(out, ctx);
  out += "}";
}

void Array::print(std::string &out, PrintContext &ctx) const {
  out += "[\n";
  ctx.mCurrentIndentation += ctx.mIndentationSpaces;
  for (auto i = 0ULL; i < mElements.size(); ++i) {
    const auto &element = mElements[i];
    printIndent(out, ctx);
    if (element)
      element->print(out, ctx);
    else
      out += "uninitialized";
    if (i < (mElements.size() - 1))
      out += ",\n";
    else
      out += "\n";
  }
  ctx.mCurrentIndentation -= ctx.mIndentationSpaces;
  printIndent(out, ctx);
  out += "]";
}

void String::print(std::string &out, PrintContext &ctx) const {
  out += '"';
  out += mValue;
  out += '"';
}

void Number::print(std::string &out, PrintContext &ctx) const {
  std::array<char, 24> buffer;
  switch (mNumberType) {
  case UNSIGNED: {
    auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(),
                                   mInternalNumber.mUnsigned);
    const auto numberSv = std::string_view(buffer.data(), ptr);
    out += numberSv;
  } break;
  case SIGNED: {
    auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(),
                                   mInternalNumber.mSigned);
    const auto numberSv = std::string_view(buffer.data(), ptr);
    out += numberSv;
  } break;
  case FLOATING_POINT: {
    auto [ptr, ec] =
        std::to_chars(buffer.data(), buffer.data() + buffer.size(),
                      mInternalNumber.mFloat, std::chars_format::fixed, 16);
    const auto numberSv = std::string_view(buffer.data(), ptr);
    out += numberSv;
  } break;

  case UNINITIALIZED:
  default:
    break;
  }
}

void True::print(std::string &out, PrintContext &ctx) const { out += "true"; }

void False::print(std::string &out, PrintContext &ctx) const { out += "false"; }

void Null::print(std::string &out, PrintContext &ctx) const { out += "null"; }

Value &Json::getMemberValue(std::string_view name) {
  return mElement.getMemberValue(name);
}

Value &Element::getMemberValue(std::string_view name) {
  return mValue->getMemberValue(name);
}

Value &Value::getMemberValue(std::string_view name) {
  if (mValueType == ValueType::OBJECT)
    return mInternalValue.mObject.getMemberValue(name);
  throw std::runtime_error(
      "Atempted to get member value from value that is not an object");
}

Value &Object::getMemberValue(std::string_view name) {
  for (auto &member : mMembers) {
    if (member.mName.mValue == name) {
      return *member.mElement->mValue;
    }
  }
  throw std::runtime_error("member not found");
}

Number &Value::getNumber() {
  if (mValueType == ValueType::NUMBER)
    return mInternalValue.mNumber;
  throw std::runtime_error(
      "Atempted to get number from value that is not a number");
}

std::uint64_t &Number::getUnsinged() {
  if (mNumberType == NumberType::UNSIGNED)
    return mInternalNumber.mUnsigned;
  throw std::runtime_error(
      "Atempted to get unsigned from number that is not unsigned");
}
std::int64_t Number::getSigned() {
  if (mNumberType == NumberType::SIGNED)
    return mInternalNumber.mSigned;
  throw std::runtime_error(
      "Atempted to get signed from number that is not signed");
}

double Number::getFloatingPoint() {
  if (mNumberType == NumberType::FLOATING_POINT)
    return mInternalNumber.mFloat;
  throw std::runtime_error(
      "Atempted to get floating point from number that is not floating point");
}

Array &Value::getArray() {
  if (mValueType == ValueType::ARRAY)
    return mInternalValue.mArray;
  throw std::runtime_error(
      "Atempted to get number from value that is not a number");
}

} // namespace json_parser
