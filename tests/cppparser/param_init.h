#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <optional>

// --- Brace-initialized defaults (= {}) ---

// Bare `= {...}` defaults render without a type prefix, matching the source.

// CHECK: int empty_brace(std::string_view v = {})
int empty_brace(std::string_view v = {});

// CHECK: int empty_brace_vec(std::vector< int > v = {})
int empty_brace_vec(std::vector<int> v = {});

// Primitive type with empty brace init (value-initialization).
// CHECK: int empty_brace_int(int x = {})
int empty_brace_int(int x = {});

// Brace init with a single value.
// CHECK: int brace_one(int x = {5})
int brace_one(int x = {5});

// Brace init with a single string value.
// CHECK: int brace_one_str(std::string s = {"hi"})
int brace_one_str(std::string s = {"hi"});

// Brace init with multiple values.
// CHECK: int brace_vals(std::vector< int > v = {1, 2, 3})
int brace_vals(std::vector<int> v = {1, 2, 3});

// Brace init with negative values.
// CHECK: int brace_neg(std::vector< int > v = {-1, -2})
int brace_neg(std::vector<int> v = {-1, -2});

// Brace init with arithmetic expressions.
// CHECK: int brace_arith(std::vector< int > v = {(1 << 2), (3 | 4)})
int brace_arith(std::vector<int> v = {1 << 2, 3 | 4});

// Brace init reference-to-const binds a temporary.
// CHECK: int brace_const_ref(std::string const &s = {})
int brace_const_ref(const std::string &s = {});

// Brace init bound to an rvalue reference.
// CHECK: int brace_rvalue_ref(std::string &&s = {})
int brace_rvalue_ref(std::string &&s = {});

// --- Brace defaults mixed with other parameters ---

// Brace default after a non-brace default.
// CHECK: int mix_after(int x = 5, std::string_view v = {})
int mix_after(int x = 5, std::string_view v = {});

// Brace default before a non-brace default.
// CHECK: int mix_before(std::string_view v = {}, int x = 5)
int mix_before(std::string_view v = {}, int x = 5);

// Brace default sandwiched between other defaults.
// CHECK: int mix_middle(int x = 1, std::string_view sv = {}, int y = 2)
int mix_middle(int x = 1, std::string_view sv = {}, int y = 2);

// Two brace defaults in a row.
// CHECK: int two_braces(std::vector< int > a = {}, std::vector< int > b = {1, 2})
int two_braces(std::vector<int> a = {}, std::vector<int> b = {1, 2});

// --- Brace init with nested braces and trailing commas ---

// Nested braces (initializer list of pairs).
// CHECK: int nested_map(map< int, int > m = {{1, 2}, {3, 4}})
int nested_map(std::map<int, int> m = {{1, 2}, {3, 4}});

// Nested braces with vector-of-vectors.
// CHECK: int nested_vec(std::vector< std::vector< int > > v = {{1, 2}, {3, 4}})
int nested_vec(std::vector<std::vector<int>> v = {{1, 2}, {3, 4}});

// Trailing comma in initializer list.
// CHECK: int trailing_comma(std::vector< int > v = {1, 2, 3})
int trailing_comma(std::vector<int> v = {1, 2, 3,});

// Trailing commas inside nested braces.
// CHECK: int trailing_nested(std::vector< std::vector< int > > v = {{1, 2}, {3, 4}})
int trailing_nested(std::vector<std::vector<int>> v = {{1, 2,}, {3, 4,},});

// Deeply nested braces.
// CHECK: int deeply_nested(std::vector< std::vector< std::vector< int > > > v = {{{1}}})
int deeply_nested(std::vector<std::vector<std::vector<int>>> v = {{{1}}});

// Type-prefixed nested brace init keeps the prefix (it reduces via const_expr).
// CHECK: int prefix_nested(std::vector< std::vector< int > > v = vector< vector< int > >{{1, 2}, {3, 4}})
int prefix_nested(std::vector<std::vector<int>> v = std::vector<std::vector<int>>{{1, 2}, {3, 4}});

// Type-prefixed brace init with trailing comma.
// CHECK: int prefix_trailing(std::vector< int > v = vector< int >{1, 2})
int prefix_trailing(std::vector<int> v = std::vector<int>{1, 2,});

// --- Conventional `=` defaults (regression sanity) ---

// CHECK: int conv_int(int x = 5)
int conv_int(int x = 5);

// CHECK: int conv_neg(int x = -1)
int conv_neg(int x = -1);

// CHECK: int conv_bool_true(bool b = true)
int conv_bool_true(bool b = true);

// CHECK: int conv_bool_false(bool b = false)
int conv_bool_false(bool b = false);

// CHECK: int conv_char(char c = 97)
int conv_char(char c = 'a');

// CHECK: int conv_cstr(char const *s = "default")
int conv_cstr(const char *s = "default");

// CHECK: int conv_nullptr(int *p = nullptr)
int conv_nullptr(int *p = nullptr);

// CHECK: int conv_arith(int x = (1 + 2))
int conv_arith(int x = 1 + 2);

// CHECK: int conv_parens(int x = 5)
int conv_parens(int x = (5));

// --- Type-prefixed construction defaults ---

// CHECK: int prefix_construct(std::string s = string("hi"))
int prefix_construct(std::string s = std::string("hi"));

// CHECK: int prefix_brace(std::string_view v = string_view{"hi"})
int prefix_brace(std::string_view v = std::string_view{"hi"});

// CHECK: int prefix_brace_empty(std::string_view v = string_view{})
int prefix_brace_empty(std::string_view v = std::string_view{});

// Brace init with a `double` literal goes through the KW_DOUBLE rule, which
// previously mis-typed the construction as `float`.
// CHECK: int double_brace(double x = double{3.14})
int double_brace(double x = double{3.14});

// --- Enum class defaults ---

enum class Color { Red, Green, Blue };

// CHECK: int enum_default(Color c = ::Color::Red)
int enum_default(Color c = Color::Red);

// --- Templates with brace defaults ---

// CHECK: template<class T> void tmpl_brace(T value = {})
template<typename T> void tmpl_brace(T value = {});

template<typename T>
struct Wrapper {
  void store(T value = {});
};

// --- Class methods with brace defaults ---

class Cls {
public:
  Cls(std::string s = {});
  void method(std::string_view v = {});
  void multi(std::string s = {}, std::vector<int> v = {});
};
