#include <sbsc/sbsc.h>
#include <igraph.h>

void* allocate(void) {
	int* x = (int*) igraph_malloc(sizeof(int));
	*x = 0;
	return (void*) x;
}

void random_int_radius_five(void* xv) {
	int* x = (int*) xv;
	*x = igraph_rng_get_integer(igraph_rng_default(), -5, 5);
}

void copy_int(void* a, void* b) {
	*((int*) a) = *((int*) b);
}

double int_radius_five_utility(const void* x) {
	int* y = (int*) x;
	return (double) *y;
}

int int_id(void* x) {
	return *((int*) x);
}

const util_obj_intf_t int_radius_five = {
	.allocate = *allocate,
	.init_random = *random_int_radius_five,
	.copy = *copy_int,
	.destroy = *igraph_free,
	.hash = *int_id,
	.get_utility = *int_radius_five_utility,
};

int main(void) {
	// seed the rng for reproducible results
	igraph_rng_seed(igraph_rng_default(), 2025);

	// params
	sbsc_params_t basic_ints_params = {
		.util_obj_intf = int_radius_five,
		.num_agents = 100,
		.gamma = 1.0,
		.evidence_integration = 0.0,
		.connection_probability = 0.3,
		.edge_toggle_probability = 0.05,
		.rounds_opinion_exchange = 20,
		.rounds_evolve_graph = 21,
		.empty_stats_info = default_empty_stats_info(21, 5),
		.collect_statistics = default_collect_statistics,
		.destroy_stats_info = default_destroy_stats_info,
	};
	
	// create the sbsc model
	sbsc_t* the_sbsc = create_sbsc(basic_ints_params);

	// run the sbsc model
	run_sbsc(the_sbsc);

	// print statistics
	default_stats_info_t* stats = (default_stats_info_t*) the_sbsc->stats_info;
	default_csv_stats_info(stdout, stats);
	
	// print the graph
	/* printf("---------- GRAPH ----------\n"); */
	/* igraph_write_graph_dot(the_sbsc->best_connection_graph, stdout); */

	destroy_sbsc(the_sbsc);
	
	return 0;
}
