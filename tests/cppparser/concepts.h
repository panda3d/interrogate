// C++20 Concepts syntax torture test

// Minimal concept - just a bool expression
// CHECK: template<class T> concept AlwaysTrue = true
template<typename T> concept AlwaysTrue = true;

// CHECK: template<class T> concept AlwaysFalse = false
template<typename T> concept AlwaysFalse = false;

// CHECK: template<class T> concept SmallType = (sizeof(T) <= 8)
template<typename T> concept SmallType = sizeof(T) <= 8;

// CHECK: template<class T> concept AlsoSmall = ::SmallType
template<typename T> concept AlsoSmall = SmallType<T>;

// CHECK: template<class T> concept SmallOrBool = (::SmallType || ::AlwaysTrue)
template<typename T> concept SmallOrBool = SmallType<T> || AlwaysTrue<T>;

// CHECK: template<class T> concept SmallAndTrue = (::SmallType && ::AlwaysTrue)
template<typename T> concept SmallAndTrue = SmallType<T> && AlwaysTrue<T>;

// CHECK: template<class T, class U> concept SameSize = (sizeof(T) == sizeof(U))
template<typename T, typename U> concept SameSize = sizeof(T) == sizeof(U);

// Concept with non-type template parameter
// CHECK: template<class T, int N> concept SizeAtLeast = (sizeof(T) >= N)
template<typename T, int N> concept SizeAtLeast = sizeof(T) >= N;

// Variadic concept
template<typename T, typename... Us>
concept SameSizeAsAll = (SameSize<T, Us> && ...);

// requires-expressions in concept definitions

// Simple requirement (expression must be valid)
template<typename T>
concept Dereferenceable = requires(T t) {
  *t;
};

// Multiple simple requirements
template<typename T>
concept Incrementable = requires(T t) {
  ++t;
  t++;
};

// Type requirement
template<typename T>
concept HasValueType = requires {
  typename T::value_type;
};

template<typename T>
concept HasNestedTypes = requires {
  typename T::value_type;
  typename T::pointer;
  typename T::reference;
};

// Compound requirement - basic
template<typename T>
concept Swappable = requires(T a, T b) {
  { a.swap(b) };
};

// Compound requirement with noexcept
template<typename T>
concept NothrowSwappable = requires(T a, T b) {
  { a.swap(b) } noexcept;
};

// Compound requirement with return type constraint
template<typename T>
concept HasSize = requires(T t) {
  { t.size() } -> SmallType;
};

// Compound requirement with noexcept AND return type constraint
template<typename T>
concept NothrowSize = requires(T t) {
  { t.size() } noexcept -> SmallType;
};

// Nested requirement
template<typename T>
concept Container = requires(T t) {
  typename T::value_type;
  { t.size() } -> SmallType;
  requires SmallType<typename T::value_type>;
};

// requires-expression with no parameter list
template<typename T>
concept DefaultConstructible = requires {
  T{};
  T();
};

// Complex requires-expression with all features
template<typename T>
concept FullFeatured = requires(T t, T const ct, T* p) {
  // Simple requirements
  t.foo();
  *p;

  // Type requirements
  typename T::value_type;
  typename T::template rebind<int>;

  // Compound requirements
  { t.bar() };
  { t.baz() } noexcept;
  { t.size() } -> SmallType;
  { ct.data() } noexcept -> Dereferenceable;

  // Nested requirements
  requires SmallType<T>;
  requires SameSize<T, typename T::value_type> || AlwaysTrue<T>;
};

// requires-clause positions

// After template-head
template<typename T>
requires SmallType<T>
void func_requires_after_template(T);

// Trailing requires-clause
template<typename T>
void func_trailing_requires(T) requires SmallType<T>;

// Both positions simultaneously
template<typename T>
requires AlwaysTrue<T>
void func_both_requires(T) requires SmallType<T>;

// With complex constraint expression
template<typename T>
requires SmallType<T> && Incrementable<T>
void func_complex_constraint(T);

// With parenthesized constraint
template<typename T>
requires (SmallType<T> || AlwaysTrue<T>)
void func_paren_constraint(T);

// The infamous requires requires
template<typename T>
requires requires(T t) { ++t; }
void func_requires_requires(T);

// requires requires with complex body
template<typename T>
requires requires(T t) {
  { t.foo() } -> SmallType;
  typename T::value_type;
} && SmallType<T>
void func_requires_requires_complex(T);

// Multiple template parameters with different constraints
template<typename T, typename U>
requires SmallType<T> && Dereferenceable<U>
void func_multi_constrained(T, U);

// Constrained template parameters (concept as type-constraint)

// Simple constrained parameter
template<SmallType T>
void func_constrained_param(T);

// Multiple constrained parameters
template<SmallType T, Incrementable U>
void func_multi_constrained_params(T, U);

// Mixed constrained and unconstrained
template<SmallType T, typename U, Incrementable V>
void func_mixed_params(T, U, V);

// Concept with arguments in parameter position
template<SameSize<int> T>
void func_concept_with_args(T);

template<SizeAtLeast<4> T>
void func_concept_nttp_arg(T);

// Constrained auto

// Constrained auto parameter (abbreviated function template)
// CHECK: void func_auto_param(auto x)
void func_auto_param(SmallType auto x);

// Multiple constrained auto parameters
// CHECK: void func_multi_auto(auto x, auto y)
void func_multi_auto(SmallType auto x, Incrementable auto y);

// Mixed auto and regular template params
template<typename T>
void func_mixed_auto(T x, SmallType auto y);

// Constrained auto return type
SmallType auto func_auto_return();

