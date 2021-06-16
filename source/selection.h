/// These are the selection schemes we are using for this project
/// Will be broken up according to my thoughts on how $$ components work
/// These selection functions are selecting on the assumption that the problem is a maximization problem

#ifndef SEL_H
#define SEL_H

///< standard headers
#include <algorithm>
#include <map>
#include <utility>
#include <cmath>
#include <numeric>

///< empirical headers
#include "base/vector.h"
#include "tools/Random.h"
#include "tools/random_utils.h"

///< constant vars
constexpr size_t DRIFT_SIZE = 1;
constexpr double ERROR_VALD = -1.0;

class Selection
{
  // object types we are using in this class
  public:
    // vector of any ids
    using ids_t = emp::vector<size_t>;
    // vector type of org score
    using score_t = emp::vector<double>;
    // matrix type of org with multiple scores
    using fmatrix_t = emp::vector<score_t>;
    // vector holding population genomes
    using gmatrix_t = emp::vector<emp::vector<double>>;
    // map holding population id groupings by fitness (keys in decending order)
    using fitgp_t = std::map<double, ids_t, std::greater<double>>;
    // sorted score vector w/ position id and score
    using sorted_t = emp::vector<std::pair<size_t,double>>;
    // vector of double vectors for K neighborhoods
    using neigh_t = emp::vector<score_t>;
    // vector of vector position ids that represent cohort assignment
    using cohort_t = emp::vector<ids_t>;


  public:

    Selection(emp::Ptr<emp::Random> rng = nullptr) : random(rng) {emp_assert(rng);}


    ///< helper functions

    // distance function between two values
    double Distance(double a, double b) {return std::abs(a - b);}

    // p-norm function between two vector subtractions and the exponent for p
    double Pnorm(const score_t & x, const score_t & y, const double exp);

    // similarity matrix generator
    fmatrix_t SimilarityMatrix(const gmatrix_t & genome, const double exp);

    // sanity check for novelty lexicase
    bool SizeCheck(const fmatrix_t & matrix, const size_t M)
    {
      for(auto & v : matrix)
      {
        if(v.size() != 2 * M) {return false;}
      }

      return true;
    }

    // print vectors
    template <class T>
    void PrintVec(const emp::vector<T> &v, const std::string s)
    {
      std::cerr << s << ": ";
      for(auto x : v) {std::cerr << (double) x << ",";}
      std::cerr << std::endl;
    }


    ///< population structure

    /**
     * K-Nearest Neighbor Structure:
     *
     * This function will return a vector of vectors, where
     * each individual vector corresponds to that solutions nearest neighbor
     *
     * In the event of ties, we take right neighbor score.
     *
     * @param score Vector containing all solution scores.
     * @param K Size of neighborhood looking for
     *
     * @return Vector of vector ids groups as nearest neighbors.
     */
    neigh_t FitNearestN(const score_t & score, const size_t K);

    /**
     * Fitness Group Structure:
     *
     * This function will return a map of double,vector pairing where
     * each vector corresponds to solution ids share the same fitness
     *
     * @param score Vector containing all solution scores.
     *
     * @return Map grouping population ids by fitness (keys (fitness) in decending order)
     */
    fitgp_t FitnessGroup(const score_t & score);

    /**
     * Cohort Generation:
     *
     * This function will return a vector of vectors conataining position ids.
     * A vector containing all ids will first be generated, then shuffled, then partitioned
     *
     * @param N Total number of objects being partitioned
     * @param P Percentage of N that we place in each cohort
     *
     * @return Vector containing all cohort of ids
     */
    cohort_t CohortGeneration(const size_t N, const double P);



    ///< fitness transformation

    /**
     * Fitness Sharing Transformation:
     *
     * Fitness transformation function inspired from:
     * B. Sareni and L. Krahenbuhl, “Fitness sharing and niching methods revisited,”
     * in IEEE Transactions on Evolutionary Computation, vol. 2, no. 3, pp. 97-106, Sept. 1998.
     *
     *
     * @param dmat distance matrix for genome pairs, a lower diagnoal matrix is expected.
     * @param score Vector containing all solution scores.
     * @param alph Shape of sharing function.
     * @param sig Similarity threshold.
     *
     * @return Vector with parent id's that are selected.
     */
    score_t FitnessSharing(const fmatrix_t & dmat, const score_t & score, const double alph, const double sig);

