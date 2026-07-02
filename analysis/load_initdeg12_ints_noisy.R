# load_initdeg12_ints_noisy.R

library(stringr)
library(dplyr)
library(ggplot2)

# data location: ../data/initdeg12_ints_noisy
# naming scheme: initdeg12_ints_nr0_sample0
# where nr is 0, 1, 3, 5, 10
# and sample ranges from 0 to 199

# load the data

data_dir <- "../data/initdeg12_ints_noisy"

csv_files <- list.files(
	path = data_dir,
	pattern = "^initdeg12_ints_nr\\d+_sample\\d+\\.csv$",
	full.names = TRUE
)

read_one <- function(path) {
	fname <- basename(path)
	matches <- str_match(fname, "initdeg12_ints_nr(\\d+)_sample(\\d+)")

	df <- read.csv(path)
	df$noise_radius <- as.integer(matches[, 2])
	df$sample <- as.integer(matches[, 3]); 
	return(df)
}


data <- bind_rows(lapply(csv_files, read_one))

# summarize utilities over generation

utility_summary <- data |>
	group_by(noise_radius, generation) |>
	summarize(mean_avg_utility = mean(avg_utility), .groups = "drop")

ggplot(utility_summary,
	aes(x = generation, y = mean_avg_utility,
		color = factor(noise_radius), group = factor(noise_radius))) +
	geom_line()

# summarize degrees over generation
degree_summary <- data |>
	group_by(noise_radius, generation) |>
	summarize(mean_avg_degree = mean(avg_degrees), .groups = "drop")

ggplot(degree_summary,
	aes(x = generation, y = mean_avg_degree,
			color = factor(noise_radius), group = factor(noise_radius))) +
	geom_line()
