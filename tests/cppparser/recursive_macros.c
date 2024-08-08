// See panda3d/panda3d#1635

#define sched_priority    sched_priority
#define __sched_priority  (sched_priority)

// CHECK: int sched_priority = 0
int __sched_priority = 0;



#define __CONCAT(x, y) x ## y
#define __SIMD_DECL(function) __CONCAT(__DECL_SIMD_, function)
#define __MATHCALL_VEC(function, suffix) __SIMD_DECL(__CONCAT(function, suffix))

// CHECK: int __DECL_SIMD_ab
int __SIMD_DECL(__CONCAT(a, b));



#define GLOBAL(t,v) v
#define vfsList GLOBAL(sqlite3_vfs *, vfsList)

// CHECK: int pVfs = vfsList
int pVfs = vfsList;
