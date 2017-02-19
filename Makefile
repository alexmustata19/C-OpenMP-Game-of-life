build: serial parallel

serial: g_serial

g_serial: g_serial.c
	gcc g_serial.c -o g_serial

parallel: g_omp

g_omp: g_omp.c
	gcc -fopenmp g_omp.c -o g_omp

clean:
	rm g_omp -rf
	rm g_serial -rf