    /**
     * Fitness Sharing: Sharing Function
     *
     * This function will return a score of 0 or 1 - (score/sig)^alph.
     * Depending on score < sig.
     *
     * \param dist single score we are evaluating
     * \param sig similarity threshold we are using
     * \param alph shape of penalty function
     *
     * \return a tranformed fitness score, with fitness sharing integrated within the score
     */
    double SharingFunction(const double dist, const double sig, const double alph);

    /**
     * Novelty Fitness Transformation:
     *
     * This function will transform original fitness values as novelty scores.
     * Each score has its associated neighbors (2nd argument).
     *
     *
     * @param score Vector containing all solution scores.
     * @param neigh Vector containing neighboring positions in score (per solution).
     * @param K Size of each neighborhood.
     * @param exp Exponent we are using for the p norm calculations.
     *
     * @return Vector with transformed scores.
     */
    score_t Novelty(const score_t & score, const neigh_t & neigh, const size_t K);

    /**
     * Lexicase Novelty Fitness Transformation:
     *
     * This function will go through each trait and calculate the appropiate novelty score.
     * Once all novelty scores are calculated, it will return matrix of fitness and novelty scores.
     *
     *
     * @param mscore Vector containing all solution fitness vectors.
     * @param K K-nearest neighbors we are looking for.
     * @param M Total number of testcases we are expecting for all solutions
     *
     * @return Matrix with fitness and novelty scores
     */
    fmatrix_t LexicaseNoveltyFit(const fmatrix_t & mscore, const size_t K, const size_t M);


    ///< selector functions

    /**
     * (μ,λ) Elite Selector:
     *
     * This function finds the top 'm' performing solutions.
     * Once the top 'm' solutions are found, each will be placed (l/m) times in the parent list.
     *
     * @param top Number of top performing solutions to pick.
     * @param pops Population size.
     * @param group Map with fitness,vector id pairs (descending order expected). Doesn't have to be fitness but some double value in best to worst order.
     *
     * @return Vector with parent id's that are selected.
     */
    ids_t MLSelect(const size_t mu, const size_t lambda, const fitgp_t & group);

    /**
     * Tournament Selector:
     *
     * This function holds tournaments for 't' solutions randomly picked from the population.
     * In the event of ties, a solution will be selected randomly from all solutions that tie.
     *
     * @param t Tournament size.
     * @param score Vector holding the population score.
     *
     * @return Parent id of solution that won the tournament.
     */
    size_t Tournament(const size_t t, const score_t & score);

    /**
     * Drift Selector:
     *
     * This function will return a random solution as a parent.
     *
     *
     * @param score Vector containing all solution scores.
     *
     * @return Parent id that is selected.
     */
    size_t Drift(const size_t size);

    /**
     * Epsilon Lexicase Selector:
     *
     * This function will iterate through individual testcases.
     * While filtering down the population on each one, only taking the top performers.
     * The top performers must be some within some distance of the top performance to pass.
     *
     * In the event of ties on the last testcase being used, a solution will be selected at random.
     *
     * @param mscore Matrix of solution fitnesses (must be the same amount of fitness per solution)(mscore.size => # of orgs).
     * @param epsi Epsilon threshold value.
     * @param M Number of traits we are expecting.
     *
     * @return A single winning solution id.
     */
    size_t EpsiLexicase(const fmatrix_t & mscore, const double epsi, const size_t M);

    /**
     * Down Sampled Epsilon Lexicase Selector:
     *
     * This function will iterate through a subset of individual testcases.
     * While filtering down the population on each one, only taking the top performers.
     * The top performers must be some within some distance of the top performance to pass.
     *
     * In the event of ties on the last testcase being used, a solution will be selected at random.
     *
     * @param mscore Matrix of solution fitnesses (must be the same amount of fitness per solution)(mscore.size => # of orgs).
     * @param epsi Epsilon threshold value.
     * @param t_cases vector of testcases to select from.
     *
     * @return A single winning solution id.
     */
    size_t DSELexicase(const fmatrix_t & mscore, const double epsi, const ids_t & t_cases);

