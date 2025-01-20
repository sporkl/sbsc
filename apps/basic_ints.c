#include <sbsc/sbsc.h>
#include <igraph.h>

void* random_int_radius_five() {
	int* x = (int*) igraph_malloc(sizeof(int));
	*x = igraph_rng_get_integer(igraph_rng_default(), -5, 5);
	return (void*) x;
}

double int_radius_five_utility(const void* x) {
	int* y = (int*) x;
	return (double) *y;
}

const utility_object_t int_radius_five = {
	.create_random = *random_int_radius_five,
	.destroy = *igraph_free,
	.get_utility = *int_radius_five_utility,
};

// taken from original paper exception for connection_probability
const sbsc_params_t basic_ints_params = {
	.utility_object = int_radius_five,
	.num_agents = 200,
	.gamma = 1.0,
	.evidence_integration = 0.0,
	.connection_probability = 0.1,
	.num_rounds = 20,
};

int main(void) {
	// seed the rng for reproducible results
	igraph_rng_seed(igraph_rng_default(), 2025);
	
	// create the sbsc model
	sbsc_t the_sbsc = create_sbsc(basic_ints_params);
	return 0;
}
