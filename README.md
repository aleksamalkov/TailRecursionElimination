# TailRecursionElimination

LLVM pass koji vrši zamenu repne rekurzije iteracijom.

Napravljen kao projekat iz predmeta "Konstrukcija kompilatora" na Matematičkom fakultetu u Beogradu.

## Uputstvo
Postaviti projekat u direktorijum `llvm/lib/Transforms` u okviru izvornog koda projekta LLVM i dodati ga u `CMakeLists.txt`:
```
add_subdirectory(TailRecursionElimination)
```

Nakon prevođenja, pass se moze pokrenuti nad fajlom `input.ll` pokretanjem sledeće komande u direktorijumu `build`:
```
./bin/opt -load lib/LLVMOurTRE.so -S -enable-new-pm=0 -our-tre input.ll -o output.ll
```
Na primer:
```
cd build
./bin/clang -S -emit-llvm ../llvm/lib/Transforms/TailRecursionElimination/tests/tail_call_test.c
./bin/opt -load lib/LLVMOurTRE.so -S -enable-new-pm=0 -our-tre tail_call_test.ll -o tail_call_result.ll
```