    /**
     * Cohort Epsilon Lexicase Selector:
     *
     * This function will iterate through a testcase and population cohorts in pairs.
     * While filtering down the population cohort with its paired paired testcase cohort.
     * Solutions within an epsilon distance from the optimal will pass through the filter.
     *
     * In the event of ties on the last testcase being used, a solution will be selected at random.
     *
     * @param mscore Matrix of solution fitnesses (must be the same amount of fitness per solution)(mscore.size => # of orgs).
     * @param epsi Epsilon threshold value.
     * @param pop_coh Vector containing population ids.
     * @param test_coh Vector containing testcase ids.
     *
     * @return A single winning solution id.
     */
    size_t CELexicase(const fmatrix_t & mscore, const double epsi, const ids_t & pop_coh, const ids_t & test_coh);

  private:

    // random pointer from world.h
    emp::Ptr<emp::Random> random;
};

///< population structure

Selection::neigh_t Selection::FitNearestN(const score_t & score, const size_t K)
{
  // quick checks
  emp_assert(0 < score.size()); emp_assert(K < score.size());

  // create group vector returning
  neigh_t group(score.size());

  // create vector to sort scores and generate score neighborhood pairings
  // <position id in orginal score vector, original score vector value>
  sorted_t order;

  // populate the vector with pairings + sort it based on scores
  for(size_t i = 0; i < score.size(); ++i){order.emplace_back(i, score[i]);}
  std::sort(order.begin(), order.end(), [](const auto &left, const auto &right) {
    return left.second < right.second;
  });

  emp_assert(order.size() == score.size());

  // walk through sorted vector and create the k neighborhood per score value
  for(size_t i = 0; i < order.size(); ++i)
  {
    emp::vector<double> neigh;
    int left = i - 1; int right = i + 1;

    // generate the neighborhood
    while(neigh.size() != K)
    {
      //quick checks
      emp_assert(!(left < 0 && order.size() <= right));

      // reached all left neighbors possible for i
      if(left < 0)
      {
        neigh.push_back(order[right].second);
        ++right;
      }
      // reached all right neighbors possible for i
      else if (order.size() <= right)
      {
        neigh.push_back(order[left].second);
        --left;
      }
      // we find the closes neighbor to score i, between left and right
      else
      {
        // look at distances
        const double ld = Distance(order[i].second, order[left].second);
        const double rd = Distance(order[i].second, order[right].second);

        // left is smaller distance
        if(ld < rd) {neigh.push_back(order[left].second); --left;}
        else {neigh.push_back(order[right].second); ++right;}
      }
    }
    // quick checks
    emp_assert(neigh.size() == K);

    // we now have the add vector into right location
    group[order[i].first] = neigh;
  }

  return group;
}
Selection::fitgp_t Selection::FitnessGroup(const score_t & score)
{
  // quick checks
  emp_assert(0 < score.size());

  // place all solutions in map based on score
  fitgp_t group;
  for(size_t i = 0; i < score.size(); ++i)
  {
    // didn't find in group
    if(group.find(score[i]) == group.end())
    {
      ids_t p{i};
      group[score[i]] = p;
    }
    else{group[score[i]].push_back(i);}
  }

  return group;
}

Selection::cohort_t Selection::CohortGeneration(const size_t N, const double P)
{
  // quick checks
  emp_assert(0 < N); emp_assert(0 < P);

  // number of objects in a cohort
  const double proportion = static_cast<double>(N) * P;
  const size_t coh_size = static_cast<size_t>(proportion);
  // total number of chorts
  const size_t coh_num = N / coh_size;
  // check to see that there is an even split
  emp_assert(coh_size * coh_num == N);

  // initialize the number of cohorts and their size
  // initialize with N, we know that the position N is outside the range of N
  cohort_t cohorts(coh_num);
  for(auto & c : cohorts) {c.resize(coh_size, N);}

  // initialize position ids and shuffle
  ids_t pop(N);
  std::iota(pop.begin(), pop.end(), 0);
  emp::Shuffle(*random, pop);

  // distrubute the shuffled population ids into the cohorts
  emp_assert(cohorts.size() == coh_num);
  size_t pop_i = 0;
  for(auto & coh : cohorts)
  {
    emp_assert(coh.size() == coh_size);
    for(size_t c = 0; c < coh.size(); ++c, ++pop_i)
    {
      coh[c] = pop[pop_i];
    }
  }

  // make sure that pop_i reached the end
  emp_assert(pop_i == N);

  return cohorts;
}


///< fitness transformation >///

