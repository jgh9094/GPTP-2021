/// World that will manage solutions during the evolutionary run

#ifndef DIA_WORLD_H
#define DIA_WORLD_H

///< standard headers
#include <functional>
#include <map>
#include <set>

///< empirical headers
#include "Evolve/World.h"
#include "tools/Random.h"
#include "tools/random_utils.h"

///< experiment headers
#include "config.h"
#include "org.h"
#include "problem.h"
#include "selection.h"


template <typename PHEN_TYPE>
struct pheno_info
{
  /// Track information related to the mutational landscape
  /// Maps a string representing a type of mutation to a count representing
  /// the number of that type of mutation that occured to bring about this taxon.
  using phen_t = PHEN_TYPE;
  using has_phen_t = std::true_type;
  using has_mutations_t = std::false_type;
  using has_fitness_t = std::true_type;
  // using has_phenotype_t = true;

  emp::DataNode<double, emp::data::Current, emp::data::Range> fitness; /// This taxon's fitness (for assessing deleterious mutational steps)
  PHEN_TYPE phenotype; /// This taxon's phenotype (for assessing phenotypic change)

  const PHEN_TYPE & GetPhenotype() const {
    return phenotype;
  }

  const double GetFitness() const {
    return fitness.GetMean();
  }

  void RecordFitness(double fit) {
    fitness.Add(fit);
  }

  void RecordPhenotype(PHEN_TYPE phen) {
    phenotype = phen;
  }
};

class DiagWorld : public emp::World<Org>
{
  // object types for consistency between working class
  public:
    ///< Org related

    // solution genome + diagnotic problem types
    using genome_t = emp::vector<double>;
    // score vector for a solution
    using score_t = emp::vector<double>;
    // boolean optimal vector per objective
    using optimal_t = emp::vector<bool>;
    // target vector type
    using target_t = emp::vector<double>;

    ///< selection related types

    // vector of position ids
    using ids_t = emp::vector<size_t>;
    // matrix of population score vectors
    using fmatrix_t = emp::vector<score_t>;
    // matrix of population genomes
    using gmatrix_t = emp::vector<genome_t>;
    // map holding population id groupings by fitness (keys in decending order)
    using fitgp_t = std::map<double, ids_t, std::greater<double>>;
    // vector of double vectors for K neighborhoods
    using neigh_t = emp::vector<score_t>;
    // vector of vector position ids that represent cohort assignment
    using cohort_t = emp::vector<ids_t>;

    ///< world related types

    // evaluation function type
    using eval_t = std::function<double(Org &)>;
    // selection function type
    using sele_t = std::function<ids_t()>;

    ///< data tracking stuff (ask about)
    using nodef_t = emp::Ptr<emp::DataMonitor<double>>;
    using nodeo_t = emp::Ptr<emp::DataMonitor<size_t>>;
    using como_t = std::map<size_t, ids_t>;

    ///< systematics tracking types
    using systematics_t = emp::Systematics<Org, Org::genome_t, pheno_info<typename Org::score_t>>;
    using taxon_t = typename systematics_t::taxon_t;

    using config_t = DiaConfig;


  public:

    DiagWorld(DiaConfig & _config) : config(_config), data_file(_config.OUTPUT_DIR() + "data.csv")
    {
      // set random pointer seed
      random_ptr = emp::NewPtr<emp::Random>(config.SEED());

      // initialize the world
      Initialize();
    }

    ~DiagWorld()
    {
      selection.Delete();
      diagnostic.Delete();
      pop_fit.Delete();
      pop_opti.Delete();
      pnt_fit.Delete();
      pnt_opti.Delete();
    }

    ///< functions called to setup the world

    // call all functions to initiallize the world
    void Initialize();

    // set OnUpdate function from World.h
    void SetOnUpdate();

    // set mutation operator from World.h
    void SetMutation();

    // set selction scheme
    void SetSelection();

    // set what to do when offspring is ready to go
    void SetOnOffspringReady();

    // set evaluation function
    void SetEvaluation();

    // set data tracking with data nodes
    void SetDataTracking();

    // populate the world with initial solutions
    void PopulateWorld();


    ///< principle steps during an evolutionary run

    // reset all data step
    void ResetData();

    // evaluation step
    void EvaluationStep();

    // selction step
    void SelectionStep();

    // reprodutive step
    void ReproductionStep();

    // record data step
    void RecordData();


    ///< selection scheme implementations

    void MuLambda();

    void Tournament();

    void FitnessSharing();

    void NoveltySearch();

    void EpsilonLexicase();

    void DownSampledLexicase();

    void CohortLexicase();

    void NoveltyLexicase();


    ///< evaluation function implementations

    void Exploitation();

    void StructuredExploitation();

    void StrongEcology();

    void Exploration();

    void WeakEcology();


    ///< data tracking

    size_t UniqueObjective();

    size_t FindElite();

    size_t FindCommon();

    size_t FindOptimized();

    size_t FindUniqueStart();

    void SnapshotPhylogony();

    void SnapshotConfig(const config_t & config);

    ///< helper functions

    // create a matrix of popultion score vectors
    fmatrix_t PopFitMat();

    // create matrix of population genomes
    gmatrix_t PopGenomes();


  private:
    // experiment configurations
    DiaConfig & config;
    double SIGMA = -1.0;


    // target vector
    target_t target;
    // vector holding population aggregate scores (by position id)
    score_t fit_vec;
    // vector holding parent solutions selected by selection scheme
    ids_t parent_vec;


