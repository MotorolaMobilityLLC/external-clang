// RUN: clang -fsyntax-only -verify %s -fblocks

int (*FP)();
int (^IFP) ();
int (^II) (int);
int main() {
  int (*FPL) (int) = FP; // C doesn't consider this an error.
  
  // For Blocks, the ASTContext::typesAreBlockCompatible() makes sure this is an error.
	int (^PFR) (int) = IFP;	// expected-warning {{incompatible block pointer types initializing 'int (^)()', expected 'int (^)(int)'}}
	PFR = II;	// OK

	int (^IFP) () = PFR;	// expected-warning {{incompatible block pointer types initializing 'int (^)(int)', expected 'int (^)()'}}


	const int (^CIC) () = IFP; // expected-warning {{incompatible block pointer types initializing 'int (^)()', expected 'int const (^)()'}}


	const int (^CICC) () = CIC;

	int * const (^IPCC) () = 0;

	int * const (^IPCC1) () = IPCC; 

	int * (^IPCC2) () = IPCC;	// expected-warning {{incompatible block pointer types initializing 'int *const (^)()', expected 'int *(^)()'}}

	int (^IPCC3) (const int) = PFR;	// expected-warning {{incompatible block pointer types initializing 'int (^)(int)', expected 'int (^)(int const)'}}


	int (^IPCC4) (int, char (^CArg) (double));


	int (^IPCC5) (int, char (^CArg) (double)) = IPCC4;

	int (^IPCC6) (int, char (^CArg) (float))  = IPCC4; // expected-warning {{incompatible block pointer types initializing 'int (^)(int, char (^)(double))', expected 'int (^)(int, char (^)(float))'}}

	IPCC2 = 0;
	IPCC2 = 1; // expected-error {{invalid conversion assigning integer 'int', expected block pointer 'int *(^)()'}}
	int (^x)() = 0;
	int (^y)() = 3;   // expected-error {{invalid conversion initializing integer 'int', expected block pointer 'int (^)()'}}
	int a = 1;
	int (^z)() = a+4;   // expected-error {{invalid conversion initializing integer 'int', expected block pointer 'int (^)()'}}
}

int blah() {
	int (^IFP) (float);
	char (^PCP)(double, double, char);

	IFP(1.0);
	IFP (1.0, 2.0);	// expected-error {{too many arguments to block call}}

	char ch = PCP(1.0, 2.0, 'a');
	return PCP(1.0, 2.0);	// expected-error {{too few arguments to block}}
}

