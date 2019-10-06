#/bin/bash 

mv tlb.test tlb.test.old

>&2 echo Running

for t in {0..1}; do \
	for p in {1..16}; do \
		g++ -o0 tlb.cpp -o tlb.exe -DNUMPROC=$p -DTLB_INV=$t ; \
		for i in {1..10}; do \
			>&2 echo Running PROC=$p INV=$t ITER=$i ; \
			./tlb.exe ; sleep 1 ; done  \
		done  \
	done >> tlb.test

>&2 echo Done
