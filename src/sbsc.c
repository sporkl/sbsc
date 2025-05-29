#include <sbsc/sbsc.h>

sbsc_t* create_sbsc(sbsc_params_t params) {
	// initialize the graph
	igraph_t* connection_graph = igraph_malloc(sizeof(igraph_t));
	igraph_erdos_renyi_game_gnp(
			connection_graph,
			params.num_agents,
			params.connection_probability,
			true, // directed
			false // not self-connected
		);
	
	// initialize "prev" graph
	igraph_t* prev_connection_graph = igraph_malloc(sizeof(igraph_t));
	igraph_copy(prev_connection_graph, connection_graph);

	// initialize the utility objects
	void** actor_utility_objects = igraph_malloc(params.num_agents * sizeof(void*));
	void** prev_util_objects = igraph_malloc(params.num_agents * sizeof(void*));
	for (int i = 0; i < params.num_agents; i++) {
		actor_utility_objects[i] = params.util_obj_intf.create_random();
		prev_util_objects[i] = params.util_obj_intf.create_unit();
		params.util_obj_intf.copy(prev_util_objects[i], actor_utility_objects[i]);
	}

	sbsc_t* new_sbsc = igraph_malloc(sizeof(sbsc_t));

	new_sbsc->params = params;
	new_sbsc->connection_graph = connection_graph;
	new_sbsc->prev_connection_graph = prev_connection_graph;
	new_sbsc->actor_util_objs = actor_utility_objects;
	new_sbsc->prev_util_objs = prev_util_objects;
	new_sbsc->prev_avg_utility = -INFINITY;
	new_sbsc->stats_info = params.empty_stats_info(params.rounds_evolve_graph);

	return new_sbsc;
}

// deallocation and cleanup
void destroy_sbsc(sbsc_t* s) {
	igraph_free(s->connection_graph);
	igraph_free(s->prev_connection_graph);

	for (int i = 0; i < s->params.num_agents; i++) {
		s->params.util_obj_intf.destroy(s->actor_util_objs[i]);
	}

	igraph_free(s->actor_util_objs);
	igraph_free(s->prev_util_objs);

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


void update_util_objs(sbsc_t* s) {

	// swap current and prev util obj arrays
	void** temp = s->prev_util_objs;
	s->prev_util_objs = s->actor_util_objs;
	s->actor_util_objs = temp;

	// update current util objs per agent
	// can overwrite because memory slot contains double-previous info
	for (int a = 0; a < s->params.num_agents; a++) {

		// get num neighbors
		igraph_integer_t num_neighbors;
		igraph_degree_1(
			s->connection_graph,
			&num_neighbors,
			a,
			IGRAPH_OUT,
			true
		);

		// if no neighbors, keep current object
		if (num_neighbors == 0) {
			s->params.util_obj_intf.copy(s->actor_util_objs[a], s->prev_util_objs[a]);
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
		igraph_neighbors(s->connection_graph, &neighbors, a, IGRAPH_OUT);

		for (int ni = 0; ni < num_neighbors; ni++) {
			// get the neighbor and it's util object
			int n = igraph_vector_int_get(&neighbors, ni);
			void* uo = s->prev_util_objs[n];

			// add the util object to the hashmap if it's not there yet
			int hash = s->params.util_obj_intf.hash(uo);
			util_obj_score_t* uos = (util_obj_score_t*) sc_map_get_intv(&util_obj_score_map, hash);
			if (!sc_map_found(&util_obj_score_map)) {
				util_obj_scores[util_obj_scores_len].obj = uo;
				util_obj_scores[util_obj_scores_len].score = 0.0;
				sc_map_put_intv(&util_obj_score_map, hash, &(util_obj_scores[util_obj_scores_len]));
				uos = &(util_obj_scores[util_obj_scores_len]);
				util_obj_scores_len++;
			}
	
			uos->score += s->params.util_obj_intf.get_utility(uos->obj);

		}
		// if want to incorporate own uo, then have arrow to self.
		
		// normalize evidence
		// need to update this if add different weights
		double normalization_denom = (s->params.evidence_integration * ((double) util_obj_scores_len - 1)) + 1;

		for (int uoi = 0; uoi < util_obj_scores_len; uoi++) {
			util_obj_scores[uoi].score /= normalization_denom;
		}

		double final_util_obj_scores[util_obj_scores_len];

		for (int i = 0; i < util_obj_scores_len; i++) {
			final_util_obj_scores[i] = util_obj_scores[i].score;
		}

		// use softmax_decide to choose a new utility object
		int chosen_util_obj_index = softmax_decide(final_util_obj_scores, util_obj_scores_len, s->params.gamma);
		
		s->params.util_obj_intf.copy(s->actor_util_objs[a], util_obj_scores[chosen_util_obj_index].obj);

	}
}

void evolve_graph(sbsc_t* s) {

	// get average utility of current graph
	double current_average_utility = 0.0;
	for (int i = 0; i < s->params.num_agents; i++) {
		current_average_utility += s->params.util_obj_intf.get_utility(s->actor_util_objs[i]);
	}
	current_average_utility /= (double) s->params.num_agents;

	// if the new graph is an improvement, update the prev graph
	if (current_average_utility > s->prev_avg_utility) {
		igraph_copy(s->prev_connection_graph, s->connection_graph);
		s->prev_avg_utility = current_average_utility;
	}

	// collect statistics
	s->params.collect_statistics(s->stats_info, s);

	// generate a new current graph
	igraph_erdos_renyi_game_gnp(
			s->connection_graph,
			s->params.num_agents,
			s->params.connection_probability,
			true, // directed
			false // no self-connections
		);
	
}

// default statistics collection

void* default_empty_stats_info(int num_rounds) {
	default_stats_info_t* stats_info = igraph_malloc(sizeof(default_stats_info_t));

	stats_info->current_round = 0;
	stats_info->total_rounds = num_rounds;
	stats_info->avg_utilities = igraph_malloc(num_rounds * sizeof(double));
	stats_info->avg_degrees = igraph_malloc(num_rounds * sizeof(int));
	stats_info->reciprocity = igraph_malloc(num_rounds * sizeof(double));
	stats_info->clustering_coeff = igraph_malloc(num_rounds * sizeof(double));
	
	return (void*) stats_info;

}

void default_collect_statistics(void* si, void* sbsc) {
	default_stats_info_t* stats_info = (default_stats_info_t*) si;
	sbsc_t* s = (sbsc_t*) sbsc;
	int n = stats_info->current_round;
	stats_info->avg_utilities[n] = s->prev_avg_utility;
	igraph_mean_degree(s->prev_connection_graph, &(stats_info->avg_degrees[n]), true);
	igraph_reciprocity(s->prev_connection_graph, &(stats_info->reciprocity[n]), false, IGRAPH_RECIPROCITY_DEFAULT);
	igraph_transitivity_undirected(s->prev_connection_graph, &(stats_info->clustering_coeff[n]), IGRAPH_TRANSITIVITY_ZERO);

	if (n < stats_info->total_rounds) {
		stats_info->current_round++;
	}
}

void default_destroy_stats_info(void* stats_info) {
	igraph_free(stats_info);
}

// run the experiment

void run_sbsc(sbsc_t* s) {
	for (int egi = 0; egi < s->params.rounds_evolve_graph; egi++) {
		for (int oei = 0; oei < s->params.rounds_opinion_exchange; oei++) {
			update_util_objs(s);
		}
		evolve_graph(s);
	}
}
