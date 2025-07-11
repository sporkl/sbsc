#include <sbsc/sbsc.h>


igraph_error_t initialize_graph(igraph_t* connection_graph, int num_agents, double connection_probability) {

	igraph_error_t err;

	// create the graph
	err = igraph_erdos_renyi_game_gnp(
		connection_graph,
		num_agents,
		connection_probability,
		true, // directed
		false // not probabilistically self-connected
	);

	// return error if applicable
	if (err != IGRAPH_SUCCESS) {
		return err;
	}

	// enforce self-connection
	for (int a = 0; a < num_agents; a++) {
		
		// create the edge
		err = igraph_add_edge(connection_graph, a, a);

		if (err != IGRAPH_SUCCESS) {
			return err;
		}
	}

	return IGRAPH_SUCCESS;
}

// initialize utility objects
void initialize_util_objs(sbsc_t* s) {
	for (int a = 0; a < s->params.num_agents; a++) {
		s->params.util_obj_intf.init_random(s->util_objs[a]);
		s->params.util_obj_intf.copy(s->prev_util_objs[a], s->util_objs[a]);
		s->params.util_obj_intf.copy(s->best_util_objs[a], s->util_objs[a]);
		s->params.util_obj_intf.copy(s->prev_best_util_objs[a], s->util_objs[a]);
	}
}

void free_array_objs(int len, void** objs, void (*destroy)(void*)) {
	for (int i = 0; i < len; i++) {
		destroy(objs[i]);
	}
}

sbsc_t* create_sbsc(sbsc_params_t params) {
	// initialize the graph
	igraph_t* connection_graph = igraph_malloc(sizeof(igraph_t));
	initialize_graph(connection_graph, params.num_agents, params.connection_probability);
	
	// initialize "best" graph
	igraph_t* best_connection_graph = igraph_malloc(sizeof(igraph_t));
	igraph_copy(best_connection_graph, connection_graph);

	// initialize the utility objects
	void** util_objs = igraph_malloc(params.num_agents * sizeof(void*));
	void** prev_util_objs = igraph_malloc(params.num_agents * sizeof(void*));
	void** best_util_objs = igraph_malloc(params.num_agents * sizeof(void*));
	void** prev_best_util_objs = igraph_malloc(params.num_agents * sizeof(void*));
	for (int a = 0; a < params.num_agents; a++) {
		util_objs[a] = params.util_obj_intf.allocate();
		prev_util_objs[a] = params.util_obj_intf.allocate();
		best_util_objs[a] = params.util_obj_intf.allocate();
		prev_best_util_objs[a] = params.util_obj_intf.allocate();
	}

	sbsc_t* new_sbsc = igraph_malloc(sizeof(sbsc_t));

	new_sbsc->params = params;
	new_sbsc->connection_graph = connection_graph;
	new_sbsc->best_connection_graph = best_connection_graph;
	new_sbsc->util_objs = util_objs;
	new_sbsc->prev_util_objs = prev_util_objs;
	new_sbsc->best_util_objs = best_util_objs;
	new_sbsc->prev_best_util_objs = prev_best_util_objs;
	new_sbsc->stats_info = params.empty_stats_info;

	initialize_util_objs(new_sbsc);

	return new_sbsc;
}

// deallocation and cleanup
void destroy_sbsc(sbsc_t* s) {
	igraph_free(s->connection_graph);
	igraph_free(s->best_connection_graph);

	free_array_objs(s->params.num_agents, s->util_objs, s->params.util_obj_intf.destroy);
	free_array_objs(s->params.num_agents, s->prev_util_objs, s->params.util_obj_intf.destroy);
	free_array_objs(s->params.num_agents, s->best_util_objs, s->params.util_obj_intf.destroy);
	free_array_objs(s->params.num_agents, s->prev_best_util_objs, s->params.util_obj_intf.destroy);

	igraph_free(s->util_objs);
	igraph_free(s->prev_util_objs);
	igraph_free(s->best_util_objs);
	igraph_free(s->prev_best_util_objs);

	s->params.destroy_stats_info(s->stats_info);

	igraph_free(s);
}

// hold a util obj and score in a hash table
typedef struct util_obj_score {
	void* obj;
	double score;
} util_obj_score_t;

void normalize_probabilities(double* probs, int start_index, int end_index) {
	float total = 0.0;
	for (int i = start_index; i < end_index; i++) {
		total += probs[i];
	}

	for (int i = start_index; i < end_index; i++) {
		probs[i] /= total;
	}
}