    // evaluation lambda we set
    eval_t evaluate;
    // selection lambda we set
    sele_t select;


    // select.h var
    emp::Ptr<Selection> selection;
    // problem.h var
    emp::Ptr<Diagnostic> diagnostic;

    ///< data file & node related variables

    // file we are working with
    emp::DataFile data_file;
    // systematics tracking
    // emp::Ptr<systematics_t> sys_ptr;
    // node to track population fitnesses
    nodef_t pop_fit;
    // node to track population opitmized count
    nodeo_t pop_opti;
    // node to track parent fitnesses
    nodef_t pnt_fit;
    // node to track parent optimized count
    nodeo_t pnt_opti;

    ///< data we are tracking during an evolutionary run

    // elite solution position
    size_t elite_pos;
    // common solution position
    size_t comm_pos;
    // optimal solution position
    size_t opti_pos;
    // common solution dictionary
    como_t common;
};

///< functions called to setup the world

void DiagWorld::Initialize()
{
  std::cerr << "==========================================" << std::endl;
  std::cerr << "BEGINNING INITIAL SETUP" << std::endl;
  std::cerr << "==========================================" << std::endl;

  // reset the world upon start
  Reset();
  // set world to well mixed so we don't over populate
  SetPopStruct_Mixed(true);


  // stuff we need to initialize for the experiment
  SetEvaluation();
  SetMutation();
  SetOnUpdate();
  SetDataTracking();
  SetSelection();
  SetOnOffspringReady();
  PopulateWorld();

  SnapshotConfig(config);

  std::cerr << "==========================================" << std::endl;
  std::cerr << "FINISHED INITIAL SETUP" << std::endl;
  std::cerr << "==========================================" << std::endl;
}

void DiagWorld::SetOnUpdate()
{
  std::cerr << "------------------------------------------------" << std::endl;
  std::cerr << "Setting OnUpdate function..." << std::endl;

  // set up the evolutionary algorithm
  OnUpdate([this](size_t gen)
  {
    // step 0: reset all data collection variables
    ResetData();

    // step 1: evaluate all solutions on diagnostic
    EvaluationStep();

    // take a snapshot if nessecaryn (ask if appropiate place to take snapshot)
    // if(GetUpdate() == config.MAX_GENS() - 1){SnapshotPhylogony();}

    // step 2: select parent solutions for
    SelectionStep();

    // step 3: gather and record data
    RecordData();

    // step 4: reproduce and create new solutions
    ReproductionStep();
  });

  std::cerr << "Finished setting the OnUpdate function! \n" << std::endl;
}

void DiagWorld::SetMutation()
{
  std::cerr << "------------------------------------------------" << std::endl;
  std::cerr << "Setting mutation function..." << std::endl;

  // set the mutation function
  SetMutFun([this](Org & org, emp::Random & random)
  {
    // number of mutations and solution genome
    size_t mcnt = 0;
    genome_t & genome = org.GetGenome();

    // quick checks
    emp_assert(genome.size() == config.OBJECTIVE_CNT());
    emp_assert(target.size() == config.OBJECTIVE_CNT());

    for(size_t i = 0; i < genome.size(); ++i)
    {
      // if we do a mutation at this objective
      if(random_ptr->P(config.MUTATE_PER()))
      {
        const double mut = random_ptr->GetRandNormal(config.MEAN(), config.STD());

        // mutation puts objective above target
        if(config.TARGET() < genome[i] + mut)
        {
          // we wrap it back around target value (discuss with Charles)
          genome[i] = target[i] - (genome[i] + mut - target[i]);
        }
        // mutation puts objective in the negatives (discuss with Charles)
        else if(genome[i] + mut < 0.0)
        {
          genome[i] = 0.0;
        }
        else
        {
          // else we can simply add mutation
          genome[i] = genome[i] + mut;
        }

        ++mcnt;
      }
    }

    return mcnt;
  });

  std::cerr << "Mutation function set!\n" << std::endl;
}

void DiagWorld::SetSelection()
{
  std::cerr << "------------------------------------------------" << std::endl;
  std::cerr << "Setting Selection function..." << std::endl;

  selection = emp::NewPtr<Selection>(random_ptr);
  std::cerr << "Created selection emp::Ptr" << std::endl;

  switch (config.SELECTION())
  {
    case 0: // mu lambda
      MuLambda();
      break;

    case 1: // tournament
      Tournament();
      break;

    case 2: // fitness sharing
      FitnessSharing();
      break;

    case 3: // novelty search
      NoveltySearch();
      break;

    case 4: // epsilon lexicase
      EpsilonLexicase();
      break;

    case 5: // down sampled epsilon lexicase
      DownSampledLexicase();
      break;

    case 6: // cohort epsilon lexicase selection
      CohortLexicase();
      break;

    case 7: // novelty epsilon lexicase selection
      NoveltyLexicase();
      break;

    default:
      std::cerr << "ERROR UNKNOWN SELECTION CALL" << std::endl;
      emp_assert(true);
      break;
  }

  std::cerr << "Finished setting the Selection function! \n" << std::endl;
}

