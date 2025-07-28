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

	// params
	sbsc_params_t main_ints_params = {
		.util_obj_intf = int_radius_five,
		.num_agents = 50,
		.gamma = 1.0,
		.evidence_integration = 0.0,
		.connection_probability = 0.5,
		.edge_toggle_probability = 0.05,
		.rounds_opinion_exchange = 50,
		.rounds_evolve_graph = 5001,
		.empty_stats_info = default_empty_stats_info(5001, 50),
		.reset_stats_info = default_reset_stats_info,
		.collect_statistics = default_collect_statistics,
		.destroy_stats_info = default_destroy_stats_info,
	};

	sbsc_t* sbsc = create_sbsc(main_ints_params);

	// string for storing filename
	// should be less than 50 chars
	char* filename = malloc(sizeof(char) * 50);

	for (int run = 0; run < 1; run++) {
		
		// reset stats info
		sbsc->params.reset_stats_info(sbsc->stats_info);

		// run the sbsc model
		run_sbsc(sbsc);

		// write out graph
		sprintf(filename, "main_ints_graph_%d.dot", run);
		FILE* graphfile = fopen(filename, "w");
		igraph_write_graph_dot(sbsc->best_connection_graph, graphfile);
		fclose(graphfile);

		sprintf(filename, "main_ints_stats_%d.csv", run);
		FILE* statsfile = fopen(filename, "w");
		default_csv_stats_info(statsfile, sbsc->stats_info);
		fclose(statsfile);

	}

	free(filename);

	destroy_sbsc(sbsc);
	
	return 0;
}