int softmax_decide(double evidence[], int evidence_len, double gamma) {
	// generate probabilities
	double probabilities[evidence_len];
	for (int i = 0; i < evidence_len; i++) {
		probabilities[i] = exp(evidence[i] * gamma);
	}

	// sample according to probabilities
	for (int i = 0; i < evidence_len; i++) {
		normalize_probabilities(probabilities, i, evidence_len);
		double roll = igraph_rng_get_unif01(igraph_rng_default());
		if (roll < probabilities[i]) {
			return i;
		}
	}

	return evidence_len - 1;

}

// updates the util_obj assignments in util_objs, reading from prev_util_objs
// util_objs gets overwritten!
void update_util_objs(sbsc_params_t params, igraph_t* connection_graph, void** util_objs, void** prev_util_objs) {

	// update current util objs per agent
	// can overwrite because memory slot contains double-previous info
	for (int a = 0; a < params.num_agents; a++) {

		// get num neighbors
		igraph_integer_t num_neighbors;
		igraph_degree_1(
			connection_graph,
			&num_neighbors,
			a,
			IGRAPH_OUT,
			true
		);

		// if no neighbors, keep current object
		if (num_neighbors == 0) {
			params.util_obj_intf.copy(util_objs[a], prev_util_objs[a]);
			continue;
		}

		// score each util object

		typedef struct sc_map_intv sc_map_intv_t;
		
		util_obj_score_t util_obj_scores[num_neighbors];
		int util_obj_scores_len = 0;

		sc_map_intv_t util_obj_score_map;
		sc_map_init_intv(&util_obj_score_map, num_neighbors, 0);

		igraph_vector_int_t neighbors;
		igraph_vector_int_init(&neighbors, 0);
		igraph_neighbors(connection_graph, &neighbors, a, IGRAPH_OUT);

		for (int ni = 0; ni < num_neighbors; ni++) {
			// get the neighbor and it's util object
			int n = igraph_vector_int_get(&neighbors, ni);
			void* uo = prev_util_objs[n];

			// add the util object to the hashmap if it's not there yet
			int hash = params.util_obj_intf.hash(uo);
			util_obj_score_t* uos = (util_obj_score_t*) sc_map_get_intv(&util_obj_score_map, hash);
			if (!sc_map_found(&util_obj_score_map)) {
				util_obj_scores[util_obj_scores_len].obj = uo;
				util_obj_scores[util_obj_scores_len].score = 0.0;
				sc_map_put_intv(&util_obj_score_map, hash, &(util_obj_scores[util_obj_scores_len]));
				uos = &(util_obj_scores[util_obj_scores_len]);
				util_obj_scores_len++;
			}
	
			uos->score += params.util_obj_intf.get_utility(uos->obj);

		}
		
		// normalize evidence
		// need to update this if add different weights
		double normalization_denom = (params.evidence_integration * ((double) util_obj_scores_len - 1)) + 1;

		for (int uoi = 0; uoi < util_obj_scores_len; uoi++) {
			util_obj_scores[uoi].score /= normalization_denom;
		}

		double final_util_obj_scores[util_obj_scores_len];

		for (int i = 0; i < util_obj_scores_len; i++) {
			final_util_obj_scores[i] = util_obj_scores[i].score;
		}

		// use softmax_decide to choose a new utility object
		int chosen_util_obj_index = softmax_decide(final_util_obj_scores, util_obj_scores_len, params.gamma);
		
		params.util_obj_intf.copy(util_objs[a], util_obj_scores[chosen_util_obj_index].obj);

	}
}

void evolve_graph(sbsc_t* s) {

	// get average utility of current graph
	double current_average_utility = 0.0;
	for (int i = 0; i < s->params.num_agents; i++) {
		current_average_utility += s->params.util_obj_intf.get_utility(s->util_objs[i]);
	}
	current_average_utility /= (double) s->params.num_agents;

	// get average utility of "best" graph
	double best_average_utility = 0.0;
	for (int i = 0; i < s->params.num_agents; i++) {
		best_average_utility += s->params.util_obj_intf.get_utility(s->best_util_objs[i]);
	}
	best_average_utility /= (double) s->params.num_agents;
	

	// if the new graph is an improvement, update the prev graph
	if (current_average_utility > best_average_utility) {
		igraph_copy(s->best_connection_graph, s->connection_graph);
		best_average_utility = current_average_utility;
	}

	// collect statistics
	s->params.collect_statistics(s->stats_info, s, best_average_utility);

	// generate a new current graph by rewiring edges
	igraph_copy(s->connection_graph, s->best_connection_graph);
	igraph_rewire_edges(s->connection_graph, s->params.rewire_probability, true, false);
}

