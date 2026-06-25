#include <sbsc/sbsc.h>
#include <igraph.h>
#include <stdio.h>
#include <igraph.h>

// util_obj

void* allocate(void) {
	int* x = (int*) igraph_malloc(sizeof(int));
	*x = 0;
	return (void*) x;
}

void random_int_radius_fifty(void* xv) {
	int* x = (int*) xv;
	*x = igraph_rng_get_integer(igraph_rng_default(), 1, 50);
}

void copy_int(void* a, void* b) {
	*((int*) a) = *((int*) b);
}

double int_radius_five_utility(const void* x) {
	return (double) *((int*) x);
}

int int_id(void* x) {
	return *((int*) x);
}

const util_obj_intf_t int_radius_five = {
	.allocate = *allocate,
	.init_random = *random_int_radius_fifty,
	.copy = *copy_int,
	.destroy = *igraph_free,
	.hash = *int_id,
	.get_utility = *int_radius_five_utility,
	.base_utility = 20.0,
	.count = 50,
};

// stats_info

typedef struct ultrafine_stats_info {
	int current_gen;
	int data_len;
	double* prev_avg_utilities;
	double* avg_utilities;
	double* avg_degrees;
} ultrafine_stats_info_t;

void* empty_ultrafine_stats_info(int num_rounds) {
	ultrafine_stats_info_t* stats_info = igraph_malloc(sizeof(ultrafine_stats_info_t));

	stats_info->current_gen = 0;
	stats_info->data_len = num_rounds;
	stats_info->prev_avg_utilities = igraph_malloc(num_rounds * sizeof(double));
	stats_info->avg_utilities = igraph_malloc(num_rounds * sizeof(double));
	stats_info->avg_degrees = igraph_malloc(num_rounds * sizeof(double));

	return (void*) stats_info;
}

void reset_ultrafine_stats_info(void* stats_info_ptr) {
	ultrafine_stats_info_t* stats_info = (ultrafine_stats_info_t*) stats_info_ptr;

	stats_info->current_gen = 0;
}

void collect_statistics_ultrafine_stats_info(void* stats_info_ptr, void* sbsc_ptr, double utility) {
	ultrafine_stats_info_t* stats_info = (ultrafine_stats_info_t*) stats_info_ptr;
	sbsc_t* sbsc = (sbsc_t*) sbsc_ptr;

	double prev_avg_utility = 0;
	double avg_utility = 0;
	for (int a = 0; a < sbsc->params.num_agents; a++) {
		prev_avg_utility += sbsc->params.util_obj_intf.get_utility(sbsc->best_util_objs[a]);
	}
	prev_avg_utility /= sbsc->params.num_agents;

	stats_info->prev_avg_utilities[stats_info->current_gen] = prev_avg_utility;
	stats_info->avg_utilities[stats_info->current_gen] = utility;

	igraph_mean_degree(sbsc->best_connection_graph, &(stats_info->avg_degrees[stats_info->current_gen]), true);

	stats_info->current_gen++;
}

void destroy_ultrafine_stats_info(void* stats_info_ptr) {
	ultrafine_stats_info_t* stats_info = (ultrafine_stats_info_t*) stats_info_ptr;

	igraph_free(stats_info->prev_avg_utilities);
	igraph_free(stats_info->avg_utilities);
	igraph_free(stats_info->avg_degrees);

	igraph_free(stats_info);
}

void csv_ultrafine_stats_info(FILE* f, void* stats_info_ptr) {
	ultrafine_stats_info_t* stats_info = (ultrafine_stats_info_t*) stats_info_ptr;
	
	fprintf(f, "generation,prev_avg_utility,avg_utility,avg_degree\n");
	for (int g = 0; g < stats_info->current_gen; g++) {
		fprintf(f, "%d,%f,%f,%f\n", g, stats_info->prev_avg_utilities[g], stats_info->avg_utilities[g], stats_info->avg_degrees[g]);
	}
}

int main(void) {
	// seed the rng for reproducible results
	igraph_rng_seed(igraph_rng_default(), 2026);

	// params
	sbsc_params_t main_ints_params = {
		.util_obj_intf = int_radius_five,
		.num_agents = 50,
		.gamma = 1.0,
		.evidence_integration = 0.0,
		.connection_probability = 12.0 / 50.0,
		.edge_toggle_probability = 0.05,
		.rounds_opinion_exchange = 50,
		.rounds_evolve_graph = 5001,
		.empty_stats_info = empty_ultrafine_stats_info(5001),
		.reset_stats_info = reset_ultrafine_stats_info,
		.collect_statistics = collect_statistics_ultrafine_stats_info,
		.destroy_stats_info = destroy_ultrafine_stats_info
	};

	// create the sbsc
	sbsc_t* sbsc = create_sbsc(main_ints_params);
	sbsc->params.reset_stats_info(sbsc->stats_info);

	// run the sbsc model
	run_sbsc(sbsc);

	// write statistics
	FILE* stats_file = fopen("initdeg12_ints_ultrafine.csv", "w");
	csv_ultrafine_stats_info(stats_file, sbsc->stats_info);
	fclose(stats_file);

	// destroy the sbsc
	destroy_sbsc(sbsc);



	return 0;
}

