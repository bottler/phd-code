C_=g++ -I/home/jeremyr/libs/boost_1_59_0
C=clang++-3.6 -I/home/jeremyr/libs/boost_1_59_0

go:
	$(C) testarena_old.cpp -o testarena_old
	#clang++ testarena.cpp -std=c++11 -o testarena_clang
	#$(C) testarena.cpp -std=c++11 -o testarena
	$(C) -Wall spacedVector.cpp -std=c++11 -o spacedVector
	./testarena
clean:
	rm testarena testarena_old testarena_clang spacedVector
