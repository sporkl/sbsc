
# P(low + noise > high + noise, where noise in range [-noise_radius, noise_radius]
# apparently this math is wrong, fix. rest of file should be good now.
prob_low_gt_high <- function(low, high, noise_radius) {
	d <- high - low
	if (d > 2 * noise_radius) { return(0.0) }
	if (d < -2 * noise_radius) { return(1.0) }
	return(((2 * noise_radius) - d) * ((2 * noise_radius) - d + 1) / (2 * (((2 * noise_radius) + 1) ** 2)))
}

# P(low + noise > high _ noise), where low <= high, and both in range [1, n]
prob_low_gt_high_range <- function(n, noise_radius) {
	prob_sum <- 0
	for (low in seq(1, n)) {
		for (high in seq(low, n)) {
			prob_sum = prob_sum + prob_low_gt_high(low, high, noise_radius)
		}
	}
	return(prob_sum / (((n ** 2) + n) / 2))
}


prob_data <- data.frame(noise_radius = integer(), p_low_gt_high = double())
for (nr in seq(0, 50, by=5)) {
	prob_data <- rbind(prob_data, data.frame(noise_radius = nr, p_low_gt_high = prob_low_gt_high_range(50, nr)))
}

print(prob_data)
