# load_initdeg12_ints.R
# Reads all initdeg12_ints CSV files into a single tidy data frame
# and produces summary plots. 300 replications, initial degree = 12.
#
# Each CSV file is named:
#   initdeg12_ints_sample{S}.csv
# and contains columns:
#   generation, avg_utility, avg_degrees, reciprocity,
#   spinglass_modularity, cluster_coeff

library(dplyr)
library(stringr)
library(ggplot2)
library(tidyr)
library(igraph)
# also requires RSpectra and Matrix

# ── 1. Point this at the data folder ────────────────────────────────────────
data_dir <- "../data/initdeg12_ints_v2"   # update if stored elsewhere

# ── 2. Find all CSV files ─────────────────────────────────────────────────────
csv_files <- list.files(
  path    = path.expand(data_dir),
  pattern = "^initdeg12_ints_sample\\d+\\.csv$",
  full.names = TRUE
)
message("Found ", length(csv_files), " CSV files.")

# ── 3. Read and label each file ───────────────────────────────────────────────
read_one <- function(path) {
  fname   <- basename(path)
  matches <- str_match(fname, "sample(\\d+)")
  df <- read.csv(path)
  df$sample <- as.integer(matches[, 2])
  df
}

initdeg12 <- bind_rows(lapply(csv_files, read_one))

# ── 4. Tidy up column order ───────────────────────────────────────────────────
initdeg12 <- initdeg12[, c("sample", "generation",
                           "avg_utility", "avg_degrees", "reciprocity",
                           "spinglass_modularity", "cluster_coeff")]

# ── 5. Quick sanity check ─────────────────────────────────────────────────────
message("Rows: ", nrow(initdeg12))
message("Samples: ", length(unique(initdeg12$sample)))
message("Generation range: ", min(initdeg12$generation), " – ", max(initdeg12$generation))
print(head(initdeg12))

# ── 6. Plot: mean average degree over generations ─────────────────────────────
deg_summary <- initdeg12 |>
  group_by(generation) |>
  summarise(mean_avg_degrees = mean(avg_degrees, na.rm = TRUE), .groups = "drop")

p <- ggplot(deg_summary, aes(x = generation, y = mean_avg_degrees)) +
  geom_line(linewidth = 0.8, color = "#377EB8") +
  labs(
    title    = "Evolution of Mean Network Degree (initial degree = 12)",
    subtitle = "Averaged across 300 replications",
    x        = "Generation",
    y        = "Mean average degree"
  ) +
  theme_minimal(base_size = 13)
print(p)
# the degree goes from 12 up to almost 12.9 and then systematically go down!
# My optimistic interpretation is that the networks are changing something
# so that the higher degrees are no longer needed/good. One of the things that they may
# be changing is local connectivity - see below.  The connections are becoming less
#reciprocal and modularity is decreasing - the networks are getting more diverse/longrange connected

# ── 7. Plot: mean average utility over generations ────────────────────────────
utility_summary <- initdeg12 |>
  group_by(generation) |>
  summarise(mean_avg_utility = mean(avg_utility, na.rm = TRUE), .groups = "drop")

p <- ggplot(utility_summary, aes(x = generation, y = mean_avg_utility)) +
  geom_line(linewidth = 0.8, color = "#E41A1C") +
  labs(
    title    = "Evolution of Mean Network Utility (initial degree = 12)",
    subtitle = "Averaged across 300 replications",
    x        = "Generation",
    y        = "Mean average utility"
  ) +
  theme_minimal(base_size = 13)
print(p)
#You could argue that the fitness is just tracking degree for the first 1000 generations,
# but then when degree comes down, fitness stays the same, or arguably even goes up to 38.
# That might be enough of a signal to look into the graphs to figure out how they're changing.

# ── 8. Plot: reciprocity, clustering coefficient, spinglass modularity ─────────
measures_summary <- initdeg12 |>
  group_by(generation) |>
  summarise(
    Reciprocity              = mean(reciprocity,           na.rm = TRUE),
    `Clustering coefficient` = mean(cluster_coeff,         na.rm = TRUE),
    `Spinglass modularity`   = mean(spinglass_modularity,  na.rm = TRUE),
    .groups = "drop"
  ) |>
  pivot_longer(cols = -generation, names_to = "measure", values_to = "value")