void DiagWorld::SetOnOffspringReady()
{
  std::cerr << "------------------------------------------------" << std::endl;
  std::cerr << "Setting OnOffspringReady function..." << std::endl;

  OnOffspringReady([this](Org & org, size_t parent_pos)
  {
    // quick checks
    emp_assert(fun_do_mutations); emp_assert(random_ptr);
    emp_assert(org.GetGenome().size() == config.OBJECTIVE_CNT());
    emp_assert(org.GetM() == config.OBJECTIVE_CNT());

    // do mutations on offspring
    size_t mcnt = fun_do_mutations(org, *random_ptr);

    // no mutations were applied to offspring
    if(mcnt == 0)
    {
      Org & parent = *pop[parent_pos];

      // quick checks
      emp_assert(parent.GetGenome().size() == config.OBJECTIVE_CNT());
      emp_assert(parent.GetM() == config.OBJECTIVE_CNT());

      // give everything to offspring from parent
      org.MeClone();
      org.Inherit(parent.GetScore(), parent.GetOptimal(), parent.GetCount(), parent.GetAggregate(), parent.GetStart());
    }
    else{org.Reset();}
  });

  std::cerr << "Finished setting OnOffspringReady function!\n" << std::endl;
}

void DiagWorld::SetEvaluation()
{
  std::cerr << "------------------------------------------------" << std::endl;
  std::cerr << "Setting Evaluation function..." << std::endl;

  target_t tar(config.OBJECTIVE_CNT(), config.TARGET());
  target.clear(); target.resize(config.OBJECTIVE_CNT());
  std::copy(tar.begin(), tar.end(), target.begin());

  diagnostic = emp::NewPtr<Diagnostic>(target, config.CREDIT());
  std::cerr << "Created diagnostic emp::Ptr" << std::endl;

  switch (config.DIAGNOSTIC())
  {
    case 0: // exploitation
      Exploitation();
      break;

    case 1: // structured exploitation
      StructuredExploitation();
      break;

    case 2: // strong ecology
      StrongEcology();
      break;

    case 3: // exploration
      Exploration();
      break;

    case 4: // weak ecology
      WeakEcology();
      break;

    default: // error, unknown diganotic
      std::cerr << "ERROR: UNKNOWN DIAGNOSTIC" << std::endl;
      emp_assert(true);
      break;
  }

  std::cerr << "Evaluation function set!\n" <<std::endl;
}