Selection::score_t Selection::FitnessSharing(const fmatrix_t & dmat, const score_t & score, const double alph, const double sig)
{
  // quick checks
  emp_assert(dmat.size() == score.size()); emp_assert(0 <= alph); emp_assert(0 <= sig);
  emp_assert(0 < dmat.size()); emp_assert(0 < score.size());

  score_t tscore(score.size());

  for(size_t i = 0; i < dmat.size(); ++i)
  {
    // quick checks
    emp_assert(dmat[i].size() == score.size());

    // mi value that holds niche count (or scaling factor)
    // we can start at 1 because sh(d_ij) (where i == j), will equal 1.0
    double mi = 1.0;

    for(size_t j = 0; j < dmat.size(); ++j)
    {
      // ignore similar pairs, already accounted for
      if(i == j) {continue;}

      // only look at lower triangular diagonal values
      if(j < i)
      {
        emp_assert(dmat[i][j] != ERROR_VALD);
        mi += SharingFunction(dmat[i][j], sig, alph);
      }
      else
      {
        emp_assert(dmat[j][i] != ERROR_VALD);
        mi += SharingFunction(dmat[j][i], sig, alph);
      }
    }

    tscore[i] = score[i] / mi;
  }

  return tscore;
}
double Selection::SharingFunction(const double dist, const double sig, const double alph)
{
  // quick checks
  emp_assert(0.0 <= dist); emp_assert(0.0 <= sig); emp_assert(0.0 <= alph);

  if(dist < sig)
  {
    // right side of equation
    const double rse = std::pow((dist/sig), alph);
    return 1.0 - rse;
  }

  return 0.0;
}

Selection::score_t Selection::Novelty(const score_t & score, const neigh_t & neigh, const size_t K)
{
  // quick checks
  emp_assert(score.size() == neigh.size()); emp_assert(0 <= K);
  emp_assert(0 < score.size()); emp_assert(0 < neigh.size());

  // novelty score transformed from orignial scores
  score_t nscore(score.size());

  // edge case where K == 0
  if(K == 0){
    std::copy(score.begin(), score.end(), nscore.begin());

    // quick checks and return the score vector unaltered
    emp_assert(score[0] == nscore[0]);
    return nscore;
  }

  // iterate through both score and neighborhood vectors
  for(size_t i = 0; i < score.size(); ++i)
  {
    // quick checks
    emp_assert(neigh[i].size() == K);

    // keep track of distances
    double numer = 0.0;

    // euclidean distance for two points
    for(size_t k = 0; k < K; ++k){numer += Distance(score[i], neigh[i][k]);}

    // calculate the average
    const double denom = static_cast<double>(K);

    nscore[i] = numer / denom;
  }

  return nscore;
}

Selection::fmatrix_t Selection::LexicaseNoveltyFit(const fmatrix_t & mscore, const size_t K, const size_t M)
{
  // quick checks
  emp_assert(0 < mscore.size()); emp_assert(0 <= K);

  // initialize transformed score matrix to number of solutions
  fmatrix_t tscore(mscore.size());
  // resize all vectors to hold current fitness performance
  for(auto & s : tscore){s.resize(M);}
  // store all fitness performances in fitness & novelty matrix
  for(size_t sol = 0; sol < mscore.size(); ++sol)
  {
    for(size_t test = 0; test < M; ++test) {tscore[sol][test] = mscore[sol][test];}
  }

  // if k = 0, return as is
  if(K == 0) {return tscore;}

  // iterate through testcases individually and send them off for grouping
  for(size_t test = 0; test < M; ++test)
  {
    // iterate through all solutions and store their testcase performance
    score_t tc_score(mscore.size());
    for(size_t sol = 0; sol < mscore.size(); ++sol){tc_score[sol] = mscore[sol][test];}

    // construct the fitness k-nearest neighbor groupings
    neigh_t neighbor = FitNearestN(tc_score, K);
    // transform the fitness values into novelty values
    score_t transform = Novelty(tc_score, neighbor, K);

    // store novelty score in tranformed matrix (fitness and novelty values)
    emp_assert(transform.size() == tscore.size());
    for(size_t sol = 0 ; sol < tscore.size(); ++sol){tscore[sol].push_back(transform[sol]);}
  }

  emp_assert(SizeCheck(tscore, M));

  return tscore;
}


///< selector functions