p <- ggplot(measures_summary,
            aes(x = generation, y = value, color = measure, group = measure)) +
  geom_line(linewidth = 0.9) +
  scale_color_manual(
    values = c(
      "Reciprocity"              = "#E41A1C",
      "Clustering coefficient"   = "#377EB8",
      "Spinglass modularity"     = "#4DAF4A"
    ),
    name = NULL
  ) +
  labs(
    title    = "Evolution of Network Structure Measures (initial degree = 12)",
    subtitle = "Averaged across 300 replications",
    x        = "Generation",
    y        = "Mean value"
  ) +
  theme_minimal(base_size = 13) +
  theme(legend.position = "right")
print(p)
#reciprocity CONTINUES to go down after Generation 1000, so this decrease can't only be blamed
# on the evolution of overall degree! In fact, reciprocity is DECREASING even when
# average degree is INCREASING, for the first 1000 generations. My interpretation is: it's not useful for
# Agents A and B to be reciprocally connected because they aren't providing new information
# to each other. It would be better to have them branch out and see what other agents'
# opinions are. This is consistent with modularity also decreasing past Generation 1000.
# Clustering coefficient mostly tracks increasing degree. There is no benefit to
# greater clustering after the degree has hit 12.9.
# Importantly, degree and distribution are not playing the same role.  It seems to be better
# to have further-flung connections even when there is not a benefit to increasing degree

# Compute statistics direclty from the graphs that are produced
all_files <- list.files(
  path       = path.expand(data_dir),
  pattern    = "^initdeg12_ints_sample\\d+_gen\\d+\\.graphml$",
  full.names = TRUE
)
message("Found ", length(all_files), " graphml files.")

file_meta <- data.frame(
  path       = all_files,
  fname      = basename(all_files),
  stringsAsFactors = FALSE
) |>
  mutate(
    sample     = as.integer(str_match(fname, "sample(\\d+)")[, 2]),
    generation = as.integer(str_match(fname, "gen(\\d+)")[,   2])
  ) |>
  arrange(sample, generation)

message("Samples: ",     length(unique(file_meta$sample)))
message("Generations: ", length(unique(file_meta$generation)))
message("Total graphs: ", nrow(file_meta))

graph_stats <- function(g) {
  
  n       <- igraph::vcount(g)
  deg_in  <- igraph::degree(g, mode = "in")
  deg_out <- igraph::degree(g, mode = "out")
  deg_all <- igraph::degree(g, mode = "all")
  
  comp       <- igraph::components(g, mode = "weak")
  giant_idx  <- which.max(comp$csize)
  giant_g    <- igraph::induced_subgraph(g, which(comp$membership == giant_idx))
  
  mean_dist <- tryCatch(
    igraph::mean_distance(giant_g, directed = TRUE, unconnected = TRUE),
    error = function(e) NA_real_
  )
  diam <- tryCatch(
    igraph::diameter(giant_g, directed = TRUE, unconnected = TRUE),
    error = function(e) NA_real_
  )
  
  g_undir        <- igraph::as.undirected(g, mode = "collapse")
  comm           <- igraph::cluster_louvain(g_undir)
  modularity_val <- igraph::modularity(comm)
  
  data.frame(
    n_nodes              = n,
    n_edges              = igraph::ecount(g),
    density              = igraph::edge_density(g),
    mean_degree          = mean(deg_all),
    mean_in_degree       = mean(deg_in),
    mean_out_degree      = mean(deg_out),
    sd_degree            = sd(deg_all),
    degree_cv            = sd(deg_all) / mean(deg_all),
    n_components         = comp$no,
    giant_component_frac = max(comp$csize) / n,
    mean_path_length     = mean_dist,
    diameter             = diam,
    transitivity_global  = igraph::transitivity(g, type = "global"),
    transitivity_avg     = igraph::transitivity(g, type = "average"),
    modularity           = modularity_val,
    n_communities        = length(comm),
    assortativity        = igraph::assortativity_degree(g, directed = TRUE),
    reciprocity          = igraph::reciprocity(g)
  )
}

# ── 3. Process all graphs ─────────────────────────────────────────────────────
total  <- nrow(file_meta)
result <- vector("list", total)

message("Processing ", total, " graphs — this may take a while...")
t0 <- proc.time()