void DiagWorld::SetDataTracking()
{
  std::cerr << "------------------------------------------------" << std::endl;
  std::cerr << "Setting up data tracking..." << std::endl;

  // systematic tracking (ask alex about it)
  std::cerr << "Setting up systematics tracking..." << std::endl;

  // sys_ptr = emp::NewPtr<systematics_t>([](const Org & o) {return o.GetGenome();});

  // sys_ptr->AddSnapshotFun([](const taxon_t & taxon) {
  //   return emp::to_string(taxon.GetData().GetFitness());
  // }, "fitness", "Taxon fitness");

  // sys_ptr->AddSnapshotFun([](const taxon_t & taxon) {
  //   return emp::ToString(taxon.GetData().GetPhenotype());
  // }, "phenotype", "Taxon Phenotype");

  // sys_ptr->AddSnapshotFun([](const taxon_t & taxon) {
  //   return emp::ToString(taxon.GetInfo());
  // }, "genotype", "Taxon Genotype");

  // will add it to the world for tracking purposes
  // AddSystematics(sys_ptr);
  // summary stats (whatever resolution we want)
  // SetupSystematicsFile(0, config.OUTPUT_DIR() + "systematics.csv").SetTimingRepeat(config.PRINT_INTERVAL());

  // std::cerr << "Systematics tracking complete!" << std::endl;

  // initialize all nodes (ask charles)
  std::cerr << "Initializing nodes..." << std::endl;
  pop_fit.New(); pop_opti.New(); pnt_fit.New(); pnt_opti.New();
  std::cerr << "Nodes initialized!" << std::endl;

  // track population aggregate score stats: average, variance, min, max
  data_file.AddMean(*pop_fit, "pop_fit_avg", "Population average aggregate performance.");
  data_file.AddVariance(*pop_fit, "pop_fit_var", "Population variance aggregate performance.");
  data_file.AddMax(*pop_fit, "pop_fit_max", "Population maximum aggregate performance.");
  data_file.AddMin(*pop_fit, "pop_fit_min", "Population minimum aggregate performance.");

  // track population optimized objective count stats: average, variance, min, max
  data_file.AddMean(*pop_opti, "pop_opt_avg", "Population average objective optimization count.");
  data_file.AddVariance(*pop_opti, "pop_opt_var", "Population variance objective optimization count.");
  data_file.AddMax(*pop_opti, "pop_opt_max", "Population maximum objective optimization count.");
  data_file.AddMin(*pop_opti, "pop_opt_min", "Population minimum objective optimization count.");

  // track parent aggregate score stats: average, variance, min, max
  data_file.AddMean(*pnt_fit, "pnt_fit_avg", "Parent average aggregate performance.");
  data_file.AddVariance(*pnt_fit, "pnt_fit_var", "Parent variance aggregate performance.");
  data_file.AddMax(*pnt_fit, "pnt_fit_max", "Parent maximum aggregate performance.");
  data_file.AddMin(*pnt_fit, "pnt_fit_min", "Parent minimum aggregate performance.");

  // track parent optimized objective count stats: average, variance, min, max
  data_file.AddMean(*pnt_opti, "pnt_opt_avg", "Parent average objective optimization count.");
  data_file.AddVariance(*pnt_opti, "pnt_opt_var", "Parent variance objective optimization count.");
  data_file.AddMax(*pnt_opti, "pnt_opt_max", "Parent maximum objective optimization count.");
  data_file.AddMin(*pnt_opti, "pnt_opt_min", "Parent minimum objective optimization count.");

  std::cerr << "Added all data nodes to data file!" << std::endl;

  // update we are at
  data_file.AddFun<size_t>([this]()
  {
    return update;
  }, "gen", "Current generation at!");

  // unique optimized objectives count
  data_file.AddFun<size_t>([this]()
  {
    return UniqueObjective();
  }, "pop_uni_obj", "Number of unique optimized traits per generation!");

    // unique starting positions
  data_file.AddFun<size_t>([this]()
  {
    return FindUniqueStart();
  }, "uni_str_pos", "Number of unique starting positions in the population!");

  // count of common solution in the population
  data_file.AddFun<size_t>([this]()
  {
    // quick checks
    emp_assert(0 < common.size());
    emp_assert(comm_pos != config.POP_SIZE());

    // find iterator to common org
    const auto it = common.find(comm_pos);
    emp_assert(it != common.end());
    emp_assert(0 < it->second.size());

    return it->second.size();
  }, "com_sol_cnt", "Count of genetically common solution!");

  // elite solution aggregate performance
  data_file.AddFun<double>([this]()
  {
    // quick checks
    emp_assert(elite_pos != config.POP_SIZE());
    emp_assert(pop.size() == config.POP_SIZE());

    Org & org = *pop[elite_pos];

    return org.GetAggregate();
  }, "ele_agg_per", "Elite solution aggregate performance!");

  // elite solution optimized objectives count
  data_file.AddFun<size_t>([this]()
  {
    // quick checks
    emp_assert(elite_pos != config.POP_SIZE());
    emp_assert(pop.size() == config.POP_SIZE());

    Org & org = *pop[elite_pos];

    return org.GetCount();
  }, "ele_opt_cnt", "Elite solution optimized objective count!");

  // common solution aggregate performance
  data_file.AddFun<double>([this]()
  {
    // quick checks
    emp_assert(comm_pos != config.POP_SIZE());
    emp_assert(pop.size() == config.POP_SIZE());

    Org & org = *pop[comm_pos];

    return org.GetAggregate();
  }, "com_agg_per", "Common solution aggregate performance!");

  // common solution optimized objectives count
  data_file.AddFun<size_t>([this]()
  {
    // quick checks
    emp_assert(comm_pos != config.POP_SIZE());
    emp_assert(pop.size() == config.POP_SIZE());

    Org & org = *pop[comm_pos];

    return org.GetCount();
  }, "com_opt_cnt", "Common solution optimized objective count!");

  // optimized solution aggregate performance
  data_file.AddFun<double>([this]()
  {
    // quick checks
    emp_assert(opti_pos != config.POP_SIZE());
    emp_assert(pop.size() == config.POP_SIZE());

    Org & org = *pop[opti_pos];

    return org.GetAggregate();
  }, "opt_agg_per", "Otpimal solution aggregate performance");

  // optimized solution optimized objectives count
  data_file.AddFun<size_t>([this]()
  {
    // quick checks
    emp_assert(opti_pos != config.POP_SIZE());
    emp_assert(pop.size() == config.POP_SIZE());

    Org & org = *pop[opti_pos];

    return org.GetCount();
  }, "opt_obj_cnt", "Otpimal solution aggregate performance");

  // loss of diversity
  data_file.AddFun<double>([this]()
  {
    // quick checks
    emp_assert(parent_vec.size() == config.POP_SIZE());

    std::set<size_t> unique;
    for(auto & id : parent_vec) {unique.insert(id);}

    // ask Charles
    const double num = static_cast<double>(unique.size());
    const double dem = static_cast<double>(config.POP_SIZE());

    return num / dem;
  }, "los_div", "Loss in diversity generated by the selection scheme!");

  // selection pressure
  data_file.AddFun<double>([this]()
  {
    // quick checks
    emp_assert(pop_fit->GetCount() == config.POP_SIZE());
    emp_assert(pnt_fit->GetCount() == config.POP_SIZE());

    const double pop = pop_fit->GetMean();
    const double pnt = pnt_fit->GetMean();
    const double var = pop_fit->GetVariance();

    if(var == 0.0) {return 0.0;}

    return (pop - pnt) / var;
  }, "sel_pre", "Selection pressure applied by selection scheme!");

  // selection variance
  data_file.AddFun<double>([this]()
  {
    // quick checks
    emp_assert(pop_fit->GetCount() == config.POP_SIZE());
    emp_assert(pnt_fit->GetCount() == config.POP_SIZE());

    const double pop = pop_fit->GetVariance();
    const double pnt = pnt_fit->GetVariance();

    if(pnt == 0.0) {return 0.0;}

    return pop / pnt;
  }, "sel_var", "Selection pressure applied by selection scheme!");

  data_file.PrintHeaderKeys();

  std::cerr << "Finished setting data tracking!\n" << std::endl;
}

void DiagWorld::PopulateWorld()
{
  std::cerr << "------------------------------------------" << std::endl;
  std::cerr << "Populating world with initial solutions..." << std::endl;

  // Fill the workd with requested population size!
  Org org(config.OBJECTIVE_CNT());
  Inject(org.GetGenome(), config.POP_SIZE());

  std::cerr << "Initialing world complete!" << std::endl;
}


///< principle steps during an evolutionary run

