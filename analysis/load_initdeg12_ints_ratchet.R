
library(ggplot2)

data <- read.csv("../data/initdeg12_ints_v2_ratchet/initdeg12_ints_v2_ratchet.csv")

data$improvement_delta = data$gen5000_avg_utility - data$gen1000_avg_utility

message("mean improvement delta: ", mean(data$improvement_delta))
message("mean gen1000 utility: ", mean(data$gen1000_avg_utility))
message("mean gen5000 utility: ", mean(data$gen5000_avg_utility))
				
# ggplot(data, aes(x = improvement_delta, y = 0)) + geom_point()

