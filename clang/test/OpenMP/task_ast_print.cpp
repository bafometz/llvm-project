// RUN: %clang_cc1 -verify -fopenmp -fopenmp-version=50 -ast-print %s | FileCheck %s
// RUN: %clang_cc1 -fopenmp -fopenmp-version=50 -x c++ -std=c++11 -emit-pch -o %t %s
// RUN: %clang_cc1 -fopenmp -fopenmp-version=50 -std=c++11 -include-pch %t -fsyntax-only -verify %s -ast-print | FileCheck %s

// RUN: %clang_cc1 -verify -fopenmp-simd -fopenmp-version=50 -ast-print %s | FileCheck %s
// RUN: %clang_cc1 -fopenmp-simd -fopenmp-version=50 -x c++ -std=c++11 -emit-pch -o %t %s
// RUN: %clang_cc1 -fopenmp-simd -fopenmp-version=50 -std=c++11 -include-pch %t -fsyntax-only -verify %s -ast-print | FileCheck %s
// expected-no-diagnostics

#ifndef HEADER
#define HEADER

typedef void *omp_depend_t;
typedef unsigned long omp_event_handle_t;

void foo() {}

struct S1 {
  S1(): a(0) {}
  S1(int v) : a(v) {}
  int a;
  typedef int type;
  S1 operator +(const S1&);
};

template <typename T>
class S7 : public T {
protected:
  T a, b;
  S7() : a(0) {}

public:
  S7(typename T::type v) : a(v) {
    omp_depend_t x;
    omp_event_handle_t evt;
#pragma omp taskgroup allocate(b) task_reduction(+:b)
#pragma omp task private(a) private(this->a) private(T::a) in_reduction(+:this->b) allocate(b) depend(depobj:x) detach(evt)
    for (int k = 0; k < a.a; ++k)
      ++this->a.a;
  }
  S7 &operator=(S7 &s) {
#pragma omp task private(a) private(this->a)
    for (int k = 0; k < s.a.a; ++k)
      ++s.a.a;
    return *this;
  }
};

// CHECK: #pragma omp taskgroup allocate(this->b) task_reduction(+: this->b)
// CHECK: #pragma omp task private(this->a) private(this->a) private(T::a) in_reduction(+: this->b) allocate(this->b) depend(depobj : x) detach(evt){{$}}
// CHECK: #pragma omp task private(this->a) private(this->a)
// CHECK: #pragma omp task private(this->a) private(this->a) private(this->S1::a)

class S8 : public S7<S1> {
  S8() {}

public:
  S8(int v) : S7<S1>(v){
#pragma omp task private(a) private(this->a) private(S7<S1>::a)
    for (int k = 0; k < a.a; ++k)
      ++this->a.a;
  }
  S8 &operator=(S8 &s) {
#pragma omp task private(a) private(this->a)
    for (int k = 0; k < s.a.a; ++k)
      ++s.a.a;
    return *this;
  }
};

// CHECK: #pragma omp task private(this->a) private(this->a) private(this->S7<S1>::a)
// CHECK: #pragma omp task private(this->a) private(this->a)

template <class T>
struct S {
  operator T() { return T(); }
  static T TS;
#pragma omp threadprivate(TS)
};

// CHECK:      template <class T> struct S {
// CHECK:        static T TS;
// CHECK-NEXT:   #pragma omp threadprivate(S::TS)
// CHECK:      };
// CHECK:      template<> struct S<int> {
// CHECK:        static int TS;
// CHECK-NEXT:   #pragma omp threadprivate(S<int>::TS)
// CHECK-NEXT: }
// CHECK:      template<> struct S<long> {
// CHECK:        static long TS;
// CHECK-NEXT:   #pragma omp threadprivate(S<long>::TS)
// CHECK-NEXT: }

template <typename T, int C>
T tmain(T argc, T *argv) {
  T b = argc, c, d, e, f, g;
  static T a;
  S<T> s;
  T arr[argc];
  omp_depend_t x;
  omp_event_handle_t evt;
#pragma omp task untied depend(in : argc, argv[b:argc], arr[:]) if (task : argc > 0) depend(depobj: x) detach(evt)
  a = 2;
#pragma omp task default(none), private(argc, b) firstprivate(argv) shared(d) if (argc > 0) final(S<T>::TS > 0) priority(argc)
  foo();
#pragma omp taskgroup task_reduction(-: argc)
#pragma omp task if (C) mergeable priority(C) in_reduction(-: argc)
  foo();
  return 0;
}