void DiagWorld::ResetData()
{
  // reset all data nodes
  pop_fit->Reset();
  pop_opti->Reset();
  pnt_fit->Reset();
  pnt_opti->Reset();

  // reset all positon ids
  elite_pos = config.POP_SIZE();
  comm_pos = config.POP_SIZE();
  opti_pos = config.POP_SIZE();

  // reset all vectors/maps holding current gen data
  fit_vec.clear();
  parent_vec.clear();
  common.clear();
}

void DiagWorld::EvaluationStep()
{
  // quick checks
  emp_assert(fit_vec.size() == 0); emp_assert(0 < pop.size());
  emp_assert(pop.size() == config.POP_SIZE());

  // iterate through the world and populate fitness vector
  fit_vec.resize(config.POP_SIZE());
  for(size_t i = 0; i < pop.size(); ++i)
  {
    Org & org = *pop[i];

    // no evaluate needed if offspring is a clone
    fit_vec[i] = (org.GetClone()) ? org.GetAggregate() : evaluate(org);

    // systematic stuff
    // emp::Ptr<taxon_t> taxon = sys_ptr->GetTaxonAt(i);
    // taxon->GetData().RecordFitness(org.GetAggregate());
    // taxon->GetData().RecordPhenotype(org.GetScore());
  }
}

void DiagWorld::SelectionStep()
{
  // quick checks
  emp_assert(parent_vec.size() == 0); emp_assert(0 < pop.size());
  emp_assert(pop.size() == config.POP_SIZE());

  // store parents
  auto parents = select();
  emp_assert(parents.size() == config.POP_SIZE());

  parent_vec.resize(config.POP_SIZE());
  std::copy(parents.begin(), parents.end(), parent_vec.begin());
}

void DiagWorld::RecordData()
{
  /// Add data to all nodes

  // get pop data
  emp_assert(pop.size() == config.POP_SIZE());
  for(size_t i = 0; i < pop.size(); ++i)
  {
    Org & org = *pop[i];
    pop_fit->Add(org.GetAggregate());
    pop_opti->Add(org.GetCount());
  }
  emp_assert(pop_fit->GetCount() == config.POP_SIZE());
  emp_assert(pop_opti->GetCount() == config.POP_SIZE());

  // get parent data
  emp_assert(parent_vec.size() == config.POP_SIZE());
  for(size_t i = 0; i < parent_vec.size(); ++i)
  {
    Org & org = *pop[parent_vec[i]];
    pnt_fit->Add(org.GetAggregate());
    pnt_opti->Add(org.GetCount());
  }
  emp_assert(pnt_fit->GetCount() == config.POP_SIZE());
  emp_assert(pnt_opti->GetCount() == config.POP_SIZE());

  /// get all position ids

  elite_pos = FindElite();
  emp_assert(elite_pos != config.POP_SIZE());

  comm_pos = FindCommon();
  emp_assert(comm_pos != config.POP_SIZE());

  opti_pos = FindOptimized();
  emp_assert(opti_pos != config.POP_SIZE());

  /// fill vectors & map
  emp_assert(fit_vec.size() == config.POP_SIZE()); // should be set already
  emp_assert(parent_vec.size() == config.POP_SIZE()); // should be set already
  emp_assert(0 < common.size());  // should already be set in FindCommon

  /// update the file
  if ( !(GetUpdate() % config.DATA_INTERVAL()) || (GetUpdate() == config.MAX_GENS()) ) {
    data_file.Update();
  }

  if ( !(GetUpdate() % config.PRINT_INTERVAL()) || (GetUpdate() == config.MAX_GENS()) ) {
    // output this so we know where we are in terms of generations and fitness
    Org & org = *pop[elite_pos];
    Org & opt = *pop[opti_pos];
    std::cout << "gen=" << GetUpdate() << ", max_fit=" << org.GetAggregate()  << ", max_opt=" << opt.GetCount() << std::endl;
  }

}

void DiagWorld::ReproductionStep()
{
  // quick checks
  emp_assert(parent_vec.size() == config.POP_SIZE());
  emp_assert(pop.size() == config.POP_SIZE());

  // go through parent ids and do births
  for(auto & id : parent_vec){DoBirth(GetGenomeAt(id), id);}
}


///< selection scheme implementations

void DiagWorld::MuLambda()
{
  std::cerr << "Setting selection scheme: MuLambda" << std::endl;

  // set select lambda to mu lambda selection
  select = [this]()
  {
    // quick checks
    emp_assert(selection); emp_assert(pop.size() == config.POP_SIZE());
    emp_assert(0 < pop.size()); emp_assert(fit_vec.size() == config.POP_SIZE());

    // group population by fitness
    fitgp_t group = selection->FitnessGroup(fit_vec);

    return selection->MLSelect(config.MU(), config.POP_SIZE(), group);
  };

  std::cerr << "MuLambda selection scheme set!" << std::endl;
}

void DiagWorld::Tournament()
{
  std::cerr << "Setting selection scheme: Tournament" << std::endl;

  select = [this]()
  {
    // quick checks
    emp_assert(selection); emp_assert(pop.size() == config.POP_SIZE());
    emp_assert(0 < pop.size()); emp_assert(fit_vec.size() == config.POP_SIZE());

    // will hold parent ids + get pop agg score values
    ids_t parent(pop.size());

    // get pop size amount of parents
    for(size_t i = 0; i < parent.size(); ++i)
    {
      parent[i] = selection->Tournament(config.TOUR_SIZE(), fit_vec);
    }

    return parent;
  };

  std::cerr << "Tournament selection scheme set!" << std::endl;
}