Selection::ids_t Selection::MLSelect(const size_t mu, const size_t lambda, const fitgp_t & group)
{
  // quick checks
  emp_assert(0 < mu); emp_assert(0 < lambda);
  emp_assert(mu <= lambda); emp_assert(0 < group.size());

  // in the event that we are asking for the whole population
  // just return the vector of ids from 0 to pop size (lambda)
  if(mu == lambda)
  {
    ids_t pop;
    for(const auto & g : group)
    {
      for(const auto & id : g.second)
      {
        pop.push_back(id);
      }
    }

    emp_assert(pop.size() == lambda);
    return pop;
  }

  // go through the ordered scores and get our top mu solutions
  ids_t topmu;
  for(auto & g : group)
  {
    auto gt = g.second;
    emp::Shuffle(*random, gt);
    for(auto id : gt)
    {
      topmu.push_back(id);
      if(topmu.size() == mu) {break;}
    }

    if(topmu.size() == mu) {break;}
  }

  // insert the correct amount of ids
  ids_t parent;
  size_t lm = lambda / mu;
  for(auto id : topmu)
  {
    for(size_t i = 0; i < lm; ++i){parent.push_back(id);}
  }

  emp_assert(parent.size() == lambda);

  return parent;
}

size_t Selection::Tournament(const size_t t, const score_t & score)
{
  // quick checks
  emp_assert(0 < t); emp_assert(0 < score.size());
  emp_assert(t <= score.size());

  // get tournament ids
  emp::vector<size_t> tour = emp::Choose(*random, score.size(), t);

  // store all scores for the tournament
  emp::vector<double> subscore(t);
  for(size_t i = 0; i < tour.size(); ++i) {subscore[i] = score[tour[i]];}

  // group scores by fitness and position;
  auto group = FitnessGroup(subscore);
  emp::vector<size_t> opt = group.begin()->second;

  //shuffle the vector with best fitness ids
  emp::Shuffle(*random, opt);
  emp_assert(0 < opt.size());

  return tour[opt[0]];
}

size_t Selection::Drift(const size_t size)
{
  // quick checks
  emp_assert(size > 0);

  // return a random org id
  auto win = emp::Choose(*random, size, DRIFT_SIZE);
  emp_assert(win.size() == DRIFT_SIZE);

  return win[0];
}

size_t Selection::EpsiLexicase(const fmatrix_t & mscore, const double epsi, const size_t M)
{
  // quick checks
  emp_assert(0 < mscore.size()); emp_assert(0 <= epsi); emp_assert(0 < M);

  // create vector of shuffled testcase ids
  ids_t test_id(M);
  std::iota(test_id.begin(), test_id.end(), 0);
  emp::Shuffle(*random, test_id);

  // vector to hold filterd elite solutions
  ids_t filter(mscore.size());
  std::iota(filter.begin(), filter.end(), 0);

  // iterate through testcases until we run out or have a single winner
  size_t tcnt = 0;
  while(tcnt < M && filter.size() != 1)
  {
    // testcase we are randomly evaluating
    size_t testcase = test_id[tcnt];

    // create vector of current filter solutions
    score_t scores(filter.size());
    for(size_t i = 0; i < filter.size(); ++i)
    {
      // make sure each solutions vector is the right size
      emp_assert(mscore[filter[i]].size() == M);
      scores[i] = mscore[filter[i]][testcase];
    }

    // group org ids by performance in descending order
    fitgp_t group = FitnessGroup(scores);

    // update the filter vector with pop ids that are worthy
    ids_t temp_filter = filter;
    filter.clear();
    for(const auto & p : group)
    {
      if(Distance(group.begin()->first, p.first) <= epsi)
      {
        for(auto id : p.second)
        {
          filter.push_back(temp_filter[id]);
        }
      }
      else{break;}
    }

    ++tcnt;
  }

  // Get a random position from the remaining filtered solutions (may be one left too)
  emp_assert(0 < filter.size());
  size_t wid = emp::Choose(*random, filter.size(), 1)[0];

  return filter[wid];
}