// CHECK: template <typename T, int C> T tmain(T argc, T *argv) {
// CHECK-NEXT: T b = argc, c, d, e, f, g;
// CHECK-NEXT: static T a;
// CHECK-NEXT: S<T> s;
// CHECK-NEXT: T arr[argc];
// CHECK-NEXT: omp_depend_t x;
// CHECK-NEXT: omp_event_handle_t evt;
// CHECK-NEXT: #pragma omp task untied depend(in : argc,argv[b:argc],arr[:]) if(task: argc > 0) depend(depobj : x) detach(evt)
// CHECK-NEXT: a = 2;
// CHECK-NEXT: #pragma omp task default(none) private(argc,b) firstprivate(argv) shared(d) if(argc > 0) final(S<T>::TS > 0) priority(argc)
// CHECK-NEXT: foo()
// CHECK-NEXT: #pragma omp taskgroup task_reduction(-: argc)
// CHECK-NEXT: #pragma omp task if(C) mergeable priority(C) in_reduction(-: argc)
// CHECK-NEXT: foo()
// CHECK: template<> int tmain<int, 5>(int argc, int *argv) {
// CHECK-NEXT: int b = argc, c, d, e, f, g;
// CHECK-NEXT: static int a;
// CHECK-NEXT: S<int> s;
// CHECK-NEXT: int arr[argc];
// CHECK-NEXT: omp_depend_t x;
// CHECK-NEXT: omp_event_handle_t evt;
// CHECK-NEXT: #pragma omp task untied depend(in : argc,argv[b:argc],arr[:]) if(task: argc > 0) depend(depobj : x) detach(evt)
// CHECK-NEXT: a = 2;
// CHECK-NEXT: #pragma omp task default(none) private(argc,b) firstprivate(argv) shared(d) if(argc > 0) final(S<int>::TS > 0) priority(argc)
// CHECK-NEXT: foo()
// CHECK-NEXT: #pragma omp taskgroup task_reduction(-: argc)
// CHECK-NEXT: #pragma omp task if(5) mergeable priority(5) in_reduction(-: argc)
// CHECK-NEXT: foo()
// CHECK: template<> long tmain<long, 1>(long argc, long *argv) {
// CHECK-NEXT: long b = argc, c, d, e, f, g;
// CHECK-NEXT: static long a;
// CHECK-NEXT: S<long> s;
// CHECK-NEXT: long arr[argc];
// CHECK-NEXT: omp_depend_t x;
// CHECK-NEXT: omp_event_handle_t evt;
// CHECK-NEXT: #pragma omp task untied depend(in : argc,argv[b:argc],arr[:]) if(task: argc > 0) depend(depobj : x) detach(evt)
// CHECK-NEXT: a = 2;
// CHECK-NEXT: #pragma omp task default(none) private(argc,b) firstprivate(argv) shared(d) if(argc > 0) final(S<long>::TS > 0) priority(argc)
// CHECK-NEXT: foo()
// CHECK-NEXT: #pragma omp taskgroup task_reduction(-: argc)
// CHECK-NEXT: #pragma omp task if(1) mergeable priority(1) in_reduction(-: argc)
// CHECK-NEXT: foo()

enum Enum {};

int main(int argc, char **argv) {
  long x;
  int b = argc, c, d, e, f, g;
  static int a;
  int arr[10], arr1[argc];
  omp_depend_t y;
  omp_event_handle_t evt;
#pragma omp threadprivate(a)
  Enum ee;
// CHECK: Enum ee;
#pragma omp task untied mergeable depend(out:argv[:a][1], (arr)[0:]) if(task: argc > 0) priority(f) depend(depobj:y)
  // CHECK-NEXT: #pragma omp task untied mergeable depend(out : argv[:a][1],(arr)[0:]) if(task: argc > 0) priority(f) depend(depobj : y)
  a = 2;
// CHECK-NEXT: a = 2;
#pragma omp taskgroup task_reduction(min: arr1)
#pragma omp task default(none), private(argc, b) firstprivate(argv, evt) if (argc > 0) final(a > 0) depend(inout : a, argv[:argc],arr[:a]) priority(23) in_reduction(min: arr1), detach(evt)
  // CHECK-NEXT: #pragma omp taskgroup task_reduction(min: arr1)
  // CHECK-NEXT: #pragma omp task default(none) private(argc,b) firstprivate(argv,evt) if(argc > 0) final(a > 0) depend(inout : a,argv[:argc],arr[:a]) priority(23) in_reduction(min: arr1) detach(evt)
  foo();
  // CHECK-NEXT: foo();
#pragma omp taskgroup task_reduction(min: arr1)
#pragma omp parallel reduction(+:arr1)
#pragma omp task in_reduction(min: arr1)
  // CHECK-NEXT: #pragma omp taskgroup task_reduction(min: arr1)
  // CHECK-NEXT: #pragma omp parallel reduction(+: arr1)
  // CHECK-NEXT: #pragma omp task in_reduction(min: arr1)
  foo();
  // CHECK-NEXT: foo();
  // CHECK-NEXT: #pragma omp task in_reduction(+: arr1)
#pragma omp task in_reduction(+: arr1)
  foo();
  // CHECK-NEXT: foo();
  return tmain<int, 5>(b, &b) + tmain<long, 1>(x, &x);
}

extern template int S<int>::TS;
extern template long S<long>::TS;

#endif
