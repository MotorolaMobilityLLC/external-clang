// RUN: %clang_cc1 -verify -fopenmp -ferror-limit 100 %s

void foo() {
}

bool foobool(int argc) {
  return argc;
}

struct S1; // expected-note {{declared here}}

template <class T, class S> // expected-note {{declared here}}
int tmain(T argc, S **argv) {
  #pragma omp taskloop priority // expected-error {{expected '(' after 'priority'}}
  for (int i = 0; i < 10; ++i)
    foo();
  #pragma omp taskloop priority ( // expected-error {{expected expression}} expected-error {{expected ')'}} expected-note {{to match this '('}}
  for (int i = 0; i < 10; ++i)
    foo();
  #pragma omp taskloop priority () // expected-error {{expected expression}}
  for (int i = 0; i < 10; ++i)
    foo();
  #pragma omp taskloop priority (argc // expected-error {{expected ')'}} expected-note {{to match this '('}}
  for (int i = 0; i < 10; ++i)
    foo();
  #pragma omp taskloop priority (argc)) // expected-warning {{extra tokens at the end of '#pragma omp taskloop' are ignored}}
  for (int i = 0; i < 10; ++i)
    foo();
  #pragma omp taskloop priority (argc > 0 ? argv[1][0] : argv[2][argc])
  for (int i = 0; i < 10; ++i)
    foo();
  #pragma omp taskloop priority (foobool(argc)), priority (true) // expected-error {{directive '#pragma omp taskloop' cannot contain more than one 'priority' clause}}
  for (int i = 0; i < 10; ++i)
    foo();
  #pragma omp taskloop priority (S) // expected-error {{'S' does not refer to a value}}
  for (int i = 0; i < 10; ++i)
    foo();
  #pragma omp taskloop priority (argc argc) // expected-error {{expected ')'}} expected-note {{to match this '('}}
  for (int i = 0; i < 10; ++i)
    foo();
  #pragma omp taskloop priority(0)
  for (int i = 0; i < 10; ++i)
    foo();
  #pragma omp taskloop priority(-1) // expected-error {{argument to 'priority' clause must be a non-negative integer value}}
  for (int i = 0; i < 10; ++i)
    foo();

  return 0;
}

int main(int argc, char **argv) {
  #pragma omp taskloop priority // expected-error {{expected '(' after 'priority'}}
  for (int i = 0; i < 10; ++i)
    foo();
  #pragma omp taskloop priority ( // expected-error {{expected expression}} expected-error {{expected ')'}} expected-note {{to match this '('}}
  for (int i = 0; i < 10; ++i)
    foo();
  #pragma omp taskloop priority () // expected-error {{expected expression}}
  for (int i = 0; i < 10; ++i)
    foo();
  #pragma omp taskloop priority (argc // expected-error {{expected ')'}} expected-note {{to match this '('}}
  for (int i = 0; i < 10; ++i)
    foo();
  #pragma omp taskloop priority (argc)) // expected-warning {{extra tokens at the end of '#pragma omp taskloop' are ignored}}
  for (int i = 0; i < 10; ++i)
    foo();
  #pragma omp taskloop priority (argc > 0 ? argv[1][0] : argv[2][argc])
  for (int i = 0; i < 10; ++i)
    foo();
  #pragma omp taskloop priority (foobool(argc)), priority (true) // expected-error {{directive '#pragma omp taskloop' cannot contain more than one 'priority' clause}}
  for (int i = 0; i < 10; ++i)
    foo();
  #pragma omp taskloop priority (S1) // expected-error {{'S1' does not refer to a value}}
  for (int i = 0; i < 10; ++i)
    foo();
  #pragma omp taskloop priority (argc argc) // expected-error {{expected ')'}} expected-note {{to match this '('}}
  for (int i = 0; i < 10; ++i)
    foo();
  #pragma omp taskloop priority (1 0) // expected-error {{expected ')'}} expected-note {{to match this '('}}
  for (int i = 0; i < 10; ++i)
    foo();
  #pragma omp taskloop priority(if(tmain(argc, argv) // expected-error {{expected expression}} expected-error {{expected ')'}} expected-note {{to match this '('}}
  for (int i = 0; i < 10; ++i)
    foo();
  #pragma omp taskloop priority(0)
  for (int i = 0; i < 10; ++i)
    foo();
  #pragma omp taskloop priority(-1) // expected-error {{argument to 'priority' clause must be a non-negative integer value}}
  for (int i = 0; i < 10; ++i)
    foo();

  return tmain(argc, argv);
}