for (i in seq_len(total)) {
  result[[i]] <- tryCatch({
    g    <- igraph::read_graph(file_meta$path[i], format = "graphml")
    stats <- graph_stats(g)
    stats$sample     <- file_meta$sample[i]
    stats$generation <- file_meta$generation[i]
    stats
  }, error = function(e) {
    message("  ERROR on ", file_meta$fname[i], ": ", conditionMessage(e))
    NULL
  })
  
  if (i %% 500 == 0 || i == total) {
    elapsed <- round((proc.time() - t0)["elapsed"])
    rate    <- round(i / elapsed, 1)
    eta     <- round((total - i) / rate / 60, 1)
    message(sprintf("  [%d/%d]  %.0fs elapsed  ~%.1f graphs/s  ETA %.1f min",
                    i, total, elapsed, rate, eta))
  }
}

graph_stats_df <- bind_rows(result) |>
  select(sample, generation, everything()) |>
  arrange(sample, generation)

message("Done. Rows: ", nrow(graph_stats_df),
        "  Columns: ", ncol(graph_stats_df))
stats_over_time <- graph_stats_df |>
  group_by(generation) |>
  summarise(across(where(is.numeric) & !matches("^sample$"),
                   mean, na.rm = TRUE),
            .groups = "drop")

# ── 2. Pivot to long format for faceting ─────────────────────────────────────
stats_long <- stats_over_time |>
  pivot_longer(cols = -generation,
               names_to  = "statistic",
               values_to = "value")

# ── 3. Clean up statistic labels for display ─────────────────────────────────
label_map <- c(
  n_nodes              = "Nodes",
  n_edges              = "Edges",
  density              = "Density",
  mean_degree          = "Mean degree (total)",
  mean_in_degree       = "Mean in-degree",
  mean_out_degree      = "Mean out-degree",
  sd_degree            = "SD degree",
  degree_cv            = "Degree CV",
  n_components         = "# Components",
  giant_component_frac = "Giant component fraction",
  mean_path_length     = "Mean path length",
  diameter             = "Diameter",
  transitivity_global  = "Global transitivity",
  transitivity_avg     = "Avg local transitivity",
  modularity           = "Modularity",
  n_communities        = "# Communities",
  assortativity        = "Assortativity",
  reciprocity          = "Reciprocity"
)

stats_long <- stats_long |>
  mutate(statistic = label_map[statistic])

# ── 4. Faceted plot — all statistics in one figure ───────────────────────────
p <- ggplot(stats_long, aes(x = generation, y = value)) +
  geom_line(linewidth = 0.7, color = "#377EB8") +
  facet_wrap(~ statistic, scales = "free_y", ncol = 3) +
  labs(
    title    = "Network statistics over generations (initial degree = 12)",
    subtitle = "Each panel averaged across all samples",
    x        = "Generation",
    y        = NULL
  ) +
  theme_minimal(base_size = 11) +
  theme(
    strip.text       = element_text(face = "bold", size = 9),
    panel.spacing    = unit(0.8, "lines"),
    axis.text        = element_text(size = 7)
  )

print(p)

# conduct a triad census analysis

# Standard names for igraph's triad_census() output (position order)
TRIAD_NAMES <- c(
  "003",   # 1:  no edges
  "012",   # 2:  one directed edge (A→B)
  "102",   # 3:  one mutual edge (A↔B)
  "021D",  # 4:  fork – two out from one node
  "021U",  # 5:  collector – two into one node
  "021C",  # 6:  directed path A→B→C
  "111D",  # 7:  mutual + outgoing
  "111U",  # 8:  mutual + incoming
  "030T",  # 9:  TRANSITIVE TRIPLE: A→B, A→C, B→C  ← key
  "030C",  # 10: CYCLIC TRIPLE:     A→B, B→C, C→A  ← key
  "201",   # 11: two mutual edges
  "120D",  # 12: two in + mutual
  "120U",  # 13: two out + mutual
  "120C",  # 14: directed path + mutual
  "210",   # 15: five edges
  "300"    # 16: complete mutual triad
)


# ── 1. Run census on all graphs ───────────────────────────────────────────────
total        <- nrow(file_meta)
census_rows  <- vector("list", total)

message("Running triad census on ", total, " graphs...")
t0 <- proc.time()

