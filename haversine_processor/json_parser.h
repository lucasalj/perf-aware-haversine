#pragma once

#include <memory>
#include <string_view>
#include <vector>

namespace json_parser {
struct Context {
  std::string_view mInput;
  std::uint64_t mCurrentPos = 0;
  std::uint64_t mCurrentLine = 0;
  std::uint64_t mCurrentColumn = 0;
  bool mAbort = false;
  std::string mErrorMessage;
};

struct PrintContext {
  std::uint32_t mIndentationSpaces = 2;
  std::uint32_t mCurrentIndentation = 0;
};

struct String {
  static void parse(Context &ctx, String &out);
  void print(std::string &out, PrintContext &ctx) const;
  std::string mValue;
};

struct Value;

struct Element {
  static void parse(Context &ctx, Element &out);
  void print(std::string &out, PrintContext &ctx) const;
  Value &getMemberValue(std::string_view name);
  std::unique_ptr<Value> mValue;
};

struct Member {
  String mName;
  std::unique_ptr<Element> mElement;
};

struct Object {
  static void parse(Context &ctx, Object &out);
  void print(std::string &out, PrintContext &ctx) const;
  Value &getMemberValue(std::string_view name);
  std::vector<Member> mMembers;
};

struct Array {
  static void parse(Context &ctx, Array &out);
  void print(std::string &out, PrintContext &ctx) const;
  std::vector<std::unique_ptr<Element>> mElements;
};

struct Number {
  static void parse(Context &ctx, Number &out);
  void print(std::string &out, PrintContext &ctx) const;

  std::uint64_t &getUnsinged();
  std::int64_t getSigned();
  double getFloatingPoint();

  ~Number();

  enum NumberType : std::uint8_t {
    UNINITIALIZED,
    UNSIGNED,
    SIGNED,
    FLOATING_POINT
  };
  union InternalNumber {
    InternalNumber();
    ~InternalNumber();
    double mFloat;
    std::uint64_t mUnsigned;
    std::int64_t mSigned;
  };
  NumberType mNumberType{UNINITIALIZED};
  InternalNumber mInternalNumber{};
};

struct True {
  static void parse(Context &ctx, True &out);
  void print(std::string &out, PrintContext &ctx) const;
};

struct False {
  static void parse(Context &ctx, False &out);
  void print(std::string &out, PrintContext &ctx) const;
};

struct Null {
  static void parse(Context &ctx, Null &out);
  void print(std::string &out, PrintContext &ctx) const;
};

struct Value {
  static void parse(Context &ctx, Value &out);
  void print(std::string &out, PrintContext &ctx) const;
  Value &getMemberValue(std::string_view name);
  Number &getNumber();
  Array &getArray();

  ~Value();

  enum ValueType : std::uint8_t {
    UNINITIALIZED,
    OBJECT,
    ARRAY,
    STRING,
    NUMBER,
    TRUE,
    FALSE,
    JSON_NULL
  };

  union InternalValue {
    InternalValue();
    ~InternalValue();
    Object mObject;
    Array mArray;
    String mString;
    Number mNumber;
    True mTrue;
    False mFalse;
    Null mNull;
  };
  ValueType mValueType{UNINITIALIZED};
  InternalValue mInternalValue{};
};

struct Json {
  static Json from(Element element);
  static Json parse(std::string_view input);
  void print(std::string &out) const;
  Value &getMemberValue(std::string_view name);

  Element mElement;
};
} // namespace json_parser
