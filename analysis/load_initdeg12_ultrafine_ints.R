# load_initdeg12_ultrafine_ints.R

library(ggplot2)

# load csv
data <- read.csv("../data/initdeg12_ints_v2_ultrafine/initdeg12_ints_ultrafine.csv")

# plot
plot <- ggplot(data, aes(generation)) +
	geom_line(aes(y = prev_avg_utility, color = "prev_avg_utility")) +
	geom_line(aes(y = avg_utility, color = "avg_utility")) +
	geom_line(aes(y = avg_degree, color = "avg_degree"))

print(plot)

# check that avg_utility > prev_avg_utility always, and count when is worse than previous-generation utility

utility_gets_worse_valid <- 0
worse_utility_chosen <- 0

print("checking that average utility is always equal or better to previous")
for (row in 2:nrow(data)) {
	if (data[row, "avg_utility"] < data[row - 1, "avg_utility"]) {
		if (data[row, "prev_avg_utility"] < data[row, "avg_utility"]) {
			utility_gets_worse_valid <- utility_gets_worse_valid + 1
		}
	}

	if (data[row, "prev_avg_utility"] > data[row, "avg_utility"]) {
		print(sprintf("worse utility chosen for generation: %d", row))
		worse_utility_chosen <- worse_utility_chosen + 1
	}
}

print(sprintf("utility_gets_worse_valid: %d", utility_gets_worse_valid))
print(sprintf("worse_utility_chosen: %d", worse_utility_chosen))