// default statistics collection

void* default_empty_stats_info(int num_rounds, int stride) {
	default_stats_info_t* stats_info = igraph_malloc(sizeof(default_stats_info_t));

	int data_len = (num_rounds / stride) + 1; // 21 / 5 is 4, but needs slots for 0, 5, 10, 15, 20

	stats_info->current_gen = 0;
	stats_info->data_len = data_len;
	stats_info->stride = stride;
	stats_info->generation_nums = igraph_malloc(data_len * sizeof(int));
	stats_info->avg_utilities = igraph_malloc(data_len * sizeof(double));
	stats_info->avg_degrees = igraph_malloc(data_len * sizeof(double));
	stats_info->reciprocity = igraph_malloc(data_len * sizeof(double));
	stats_info->spinglass_modularity = igraph_malloc(data_len * sizeof(double));
	stats_info->clustering_coeff = igraph_malloc(data_len * sizeof(double));
	
	return (void*) stats_info;

}

void default_collect_statistics(void* si, void* sbsc, float utility) {
	default_stats_info_t* stats_info = (default_stats_info_t*) si;
	sbsc_t* s = (sbsc_t*) sbsc;

	if (stats_info->current_gen % stats_info->stride == 0) {
		int n = stats_info->current_gen / stats_info->stride;
		stats_info->generation_nums[n] = stats_info->current_gen;
		stats_info->avg_utilities[n] = utility;
		igraph_mean_degree(s->best_connection_graph, &(stats_info->avg_degrees[n]), true);
		igraph_reciprocity(s->best_connection_graph, &(stats_info->reciprocity[n]), false, IGRAPH_RECIPROCITY_DEFAULT);
		igraph_community_spinglass(s->best_connection_graph, NULL, &(stats_info->spinglass_modularity[n]),
					NULL, NULL, NULL, /* num spins */ 4, false, 1.0, 0.01, 0.99,
					IGRAPH_SPINCOMM_UPDATE_SIMPLE, /* gamma */ 1.0, IGRAPH_SPINCOMM_IMP_ORIG, 0.0);
		igraph_transitivity_undirected(
				s->best_connection_graph, &(stats_info->clustering_coeff[n]), IGRAPH_TRANSITIVITY_ZERO);
	}

	stats_info->current_gen++;
}

void default_destroy_stats_info(void* stats_info_pointer) {
	default_stats_info_t* stats_info = (default_stats_info_t*) stats_info_pointer;

	igraph_free(stats_info->generation_nums);
	igraph_free(stats_info->avg_utilities);
	igraph_free(stats_info->avg_degrees);
	igraph_free(stats_info->reciprocity);
	igraph_free(stats_info->spinglass_modularity);
	igraph_free(stats_info->clustering_coeff);

	igraph_free(stats_info);
}

void default_csv_stats_info(FILE* f, void* stats_info_ptr) {
	default_stats_info_t* stats = (default_stats_info_t*) stats_info_ptr;
	fprintf(f, "generation,avg_utility,avg_degrees,reciprocity,spinglass_modularity,cluster_coeff\n");
	for (int r = 0; r < stats->data_len; r++) {
		fprintf(f, "%d,%f,%f,%f,%f,%f\n",
				stats->generation_nums[r], stats->avg_utilities[r], stats->avg_degrees[r],
				stats->reciprocity[r], stats->spinglass_modularity[r], stats->clustering_coeff[r]);
	}
}

// run the experiment

void run_sbsc(sbsc_t* s) {
	for (int egi = 0; egi < s->params.rounds_evolve_graph; egi++) {
		
		// rounds of opinion exchange
		for (int oei = 0; oei < s->params.rounds_opinion_exchange; oei++) {
			
			// swap current and prev util objs
			void** temp;
			temp = s->prev_util_objs;
			s->prev_util_objs = s->util_objs;
			s->util_objs = temp;

			temp = s->prev_best_util_objs;
			s->prev_best_util_objs = s->best_util_objs;
			s->best_util_objs = temp;

			update_util_objs(s->params, s->connection_graph, s->util_objs, s->prev_util_objs);
			update_util_objs(s->params, s->best_connection_graph, s->best_util_objs, s->prev_best_util_objs);
		}

		// evolve the graph
		evolve_graph(s);

		// reset opinions
		initialize_util_objs(s);
	}
}
