all: main

main:
	$(CXX) main.cpp -std=c++11 -Wall -O3
	./a.exe
	gnuplot plot

# cat a.wav >> /dev/dsp

#   for assembler
# 	g++ main.cpp \
		-S -g -masm=intel \
		-std=c++11 -Wall -O3 \
		-mavx \
		-fopenmp
#	./a.out