for (i in seq_len(total)) {
  census_rows[[i]] <- tryCatch({
    g      <- igraph::read_graph(file_meta$path[i], format = "graphml")
    counts <- igraph::triad_census(g)
    row    <- as.data.frame(t(counts))
    colnames(row) <- TRIAD_NAMES
    row$sample     <- file_meta$sample[i]
    row$generation <- file_meta$generation[i]
    row
  }, error = function(e) {
    message("  ERROR on ", file_meta$fname[i], ": ", conditionMessage(e))
    NULL
  })
  
  if (i %% 500 == 0 || i == total) {
    elapsed <- round((proc.time() - t0)["elapsed"])
    rate    <- if (elapsed > 0) round(i / elapsed, 1) else NA
    eta     <- if (!is.na(rate)) round((total - i) / rate / 60, 1) else NA
    message(sprintf("  [%d/%d]  %.0fs elapsed  ~%.1f graphs/s  ETA %.1f min",
                    i, total, elapsed, rate, eta))
  }
}

#Decrease over generations: one mutual edge (A↔B), mutual + outgoing, mutual + incoming, two mutual edges, two in + mutual, two out + mutual, directed path + mutual, complete mutual triad, five edges
#Increase over generations: fork – two out from one node, collector – two into one node, directed path A→B→C, TRANSITIVE TRIPLE: A→B, A→C, B→C, CYCLIC TRIPLE:     A→B, B→C, C→A

census_df <- bind_rows(census_rows)

# ── 2. Normalise to proportions of all triads ─────────────────────────────────
census_df <- census_df |>
  mutate(total_triads = rowSums(across(all_of(TRIAD_NAMES)))) |>
  mutate(across(all_of(TRIAD_NAMES), ~ . / total_triads)) |>
  select(-total_triads)

# ── 3. Average across samples per generation ──────────────────────────────────
census_mean <- census_df |>
  group_by(generation) |>
  summarise(across(all_of(TRIAD_NAMES), mean, na.rm = TRUE), .groups = "drop")

census_long <- census_mean |>
  pivot_longer(-generation, names_to = "triad", values_to = "proportion") |>
  mutate(triad = factor(triad, levels = TRIAD_NAMES))

# ── 4. Plot A: all 16 types faceted ───────────────────────────────────────────
p_all <- ggplot(census_long, aes(x = generation, y = proportion)) +
  geom_line(linewidth = 0.7, color = "#377EB8") +
  facet_wrap(~ triad, scales = "free_y", ncol = 4) +
  labs(
    title    = "Directed triad census over generations (initial degree = 12)",
    subtitle = "Proportion of all triads; averaged across all samples",
    x = "Generation", y = "Proportion"
  ) +
  theme_minimal(base_size = 10) +
  theme(strip.text = element_text(face = "bold"))

print(p_all)

# ── 5. Plot B: non-trivial types (exclude 003 and 012 which dominate) ─────────
nontrivial <- c("102", "021C", "021D", "021U",
                "030T", "030C", "111D", "111U",
                "201", "120C", "120D", "120U", "210", "300")

p_nontrivial <- census_long |>
  filter(triad %in% nontrivial) |>
  ggplot(aes(x = generation, y = proportion)) +
  geom_line(linewidth = 0.7, color = "#377EB8") +
  facet_wrap(~ triad, scales = "free_y", ncol = 4) +
  labs(
    title    = "Triad census — non-trivial types only",
    subtitle = "Excluding empty (003) and single-edge (012) triads",
    x = "Generation", y = "Proportion"
  ) +
  theme_minimal(base_size = 10) +
  theme(strip.text = element_text(face = "bold"))

print(p_nontrivial)

# ── 6. Plot C: the key diagnostic — 030T vs 030C vs 102 vs 021C ───────────────
p_key <- census_long |>
  filter(triad %in% c("030T", "030C", "102", "021C")) |>
  ggplot(aes(x = generation, y = proportion, color = triad, group = triad)) +
  geom_line(linewidth = 1.1) +
  scale_color_manual(
    values = c(
      "030T" = "#E41A1C",
      "030C" = "#377EB8",
      "102"  = "#4DAF4A",
      "021C" = "#FF7F00"
    ),
    labels = c(
      "030T" = "030T  transitive (A→B, A→C, B→C)",
      "030C" = "030C  cyclic (A→B, B→C, C→A)",
      "102"  = "102   reciprocal pair (A↔B)",
      "021C" = "021C  open directed path (A→B→C)"
    ),
    name = NULL
  ) +
  labs(
    title    = "Key triad types over generations",
    subtitle = "030T rising + 102 falling = directional transitive closure without reciprocity",
    x = "Generation", y = "Proportion of all triads"
  ) +
  theme_minimal(base_size = 12) +
  theme(legend.position = "bottom",
        legend.text = element_text(size = 9))

