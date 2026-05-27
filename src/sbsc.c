#include <sbsc/sbsc.h>

igraph_error_t initialize_graph(igraph_t* connection_graph, int num_agents, double connection_probability) {

	igraph_error_t err;

	// create the graph
	err = igraph_erdos_renyi_game_gnp(
		connection_graph,
		num_agents,
		connection_probability,
		true, // directed
		IGRAPH_SIMPLE_SW, // no self loops
		false // not edge-labeled, I guess?
	);

	// return error if applicable
	if (err != IGRAPH_SUCCESS) {
		return err;
	}

	return IGRAPH_SUCCESS;
}

// resets util_objs of main connection graph to template
void reset_util_objs(sbsc_t* s) {
	for (int a = 0; a < s->params.num_agents; a++) {
		s->params.util_obj_intf.copy(s->util_objs[a], s->template_util_objs[a]);
		s->params.util_obj_intf.copy(s->prev_util_objs[a], s->template_util_objs[a]);
		s->params.util_obj_intf.copy(s->best_util_objs[a], s->template_util_objs[a]);
		s->params.util_obj_intf.copy(s->prev_best_util_objs[a], s->template_util_objs[a]);

		SETVAN(s->connection_graph, "utility", a, s->params.util_obj_intf.get_utility(s->util_objs[a]));
		SETVAN(s->best_connection_graph, "utility", a, s->params.util_obj_intf.get_utility(s->best_util_objs[a]));
	}
}

// initialize utility objects
void initialize_util_objs(sbsc_t* s) {
	for (int a = 0; a < s->params.num_agents; a++) {
		s->params.util_obj_intf.init_random(s->template_util_objs[a]);
	}

	reset_util_objs(s);
}


void free_array_objs(int len, void** objs, void (*destroy)(void*)) {
	for (int i = 0; i < len; i++) {
		destroy(objs[i]);
	}
}

sbsc_t* create_sbsc(sbsc_params_t params) {

	// make sure have access to weight attributes
	igraph_set_attribute_table(&igraph_cattribute_table);

	// allocate the graph
	igraph_t* connection_graph = igraph_malloc(sizeof(igraph_t));
	
	// allocate "best" graph
	igraph_t* best_connection_graph = igraph_malloc(sizeof(igraph_t));

	// initialize the utility objects
	void** template_util_objs = igraph_malloc(params.num_agents * sizeof(void*));
	void** util_objs = igraph_malloc(params.num_agents * sizeof(void*));
	void** prev_util_objs = igraph_malloc(params.num_agents * sizeof(void*));
	void** best_util_objs = igraph_malloc(params.num_agents * sizeof(void*));
	void** prev_best_util_objs = igraph_malloc(params.num_agents * sizeof(void*));
	for (int a = 0; a < params.num_agents; a++) {
		template_util_objs[a] = params.util_obj_intf.allocate();
		util_objs[a] = params.util_obj_intf.allocate();
		prev_util_objs[a] = params.util_obj_intf.allocate();
		best_util_objs[a] = params.util_obj_intf.allocate();
		prev_best_util_objs[a] = params.util_obj_intf.allocate();
	}

	// set up the sbsc object
	sbsc_t* new_sbsc = igraph_malloc(sizeof(sbsc_t));

	new_sbsc->params = params;
	new_sbsc->connection_graph = connection_graph;
	new_sbsc->best_connection_graph = best_connection_graph;
	new_sbsc->template_util_objs = template_util_objs;
	new_sbsc->util_objs = util_objs;
	new_sbsc->prev_util_objs = prev_util_objs;
	new_sbsc->best_util_objs = best_util_objs;
	new_sbsc->prev_best_util_objs = prev_best_util_objs;
	new_sbsc->stats_info = params.empty_stats_info;

	return new_sbsc;
}

