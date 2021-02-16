#include "util.h"

int main(int argc, char** argv)
{
	cpu_set_t  mask;
	assign_to_core(CPU_AFFINITY);
	
	__attribute__ ((aligned (64))) uint64_t prime_array[9*4096];
	__attribute__ ((aligned (64))) uint64_t probe_array[4096];


	// if(argc != 2)
	// {
	// 	printf("Usage: ./main [input string]\n");
	// 	exit(-1);
	// }
	// else
	// 	demo(argv[1], prime_array, probe_array);

	demo_parent_child("hi", prime_array, probe_array);


    return 0;
}