print(p_key)
#So, any triadic connectivity with reciprocity decreases over generations. Everything without reciprocity increases.
# Cyclic patterns increase less than transitive or open paths.  This strongly supports the premise that network evolution
# is favoring more distributed connectivity.  I think this kind of local triad analysis makes a lot of sense given the local
# rule for evolving network structure (e.g. each edge is randomly toggled between on/off).



# For directed graphs, "eigenvector analysis" has several distinct flavours:
#
#   eigen_centrality   – right eigenvector: high score = pointed to by other
#                        high-scoring nodes (authority/prestige)
#   hub_score          – HITS hub:       who points to many high-authority nodes
#   authority_score    – HITS authority: who is pointed to by many high-hub nodes
#   page_rank          – damped random-walk prestige (handles dangling nodes)
#   spectral_radius    – leading eigenvalue of adjacency matrix; governs how
#                        fast influence/contagion spreads through the network
#
# Per-graph summary statistics extracted:
#   centralization_eigen   – 0=perfectly equal, 1=pure star (right eigenvector)
#   gini_eigen             – inequality of eigenvector centrality distribution
#   gini_pagerank          – inequality of PageRank distribution
#   hub_auth_correlation   – are the same nodes both hubs and authorities?
#                            Low r → clear role differentiation (source vs. sink)
#   degree_eigen_corr      – does degree predict eigenvector centrality?
#                            Low r → hidden influencers independent of degree
#   spectral_radius        – leading eigenvalue of adjacency matrix
# ── Helper: Gini coefficient ──────────────────────────────────────────────────
gini <- function(x) {
  x <- sort(x[!is.na(x) & x >= 0])
  n <- length(x)
  if (n == 0 || sum(x) == 0) return(NA_real_)
  2 * sum(seq_len(n) * x) / (n * sum(x)) - (n + 1) / n
}

# ── Per-graph eigenvector statistics ─────────────────────────────────────────
eigen_stats <- function(g) {
  
  # Right eigenvector centrality (authority / prestige)
  ec  <- tryCatch(igraph::eigen_centrality(g, directed = TRUE, scale = TRUE),
                  error = function(e) NULL)
  
  # HITS hub and authority scores
  hits   <- tryCatch(igraph::hits_scores(g), error = function(e) NULL)
  hub    <- if (!is.null(hits)) hits$hub       else NULL
  auth   <- if (!is.null(hits)) hits$authority else NULL
  
  # PageRank
  pr <- tryCatch(igraph::page_rank(g, directed = TRUE)$vector,
                 error = function(e) NULL)
  
  # Degree (total) for correlation check
  deg <- igraph::degree(g, mode = "all")
  
  # Leading eigenvalue (spectral radius) of adjacency matrix
  # arpack gives the top k eigenvalues without full decomposition — fast
  spec_radius <- tryCatch({
    A <- igraph::as_adjacency_matrix(g, sparse = TRUE)
    Re(RSpectra::eigs(A, k = 1, which = "LM")$values[1])
  }, error = function(e) {
    # Fallback for small/disconnected graphs
    tryCatch({
      A <- as.matrix(igraph::as_adjacency_matrix(g))
      max(Re(base::eigen(A, only.values = TRUE)$values))
    }, error = function(e2) NA_real_)
  })
  
  data.frame(
    # Eigenvector centrality summaries
    centralization_eigen = if (!is.null(ec))
      igraph::centr_eigen(g, directed = TRUE)$centralization else NA_real_,
    gini_eigen           = if (!is.null(ec)) gini(ec$vector)  else NA_real_,
    mean_eigen           = if (!is.null(ec)) mean(ec$vector)  else NA_real_,
    sd_eigen             = if (!is.null(ec)) sd(ec$vector)    else NA_real_,
    
    # PageRank summaries
    gini_pagerank        = if (!is.null(pr)) gini(pr)         else NA_real_,
    sd_pagerank          = if (!is.null(pr)) sd(pr)           else NA_real_,
    
    # HITS role differentiation
    hub_auth_correlation = if (!is.null(hub) && !is.null(auth))
      cor(hub, auth, use = "complete.obs") else NA_real_,
    
    # Does degree predict eigenvector centrality?
    degree_eigen_corr    = if (!is.null(ec))
      cor(deg, ec$vector, use = "complete.obs") else NA_real_,
    
    # Does degree predict PageRank?
    degree_pr_corr       = if (!is.null(pr))
      cor(deg, pr, use = "complete.obs") else NA_real_,
    
    # Spectral radius
    spectral_radius      = spec_radius
  )
}

