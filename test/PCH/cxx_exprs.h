// Header for PCH test cxx_exprs.cpp


// CXXStaticCastExpr
typedef __typeof__(static_cast<void *>(0)) static_cast_result;

// CXXDynamicCastExpr
struct Base { virtual void f(int x = 492); };
struct Derived : Base { void g(); };
Base *base_ptr;
typedef __typeof__(dynamic_cast<Derived *>(base_ptr)) dynamic_cast_result;

// CXXReinterpretCastExpr
typedef __typeof__(reinterpret_cast<void *>(0)) reinterpret_cast_result;

// CXXConstCastExpr
const char *const_char_ptr_value;
typedef __typeof__(const_cast<char *>(const_char_ptr_value)) const_cast_result;

// CXXFunctionalCastExpr
int int_value;
typedef __typeof__(double(int_value)) functional_cast_result;

// CXXBoolLiteralExpr
typedef __typeof__(true) bool_literal_result;
const bool true_value = true;
const bool false_value = false;

// CXXNullPtrLiteralExpr
typedef __typeof__(nullptr) cxx_null_ptr_result;

void foo(Derived *P) {
  // CXXMemberCallExpr
  P->f(12);
}


// FIXME: This is a hack until <typeinfo> works completely.
namespace std {
  class type_info {};
}

// CXXTypeidExpr - Both expr and type forms.
typedef __typeof__(typeid(int))* typeid_result1;
typedef __typeof__(typeid(2))*   typeid_result2;

void Derived::g() {
  // CXXThisExpr
  f(2);        // Implicit
  this->f(1);  // Explicit
  
  // CXXThrowExpr
  throw;
  throw 42;
  
  // CXXDefaultArgExpr
  f();
}