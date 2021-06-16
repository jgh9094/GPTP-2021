#ifndef DIA_CONFIG_H
#define DIA_CONFIG_H

#include "config/config.h"

EMP_BUILD_CONFIG(DiaConfig,
  GROUP(WORLD, "How should the world be setup?"),
  VALUE(POP_SIZE,     size_t,      512,    "Population size."),
  VALUE(MAX_GENS,     size_t,    40001,    "Maximum number of generations."),
  VALUE(SEED,           int,         0,    "Random number seed."),

  GROUP(DIAGNOSTICS, "How are the diagnostics setup?"),
  VALUE(TARGET,              double,     100.0,      "Target that traits are trying to optimize towards."),
  VALUE(ACCURACY,            double,      0.99,      "Accuracy percentage needed to be considered an optimal trait"),
  VALUE(CREDIT,              double,      0.00,      "Maximum credit a solution can get on an objective if applicable"),
  VALUE(OBJECTIVE_CNT,       size_t,       100,      "Number of traits an organism has"),
  VALUE(SELECTION,           size_t,         0,      "Which selection are we doing? \n0: (μ,λ)\n1: Tournament\n2: Fitness Sharing\n3: Novelty Search\n4: Espilon Lexicase\n5: Down Sampled Lexicase \n6: Cohort Lexicase \n7: Novelty Lexicase"),
  VALUE(DIAGNOSTIC,          size_t,         0,      "Which diagnostic are we doing? \n0: Exploitation\n1: Structured Exploitation\n2: Strong Ecology \n3: Exploration \n4: Weak Ecology"),

  GROUP(MUTATIONS, "Mutation rates for organisms."),
  VALUE(MUTATE_PER,       double,     0.007,        "Probability of instructions being mutated"),
  VALUE(MEAN,             double,     0.0,          "Mean of Gaussian Distribution for mutations"),
  VALUE(STD,              double,     1.0,          "Standard Deviation of Gaussian Distribution for mutations"),

  GROUP(PARAMETERS, "Parameter estimations all selection schemes."),
  VALUE(MU,               size_t,           512,       "Parameter estiamte for μ."),
  VALUE(TOUR_SIZE,        size_t,           512,       "Parameter estiamte for tournament size."),
  VALUE(FIT_SIGMA,        double,           0.0,       "Parameter estiamte for proportion of similarity threshold sigma (based on maximum distance between solutions)."),
  VALUE(FIT_ALPHA,        double,           1.0,       "Parameter estiamte for penalty function shape alpha."),
  VALUE(PNORM_EXP,        double,           2.0,       "Paramter we are using for the p-norm function."),
  VALUE(NOVEL_K,          size_t,           256,       "Parameter estiamte k-nearest neighbors."),
  VALUE(LEX_EPS,          double,           1.0,       "Parameter estimate for lexicase epsilon."),
  VALUE(DSLEX_PROP,       double,           1.0,       "Parameter for down sampled proportion"),
  VALUE(COH_LEX_PROP,     double,           1.0,       "Parameter for cohort proportions"),

  GROUP(SYSTEMATICS, "Output rates for OpenWorld"),
  VALUE(SNAP_INTERVAL,             size_t,             1000,          "How many updates between prints?"),
  VALUE(DATA_INTERVAL,             size_t,                10,          "How many updates between writing data to file?"),
  VALUE(PRINT_INTERVAL,            size_t,                 1,          "How many updates between prints?"),
  VALUE(OUTPUT_DIR,           std::string,              "./",          "What directory are we dumping all this data")
)

#endif