# Spread of Beliefs in (Partly) Modularized Communities

system dependencies:
- igraph
- 

*2025-05-29:*
- cmake debug build: `cmake -DCMAKE_BUILD_TYPE=Debug ..` from build directory
- the issue is that s.actor_util_objs array becomes a null pointer by the end of update_util_objs
    - happens because prev_util_objs is a nullptr
    - issue was that was not setting .actor_util_objs of new_sbsc in create_sbsc.
- TODO: figure out why avg utility doesn't change
    - avg utility being recorded properly
    - if so, that opinion exchange is working
    - softmax_decide seems to be producing a negative index?
    - happening because util_obj_scores_len = 0 for some reason
- simplify collect_statistics to only take one arg? b/c sbsc arg contains stats_info?
- new issue after passing sbsc_t pointers around: s->prev_connection_graph uninitialized in default_collect_statistics
    - issue was passing sbsc_t** to collect_statistics instead of sbsc_t*
- questions for going forward:
    - should reset to starting opinions when graph updates?
        - right now doesn't. then when graph not updated, it accumulates more rounds of opinion exchange
    - should actors consider their own opinions?

*2025-05-28:*
- TODO: figure out memory issue
    - get debug build working, then use debugger and valgrind
    - try move to bazel.

*2025-04-24:*
- add code to update graph connectivity, not just opinions
- add code in sbsc to run simulation for some number of rounds
- add code to log graph statistics. maybe make generic
- figure out if sbsc_t should be passed as pointer or raw
- TODO: function to print statistics
- TODO: figure out why igraph_copy in initializing sbsc results in memory issue

*2025-02-14:*
- run x rounds of opinion exchange
- try randomly tweaking network (weighted?). probability of new based on weighted (exponentially) difference
    - tweak whole network at a time rather than individuals for now. might result in self-optimization against total optimization
- repeat.
- mcmc, gibb sampling, optimization with particle filters (weighted updating)
 - ~50 rounds of opinion exchange, * ~1000 iterations of network evolution, then write out final network config. then repeat with other networks or parameters
- keep intermediate graphs? or just average utility?
- modularize statistics-keeping stuff (avg utility, average degree, communities in the graph, other graph statistics stuff, spinglass?, reciprocity)
- keep intermediate statistics, but not neccesarily intermediate graphs
- 50 rounds opinion exchange, 1000 iterations of network evolution, 
- randomness of original beliefs might overshadow the difference of tweaking connections
- run the 50 rounds of opinion exchagne with different starting opinions (10 or so). network connection evolution is the thing that matters.
- beyond 100 agent probably to much, reccomend under 100, maybe 50.
- number of opinions: ~10.
- for problems with "unheld" opinions, need some weight for "unheld" opinions so that they could be chosen.
- probability of two nodes being connected, start at 50%.
- probably start with evidence integration strategy as 0 or 1 (just look at which one did better in original paper)

- reccomended to apply for goldstone scholarship. maximum is 1000.

*2025-01-19:*
- things to model:
    - agents: modeled by vertices in the network
    - influences modeled by directed edges in the network
    - solutions: modeled by an arbitrary structure
    - utilities: modeled by some function on that structure
- important implementation details
    - desicion-making incorporates evidence according to details in paper
    - descision-making can switch between "averaging" and "summing" as detailed in paper.
    - desicion-making is probabilistic according to details in paper (parameterized softmax)
    - 
- hyperparameters:
    - number of agents N
    - number of solutions N_o
    - choice determinism (paramterizes softmax) gamma
    - evidence integration strategy (average vs sum) g
    - probability of forming a connection to another vertex P
    - number of rounds to simulate r
- notes for initial versions
    - no "communities", instead just connect to anyone randomly
    - store graph of previous iteration, so vertex can restore connections of prev. evolutionary.
    - don't take into account current decision when making new ones?
    - r version of code: https://github.com/rgoldsto/SBSC
- other notes
    - igraph vertices have ids from 0 to n-1. so just use arrays to store utility objects
    - each stage should go: update utility object, then update graph connection
        - if switched last time and new is better, then try switching again
        - if switched last time and old is better, then reset to prev
        - if reset back last time, try switching.
    - above assumes switch every time. maybe only switch some of the time?
        - vary current connections instead of switching completely?
        - look into what R implementation does before implementing.

*2025-01-08:*
- questions
	- what exactly is the delta function?
	- how should the graph itself evolve? randomly?
	- how large should the model be?
- new networks created by random permutation
- temperature (softmax descision rule/simulated annealing) based for choosing network?
- compare random to previous and probabalistically choose better
- consider own solution in softmax, but it's random so might result in other solution
- low baseline utility
- start with unstructured, single "utility" thing, but structure to generalize to other potential problems
- nk landscapes stuart kaufman as eventual goal
	- n is # bits in solution/# nodes
	- k is degree
	- solutions represented as networks
	- each bit has utility for 0 and utility for 1
	- utility also has interactions/combinations
	- K is how much interaction each node has with others for interaction
- for combining solutions can be crossover + mutation, or just one of them
- stochastic block model
	- parameterized by between-block clumpiness and and within-block clumpiness
- connections are one-directional
- probably use a graph library for C, probably igraph
- look into using supercomputer for running the actual tests
- what do the networks look like at the end
- randy bier has relevant stuff?

