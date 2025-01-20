#include <sbsc/sbsc.h>

sbsc_t create_sbsc(sbsc_params_t params) {
	// initialize the graph
	igraph_t connection_graph;
	igraph_erdos_renyi_game_gnp(
			&connection_graph,
			params.num_agents,
			params.connection_probability,
			true, // directed
			false // no self-connections
		);
	
	// initialize "prev" graph
	igraph_t prev_connection_graph;
	igraph_copy(&prev_connection_graph, &connection_graph);

	// initialize the utility objects
	void** actor_utility_objects = igraph_malloc(params.num_agents * sizeof(void*));
	for (int i = 0; i < params.num_agents; i++) {
		actor_utility_objects[i] = params.utility_object.create_random();
	}

	sbsc_t new_sbsc = {
		.params = params,
		.connection_graph = connection_graph,
		.prev_connection_graph = prev_connection_graph,
		.actor_utility_objects = actor_utility_objects,
	};
	return new_sbsc;
}

void destroy_sbsc(sbsc_t s) {
	for (int i = 0; i < s.params.num_agents; i++) {
		s.params.utility_object.destroy(s.actor_utility_objects[i]);
	}
	igraph_free(s.actor_utility_objects);
}

void update_utility_objects(sbsc_t* s) {
	for (int a = 0; a < s->params.num_agents; a++) {
		
	}
}
