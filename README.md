# TailRecursionElimination

U direktorijumu `llvm/lib/Transforms` u fajl `CMakeLists.txt` dodaj liniju:
```
add_subdirectory(TailRecursionElimination)
```

Zatim u istom direktorijumu kloniraj ovaj repozitorijum:
```
git clone https://github.com/aleksamalkov/TailRecursionElimination.git
```

Isprobaj da li radi tako sto u korenu projekta pokrenes:
```
./make_llvm.sh
cd build
./bin/clang -S -emit-llvm ../llvm/lib/Transforms/TailRecursionElimination/tests/tail_call_test.c
./bin/opt -load lib/LLVMOurTRE.so -S -enable-new-pm=0 -our-tre tail_call_test.ll -o tail_call_result.ll
```
