#include <sbsc/sbsc.h>
#include <igraph.h>
#include <stdio.h>
#include <igraph.h>

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
	.base_utility = 1.0,
	.count = 50,
};

int main(void) {
	// seed the rng for reproducible results
	igraph_rng_seed(igraph_rng_default(), 2025);

	// iterate over starting degrees
	for (int init_deg_percent = 10; init_deg_percent <= 100; init_deg_percent += 10) {
		
		// 30 samples for each starting degree, so 300 total
		for (int sample = 0; sample < 30; sample++) {

			// base filename prefix for storing data
			char* file_prefix;
			asprintf(&file_prefix, "main_ints_initdegpercent%d_sample%d", init_deg_percent, sample);

			// params
			sbsc_params_t main_ints_params = {
				.util_obj_intf = int_radius_five,
				.num_agents = 50,
				.gamma = 1.0,
				.evidence_integration = 0.0,
				.connection_probability = ((double) init_deg_percent) / 100.0,
				.edge_toggle_probability = 0.05,
				.rounds_opinion_exchange = 50,
				.rounds_evolve_graph = 3001,
				.empty_stats_info = default_and_graphwrite_empty_stats_info(3001, 50, file_prefix),
				.reset_stats_info = default_and_graphwrite_reset_stats_info,
				.collect_statistics = default_and_graphwrite_collect_statistics,
				.destroy_stats_info = default_and_graphwrite_destroy_stats_info,
			};

			// create the sbsc
			sbsc_t* sbsc = create_sbsc(main_ints_params);
			sbsc->params.reset_stats_info(sbsc->stats_info);

			// run the sbsc model
			run_sbsc(sbsc);

			// destroy the sbsc
			destroy_sbsc(sbsc);

			// free file_prefix
			free(file_prefix);

		}
	}

	return 0;
}