# ── Process all graphs ────────────────────────────────────────────────────────
# Install RSpectra if not present (used for fast leading eigenvalue)
if (!requireNamespace("RSpectra", quietly = TRUE)) {
  message("Installing RSpectra for fast spectral radius computation...")
  install.packages("RSpectra")
}
if (!requireNamespace("Matrix", quietly = TRUE)) install.packages("Matrix")

total      <- nrow(file_meta)
eig_rows   <- vector("list", total)

message("Computing eigenvector statistics for ", total, " graphs...")
t0 <- proc.time()

for (i in seq_len(total)) {
  eig_rows[[i]] <- tryCatch({
    g    <- igraph::read_graph(file_meta$path[i], format = "graphml")
    row  <- eigen_stats(g)
    row$sample     <- file_meta$sample[i]
    row$generation <- file_meta$generation[i]
    row
  }, error = function(e) {
    message("  ERROR on ", file_meta$fname[i], ": ", conditionMessage(e))
    NULL
  })
  
  if (i %% 500 == 0 || i == total) {
    elapsed <- round((proc.time() - t0)["elapsed"])
    rate    <- if (elapsed > 0) round(i / elapsed, 1) else NA
    eta     <- if (!is.na(rate)) round((total - i) / rate / 60, 1) else NA
    message(sprintf("  [%d/%d]  %.0fs elapsed  ~%.1f graphs/s  ETA %.1f min",
                    i, total, elapsed, rate, eta))
  }
}

eigen_df <- bind_rows(eig_rows) |>
  select(sample, generation, everything()) |>
  arrange(sample, generation)

message("Done. Rows: ", nrow(eigen_df))

# ── Average across samples per generation ─────────────────────────────────────
eigen_mean <- eigen_df |>
  group_by(generation) |>
  summarise(across(where(is.numeric) & !matches("^sample$"),
                   mean, na.rm = TRUE), .groups = "drop")

# ── Plot: all statistics faceted ─────────────────────────────────────────────
label_map <- c(
  centralization_eigen = "Eigen centralization\n(0=equal, 1=star)",
  gini_eigen           = "Gini — eigenvector centrality\n(inequality)",
  mean_eigen           = "Mean eigenvector centrality",
  sd_eigen             = "SD eigenvector centrality",
  gini_pagerank        = "Gini — PageRank\n(inequality)",
  sd_pagerank          = "SD PageRank",
  hub_auth_correlation = "Hub–authority correlation\n(low = clear role split)",
  degree_eigen_corr    = "Degree–eigenvector correlation\n(low = hidden influencers)",
  degree_pr_corr       = "Degree–PageRank correlation",
  spectral_radius      = "Spectral radius\n(leading eigenvalue)"
)

eigen_long <- eigen_mean |>
  pivot_longer(-generation, names_to = "statistic", values_to = "value") |>
  mutate(statistic = label_map[statistic])

p_all <- ggplot(eigen_long, aes(x = generation, y = value)) +
  geom_line(linewidth = 0.7, color = "#377EB8") +
  facet_wrap(~ statistic, scales = "free_y", ncol = 2) +
  labs(
    title    = "Eigenvector analysis over generations (initial degree = 12)",
    subtitle = "Each panel averaged across all samples",
    x = "Generation", y = NULL
  ) +
  theme_minimal(base_size = 10) +
  theme(strip.text    = element_text(face = "bold", size = 8),
        panel.spacing = unit(1, "lines"))

print(p_all)

# ── Focus plot: change just to include crucial stats
focus_stats <- c(
  "Hub–authority correlation\n(low = clear role split)",
  "Degree–eigenvector correlation\n(low = hidden influencers)",
  "Degree–PageRank correlation"
)