size_t Selection::DSELexicase(const fmatrix_t & mscore, const double epsi, const ids_t & t_cases)
{
  // quick checks
  emp_assert(0 < mscore.size()); emp_assert(0.0 <= epsi);
  emp_assert(0 < t_cases.size());

  // create a vector of shuffled testcase ids
  ids_t test_id(t_cases.size());
  std::iota(test_id.begin(), test_id.end(), 0);
  emp::Shuffle(*random, test_id);

  // create vector to hold filtered elite solutions
  ids_t filter(mscore.size());
  std::iota(filter.begin(), filter.end(), 0);

  // iterate through testcases until we are out or have a winner
  size_t tcnt = 0;
  while(tcnt < t_cases.size() && filter.size() != 1)
  {
    // testcase we are randomly evaluating
    size_t testcase = t_cases[test_id[tcnt]];

    // create vector of current filter solutions
    score_t scores(filter.size());
    for(size_t i = 0; i < filter.size(); ++i)
    {
      // make sure each solutions vector is the right size
      scores[i] = mscore[filter[i]][testcase];
    }

    // group org ids by performance in descending order
    fitgp_t group = FitnessGroup(scores);

    // update the filter vector with pop ids that are worthy
    ids_t temp_filter = filter;
    filter.clear();
    for(const auto & p : group)
    {
      if(Distance(group.begin()->first, p.first) <= epsi)
      {
        for(auto id : p.second)
        {
          filter.push_back(temp_filter[id]);
        }
      }
      else{break;}
    }

    ++tcnt;
  }

  // Get a random position from the remaining filtered solutions (may be one left too)
  emp_assert(0 < filter.size());
  size_t wid = emp::Choose(*random, filter.size(), 1)[0];

  return filter[wid];
}

size_t Selection::CELexicase(const fmatrix_t & mscore, const double epsi, const ids_t & pop_coh, const ids_t & test_coh)
{
  // quick checks
  emp_assert(0 < mscore.size()); emp_assert(0 <= epsi);
  emp_assert(0 < pop_coh.size()); emp_assert(0 < test_coh.size());

  // create a vector of shuffled testcase ids
  ids_t test_id(test_coh.size());
  std::iota(test_id.begin(), test_id.end(), 0);
  emp::Shuffle(*random, test_id);

  // create vector to hold filtered elite solutions from pop_coh
  ids_t filter(pop_coh.size());
  std::iota(filter.begin(), filter.end(), 0);

  // iterate through testcases until we are out or have a winner
  size_t tcnt = 0;
  while(tcnt < test_coh.size() && filter.size() != 1)
  {
    // testcase we are randomly evaluating
    size_t testcase = test_coh[test_id[tcnt]];

    // create vector of current filter solutions
    score_t scores(filter.size());
    for(size_t i = 0; i < filter.size(); ++i)
    {
      // grab solution from pop_coh
      const size_t sol = pop_coh[filter[i]];

      // make sure each solutions vector is the right size
      scores[i] = mscore[sol][testcase];
    }

    // group org ids by performance in descending order
    fitgp_t group = FitnessGroup(scores);

    // update the filter vector with pop ids that are worthy
    ids_t temp_filter = filter;
    filter.clear();
    for(const auto & p : group)
    {
      if(Distance(group.begin()->first, p.first) <= epsi)
      {
        for(auto id : p.second)
        {
          filter.push_back(temp_filter[id]);
        }
      }
      else{break;}
    }

    ++tcnt;
  }

  // Get a random position from the remaining filtered solutions (may be one left too)
  emp_assert(0 < filter.size());
  size_t wid = emp::Choose(*random, filter.size(), 1)[0];

  return pop_coh[filter[wid]];
}

///< helper functions

double Selection::Pnorm(const score_t & x, const score_t & y, const double exp)
{
  // quick checks
  emp_assert(0 < x.size()); emp_assert(0 < y.size());
  emp_assert(x.size() == y.size()); emp_assert(0 <= exp);

  // subtract one vector from the other and take the exp of that
  score_t diff(x.size());
  for(size_t i = 0; i < x.size(); ++i){diff[i] = std::pow((x[i] - y[i]), exp);}

  double tot = std::accumulate(diff.begin(), diff.end(), 0.0);

  return std::pow(tot, (1.0/exp));
}

Selection::fmatrix_t Selection::SimilarityMatrix(const gmatrix_t & genome, const double exp)
{
  // quick checks
  emp_assert(1 <= exp); emp_assert(1 < genome.size());

  // generate the matrix, lower diagnonal matrix filled (not include i == j)
  fmatrix_t similar(genome.size());
  for(auto & s : similar) {s.resize(genome.size(), ERROR_VALD);}

  for(size_t i = 0; i < genome.size(); ++i)
  {
    for(size_t j = 0; j < i; ++j)
    {
      similar[i][j] = Pnorm(genome[i], genome[j], exp);
    }
  }

  return similar;
}

#endif