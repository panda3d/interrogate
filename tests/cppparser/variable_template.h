template<typename T>
constexpr T three = 3;

// CHECK: int three_int = ::three< int >
int three_int = three<int>;
