# load_main_ints.R
# Reads all main_ints CSV files into a single tidy data frame.
#
# Each CSV file is named:
#   main_ints_initdegpercent{D}_sample{S}.csv
# and contains columns:
#   generation, avg_utility, avg_degrees, reciprocity,
#   spinglass_modularity, cluster_coeff
#
# The resulting data frame adds two columns parsed from the filename:
#   init_deg_pct  – initial edge density (10, 20, …, 100)
#   sample        – replication index (0–29)

library(dplyr)   # for bind_rows, mutate
library(stringr) # for str_match
library(ggplot2)
library(tidyr)
library(igraph)

# ── 1. Point this at your data folder ────────────────────────────────────────
data_dir <- "../data/main_ints"

# ── 2. Find all CSV files ─────────────────────────────────────────────────────
csv_files <- list.files(
  path       = data_dir,
  pattern    = "^main_ints_initdegpercent\\d+_sample\\d+\\.csv$",
  full.names = TRUE
)

message("Found ", length(csv_files), " CSV files.")

# ── 3. Read and label each file ───────────────────────────────────────────────
read_one <- function(path) {
  # Extract initdegpercent and sample from the filename
  fname   <- basename(path)
  matches <- str_match(fname, "initdegpercent(\\d+)_sample(\\d+)")

  df <- read.csv(path)
  df$init_deg_pct <- as.integer(matches[, 2])
  df$sample       <- as.integer(matches[, 3])
  df
}

main_ints <- bind_rows(lapply(csv_files, read_one))

# ── 4. Tidy up column order ───────────────────────────────────────────────────
main_ints <- main_ints[, c("init_deg_pct", "sample", "generation",
                            "avg_utility", "avg_degrees", "reciprocity",
                            "spinglass_modularity", "cluster_coeff")]

# ── 5. Quick sanity check ─────────────────────────────────────────────────────
message("Rows: ", nrow(main_ints))
message("init_deg_pct levels: ", paste(sort(unique(main_ints$init_deg_pct)), collapse = ", "))
message("Samples per degree: ", length(unique(main_ints$sample)))
message("Generation range: ", min(main_ints$generation), " – ", max(main_ints$generation))
print(head(main_ints))

# main_ints is now ready for analysis.

# Summarise: mean avg_degrees per generation × init_deg_pct ──────────────
deg_summary <- main_ints |>
  group_by(init_deg_pct, generation) |>
  summarise(mean_avg_degrees = mean(avg_degrees, na.rm = TRUE), .groups = "drop")
p <- ggplot(deg_summary,
            aes(x = generation,
                y = mean_avg_degrees,
                color = factor(init_deg_pct),
                group = factor(init_deg_pct))) +
  geom_line(linewidth = 0.8) +
  scale_color_brewer(
    palette = "RdYlBu",
    name    = "Initial degree\ndensity (%)"
  ) +
  labs(
    title = "Evolution of Mean Network Degree",
    subtitle = "Averaged across 30 replications per condition",
    x = "Generation",
    y = "Mean average degree"
  ) +
  theme_minimal(base_size = 13) +
  theme(legend.position = "right")
print(p)

#  Summarise: mean avg_utility per generation × init_deg_pct ──────────────
utility_summary <- main_ints |>
  group_by(init_deg_pct, generation) |>
  summarise(mean_avg_utility = mean(avg_utility, na.rm = TRUE), .groups = "drop")
p <- ggplot(utility_summary,
            aes(x = generation,
                y = mean_avg_utility,
                color = factor(init_deg_pct),
                group = factor(init_deg_pct))) +
  geom_line(linewidth = 0.8) +
  scale_color_brewer(
    palette = "RdYlBu",
    name    = "Initial degree\ndensity (%)"
  ) +
  labs(
    title    = "Evolution of Mean Network Utility",
    subtitle = "Averaged across 30 replications per condition",
    x        = "Generation",
    y        = "Mean average utility"
  ) +
  theme_minimal(base_size = 13) +
  theme(legend.position = "right")
print(p)

#  Summarise 3 network statistics
measures_summary <- main_ints |>
  group_by(generation) |>
  summarise(
    Reciprocity           = mean(reciprocity,          na.rm = TRUE),
    `Clustering coefficient` = mean(cluster_coeff,    na.rm = TRUE),
    `Spinglass modularity`   = mean(spinglass_modularity, na.rm = TRUE),
    .groups = "drop"
  ) |>
  pivot_longer(
    cols      = -generation,
    names_to  = "measure",
    values_to = "value"
  )
p <- ggplot(measures_summary,
            aes(x = generation, y = value,
                color = measure, group = measure)) +
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
    title    = "Evolution of Network Structure Measures",
    subtitle = "Averaged across all initial degree conditions and replications",
    x        = "Generation",
    y        = "Mean value"
  ) +
  theme_minimal(base_size = 13) +
  theme(legend.position = "right")
print(p)

#Show some of the graphs
# ── Build file paths for samples 0–9, gen 3000 ────────────────────────────
files <- file.path(
  data_dir,
  paste0("main_ints_initdegpercent10_sample", 0:9, "_gen3000.graphml")
)
for (i in seq_along(files)) {
  g <- read_graph(files[i], format = "graphml")
  # Use a consistent layout seed so graphs are comparable across samples
  set.seed(42)
  lay <- layout_with_fr(g)
  plot(
    g,
    layout          = lay,
    vertex.size     = 4,
    vertex.color    = "#377EB8",
    vertex.frame.color = NA,
    vertex.label    = NA,
    edge.arrow.size = 0.1,
    edge.color      = adjustcolor("gray30", alpha.f = 0.5),
    main            = paste0("Sample ", i - 1)
  )
}
title(
  main  = "Final networks (gen 3000) — initial degree 10%",
  outer = TRUE,
  line  = -1,
  cex.main = 1.2
)
