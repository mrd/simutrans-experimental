#ifndef SIMTOOLS_H
#define SIMTOOLS_H

#include <stddef.h>
#include "simtypes.h"
#ifdef DEBUG_SIMRAND_CALLS
#include "tpl/fixed_list_tpl.h"
#endif

uint32 get_random_seed();

uint32 setsimrand(uint32 seed, uint32 noise_seed);

/* generates a random number on [0,max-1]-interval
 * without affecting the game state
 * Use this for UI etc.
 */
uint32 sim_async_rand(const uint32 max);

/* generates a random number on [0,max-1]-interval */
#ifdef DEBUG_SIMRAND_CALLS
uint32 simrand(const uint32 max, const char* caller);
#else
uint32 simrand(const uint32 max, const char*);
#endif

/* generates a random number on [0,0xFFFFFFFFu]-interval */
uint32 simrand_plain(void);

/* generates random number with gaussian(normal) distribution */
double simrand_gauss(const double mean, const double sigma);
double perlin_noise_2D(const double x, const double y, const double persistence, const sint32 map_size = 512);

// for netowrk debugging, i.e. finding hidden simrands in worng places
enum { INTERACTIVE_RANDOM=1, STEP_RANDOM=2, SYNC_STEP_RANDOM=4, LOAD_RANDOM=8, MAP_CREATE_RANDOM=16 };
void set_random_mode( uint16 );
void clear_random_mode( uint16 );
uint16 get_random_mode();

// just more speed with those (generate a precalculated map, which needs only smoothing)
void init_perlin_map( sint32 w, sint32 h );
void exit_perlin_map();

/* Randomly select an entry from the given array. */
template<typename T, size_t N> T const& pick_any(T const (&array)[N])
{
	return array[simrand(N, "template<typename T, size_t N> T const& pick_any(T const (&array)[N])")];
}

/* Randomly select an entry from the given container. */
template<typename T, template<typename> class U> T const& pick_any(U<T> const& container)
{
	return container[simrand(container.get_count(), "template<typename T, template<typename> class U> T const& pick_any(U<T> const& container)")];
}

/* Randomly select an entry from the given weighted container. */
template<typename T, template<typename> class U> T const& pick_any_weighted(U<T> const& container)
{
	return container.at_weight(simrand(container.get_sum_weight(), "template<typename T, template<typename> class U> T const& pick_any_weighted(U<T> const& container)"));
}

/* Randomly select an entry from the given subset of a weighted container. */
template<typename T, typename U> T const& pick_any_weighted_subset(U const& container)
{
	return container.at_weight(simrand(container.get_sum_weight()));
}


// compute integer log10
/*uint32 log10(uint32 v);*/

#endif
