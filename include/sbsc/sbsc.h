#ifndef INCLUDED_SBSC_H
#define INCLUDED_SBSC_H

#include <igraph.h>
#include <stdbool.h>
#include <sc_map.h>
#include <math.h>

// utility objects are objects for which the following functions/values are defined.
// expand as needed
typedef struct util_obj_intf {
	void* (*allocate)(void);
	void (*init_random)(void*);
	void (*copy)(void*, void*); // copies second into first
	void (*destroy)(void*);
	int (*hash)(void*);
	double (*get_utility)(const void*);
} util_obj_intf_t;

// parameters of the sbsc model
typedef struct sbsc_params {
	util_obj_intf_t util_obj_intf;
	int num_agents;
	double gamma; // softmax determinism
	double evidence_integration; // 0 is summed (popularity), 1 is averaged (best utility)
	double creativity; // probability of choosing a random opinion
	double connection_probability;
	double edge_toggle_probability;
	int rounds_opinion_exchange;
	int rounds_evolve_graph;
	void* empty_stats_info;
	void (*reset_stats_info)(void* stats_info);
	void (*collect_statistics)(void* stats_info, void* sbsc, float utility);
	void (*destroy_stats_info)(void* stats_info);
} sbsc_params_t;

typedef struct sbsc {
	sbsc_params_t params;
	igraph_t* connection_graph;
	igraph_t* best_connection_graph;
	void** util_objs; // array of pointers to utility objects
	void** prev_util_objs;
	void** best_util_objs;
	void** prev_best_util_objs;
	void* stats_info;
} sbsc_t;

sbsc_t* create_sbsc(sbsc_params_t);
void destroy_sbsc(sbsc_t*);

void update_util_objs(sbsc_params_t, igraph_t*, void** util_objs, void** prev_util_objs);
void evolve_graph(sbsc_t*);

typedef struct default_stats_info {
	int current_gen;
	int data_len;
	int stride;
	int* generation_nums;
	double* avg_utilities;
	double* avg_degrees;
	double* reciprocity;
	double* spinglass_modularity;
	double* clustering_coeff;
} default_stats_info_t;

void* default_empty_stats_info(int num_rounds, int stride);
void default_reset_stats_info(void* stats_info);
void default_collect_statistics(void* stats_info, void* sbsc, float utility);
void default_destroy_stats_info(void*);
void default_print_stats_info(void*);
void default_csv_stats_info(FILE* f, void* stats_info_ptr);

void run_sbsc(sbsc_t* s);

#endif
