#include "util.h"

int cmpfunc (const void * a, const void * b) 
{
   return ( *(int*)a - *(int*)b );
}


void assign_to_core(int core_id)
{
	cpu_set_t  mask;
	int nproc = sysconf(_SC_NPROCESSORS_ONLN);
    CPU_ZERO(&mask);
    CPU_SET(core_id, &mask);
    int ret = sched_setaffinity(0, sizeof(cpu_set_t), &mask);

    if(ret == -1)
    {
    	printf("Set Affinity Failed\n");
    	exit(1);
    }

}

void char_to_bool(char character, bool* boolean)
{
	*(boolean + 0) = character & 0b00000001;
	*(boolean + 1) = character & 0b00000010;
	*(boolean + 2) = character & 0b00000100;
	*(boolean + 3) = character & 0b00001000;
	*(boolean + 4) = character & 0b00010000;
	*(boolean + 5) = character & 0b00100000;
	*(boolean + 6) = character & 0b01000000;
	*(boolean + 7) = character & 0b10000000;
}

char bool_to_char(bool* boolean)
{
	char return_character = 0;

	for (int i = 0; i < BITS; i++)
	{
		if(boolean[i])
			return_character |= 1<<i;
	}

	return return_character;
}
uint64_t* get_eviction_set(uint64_t* base_pointer, int set, int way)
{
	uint64_t tag = ((uint64_t) base_pointer) >> (INDEX_LENGTH + OFFSET_LENGTH);
	uint64_t set_index = (((uint64_t) base_pointer) >> OFFSET_LENGTH) & 0b111111;
	uint64_t offset = ((uint64_t) base_pointer) & 0b111111;
	uint64_t way_index;
	uint64_t return_address;

	if(set_index > set)
	{
		tag = tag << (INDEX_LENGTH + OFFSET_LENGTH);
		set_index = (set + SETS) << (INDEX_LENGTH);
		way_index = SETS*CACHE_LINE_SIZE*way;
		return_address = way_index + tag + set_index + offset;
	}
	else
	{
		tag = tag << (INDEX_LENGTH + OFFSET_LENGTH);
		set_index = (set) << (INDEX_LENGTH);
		way_index = SETS*CACHE_LINE_SIZE*way;
		return_address = way_index + tag + set_index + offset;
	}
	return (uint64_t*) return_address;
}


void create_eviction_chain(uint64_t* base_pointer, int associativity)
{
	uint64_t* chain_head;
	for (int i = 0; i < SETS; i++)
	{
		chain_head = get_eviction_set(base_pointer, i, 0);
		for (int j = 1; j < associativity; j++)
		{
			*(chain_head) = (uint64_t)get_eviction_set(base_pointer, i, j);
			chain_head = (uint64_t*)(*chain_head);
		}
		chain_head = NULL;
	}
	_mm_mfence();
}

inline void traverse_eviction_chain(uint64_t* base_pointer, int set)
{
	uint64_t* chain_head = get_eviction_set(base_pointer, set, 0);
	while(chain_head != NULL)
	{
		chain_head = (uint64_t*)(*chain_head);
		_mm_mfence();
	}
}

void sender(char character, uint64_t* base_pointer)
{
	int target_set;
	bool boolean[8];
	char_to_bool(character, boolean);

	for (int i = 0; i < BITS; i++)
	{
		if(boolean[i])
			traverse_eviction_chain(base_pointer, BITS*i + PIVOT);
	}

	// traverse_eviction_chain(base_pointer, target_set);
}

void receiver(uint64_t* base_pointer, int* counts)
{
	int aux = 0;
	uint64_t start;
	int time;
	int max_time = 0;
	int max_time_set;

	for (int i = 0; i < SETS/BITS; i++)
	{
		max_time = 0;
		max_time_set = BITS*i;
		for (int j = 0; j < BITS; j++)
		{
			start = __rdtscp(&aux);
			traverse_eviction_chain(base_pointer, BITS*i + j);
			time = __rdtscp(&aux) - start;
			if(time >= max_time)
			{
				max_time = time;
				max_time_set = BITS*i + j;
			}
		}
		counts[max_time_set]++;
	}

	
}