void DiagWorld::FitnessSharing()
{
  std::cerr << "Setting selection scheme: FitnessSharing" << std::endl;
  std::cerr << "Calculating SIGMA..." << std::endl;

  // calculate the sigma we are using

  // max possible
  genome_t high(config.OBJECTIVE_CNT(), config.TARGET());
  // lowest possbile
  genome_t low(config.OBJECTIVE_CNT(), 0.0);
  // caclulate sigma
  SIGMA = selection->Pnorm(high, low, config.PNORM_EXP()) * config.FIT_SIGMA();

  std::cerr << "SIGMA=" << SIGMA << std::endl;


  select = [this]()
  {
    // quick checks
    emp_assert(selection); emp_assert(pop.size() == config.POP_SIZE());
    emp_assert(0 < pop.size()); emp_assert(fit_vec.size() == config.POP_SIZE());
    emp_assert(0 <= SIGMA);

    gmatrix_t genomes = PopGenomes();

    // generate distance matrix + fitness transformation
    fmatrix_t dist_mat = selection->SimilarityMatrix(genomes, config.PNORM_EXP());
    score_t tscore = selection->FitnessSharing(dist_mat, fit_vec, config.FIT_ALPHA(), SIGMA);

    // select parent ids
    ids_t parent(pop.size());

    for(size_t i = 0; i < parent.size(); ++i)
    {
      parent[i] = selection->Tournament(config.TOUR_SIZE(), tscore);
    }

    return parent;
  };

  std::cerr << "Fitness sharing selection scheme set!" << std::endl;
}

void DiagWorld::NoveltySearch()
{
  std::cerr << "Setting selection scheme: NoveltySearch" << std::endl;
  std::cerr << "Tournament size for novelty: " << config.TOUR_SIZE() << std::endl;

  select = [this]()
  {
    // quick checks
    emp_assert(selection); emp_assert(pop.size() == config.POP_SIZE());
    emp_assert(0 < pop.size()); emp_assert(fit_vec.size() == config.POP_SIZE());

    // generate nearest neighbor pop structure
    neigh_t neighborhood = selection->FitNearestN(fit_vec, config.NOVEL_K());

    // transform original fitness into novelty fitness
    score_t tscore = selection->Novelty(fit_vec, neighborhood, config.NOVEL_K());

    // select parent ids
    ids_t parent(pop.size());

    for(size_t i = 0; i < parent.size(); ++i)
    {
      parent[i] = selection->Tournament(config.TOUR_SIZE(), tscore);
    }

    return parent;
  };

  std::cerr << "Novelty search selection scheme set!" << std::endl;
}

void DiagWorld::EpsilonLexicase()
{
  std::cerr << "Setting selection scheme: EpsilonLexicase" << std::endl;

  select = [this]()
  {
    // quick checks
    emp_assert(selection); emp_assert(pop.size() == config.POP_SIZE());
    emp_assert(0 < pop.size());

    // fitness matrix
    fmatrix_t matrix = PopFitMat();

    // select parent ids
    ids_t parent(pop.size());

    for(size_t i = 0; i < parent.size(); ++i)
    {
      parent[i] = selection->EpsiLexicase(matrix, config.LEX_EPS(), config.OBJECTIVE_CNT());
    }

    return parent;
  };

  std::cerr << "Epsilon Lexicase selection scheme set!" << std::endl;
}

void DiagWorld::DownSampledLexicase()
{
  std::cerr << "Setting selection scheme: DownSampledLexicase" << std::endl;

  select = [this]()
  {
    // quick checks
    emp_assert(selection); emp_assert(pop.size() == config.POP_SIZE());
    emp_assert(0 < pop.size()); emp_assert(0 < config.DSLEX_PROP());

    // fitness matrix
    fmatrix_t matrix = PopFitMat();

    // select parent ids
    ids_t parent(pop.size());

    // create subset of testcases to use for downsampled lexicase
    size_t subset = (double) config.OBJECTIVE_CNT() * config.DSLEX_PROP();
    ids_t test_cases = emp::Choose(*random_ptr, config.OBJECTIVE_CNT(), subset);

    for(size_t i = 0; i < parent.size(); ++i)
    {
      parent[i] = selection->DSELexicase(matrix, config.LEX_EPS(), test_cases);
    }

    return parent;
  };

  std::cerr << "Down Sampled Lexicase selection scheme set!" << std::endl;
}

void DiagWorld::CohortLexicase()
{
  std::cerr << "Setting selection scheme: CohortLexicase" << std::endl;

  select = [this]()
  {
    // quick checks
    emp_assert(selection); emp_assert(pop.size() == config.POP_SIZE());
    emp_assert(0 < pop.size()); emp_assert(0 < config.COH_LEX_PROP());

    // fitness matrix
    const fmatrix_t matrix = PopFitMat();
    // population cohorts
    const cohort_t pop_cohorts = selection->CohortGeneration(config.POP_SIZE(), config.COH_LEX_PROP());
    // testcase cohorts
    const cohort_t test_cohorts = selection->CohortGeneration(config.OBJECTIVE_CNT(), config.COH_LEX_PROP());
    // quick checks
    emp_assert(pop_cohorts.size() == test_cohorts.size());

    // select parent ids
    ids_t parent(pop.size());

    // iterate through cohort pairing
    size_t pnt_cnt = 0;
    for(size_t p = 0; p < pop_cohorts.size(); ++p)
    {
      for(size_t c = 0; c < pop_cohorts[p].size(); ++c, ++pnt_cnt)
      {
        // get winner from current cohort
        size_t pnt_win = selection->CELexicase(matrix, config.LEX_EPS(), pop_cohorts[p], test_cohorts[p]);
        // quick checks; we know that POP_SIZE is our error value
        emp_assert(pnt_win != config.POP_SIZE());
        // store parent and keep going
        parent[pnt_cnt] = pnt_win;
      }
    }

    // quick checks
    emp_assert(pnt_cnt == config.POP_SIZE());
    return parent;
  };

  std::cerr << "Cohort Lexicase selection scheme set!" << std::endl;
}

