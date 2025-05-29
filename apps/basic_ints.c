#include <sbsc/sbsc.h>
#include <igraph.h>

void* int_unit() {
	int* x = (int*) igraph_malloc(sizeof(int));
	*x = 0;
	return (void*) x;
}

void* random_int_radius_five() {
	int* x = (int*) igraph_malloc(sizeof(int));
	*x = igraph_rng_get_integer(igraph_rng_default(), -5, 5);
	return (void*) x;
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
	.create_unit = *int_unit,
	.create_random = *random_int_radius_five,
	.copy = copy_int,
	.destroy = *igraph_free,
	.hash = *int_id,
	.get_utility = *int_radius_five_utility,
};

// taken from original paper exception for connection_probability
const sbsc_params_t basic_ints_params = {
	.util_obj_intf = int_radius_five,
	.num_agents = 200,
	.gamma = 1.0,
	.evidence_integration = 0.0,
	.connection_probability = 0.1,
	.rounds_opinion_exchange = 20,
	.rounds_evolve_graph = 20,
	.empty_stats_info = default_empty_stats_info,
	.collect_statistics = default_collect_statistics,
	.destroy_stats_info = default_destroy_stats_info,
};

int main(void) {
	// seed the rng for reproducible results
	igraph_rng_seed(igraph_rng_default(), 2025);
	
	// create the sbsc model
	sbsc_t* the_sbsc = create_sbsc(basic_ints_params);

	// run the sbsc model
	run_sbsc(the_sbsc);

	// print statistics
	default_stats_info_t* stats = (default_stats_info_t*) the_sbsc->stats_info;
	
	printf("===== STATS =====\n");
	for (int r = 0; r < the_sbsc->params.rounds_evolve_graph; r++) {
		printf("round: %d\n", r);
		printf("avg utility: %f\n", stats->avg_utilities[r]);
		printf("avg degrees: %f\n", stats->avg_degrees[r]);
		printf("reciprocity: %f\n", stats->reciprocity[r]);
		printf("cluster coeff: %f\n", stats->clustering_coeff[r]);
		printf("-----------------\n");
	}
	
	return 0;
}
