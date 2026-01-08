namespace a {
  namespace b {
    int inner_var;
    struct Type;
  };
};

// CHECK: namespace new_name = a::b
namespace new_name = a::b;

// CHECK: struct a::b::Type outer_var = ::a::b::inner_var
new_name::Type outer_var = new_name::inner_var;