void DiagWorld::NoveltyLexicase()
{
  std::cerr << "Setting selection scheme: NoveltyLexicase" << std::endl;

  select = [this]()
  {
    // quick checks
    emp_assert(selection); emp_assert(pop.size() == config.POP_SIZE());
    emp_assert(0 < pop.size()); emp_assert(0 <= config.NOVEL_K());
    emp_assert(0 <= config.LEX_EPS());

    // fitness matrix
    const fmatrix_t matrix = PopFitMat();
    // create fitness and novelty value matrix
    const fmatrix_t t_matrix = selection->LexicaseNoveltyFit(matrix, config.NOVEL_K(), config.OBJECTIVE_CNT());

    // select parent ids
    ids_t parent(pop.size());

    // iterate through cohort pairing
    for(size_t i = 0; i < parent.size(); ++i)
    {
      // if K == 0, then we only expect to go to the nubmer of objectives in the problem
      if(config.NOVEL_K() == 0) {parent[i] = selection->EpsiLexicase(t_matrix, config.LEX_EPS(), config.OBJECTIVE_CNT());}
      else{parent[i] = selection->EpsiLexicase(t_matrix, config.LEX_EPS(), 2 * config.OBJECTIVE_CNT());}
    }

    return parent;
  };

  std::cerr << "Novelty Lexicase selection scheme set!" << std::endl;
}

///< evaluation function implementations

void DiagWorld::Exploitation()
{
  std::cerr << "Setting exploitation diagnostic..." << std::endl;

  evaluate = [this](Org & org)
  {
    // set score & aggregate
    score_t score = diagnostic->Exploitation(org.GetGenome());
    org.SetScore(score);
    org.AggregateScore();
    org.StartPosition();

    // set optimal vector and count
    optimal_t opti = diagnostic->OptimizedVector(org.GetGenome(), config.ACCURACY());
    org.SetOptimal(opti);
    org.CountOptimized();

    return org.GetAggregate();
  };

  std::cerr << "Exploitation diagnotic set!" << std::endl;
}

void DiagWorld::StructuredExploitation()
{
  std::cerr << "Setting structured exploitation diagnostic..." << std::endl;

  evaluate = [this](Org & org)
  {
    // set score & aggregate
    score_t score = diagnostic->StructExploitation(org.GetGenome());
    org.SetScore(score);
    org.AggregateScore();
    org.StartPosition();

    // set optimal vector and count
    optimal_t opti = diagnostic->OptimizedVector(org.GetGenome(), config.ACCURACY());
    org.SetOptimal(opti);
    org.CountOptimized();

    return org.GetAggregate();
  };

  std::cerr << "Structured exploitation diagnotic set!" << std::endl;
}

void DiagWorld::StrongEcology()
{
  std::cerr << "Setting strong ecology diagnostic..." << std::endl;

  evaluate = [this](Org & org)
  {
    // set score & aggregate
    score_t score = diagnostic->StrongEcology(org.GetGenome());
    org.SetScore(score);
    org.AggregateScore();
    org.StartPosition();

    // set optimal vector and count
    optimal_t opti = diagnostic->OptimizedVector(org.GetGenome(), config.ACCURACY());
    org.SetOptimal(opti);
    org.CountOptimized();

    return org.GetAggregate();
  };

  std::cerr << "Strong ecology diagnotic set!" << std::endl;
}

void DiagWorld::Exploration()
{
  std::cerr << "Setting exploration diagnostic..." << std::endl;

  evaluate = [this](Org & org)
  {
    // set score & aggregate
    score_t score = diagnostic->Exploration(org.GetGenome());
    org.SetScore(score);
    org.AggregateScore();
    org.StartPosition();

    // set optimal vector and count
    optimal_t opti = diagnostic->OptimizedVector(org.GetGenome(), config.ACCURACY());
    org.SetOptimal(opti);
    org.CountOptimized();

    return org.GetAggregate();
  };

  std::cerr << "Exploration diagnotic set!" << std::endl;
}

void DiagWorld::WeakEcology()
{
  std::cerr << "Setting weak ecology diagnostic..." << std::endl;

  evaluate = [this](Org & org)
  {
    // set score & aggregate
    score_t score = diagnostic->WeakEcology(org.GetGenome());
    org.SetScore(score);
    org.AggregateScore();

    // set optimal vector and count
    optimal_t opti = diagnostic->OptimizedVector(org.GetGenome(), config.ACCURACY());
    org.SetOptimal(opti);
    org.CountOptimized();
    org.StartPosition();

    return org.GetAggregate();
  };

  std::cerr << "Weak ecology diagnotic set!" << std::endl;
}

///< data tracking