// deallocation and cleanup
void destroy_sbsc(sbsc_t* s) {
	
	igraph_destroy(s->connection_graph);
	igraph_destroy(s->best_connection_graph);

	igraph_free(s->connection_graph);
	igraph_free(s->best_connection_graph);

	free_array_objs(s->params.num_agents, s->template_util_objs, s->params.util_obj_intf.destroy);
	free_array_objs(s->params.num_agents, s->util_objs, s->params.util_obj_intf.destroy);
	free_array_objs(s->params.num_agents, s->prev_util_objs, s->params.util_obj_intf.destroy);
	free_array_objs(s->params.num_agents, s->best_util_objs, s->params.util_obj_intf.destroy);
	free_array_objs(s->params.num_agents, s->prev_best_util_objs, s->params.util_obj_intf.destroy);

	igraph_free(s->template_util_objs);
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

// returns chosen index, or -1 if chooses "extra evidence"
int softmax_decide(double evidence[], int evidence_len, double gamma, double extra_evidence_val, int extra_evidence_len) {
	// generate probabilities
	double probabilities[evidence_len];
	for (int i = 0; i < evidence_len; i++) {
		probabilities[i] = exp(evidence[i] * gamma);
	}

	double extra_evidence_probability = exp(extra_evidence_val * gamma) * (double) extra_evidence_len;

	// sample according to probabilities
	for (int i = 0; i < evidence_len; i++) {

		// normalize probabilities, accounting for extra evidence
		double total = extra_evidence_probability;
		for (int j = i; j < evidence_len; j++) {
			total += probabilities[i];
		}

		for (int j = i; j < evidence_len; j++) {
			probabilities[i] /= total;
		}

		// see if choose current probability
		double roll = igraph_rng_get_unif01(igraph_rng_default());
		if (roll < probabilities[i]) {
			return i;
		}
	}

	return -1;

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
		
		util_obj_score_t util_obj_scores[num_neighbors + 1]; // + 1 for own opinion
		int util_obj_scores_len = 0;

		sc_map_intv_t util_obj_score_map;
		sc_map_init_intv(&util_obj_score_map, num_neighbors, 0);

		igraph_vector_int_t neighbors;
		igraph_vector_int_init(&neighbors, 0);
		igraph_neighbors(connection_graph, &neighbors, a, IGRAPH_OUT, IGRAPH_LOOPS_ONCE, IGRAPH_NO_MULTIPLE);

		// add own util object to hashmap
		void* uo = prev_util_objs[a];
		int hash = params.util_obj_intf.hash(uo);
		util_obj_scores[util_obj_scores_len].obj = uo;
		util_obj_scores[util_obj_scores_len].score = params.util_obj_intf.get_utility(uo);
		sc_map_put_intv(&util_obj_score_map, hash, &(util_obj_scores[util_obj_scores_len]));
		util_obj_scores_len++;

		// add neighbor util objects to hashmaps
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

		sc_map_term_intv(&util_obj_score_map);
		igraph_vector_int_destroy(&neighbors);
		
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
		int chosen_util_obj_index = softmax_decide(
				final_util_obj_scores, util_obj_scores_len,
				params.gamma,
				params.util_obj_intf.base_utility, params.util_obj_intf.count);

		if (chosen_util_obj_index < 0) {
			params.util_obj_intf.init_random(util_objs[a]);
		} else {
			params.util_obj_intf.copy(util_objs[a], util_obj_scores[chosen_util_obj_index].obj);
		}

		// update vertex utility
		SETVAN(connection_graph, "utility", a, params.util_obj_intf.get_utility(util_objs[a]));

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
		igraph_destroy(s->best_connection_graph);
		igraph_copy(s->best_connection_graph, s->connection_graph);
		best_average_utility = current_average_utility;
	}

	// collect statistics
	s->params.collect_statistics(s->stats_info, s, best_average_utility);

	//// generate a new current graph by toggling edges according to probability
	
	// start with the previous best graph
	igraph_destroy(s->connection_graph);
	igraph_copy(s->connection_graph, s->best_connection_graph);

	// figure out how many edges to toggle (binomial distribution)
	int edges_to_toggle = igraph_rng_get_binom(
			igraph_rng_default(), s->params.num_agents, s->params.edge_toggle_probability);

	// toggle those edges
	for (int i = 0; i < edges_to_toggle; i++) {
		// choose vertices (but make sure second vertex is different from first to avoid self-loops)
		int v1 = igraph_rng_get_integer(igraph_rng_default(), 0, s->params.num_agents - 1);
		int v2 = igraph_rng_get_integer(igraph_rng_default(), 0, s->params.num_agents - 2);
		if (v2 >= v1) v2++;

		// get edge id (-1 if it doesn't exist)
		igraph_integer_t eid = -1;
		igraph_get_eid(s->connection_graph, &eid, v1, v2, false, false);

		// toggle the edge
		if (eid == -1) {
			igraph_add_edge(s->connection_graph, v1, v2);
		} else {
			igraph_es_t edge_selector;
			igraph_es_1(&edge_selector, eid);
			igraph_delete_edges(s->connection_graph, edge_selector);
			igraph_es_destroy(&edge_selector);
		}

	}

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

void default_reset_stats_info(void* stats_info) {
	default_stats_info_t* si = (default_stats_info_t*) stats_info;

	si->current_gen = 0;
	// no need to clear other stuff b/c it should get overwritten

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

// statistics collection via saving graph snapshots

void* graphwrite_empty_stats_info(int stride, char* file_prefix) {
	graphwrite_stats_info_t* gsi = igraph_malloc(sizeof(graphwrite_stats_info_t));
	gsi->current_gen = 0;
	gsi->stride = stride;
	gsi->file_prefix = file_prefix;

	return gsi;
}

void graphwrite_reset_stats_info(void* stats_info) {
	graphwrite_stats_info_t* gsi = (graphwrite_stats_info_t*) stats_info;
	gsi->current_gen = 0;
}

void graphwrite_collect_statistics(void* stats_info, void* sbsc, float utility) {
	// suppress unused parameter
	(void) utility;

	graphwrite_stats_info_t* gsi = (graphwrite_stats_info_t*) stats_info;
	sbsc_t* the_sbsc = (sbsc_t*) sbsc;

	if (gsi->current_gen % gsi->stride == 0) {
		// open file with correct name
		char* filename;
		asprintf(&filename, "%s_gen%d.graphml", gsi->file_prefix, gsi->current_gen);
		FILE* f = fopen(filename, "w");

		// write the graph
		igraph_write_graph_graphml(
			the_sbsc->best_connection_graph,
			f,
			false
		);

		// close file and free up resources
		fclose(f);
		free(filename);
	}

	gsi->current_gen++;
}

void graphwrite_destroy_stats_info(void* stats_info) {
	igraph_free(stats_info);
}

// statistics collection of both default and graphwrite
void* default_and_graphwrite_empty_stats_info(int num_rounds, int stride, char* file_prefix) {
	default_and_graphwrite_stats_info_t* dgsi = igraph_malloc(sizeof(default_and_graphwrite_stats_info_t));

	dgsi->dsi = default_empty_stats_info(num_rounds, stride);
	dgsi->gsi = graphwrite_empty_stats_info(stride, file_prefix);

	return dgsi;
}

void default_and_graphwrite_reset_stats_info(void* stats_info) {

	default_and_graphwrite_stats_info_t* dgsi = (default_and_graphwrite_stats_info_t*) stats_info;

	default_reset_stats_info(dgsi->dsi);
	graphwrite_reset_stats_info(dgsi->gsi);

}

void default_and_graphwrite_collect_statistics(void* stats_info, void* sbsc, float utility) {

	default_and_graphwrite_stats_info_t* dgsi = (default_and_graphwrite_stats_info_t*) stats_info;

	default_collect_statistics(dgsi->dsi, sbsc, utility);
	graphwrite_collect_statistics(dgsi->gsi, sbsc, utility);

	sbsc_t* the_sbsc = (sbsc_t*) sbsc;

	// if this is the last collect_statistics, print csv
	if (dgsi->dsi->current_gen >= the_sbsc->params.rounds_evolve_graph) {
		
		char* filename;
		asprintf(&filename, "%s.csv", dgsi->gsi->file_prefix);
		FILE* f = fopen(filename, "w");

		default_csv_stats_info(f, dgsi->dsi);

		fclose(f);
		free(filename);
	}


}

void default_and_graphwrite_destroy_stats_info(void* stats_info) {

	default_and_graphwrite_stats_info_t* dgsi = (default_and_graphwrite_stats_info_t*) stats_info;

	default_destroy_stats_info(dgsi->dsi);
	graphwrite_destroy_stats_info(dgsi->gsi);

	igraph_free(dgsi);

}

// run the experiment

void run_sbsc(sbsc_t* s) {

	// initialize the experiment
	initialize_graph(s->connection_graph, s->params.num_agents, s->params.connection_probability);
	igraph_copy(s->best_connection_graph, s->connection_graph);

	initialize_util_objs(s);

	// evolve the graph
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
		reset_util_objs(s);
	}
}
