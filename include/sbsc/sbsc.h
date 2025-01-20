#ifndef INCLUDED_SBSC_H
#define INCLUDED_SBSC_H

#include <igraph.h>
#include <stdbool.h>

// utility object is object for which the following functions/values are defined.
// expand as needed
typedef struct utility_object {
	void* (*create_random)();
	void (*destroy)(void*);
	double (*get_utility)(const void*);
} utility_object_t;

// parameters of the sbsc model
typedef struct sbsc_params {
	utility_object_t utility_object;
	int num_agents;
	double gamma; // softmax determinism
	double evidence_integration; // 0 is summed (popularity), 1 is averaged (best utility)
	double connection_probability;
	int num_rounds;
} sbsc_params_t;

typedef struct sbsc {
	sbsc_params_t params;
	igraph_t connection_graph;
	igraph_t prev_connection_graph;
	void** actor_utility_objects; // array of pointers to utility objects
} sbsc_t;

sbsc_t create_sbsc(sbsc_params_t);
void destroy_sbsc(sbsc_t);

void update_utility_objects(sbsc_t*);

#endif