p_roles <- eigen_long |>
  filter(statistic %in% focus_stats) |>
  ggplot(aes(x = generation, y = value, color = statistic, group = statistic)) +
  geom_line(linewidth = 0.9) +
  scale_color_brewer(palette = "Set1", name = NULL) +
  labs(
    title    = "Role differentiation and influence inequality over generations",
    subtitle = "Averaged across all samples",
    x = "Generation", y = "Value"
  ) +
  theme_minimal(base_size = 12) +
  theme(legend.position = "bottom",
        legend.text = element_text(size = 8)) +
  guides(color = guide_legend(ncol = 2))
print(p_roles)
#Main results of interest:
# steadily dropped correlated between eigenvector centrality and degree over round: some nodes
# become disproportionately influential despite modest degree — "hidden influencers"
# whose position in the network matters more than their raw connection count.
# decreasing correlation between page-rank and degree over generations - Some agents become disproportionately influential despite modest degree,
# because they sit at strategic positions in the information flow — downstream from good solution-finders
# Also a reliably decreasing hub-authority correlation.  high values mean the same nodes both send and receive
# influence (reciprocal influencers). If this falls, the network is evolving distinct source and sink roles
# which is consistent with falling reciprocity.
# So, all 3 correlation decreases suggest a decentralization of different notions of importance


#############################
#Spectral analysis of graphs
K <- 20   # number of top singular values to retain per graph

# ── Helper: spectral entropy and effective rank ───────────────────────────────
spectral_entropy <- function(sigmas) {
  e  <- sigmas^2
  e  <- e / sum(e)
  e  <- e[e > 0]
  -sum(e * log(e))
}

effective_rank <- function(sigmas) {
  exp(spectral_entropy(sigmas))
}

# ── Per-graph SVD statistics ──────────────────────────────────────────────────
svd_stats <- function(g, k = K) {
  A <- igraph::as_adjacency_matrix(g, sparse = TRUE)
  n <- nrow(A)
  k_use <- min(k, n - 1)   # can't request more singular values than n-1
  
  sv <- tryCatch(
    RSpectra::svds(A, k = k_use)$d,
    error = function(e) {
      # Fallback to full SVD for very small graphs
      tryCatch(svd(as.matrix(A))$d[seq_len(k_use)],
               error = function(e2) rep(NA_real_, k_use))
    }
  )
  
  total_energy <- sum(sv^2, na.rm = TRUE)
  
  data.frame(
    sigma_1          = sv[1],
    sigma_2          = if (length(sv) >= 2) sv[2] else NA_real_,
    spectral_gap     = if (length(sv) >= 2) sv[1] - sv[2] else NA_real_,
    pct_energy_top1  = sv[1]^2 / total_energy * 100,
    pct_energy_top3  = sum(sv[1:min(3,  length(sv))]^2) / total_energy * 100,
    pct_energy_top5  = sum(sv[1:min(5,  length(sv))]^2) / total_energy * 100,
    pct_energy_top10 = sum(sv[1:min(10, length(sv))]^2) / total_energy * 100,
    spectral_entropy = spectral_entropy(sv),
    effective_rank   = effective_rank(sv),
    # Store top-k singular values for the profile plot (as separate columns)
    as.data.frame(t(setNames(sv, paste0("sv_", seq_along(sv)))))
  )
}

# ── Process all graphs ────────────────────────────────────────────────────────
total    <- nrow(file_meta)
svd_rows <- vector("list", total)

message("Computing SVD spectral statistics for ", total, " graphs (k = ", K, ")...")
t0 <- proc.time()

for (i in seq_len(total)) {
  svd_rows[[i]] <- tryCatch({
    g   <- igraph::read_graph(file_meta$path[i], format = "graphml")
    row <- svd_stats(g)
    row$sample     <- file_meta$sample[i]
    row$generation <- file_meta$generation[i]
    row
  }, error = function(e) {
    message("  ERROR on ", file_meta$fname[i], ": ", conditionMessage(e))
    NULL
  })
  
  if (i %% 500 == 0 || i == total) {
    elapsed <- round((proc.time() - t0)["elapsed"])
    rate    <- if (elapsed > 0) round(i / elapsed, 1) else NA
    eta     <- if (!is.na(rate)) round((total - i) / rate / 60, 1) else NA
    message(sprintf("  [%d/%d]  %.0fs elapsed  ~%.1f graphs/s  ETA %.1f min",
                    i, total, elapsed, rate, eta))
  }
}

svd_df <- bind_rows(svd_rows) |>
  select(sample, generation, everything()) |>
  arrange(sample, generation)

message("Done. Rows: ", nrow(svd_df))

