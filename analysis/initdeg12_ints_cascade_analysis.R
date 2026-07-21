# initdeg12_ints_cascade_analysis.R
# expect to see cascades/diversity favored over time
# after connectivity is initially pushed up

library(dplyr)
library(stringr)
library(igraph)
library(ggplot2)
library(purrr)

data_dir <- "../data/initdeg12_ints_v2"

graphs <- list.files(
	path = path.expand(data_dir),
	pattern = "^initdeg12_ints_sample\\d+_gen\\d+\\.graphml$",
	full.names = TRUE
)

graphs_dataframe <- data.frame(
	path = graphs,
	fname = basename(graphs),
	stringsAsFactors = FALSE
) |>
mutate(
	sample     = as.integer(str_match(fname, "sample(\\d+)")[, 2]),
	generation = as.integer(str_match(fname, "gen(\\d+)")[, 2])
) |>
arrange(sample, generation)

# analyze feedback arcs

calculate_feedback_arc_size <- function(path) {
	g <- igraph::read_graph(path, format = "graphml")
	length(igraph::feedback_arc_set(g, algo = "approx_eades"))
}

feedback_arc_sizes <- mapply(calculate_feedback_arc_size, graphs_dataframe$path)

graphs_dataframe$feedback_arc_sizes <- feedback_arc_sizes

feedback_arcs_over_time <- graphs_dataframe |>
	group_by(generation) |>
	summarize(mean_feedback_arc_size = mean(feedback_arc_sizes))

ggplot(feedback_arcs_over_time, aes(x = generation, y = mean_feedback_arc_size)) +
	geom_line()

# analyze utilities and connectivity

# final_generation_graphs_dataframe <- filter(graphs_dataframe, generation == 5000)

calculate_vertex_stats <- function(path, fname, sample, generation, feedback_arc_sizes) {

	g <- igraph::read_graph(path, format = "graphml")

	stats <- data.frame(
		degrees = igraph::degree(g, mode = "all"),
		in_degrees = igraph::degree(g, mode = "in"),
		out_degrees = igraph::degree(g, mode = "out"),
		utilities = igraph::V(g)$utility
	)

	stats$sample = sample
	stats$generation = generation

	stats
}

vertex_stats <- pmap_dfr(graphs_dataframe, calculate_vertex_stats)

# scatter plot of degree and utility
ggplot(vertex_stats, aes(x = degrees, y = utilities)) +
	geom_point() + facet_wrap(~ generation, scales = "free_y", ncol = 4)

ggplot(vertex_stats, aes(x = in_degrees, y = utilities)) +
	geom_point() + facet_wrap(~ generation, scales = "free_y", ncol = 4)

ggplot(vertex_stats, aes(x = out_degrees, y = utilities)) +
	geom_point() + facet_wrap(~ generation, scales = "free_y", ncol = 4)
