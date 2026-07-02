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

double int_utility_nr_0(const void* x) {
	return (double) *((int*) x);
}

double int_utility_nr_1(const void* x) {
	return (double) *((int*) x) + igraph_rng_get_integer(igraph_rng_default(), -1, 1);
}

double int_utility_nr_3(const void* x) {
	return (double) *((int*) x) + igraph_rng_get_integer(igraph_rng_default(), -3, 3);
}

double int_utility_nr_5(const void* x) {
	return (double) *((int*) x) + igraph_rng_get_integer(igraph_rng_default(), -5, 5);
}

double int_utility_nr_10(const void* x) {
	return (double) *((int*) x) + igraph_rng_get_integer(igraph_rng_default(), -10, 10);
}

int int_id(void* x) {
	return *((int*) x);
}

const util_obj_intf_t uo_int_nr_0 = {
	.allocate = *allocate,
	.init_random = *random_int_radius_fifty,
	.copy = *copy_int,
	.destroy = *igraph_free,
	.hash = *int_id,
	.get_utility = *int_utility_nr_0,
	.base_utility = 20.0,
	.count = 50,
};

const util_obj_intf_t uo_int_nr_1 = {
	.allocate = *allocate,
	.init_random = *random_int_radius_fifty,
	.copy = *copy_int,
	.destroy = *igraph_free,
	.hash = *int_id,
	.get_utility = *int_utility_nr_1,
	.base_utility = 20.0,
	.count = 50,
};

const util_obj_intf_t uo_int_nr_3 = {
	.allocate = *allocate,
	.init_random = *random_int_radius_fifty,
	.copy = *copy_int,
	.destroy = *igraph_free,
	.hash = *int_id,
	.get_utility = *int_utility_nr_3,
	.base_utility = 20.0,
	.count = 50,
};

const util_obj_intf_t uo_int_nr_5 = {
	.allocate = *allocate,
	.init_random = *random_int_radius_fifty,
	.copy = *copy_int,
	.destroy = *igraph_free,
	.hash = *int_id,
	.get_utility = *int_utility_nr_5,
	.base_utility = 20.0,
	.count = 50,
};

const util_obj_intf_t uo_int_nr_10 = {
	.allocate = *allocate,
	.init_random = *random_int_radius_fifty,
	.copy = *copy_int,
	.destroy = *igraph_free,
	.hash = *int_id,
	.get_utility = *int_utility_nr_10,
	.base_utility = 20.0,
	.count = 50,
};

int main(void) {
	// seed the rng for reproducible results
	igraph_rng_seed(igraph_rng_default(), 2026);

	util_obj_intf_t noisy_util_obj_intfs[5] = {
		uo_int_nr_0,
		uo_int_nr_1,
		uo_int_nr_3,
		uo_int_nr_5,
		uo_int_nr_10
	};
	int util_obj_intf_nrs[5] = {0, 1, 3, 5, 10};
	const int nr_len = 5;

	for (int nr_index = 0; nr_index < nr_len; nr_index++) {

		for (int sample = 0; sample < 200; sample++) {

			// base filename prefix for storing data
			char* file_prefix;
			asprintf(&file_prefix, "initdeg12_ints_nr%d_sample%d", util_obj_intf_nrs[nr_index], sample);

			// params
			sbsc_params_t main_ints_params = {
				.util_obj_intf = noisy_util_obj_intfs[nr_index],
				.num_agents = 50,
				.gamma = 1.0,
				.evidence_integration = 0.0,
				.connection_probability = 12.0 / 50.0,
				.edge_toggle_probability = 0.05,
				.rounds_opinion_exchange = 50,
				.rounds_evolve_graph = 5001,
				.empty_stats_info = default_and_graphwrite_empty_stats_info(5001, 500, file_prefix),
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