size_t DiagWorld::UniqueObjective()
{
  // quick checks
  emp_assert(0 < pop.size()); emp_assert(pop.size() == config.POP_SIZE());

  // iterate through objectives
  size_t cnt = 0;
  for(size_t o = 0; o < config.OBJECTIVE_CNT(); ++o)
  {
    // iterate pop to check is a solution has the objective optimized
    for(size_t p = 0; p < pop.size(); ++p)
    {
      Org & org = *pop[p];

      // quick checks
      emp_assert(org.GetOptimal().size() == config.OBJECTIVE_CNT());

      if(org.OptimizedAt(o))
      {
        ++cnt;
        break;
      }
    }
  }

  return cnt;
}

size_t DiagWorld::FindElite()
{
  // quick checks
  emp_assert(0 < fit_vec.size()); emp_assert(fit_vec.size() == pop.size());
  emp_assert(fit_vec.size() == config.POP_SIZE());
  emp_assert(elite_pos == config.POP_SIZE());

  // find max value position
  auto elite_it = std::max_element(fit_vec.begin(), fit_vec.end());

  return std::distance(fit_vec.begin(), elite_it);;
}

size_t DiagWorld::FindCommon()
{
  // quick checks
  emp_assert(pop.size() == config.POP_SIZE());
  emp_assert(common.size() == 0);
  emp_assert(comm_pos == config.POP_SIZE());

  // iterate through pop and place in appropiate bin
  for(size_t i = 0; i < pop.size(); ++i)
  {
    bool in_comm = false;
    const Org & org = *pop[i];

    // check if current org matches any of the existing keys
    for(const auto & p : common)
    {
      // get key orgs data
      const Org & korg = *pop[p.first];

      // get euclidean distance between both genomes
      const double dif = selection->Pnorm(org.GetGenome(), korg.GetGenome(), 2.0);

      // if they are a match
      if(dif == 0.0)
      {
        common[p.first].push_back(i);
        in_comm = true;
        break;
      }
    }

    if(!in_comm)
    {
      ids_t first{i};
      common[i] = first;
    }
  }

  // iterate through common dictionary and find common org id
  size_t max = 0; size_t max_pos = 0;
  for(const auto & p : common)
  {
    if(max < p.second.size())
    {
      max_pos = p.first;
      max = p.second.size();
    }
  }

  return max_pos;
}

size_t DiagWorld::FindOptimized()
{
  // quick checks
  emp_assert(0 < pop.size()); emp_assert(pop.size() == config.POP_SIZE());
  emp_assert(opti_pos == config.POP_SIZE());

  // iterate through pop and find optimal solution
  size_t max = 0; size_t max_pos = 0;
  for(size_t i = 0; i < pop.size(); ++i)
  {
    const Org & org = *pop[i];

    if(max < org.GetCount())
    {
      max = org.GetCount();
      max_pos = i;
    }
  }

  return max_pos;
}

size_t DiagWorld::FindUniqueStart()
{
  // quick checks
  emp_assert(0 < pop.size()); emp_assert(pop.size() == config.POP_SIZE());

  // collect number of unique starting positions
  std::set<size_t> position;

  // iterate pop to check is a solution has the objective optimized
  for(size_t p = 0; p < pop.size(); ++p)
  {
    Org & org = *pop[p];

    // check that the position has be set
    emp_assert(org.GetStart() != config.OBJECTIVE_CNT());

    // insert position into set
    position.insert(org.GetStart());
  }

  return position.size();
}

// void DiagWorld::SnapshotPhylogony()
// {
//   sys_ptr->Snapshot(config.OUTPUT_DIR() + "phylo_" + emp::to_string(GetUpdate()) + ".csv");
// }


///< helper functions

DiagWorld::fmatrix_t DiagWorld::PopFitMat()
{
  // quick checks
  emp_assert(pop.size() == config.POP_SIZE());

  // create matrix of population score vectors
  fmatrix_t matrix(pop.size());

  for(size_t i = 0; i < pop.size(); ++i)
  {
    Org & org = *pop[i];
    emp_assert(org.GetScore().size() == config.OBJECTIVE_CNT());

    // charles ask if this is the actual org score vector or a deep copy made
    matrix[i] = org.GetScore();
  }

  return matrix;
}

DiagWorld::gmatrix_t DiagWorld::PopGenomes()
{
  // quick checks
  emp_assert(pop.size() == config.POP_SIZE());

  gmatrix_t matrix(pop.size());

  for(size_t i = 0; i < pop.size(); ++i)
  {
    Org & org = *pop[i];
    emp_assert(org.GetGenome().size() == config.OBJECTIVE_CNT());

    // charles ask if this is the actual org genome or a deep copy made
    matrix[i] = org.GetGenome();
  }

  return matrix;
}

void DiagWorld::SnapshotConfig(const config_t & config) {
  // Make a new datafile for snapshot
  emp::DataFile snapshot_file(config.OUTPUT_DIR() + "/run_config.csv");
  std::function<std::string()> get_cur_param;
  std::function<std::string()> get_cur_value;
  snapshot_file.AddFun<std::string>([&get_cur_param]() -> std::string { return get_cur_param(); }, "parameter" );
  snapshot_file.AddFun<std::string>([&get_cur_value]() -> std::string { return get_cur_value(); }, "value" );
  snapshot_file.PrintHeaderKeys(); // param,value

  for (const auto & entry : config) {
    get_cur_param = [&entry]() { return entry.first; };
    get_cur_value = [&entry]() { return emp::to_string(entry.second->GetValue()); };
    snapshot_file.Update();
  }

}

#endif