# ── Average summary stats across samples per generation ───────────────────────
summary_cols <- c("sigma_1", "spectral_gap",
                  "pct_energy_top1", "pct_energy_top3",
                  "pct_energy_top5", "pct_energy_top10",
                  "spectral_entropy", "effective_rank")

svd_mean <- svd_df |>
  group_by(generation) |>
  summarise(across(all_of(summary_cols), mean, na.rm = TRUE), .groups = "drop")

# ── Plot 1: Summary statistics over generations ───────────────────────────────
label_map <- c(
  sigma_1          = "Leading singular value (σ₁)",
  spectral_gap     = "Spectral gap (σ₁ − σ₂)",
  pct_energy_top1  = "% energy in top 1 mode",
  pct_energy_top3  = "% energy in top 3 modes",
  pct_energy_top5  = "% energy in top 5 modes",
  pct_energy_top10 = "% energy in top 10 modes",
  spectral_entropy = "Spectral entropy\n(high = spread across modes)",
  effective_rank   = "Effective rank\n(≈ active flow modes)"
)

svd_long <- svd_mean |>
  pivot_longer(-generation, names_to = "statistic", values_to = "value") |>
  mutate(statistic = label_map[statistic])

p_summary <- ggplot(svd_long, aes(x = generation, y = value)) +
  geom_line(linewidth = 0.7, color = "#377EB8") +
  facet_wrap(~ statistic, scales = "free_y", ncol = 2) +
  labs(
    title    = "SVD spectral analysis over generations (initial degree = 12)",
    subtitle = "Averaged across all samples",
    x = "Generation", y = NULL
  ) +
  theme_minimal(base_size = 10) +
  theme(strip.text    = element_text(face = "bold", size = 8),
        panel.spacing = unit(1, "lines"))

print(p_summary)

# ── Plot 2: Spectral profile (scree plot) at early / mid / late generations ───
# Shows the average singular value curve at three time points
gen_vals  <- sort(unique(svd_df$generation))
gen_early <- gen_vals[1]
gen_mid   <- gen_vals[round(length(gen_vals) / 2)]
gen_late  <- gen_vals[length(gen_vals)]

sv_cols <- paste0("sv_", seq_len(K))

profile_df <- svd_df |>
  filter(generation %in% c(gen_early, gen_mid, gen_late)) |>
  select(generation, sample, all_of(intersect(sv_cols, names(svd_df)))) |>
  group_by(generation) |>
  summarise(across(everything() & !matches("sample"), mean, na.rm = TRUE),
            .groups = "drop") |>
  pivot_longer(-generation, names_to = "rank", values_to = "singular_value") |>
  mutate(rank       = as.integer(str_extract(rank, "\\d+")),
         generation = factor(generation,
                             labels = c(paste("Early (gen", gen_early, ")"),
                                        paste("Mid (gen",   gen_mid,   ")"),
                                        paste("Late (gen",  gen_late,  ")"))))

p_profile <- ggplot(profile_df,
                    aes(x = rank, y = singular_value,
                        color = generation, group = generation)) +
  geom_line(linewidth = 1.0) +
  geom_point(size = 1.5) +
  scale_color_manual(values = c("#4DAF4A", "#FF7F00", "#E41A1C"), name = NULL) +
  scale_x_continuous(breaks = seq(1, K, by = 2)) +
  labs(
    title    = "Spectral profile: singular value curve at three generations",
    subtitle = "Average across all samples — steeper drop-off = more concentrated flow",
    x = "Singular value rank", y = "Singular value"
  ) +
  theme_minimal(base_size = 12) +
  theme(legend.position = "bottom")
print(p_profile)
# After around Generation 1000 when degree peaks, overall spectral entropy is increasing, meaning structure
#is spread out broadly across scales.  Less energy is also concentrated in top 1, 3, 5, and 10 modes
# effective rank is increasing with generation, and spectral gap is decreasing
#One could also stress the two-phase structure, with these trends reversing up to generation 1000
#I'm ignoring this because I'm worried about it just being artifact of (randomly) increasing degree
# It seems very possible that the spectral analyses results are just artifacts of average degree.  They tend
#to exactly track average degree.  This could be explored by mutating networks so that average degree
#always stays the same - picking one edge and moving it between a new pair of currently unconnected agents.
# the downside of this is that it places a big constraint on the possible networks that can evolve.