// Constrained auto variable (would be in function body, shown as extern)
extern SmallType auto global_constrained;

// Pointer/reference with constrained auto
void func_auto_ref(SmallType auto& x);
void func_auto_const_ref(SmallType auto const& x);
void func_auto_ptr(SmallType auto* x);

// Class template constraints

// Constrained class template
template<typename T>
requires SmallType<T>
struct ConstrainedClass {
  T value;
};

// Class with constrained parameter
template<SmallType T>
struct ConstrainedParamClass {
  T value;
};

// Class with constrained member functions
template<typename T>
struct ClassWithConstrainedMembers {
  void always_available();

  void only_if_small() requires SmallType<T>;

  template<typename U>
  requires SameSize<T, U>
  void templated_constrained(U);

  template<SmallType U>
  void templated_constrained_param(U);

  // Constrained by enclosing + own requirement
  template<typename U>
  requires Incrementable<U>
  void double_constrained(U) requires SmallType<T>;
};

// Partial specialization with constraints
template<typename T>
requires SmallType<T>
struct ConstrainedClass<T*> {
  T* ptr;
};

// Variable templates with constraints

template<typename T>
requires SmallType<T>
constexpr bool is_small_v = true;

template<SmallType T>
constexpr int small_size_v = sizeof(T);

// Alias templates with constraints

template<typename T>
requires SmallType<T>
using SmallPtr = T*;

template<SmallType T>
using SmallRef = T&;

// Constrained lambdas (in inline function for syntax check)

inline void lambda_examples() {
  // Constrained auto in lambda parameter
  auto lam1 = [](SmallType auto x) { return x; };

  // Lambda with requires clause
  auto lam2 = []<typename T>(T x) requires SmallType<T> { return x; };

  // Lambda with constrained template parameter
  auto lam3 = []<SmallType T>(T x) { return x; };

  // Combining approaches
  auto lam4 = []<typename T>(T x)
    requires SmallType<T> && Incrementable<T> { return ++x; };
}

// requires-expression as primary expression

// In static_assert
static_assert(requires { 1 + 1; });
static_assert(requires(int x) { ++x; });

// As variable initializer
constexpr bool can_increment_int = requires(int x) { ++x; };

// In if-constexpr context (inside function)
template<typename T>
void check_features(T t) {
  if constexpr (requires { t.foo(); }) {
    t.foo();
  }
  if constexpr (requires(T x) { { x.bar() } -> SmallType; }) {
    t.bar();
  }
}

// Negated requires
template<typename T>
concept NotDereferenceable = !requires(T t) { *t; };

// requires in boolean expressions
template<typename T>
concept ComplexLogic =
  (requires { typename T::a; } || requires { typename T::b; }) &&
  !requires { typename T::forbidden; };

// Concepts in namespaces

namespace outer {
  template<typename T>
  concept OuterConcept = sizeof(T) > 0;

  namespace inner {
    template<typename T>
    concept InnerConcept = OuterConcept<T> && sizeof(T) <= 16;

    template<typename T>
    requires InnerConcept<T>
    void namespaced_func(T);
  }

  // Using nested concept
  template<inner::InnerConcept T>
  void uses_nested_concept(T);
}

// Using qualified concept name
template<typename T>
requires outer::OuterConcept<T>
void uses_qualified_concept(T);

template<outer::inner::InnerConcept T>
void uses_deeply_nested(T);

// Edge cases and exotic syntax

// Concept checking another concept's satisfaction
template<typename T>
concept MetaConcept = SmallType<T> && requires {
  requires Incrementable<T>;
};

// Long chain of constraints
template<typename T>
requires SmallType<T>
    && Incrementable<T>
    && AlwaysTrue<T>
    && (!AlwaysFalse<T>)
void long_constraint_chain(T);

// Fold expression in concept
template<typename... Ts>
concept AllSmall = (SmallType<Ts> && ...);

template<typename... Ts>
concept AnySmall = (SmallType<Ts> || ...);

template<typename... Ts>
requires AllSmall<Ts...>
void variadic_constrained(Ts...);

// Empty requires expression body
template<typename T>
concept Vacuous = requires {};

// Compound requirement with qualified concept
template<typename T>
concept UsesQualifiedReturn = requires(T t) {
  { t.get() } -> outer::OuterConcept;
};

// Dependent concept in requires-expression
template<typename T>
concept DependentCheck = requires(T t) {
  requires SameSize<decltype(t.get()), int>;
};

// requires-expression with multiple parameter packs... jk, that's not a thing

// Atomic constraint with parentheses (matters for subsumption)
template<typename T>
concept ParenthesizedAtomic = (SmallType<T>);

// Constructors and special members
template<typename T>
struct ConstrainedCtors {
  ConstrainedCtors() requires DefaultConstructible<T>;

  ConstrainedCtors(T const&) requires SmallType<T>;

  ConstrainedCtors& operator=(T const&) requires SmallType<T>;

  ~ConstrainedCtors() requires AlwaysTrue<T> = default;

  // Conditional noexcept with constraint
  void maybe_noexcept() noexcept(SmallType<T>) requires AlwaysTrue<T>;
};

// Friend declarations with constraints
template<typename T>
struct HasConstrainedFriend {
  template<typename U>
  requires SameSize<T, U>
  friend void friend_func(HasConstrainedFriend, U);

  template<SmallType U>
  friend struct FriendClass;
};

// Deduction guides with constraints
template<typename T>
struct Wrapper {
  T value;
};

template<typename T>
requires SmallType<T>
Wrapper(T) -> Wrapper<T>;

template<SmallType T>
Wrapper(T, T) -> Wrapper<T>;
