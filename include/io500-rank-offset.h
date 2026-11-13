#ifndef IO500_RANK_OFFSET_H
#define IO500_RANK_OFFSET_H

#include <stdint.h>
#include <mpi.h>

/*
 * Read-back rank offsets.
 *
 * The read, stat and delete phases make every rank access the data that
 * another rank wrote, so that the access is not served from the cache (or
 * local storage) of the node that wrote the data.  IOR (-C -Q <v>) and
 * mdtest (-N <v>) compute the rank whose data is accessed from <v> as:
 *
 *   IOR    read:          (r + (v * u_ior) % N) % N
 *          u_ior    = ppn0 if rank 1 is on the same host as rank 0, else 1
 *   mdtest stat/read/rm:  (r + k * v * u_mdtest) % N, k = 1, 2, 3
 *          u_mdtest = ppn0 if nodes > 1 and rank 1 is on the same host as
 *                     rank 0, else 1
 *
 * where N is the number of ranks and ppn0 the number of ranks on the node
 * of rank 0 (see GetNumTasksOnNode0(), GetNumNodes(), QueryNodeMapping() in
 * IOR's utilities.c, and the IOR_FAKE_* environment overrides there).
 *
 * The value <v> for each phase is drawn at random by rank 0 at the start of
 * that phase and checked against the real rank->node placement, so that it
 * cannot be known while the data is written.  Each data set (ior-easy,
 * ior-hard, mdtest-easy, mdtest-hard) is accessed by up to 3 phases after
 * it is written, and a node may cache what it wrote, stat-ed or read.  So
 * <v> must make every rank access data that its node has neither written
 * nor accessed in an earlier phase.  This needs 2 nodes for ior-hard, 3
 * for ior-easy and 4 for mdtest.  A phase for which no such <v> exists is
 * marked INVALID, but still runs.
 */

/* most rank shifts kept for one data set, the writer's 0 included */
#define IO500_RANK_OFFSET_MAX_SHIFTS 16

typedef enum {
  IO500_OFFSET_IOR,
  IO500_OFFSET_MDTEST,
} io500_offset_tool_t;

typedef struct {
  int nranks;
  int nnodes;      /* real number of nodes (MPI_COMM_TYPE_SHARED) */
  int ppn_min;     /* fewest ranks on any node */
  int ppn_max;     /* most ranks on any node */
  int *node;       /* node index of each rank, only valid on rank 0 */
  /* what IOR and mdtest compute internally for the same communicator */
  int block_mapping; /* QueryNodeMapping(): rank 1 on the host of rank 0 */
  int ppn0;          /* GetNumTasksOnNode0() */
  int tool_nnodes;   /* GetNumNodes() */
} u_node_table_t;

/* collective: build the rank->node table, call once after MPI_Init() */
void u_node_table_init(MPI_Comm comm);
u_node_table_t const * u_node_table(void);
/* print the node layout into the [run] section, rank 0 only */
void u_node_table_print(void);

/*
 * Pure helpers, independent of MPI, so they can be tested with fake tables.
 */
/* rank distance that IOR/mdtest multiply <v> by */
int u_rank_offset_unit(u_node_table_t const * t, io500_offset_tool_t tool);
/* effective rank shift for <v> and multiplier k, -1 if it overflows an int */
int u_rank_offset_shift(u_node_table_t const * t, io500_offset_tool_t tool, int v, int k);
/* 1 if, for each k in mult[], no rank accesses data written on its node */
int u_rank_offset_cross_node(u_node_table_t const * t, io500_offset_tool_t tool, int v, int const * mult, int nmult);
/*
 * 1 if no rank is on a node that wrote or accessed the data it accesses
 * with this rank shift.  Rank r accesses the data written by rank
 * r + shift, which earlier phases accessed from rank r + shift - prev[i];
 * prev[] holds the writer's shift 0 and the shifts of those phases.
 */
int u_rank_offset_uncached(u_node_table_t const * t, int shift, int const * prev, int nprev);
/*
 * Choose a random valid <v> using the random state *seed, for a phase
 * that accesses a data set with multiplier mult[0] after the phases with
 * the rank shifts prev[].  mult[1..nmult-1] are the later phases on the
 * same data set.  With same_v they use the same <v> (the mdtest -u
 * directory layout), which must then be valid for all of them.  Otherwise
 * each draws its own, and a <v> that still leaves them a valid choice is
 * preferred.  Returns -1 if no <v> is valid for mult[0].
 */
int u_rank_offset_choose(u_node_table_t const * t, io500_offset_tool_t tool, int const * prev, int nprev, int const * mult, int nmult, int same_v, uint64_t * seed);

/*
 * Rank 0 keeps the rank shifts of the phases that accessed each data set,
 * also in <resdir>/<dataset>.rank-shift for later runs reusing the result
 * directory.
 */
/* the data set is (re)written, forget earlier accesses */
void u_rank_offset_written(char const * dataset);
/* remove the saved rank shifts of the data set */
void u_rank_offset_cleanup(char const * dataset);
/*
 * Collective: rank 0 draws a fresh random <v> with u_rank_offset_choose()
 * and broadcasts it.  If none avoids every earlier access, it falls back
 * to a <v> that avoids the writer's node, or else to dflt;
 * u_rank_offset_check() reports this.
 */
int u_rank_offset_draw(char const * dataset, io500_offset_tool_t tool, int const * mult, int nmult, int same_v, int dflt);
/*
 * Record <v> and the resulting rank shift for multiplier k in the phase
 * section of the result file, flag the phase INVALID if some rank
 * accesses data that its node wrote or accessed before, and add the shift
 * to the data set's accesses.  Rank 0 only.
 */
void u_rank_offset_check(char const * dataset, io500_offset_tool_t tool, int v, int k);
/* collective: u_rank_offset_draw() and u_rank_offset_check() for mult[0] */
int u_rank_offset_phase(char const * dataset, io500_offset_tool_t tool, int const * mult, int nmult, int dflt);

#endif
