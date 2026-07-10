#include <sbsc/sbsc.h>
#include <igraph.h>
#include <limits.h>
#include <stdio.h>

// run from subdirectory of data

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
		.empty_stats_info = default_empty_stats_info(1, INT_MAX),
		.reset_stats_info = default_reset_stats_info,
		.collect_statistics = default_collect_statistics,
		.destroy_stats_info = default_destroy_stats_info,
	};

	sbsc_t* sbsc = create_sbsc(main_ints_params);
	initialize_graph(sbsc->connection_graph, sbsc->params.num_agents, sbsc->params.connection_probability);
	igraph_copy(sbsc->best_connection_graph, sbsc->connection_graph);
	initialize_util_objs(sbsc);
	sbsc->params.reset_stats_info(sbsc->stats_info);

	double gen1000_avg_utilities[300];
	double gen5000_avg_utilities[300];

	for (int sample = 0; sample < 300; sample++) {
		// load generation 1000 and generation 5000 networks
		// generation 1000 network goes into connection_graph
		// generaiton 5000 network goes into best_connection_graph
		char* gen1000_graphml_filename;
		asprintf(&gen1000_graphml_filename, "../initdeg12_ints_v2/initdeg12_ints_sample%d_gen1000.graphml", sample);
		FILE* gen1000_graphml = fopen(gen1000_graphml_filename, "r");

		char* gen5000_graphml_filename;
		asprintf(&gen5000_graphml_filename, "../initdeg12_ints_v2/initdeg12_ints_sample%d_gen5000.graphml", sample);
		FILE* gen5000_graphml = fopen(gen5000_graphml_filename, "r");

		igraph_destroy(sbsc->connection_graph);
		igraph_read_graph_graphml(sbsc->connection_graph, gen1000_graphml, 0);
		igraph_destroy(sbsc->best_connection_graph);
		igraph_read_graph_graphml(sbsc->best_connection_graph, gen5000_graphml, 0);
		
		fclose(gen1000_graphml);
		free(gen1000_graphml_filename);
		fclose(gen5000_graphml);
		free(gen5000_graphml_filename);

		// find average utility
		double gen1000_net_average_utility = 0.0;
		double gen5000_net_average_utility = 0.0;
		for (int run = 0; run < 200; run++) {
			// reset opinions
			reset_util_objs(sbsc);
			
			// run 
			exchange_opinions(sbsc);

			// add to net_average_utility
			double gen1000_net_utility = 0;
			double gen5000_net_utility = 0;
			for (int a = 0; a < sbsc->params.num_agents; a++) {
				gen1000_net_utility += sbsc->params.util_obj_intf.get_utility(sbsc->util_objs[a]);
				gen5000_net_utility += sbsc->params.util_obj_intf.get_utility(sbsc->best_util_objs[a]);
			}
			gen1000_net_average_utility += gen1000_net_utility / sbsc->params.num_agents;
			gen5000_net_average_utility += gen5000_net_utility / sbsc->params.num_agents;
		}

		gen1000_avg_utilities[sample] = gen1000_net_average_utility / 200.0;
		gen5000_avg_utilities[sample] = gen5000_net_average_utility / 200.0;

	}

	// print as csv
	FILE* csv = fopen("initdeg12_ints_v2_ratchet.csv", "w");
	fprintf(csv, "sample,gen1000_avg_utility,gen5000_avg_utility\n");

	for (int sample = 0; sample < 300; sample++) {
		fprintf(csv, "%d,%f,%f\n", sample, gen1000_avg_utilities[sample], gen5000_avg_utilities[sample]);
	}

	fclose(csv);

}