void dump_eviction_set(uint64_t* base_pointer)
{
	int way = 0;
	uint64_t* chain_head;
	for (int i = 0; i < SETS; i++)
	{
		way = 0;
		printf("Set: %d\n", i);
		chain_head = get_eviction_set(base_pointer, i, 0);

		while(chain_head != NULL)
		{
			printf("%d: %p\n", way, chain_head);
			chain_head = (uint64_t*)(*chain_head);
			way++;
		}
	}
	
}

void validate(uint64_t* base_pointer, int set, int way)
{
	uint64_t* chain_head = get_eviction_set(base_pointer, set, way);
	printf("%lx\n", (uint64_t)(*chain_head));
}


void demo(char* string, uint64_t* prime_array, uint64_t* probe_array)
{
	int counts[SETS];

	create_eviction_chain(prime_array, 8*ASSOCIATIVITY);
	create_eviction_chain(probe_array, ASSOCIATIVITY);

	int index = 0;

	while(true)
	{
		if(*(string + index) == '\0')
			break;

		memset(counts, 0, SETS*sizeof(int));

		for (int i = 0; i < ROUNDS; i++)
		{
			sender(*(string + index), prime_array);
			receiver(probe_array, counts);
		}

		
		bool boolean[8] = {false};
		for (int i = 0; i < SETS/BITS - 1; ++i)
		{
			boolean[i] = true;
			for (int j = 0; j < BITS; j++)
			{
				if(counts[i*BITS + j] > counts[i*BITS + PIVOT])
				{
					boolean[i] = false;
					break;
				}
			}
		}
		printf("%c", bool_to_char(boolean));

		index++;
	}
	

	printf("\n");
	
}

void demo_parent_child(char* string, uint64_t* prime_array, uint64_t* probe_array)
{
	int counts[SETS];

	create_eviction_chain(prime_array, 8*ASSOCIATIVITY);
	create_eviction_chain(probe_array, ASSOCIATIVITY);

	int turn = -1;
	int rounds = 5;
	pid_t child_pid;
	uint64_t time = 0;
	char buffer[2];
	int aux = 0;

	int pipe_1[2], pipe_2[2];

	if(pipe(pipe_1) == -1)
	{
		printf("Could not create the first pipe\n");
		exit(-1);
	}

	if(pipe(pipe_2) == -1)
	{
		printf("Could not create the first pipe\n");
		exit(-1);
	}


	child_pid = fork();

	if(child_pid == 0)
	{
		for (int i = 0; i < ROUNDS; i++)
		{
			time = synchronise_child();
			while(__rdtscp(&aux) - time < INTERVAL)
				sender('a', prime_array);
		}
	}

	else
	{
		memset(counts, 0, SETS*sizeof(int));

		for (int i = 0; i < ROUNDS; i++)
		{
			synchronise_parent();
			receiver(probe_array, counts);
		}

		for (int i = 0; i < BITS; ++i)
		{
			for (int j = 0; j < SETS/BITS; j++)
			{
				printf("%d\t", counts[i*BITS +j]);
			}
			printf("\n");
		}
		printf("\n");
	}
}

// uint64_t synchronise()
// {
// 	int aux = 0;

// 	while(__rdtscp(&aux) & TIME_MASK > JITTER);
// 	return __rdtscp(&aux);
// }

uint64_t synchronise_child()
{
	int aux = 0;

	while(__rdtscp(&aux) & TIME_MASK > JITTER);
	return __rdtscp(&aux);
}

uint64_t synchronise_parent()
{
	int aux = 0;

	while(__rdtscp(&aux) & TIME_MASK > JITTER);
	uint64_t time = __rdtscp(&aux);
	while(__rdtscp(&aux) - time < 2*INTERVAL);
	return __rdtscp(&aux);
}