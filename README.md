# TailRecursionElimination

LLVM pass koji vrši zamenu repne rekurzije iteracijom. Napravljen kao projekat iz predmeta "Konstrukcija kompilatora" na Matematičkom fakultetu u Beogradu.

Pass pronalazi repno rekurzivne pozive, proverava da li je bezbedno izvršiti optimizaciju i zatim je vrši.

Pored repno rekurzivnih funkcija može da optimizuje i funkcije koje izmedju rekurzivnog poziva i kraja funkcije imaju još jednu asocijativnu i komutativnu binarnu operaciju. Takve funkcije nisu striktno repno rekurzivne ali mogu biti transformisane u repno rekurzivne funkcije dodavanjem dodatnog parametra koji služi kao akumulator ili u iteraciju dodavanjem akumulatora u obliku lokalne promenljive.

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
