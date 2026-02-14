/**CFile****************************************************************

  FileName    [cecInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Combinational equivalence checking.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cecInt.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__cec__cecInt_h
#define ABC__aig__cec__cecInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "sat/bsat/satSolver.h"
#include "sat/bsat/satSolver2.h" // Added
#include "sat/glucose2/AbcGlucose2.h"
#include "misc/bar/bar.h"
#include "aig/gia/gia.h"
#include "aig/gia/giaAig.h"
#include "cec.h"
#include <math.h>

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// simulation pattern manager
typedef struct Cec_ManPat_t_ Cec_ManPat_t;
struct Cec_ManPat_t_
{
    Vec_Int_t *      vPattern1;      // pattern in terms of primary inputs
    Vec_Int_t *      vPattern2;      // pattern in terms of primary inputs
    Vec_Str_t *      vStorage;       // storage for compressed patterns
    int              iStart;         // position in the array where recent patterns begin
    int              nPats;          // total number of recent patterns
    int              nPatsAll;       // total number of all patterns
    int              nPatLits;       // total number of literals in recent patterns
    int              nPatLitsAll;    // total number of literals in all patterns
    int              nPatLitsMin;    // total number of literals in minimized recent patterns
    int              nPatLitsMinAll; // total number of literals in minimized all patterns
    int              nSeries;        // simulation series
    int              fVerbose;       // verbose stats
    // runtime statistics
    abctime          timeFind;       // detecting the pattern  
    abctime          timeShrink;     // minimizing the pattern
    abctime          timeVerify;     // verifying the result of minimisation
    abctime          timeSort;       // sorting literals 
    abctime          timePack;       // packing into sim info structures 
    abctime          timeTotal;      // total runtime  
    abctime          timeTotalSave;  // total runtime for saving  
};

// SAT solving manager
typedef struct Cec_ManSat_t_ Cec_ManSat_t;
struct Cec_ManSat_t_
{
    // parameters
    Cec_ParSat_t *   pPars;          
    // AIGs used in the package
    Gia_Man_t *      pAig;           // the AIG whose outputs are considered
    Vec_Int_t *      vStatus;        // status for each output
    // SAT solving
    sat_solver *     pSat;           // recyclable SAT solver (MiniSAT)
    bmcg2_sat_solver*pSat2;          // recyclable SAT solver (Glucose)
    int              nSatVars;       // the counter of SAT variables
    int *            pSatVars;       // mapping of each node into its SAT var
    Vec_Ptr_t *      vUsedNodes;     // nodes whose SAT vars are assigned
    int              nRecycles;      // the number of times SAT solver was recycled
    int              nCallsSince;    // the number of calls since the last recycle
    Vec_Ptr_t *      vFanins;        // fanins of the CNF node
    // counter-examples
    Vec_Int_t *      vCex;           // the latest counter-example
    Vec_Int_t *      vVisits;        // temporary array for visited nodes  
    // SAT calls statistics
    int              nSatUnsat;      // the number of proofs
    int              nSatSat;        // the number of failure
    int              nSatUndec;      // the number of timeouts
    int              nSatTotal;      // the number of calls
    int              nCexLits;
    // conflicts
    int              nConfUnsat;     // conflicts in unsat problems
    int              nConfSat;       // conflicts in sat problems
    int              nConfUndec;     // conflicts in undec problems
    // runtime stats
    int              timeSatUnsat;   // unsat
    int              timeSatSat;     // sat
    int              timeSatUndec;   // undecided
    int              timeTotal;      // total runtime
};

// combinational simulation manager
typedef struct Cec_ManSim_t_ Cec_ManSim_t;
struct Cec_ManSim_t_
{
    // parameters
    Gia_Man_t *      pAig;           // the AIG to be used for simulation
    Cec_ParSim_t *   pPars;          // simulation parameters 
    int              nWords;         // the number of simulation words
    // recycable memory
    int *            pSimInfo;       // simulation information offsets
    unsigned *       pMems;          // allocated simulaton memory
    int              nWordsAlloc;    // the number of allocated entries
    int              nMems;          // the number of used entries  
    int              nMemsMax;       // the max number of used entries 
    int              MemFree;        // next free entry
    int              nWordsOld;      // the number of simulation words after previous relink
    // internal simulation info
    Vec_Ptr_t *      vCiSimInfo;     // CI simulation info  
    Vec_Ptr_t *      vCoSimInfo;     // CO simulation info  
    // counter examples
    void **          pCexes;         // counter-examples for each output
    int              iOut;           // first failed output
    int              nOuts;          // the number of failed outputs
    Abc_Cex_t *      pCexComb;       // counter-example for the first failed output
    Abc_Cex_t *      pBestState;     // the state that led to most of the refinements
    // scoring simulation patterns
    int *            pScores;        // counters of refinement for each pattern
    // temporaries
    Vec_Int_t *      vClassOld;      // old class numbers
    Vec_Int_t *      vClassNew;      // new class numbers
    Vec_Int_t *      vClassTemp;     // temporary storage
    Vec_Int_t *      vRefinedC;      // refined const reprs
};

// combinational simulation manager
typedef struct Cec_ManFra_t_ Cec_ManFra_t;
struct Cec_ManFra_t_
{
    // parameters
    Gia_Man_t *      pAig;           // the AIG to be used for simulation
    Cec_ParFra_t *   pPars;          // SAT sweeping parameters 
    // simulation patterns
    Vec_Int_t *      vXorNodes;      // nodes used in speculative reduction
    int              nAllProved;     // total number of proved nodes
    int              nAllDisproved;  // total number of disproved nodes
    int              nAllFailed;     // total number of failed nodes
    int              nAllProvedS;    // total number of proved nodes
    int              nAllDisprovedS; // total number of disproved nodes
    int              nAllFailedS;    // total number of failed nodes
    // runtime stats
    abctime          timeSim;        // unsat
    abctime          timePat;        // unsat
    abctime          timeSat;        // sat
    abctime          timeTotal;      // total runtime
};

typedef struct Cec_ManAff_t_ Cec_ManAff_t;
struct Cec_ManAff_t_
{
    Vec_Int_t *      TT_info;      // [TT] = (piv_id, A_id to piv, b to piv) = (12bit, 16 bit, 4bit)
    // Vec_Int_t *      matrix_inv;   // A^-1
    Vec_Wec_t *      piv_Aid;      // [piv_id_1][piv_id_2] = (cost, A_id, b) = (16bit, 4bit)
    Vec_Ptr_t *      ops;          // ops[A_id] would have the best ops for A, store as Vec_Bit_t*
    // int              nClass;
    int              nVar;

};

#define CEC_FD_SAT 0
#define CEC_FD_UNSOLVE -1
#define CEC_FD_IGNORE -2
#define CEC_FD_UNKNOWN -3
#define CEC_FD_LOCK -4
#define CEC_FD_HUGE -5
#define CEC_FD_COVER -6
#define CEC_FD_TRIVIAL -7
#define CEC_FD_UNSHRINKABLE -8
typedef struct Cec_ManFd_t_ Cec_ManFd_t;
struct Cec_ManFd_t_
{
    Cec_ParFd_t *    pPars; 

    Gia_Man_t *      pGia;           // the AIG to be used for FD
    Gia_Obj_t *      pObj1;
    Gia_Obj_t *      pObj2;
    int              fPhase;         // 1 if we add XOR on pObj1, pObj2 to get pGia 
    int**            dist_long;      // dist[j][i], j > i
    int**            dist_short;    // dist[j][i], j > i
    Vec_Int_t *      vMerge;         // merge frontier
    Vec_Int_t *      vNodeInit;     // merge frontier in the begin
    
    Gia_Man_t *      pAbs;
    Gia_Obj_t *      pObjAbs1;
    Gia_Obj_t *      pObjAbs2;           
    Vec_Int_t *      vAbsMap;            // map from pGia to pAbs
    Vec_Int_t *      vGiaMap;            // map from pAbs to pGia 
    int**            dist_long_abs;
    int**            dist_short_abs;
    
    Vec_Wec_t *      vMergeSupport;      // support vMerge for each node in pGia (set as binary form)
    Vec_Wec_t *      vGSupport;          // support G for each node in pGia
    
    Vec_Wec_t *      vCuts;           // cuts on pAbs for each node
    // Vec_Int_t *      vLevel;          // level for each node in pGia
    Vec_Int_t *      vLevel1;          // level for each node in ckt1
    Vec_Int_t *      vLevel2;          // level for each node in ckt2
    Vec_Flt_t *      vCostTh;         // cost threshold

    sat_solver2 *    pSat;
    Vec_Int_t *      vNdMap;         // map from sat var to node id
    Vec_Int_t *      vVarMap;        // map from node id to sat var
    Vec_Wec_t *      vClauses;       // clauses to be added to sat solver, store as nd vars, require further translate
    int              nClauses;
    int              nVars;

    Vec_Int_t *      vStat;          // -1 for UNSOLVE, 0 for SAT, >0 for the iter that UNSAT
    Vec_Ptr_t *      vVeryStat;      // Info store as Gia_Man_t*, UNSAT: patch
    Vec_Int_t *      vConf;          // conflict that is required
    int              iter;

};
////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== cecCorr.c ============================================================*/
extern void                 Cec_ManRefinedClassPrintStats( Gia_Man_t * p, Vec_Str_t * vStatus, int iIter, abctime Time );
/*=== cecClass.c ============================================================*/
extern int                  Cec_ManSimClassRemoveOne( Cec_ManSim_t * p, int i );
extern int                  Cec_ManSimClassesPrepare( Cec_ManSim_t * p, int LevelMax );
extern int                  Cec_ManSimClassesRefine( Cec_ManSim_t * p );
extern int                  Cec_ManSimSimulateRound( Cec_ManSim_t * p, Vec_Ptr_t * vInfoCis, Vec_Ptr_t * vInfoCos );
/*=== cecIso.c ============================================================*/
extern int *                Cec_ManDetectIsomorphism( Gia_Man_t * p );
/*=== cecMan.c ============================================================*/
extern Cec_ManSat_t *       Cec_ManSatCreate( Gia_Man_t * pAig, Cec_ParSat_t * pPars );
extern void                 Cec_ManSatPrintStats( Cec_ManSat_t * p );
extern void                 Cec_ManSatStop( Cec_ManSat_t * p );
extern Cec_ManPat_t *       Cec_ManPatStart();
extern void                 Cec_ManPatPrintStats( Cec_ManPat_t * p );
extern void                 Cec_ManPatStop( Cec_ManPat_t * p );
extern Cec_ManSim_t *       Cec_ManSimStart( Gia_Man_t * pAig, Cec_ParSim_t *  pPars ); 
extern void                 Cec_ManSimStop( Cec_ManSim_t * p );  
extern Cec_ManFra_t *       Cec_ManFraStart( Gia_Man_t * pAig, Cec_ParFra_t *  pPars );  
extern void                 Cec_ManFraStop( Cec_ManFra_t * p );

/*=== cecMan.c (currently) ===============================================*/
// compute gia information
extern void                 Cec_ManSetColor( Gia_Man_t * pGia, Gia_Obj_t * pObj1, Gia_Obj_t * pObj2);
extern int**                Cec_ManDist( Gia_Man_t* pGia, int fMax );
extern void                 Cec_ManDistEasy( Gia_Man_t* pGia, Vec_Int_t* vFI, int nidFo, int fMax );
extern Vec_Wec_t*           Cec_ManDistAnalyze( Gia_Man_t* pGia, Vec_Int_t* targets, Vec_Int_t* vFI, Vec_Int_t* vFO, int** dist_long, int** dist_short);
extern Vec_Int_t*           Cec_ManGetFIO( Gia_Man_t* pGia, Vec_Int_t* targets, int fPickFO );
extern void                 Cec_ManMatch( Gia_Man_t* pGia, Gia_Man_t* pRef );
extern void                 Cec_ManGetCuts( Gia_Man_t* pGia, Gia_Obj_t* t, Gia_Obj_t* s, Vec_Int_t* vCare, Vec_Wec_t* vCuts );
extern void                 Cec_ManGetMerge_old( Gia_Man_t* pGia, Gia_Obj_t* pObj1, Gia_Obj_t* pObj2, Vec_Int_t* vMerge );
extern void                 Cec_ManGetMerge( Gia_Man_t* pGia, Vec_Int_t* vNid1, Vec_Int_t* vNid2, Vec_Int_t* vMerge, int fLow, int fSep );
extern Vec_Flt_t*           Cec_ManRecPathLen( Gia_Man_t* pGia_in, Vec_Int_t* vF, Vec_Int_t* vG, int fMax, int fType, int fDownward );
extern void                 Cec_ManReduceFrontier( Gia_Man_t* pGia, Vec_Int_t* vFrt, int limMffc, int limNum );
// get subcircuits
extern Vec_Ptr_t*           Cec_ManSubcircuit( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vLeaveId, Vec_Int_t * vCheckTFO, int fSetVal, int fGetNodes);
extern unsigned int         Cec_ManPatch( Gia_Man_t* pNew, Vec_Ptr_t* cone );
extern Gia_Man_t*           Cec_ManGetSubcircuit( Gia_Man_t* pGia, Vec_Int_t* vFI, Vec_Int_t* vFO, int fRed );
extern Gia_Man_t*           Cec_ManGetTFI( Gia_Man_t* pGia, Vec_Ptr_t* vObj, int fRedCi );
extern Vec_Int_t*           Cec_ManTFISize( Gia_Man_t* pGia );
extern Gia_Man_t*           Cec_ManGetAbs( Gia_Man_t* pGia, Gia_Obj_t* pObj1, Gia_Obj_t* pObj2, Vec_Int_t* vFI, int fVerbose );
extern Gia_Man_t*           Cec_ManPatchCleanSupport( Gia_Man_t* pGia, Vec_Int_t* vSupport );
extern Gia_Man_t*           Cec_ManSurrounding( Gia_Man_t* pGia, Vec_Int_t* vNd, int nHeight, int fMerge );
// syn
extern Gia_Man_t*           Cec_ManReplacePatch( Gia_Man_t* pGia, Gia_Man_t* pPatch, Vec_Int_t* vFO_in, Vec_Int_t* vFI_in, int fStage, int fVal );
extern Gia_Man_t*           Cec_ManFuncDepReplace( Gia_Man_t* pGia, Vec_Int_t* vF, Vec_Int_t* vG, Vec_Int_t* vMerge, Vec_Int_t* vCoef );
extern Gia_Man_t*           Cec_ManSyn( Gia_Man_t* pGia, Vec_Int_t* vCoef );
extern Gia_Man_t*           Cec_ManSimpSyn( Gia_Man_t* pGia, int fSat, int fVerbose, int nRuns );
extern Gia_Man_t*           Cec_ManPartSyn( Gia_Man_t* pGia, Vec_Int_t* vFI_in, Vec_Int_t* vFO_in, int nDc2, int fVerbose );
extern Gia_Man_t*           Cec_ManFraigSyn( Gia_Man_t* pGia, Gia_Obj_t* pObj1, Gia_Obj_t* pObj2, int fComp, int fVerbose, int nConflict, int fSimp );
extern Gia_Man_t*           Cec_ManFraigSyn_naive( Gia_Man_t* pGia, int nConflict );
extern void                 Cec_ManShowValNode( Gia_Man_t* pGia, char* pFileName );
// functional dependency setting
extern void                 Cec_ManFdDumpStat( Cec_ManFd_t* pMan, char* pFileName );
extern void                 Cec_ManFdReadStat( Cec_ManFd_t* pMan, char* pFileName, int fClear );
extern void                 Cec_ManFdAddClause( Cec_ManFd_t* pMan, sat_solver2* p );
extern void                 Cec_ManFdPrepareSolver( Cec_ManFd_t* pMan );
extern Cec_ManFd_t*         Cec_ManFdStart( Gia_Man_t* pGia, Cec_ParFd_t* pPars, char* pFileName );
extern void                 Cec_ManFdUpdate( Cec_ManFd_t* pMan, Gia_Man_t* p );
extern void                 Cec_ManFdStop( Cec_ManFd_t* pMan, int fAll );
// functional dependency computation
extern Gia_Man_t*           Cec_ManFdGetFd_Inc( Cec_ManFd_t* pMan, int nidGlobal, Vec_Int_t* vFdSupportGlobal );
extern Gia_Man_t*           Cec_ManFdGetFd( Cec_ManFd_t* pMan, int nidGlobal, Vec_Int_t* vFdSupportGlobal, Vec_Int_t* stat );
extern float                Cec_ManFdRawCost( Cec_ManFd_t* pMan, int nid, Vec_Int_t* vFdSupport, Gia_Man_t* patch );
// functional dependency update status
extern void                 Cec_ManFdUpdateStatForce( Cec_ManFd_t* pMan, int nid, Vec_Int_t* vFdSupport, Gia_Man_t* patch, float costIn );
extern int                  Cec_ManFdUpdateStat( Cec_ManFd_t* pMan, int nid, Vec_Int_t* vFdSupport, Gia_Man_t* patch, int fStrict );
extern void                 Cec_ManFdCleanOneUnknown( Cec_ManFd_t* pMan, int nid );
extern void                 Cec_ManFdCleanUnknown( Cec_ManFd_t* pMan, int fGetColor );
extern void                 Cec_ManFdTraverseUnknown( Cec_ManFd_t* pMan, int fColor );
extern Vec_Int_t*           Cec_ManFdTraverseUnknownFrt( Cec_ManFd_t* pMan, int fColor, int nRange, int fType, Vec_Flt_t* vCoef );
extern void                 Cec_ManFdTraverseUnknownLevelTest( Cec_ManFd_t* pMan, int lv_min, int lv_max, int lv_lim, int fColor );
extern void                 Cec_ManFdShrink( Cec_ManFd_t* pMan, int nid );
extern void                 Cec_ManFdShrinkSimple( Cec_ManFd_t* pMan, int nid );
extern void                 Cec_ManFdShrinkLevelTest( Cec_ManFd_t* pMan, int nid, int lv_lim );
extern void                 Cec_ManFdCleanSupport( Cec_ManFd_t* pMan, int nid );
extern void                 Cec_ManFdIncreIter( Cec_ManFd_t* pMan );
// functional dependency replacement
extern Gia_Man_t*           Cec_ManFdReplaceOne( Cec_ManFd_t* pMan, int nid );
extern Gia_Man_t*           Cec_ManFdReplaceMulti( Cec_ManFd_t* pMan, Vec_Int_t* vF );
// report
extern void                 Cec_ManFdCoverFrt( Cec_ManFd_t* pMan, int fCkt );
extern Gia_Man_t*           Cec_ManFdVisSupport( Cec_ManFd_t* pMan, int nid, int fPatch );
extern void                 Cec_ManFdPlot( Cec_ManFd_t* pMan, int fAbs, int fVecAbs, int fColor, char* pFileName, Vec_Int_t* vColor1, Vec_Int_t* vColor2, Vec_Int_t* vColor3 );
extern void                 Cec_ManFdReport( Cec_ManFd_t* pMan );


static inline Vec_Wec_t*    Cec_ManCutReform( Vec_Int_t * vCut ) {
    int buf, dummy, flag, entry;
    Vec_Wec_t* v_cuts = Vec_WecStart( 1 );
    flag = 0;
    entry = 0;
    Vec_IntForEachEntry( vCut, buf, dummy ) {
        // printf("%d, %d\n", j, val);
        if (dummy == 0) continue;
        else if (!flag) {
            flag = 1;
            continue;
        }
        else if (dummy == (Vec_IntSize(vCut) - 1)) {
            continue;
        }
        else if ( buf == -1  || dummy == (Vec_IntSize(vCut) - 1)) {
          Vec_WecPushLevel( v_cuts );
          entry += 1;
          flag = 0;
        }
        else {
          Vec_WecPush( v_cuts, entry, buf );
        }
    }
    return v_cuts;
}

// vOut: [nid1, nid2, phase]
static inline Vec_Int_t*    Cec_ManCheckMiter( Gia_Man_t* pGia ) {
  int fPhase, nid1, nid2;
  Vec_Int_t *vOut = Vec_IntStart( 3 );
  Gia_Obj_t *pObjbuf;
  if (Gia_ManCoNum(pGia) == 4) {
      nid1 = Gia_ObjFaninId0p( pGia, Gia_ManCo( pGia, 0 ) );
      nid2 = Gia_ObjFaninId0p( pGia, Gia_ManCo( pGia, 2 ) );
      fPhase = Gia_ObjFaninC0( Gia_ManCo( pGia, 0 ) ) ^ Gia_ObjFaninC0( Gia_ManCo( pGia, 2 ) );
      assert(nid1 != nid2);
  } else if (Gia_ManCoNum(pGia) == 2) {
      pObjbuf = Gia_ObjFanin0( Gia_ManCo( pGia, 0 ) );
      nid1 = Gia_ObjFaninId0p( pGia, Gia_ManCo( pGia, 0 ) );
      nid2 = Gia_ObjFaninId0p( pGia, Gia_ManCo( pGia, 1 ) );
      fPhase = Gia_ObjFaninC0( Gia_ManCo( pGia, 0 ) ) ^ Gia_ObjFaninC0( Gia_ManCo( pGia, 1 ) );
  } else if (Gia_ManCoNum(pGia) == 1) {
      pObjbuf = Gia_ObjFanin0( Gia_ManCo( pGia, 0 ) );
      nid1 = Gia_ObjFaninId0p( pGia, Gia_ManCo( pGia, 0 ) );
      nid2 = Gia_ObjFaninId0p( pGia, Gia_ManCo( pGia, 0 ) );
      // two must be same node
  } else {
      assert(0);
  }

  if (nid1 == nid2) { // assume it is a XOR or XNOR
      assert(Gia_ObjRecognizeExor( pObjbuf, 0, 0 ));
      // printf("%d, %d\n", Gia_ObjFaninC0( Gia_ManCo( pMan->pGia, 0 )) , Gia_ObjRecognizeExor( pMan->pObj1, 0, 0 ));
      // does PO inv and does PO connect to XNOR
      fPhase = Gia_ObjFaninC0( Gia_ManCo( pGia, 0 )) ^ (Gia_ObjFaninC0( Gia_ObjFanin0( pObjbuf ) ) != Gia_ObjFaninC1( Gia_ObjFanin0( pObjbuf ) ));
      nid1 = Gia_ObjFaninId0p( pGia, Gia_ObjFanin0( Gia_ManObj( pGia, nid1 ) ) );
      nid2 = Gia_ObjFaninId1p( pGia, Gia_ObjFanin0( Gia_ManObj( pGia, nid2 ) ) );
  }

  Vec_IntWriteEntry( vOut, 0, nid1 );
  Vec_IntWriteEntry( vOut, 1, nid2 );
  Vec_IntWriteEntry( vOut, 2, fPhase );
  return vOut;
}
// record pObj copyId in vMap
static inline Vec_Int_t*    Cec_ManDumpValue( Gia_Man_t* pGia ) {
  Gia_Obj_t* pObj;
  int i;
  Vec_Int_t* vValue = Vec_IntAlloc( Gia_ManObjNum(pGia) );
  Gia_ManForEachObj( pGia, pObj, i ) {
    Vec_IntPush( vValue, Gia_ObjValue(pObj) );
  }
  return vValue;
}
static inline void          Cec_ManLoadValue( Gia_Man_t* pGia, Vec_Int_t* vValue ) {
  int val, i;
  Vec_IntForEachEntry( vValue, val, i ) {
    Gia_ObjSetValue( Gia_ManObj(pGia, i), val );
  }
}
static inline Vec_Int_t*    Cec_ManDumpMark( Gia_Man_t* pGia, int fMark ) {
  assert( Gia_ManObjNum(pGia) > 0 );
  Gia_Obj_t* pObj;
  int i;
  // printf("p: %d\n", Gia_ManObjNum(pGia));
  Vec_Int_t* vMark = Vec_IntAlloc( Gia_ManObjNum(pGia) );
  // printf("i\n");
  Gia_ManForEachObj( pGia, pObj, i ) {
    // printf("%d\n", i);
    // if (vMark->pArray == NULL) {
    //   printf("Problematic gia detected! i = %d\n", i);
    //   // Gia_SelfDefShow( pGia, "problemetic gia", 0, 0, 0 );
    //   assert(0);
    // }
    if (fMark == 0 && pObj->fMark0) Vec_IntPush( vMark, Gia_ObjId(pGia, pObj) );
    else if (fMark == 1 && pObj->fMark1) Vec_IntPush( vMark, Gia_ObjId(pGia, pObj) );
  }
  // printf("j\n");
  return vMark;
}
static inline void          Cec_ManLoadMark( Gia_Man_t* pGia, Vec_Int_t* vMark, int fMark, int fToggle ) {
  assert( fMark == 0 || fMark == 1 );
  int entry, i;
  Vec_IntForEachEntry( vMark, entry, i ) {
    if (fMark == 0 && fToggle == 0) Gia_ManObj(pGia, entry)->fMark0 = 1;
    else if (fMark == 0 && fToggle == 1) Gia_ManObj(pGia, entry)->fMark0 ^= 1;
    else if (fMark == 1 && fToggle == 0) Gia_ManObj(pGia, entry)->fMark1 = 1;
    else if (fMark == 1 && fToggle == 1) Gia_ManObj(pGia, entry)->fMark1 ^= 1;
  }
}
static inline void          Cec_ManFillMark( Gia_Man_t* pGia, int fMark ) {
  Gia_Obj_t* pObj;
  int i;
  if (fMark == 0) {
    Gia_ManForEachObj( pGia, pObj, i ) {
      pObj->fMark0 = 1;
    }
  } else {
    Gia_ManForEachObj( pGia, pObj, i ) {
      pObj->fMark1 = 1;
    }
  }
}

static inline void          Cec_ManMark2Val( Gia_Man_t* pGia, int val, int mark, int fRev, int fToggle ) {
  Gia_Obj_t* pObj;
  int i;
  assert(mark == 0 || mark == 1);
  if (fRev) {
    Gia_ManForEachObj( pGia, pObj, i ) {
      if (Gia_ObjValue(pObj) == val) {
        if (mark == 0) {
          if (fToggle == 0) pObj->fMark0 = 1;
          else pObj->fMark0 ^= 1;
        } else {
          if (fToggle == 0) pObj->fMark1 = 1;
          else pObj->fMark1 ^= 1;
        }
      } else {
        if (mark == 0) {
          if (fToggle == 0) pObj->fMark0 = 0;
        } else {
          if (fToggle == 0) pObj->fMark1 = 0;
        }
      }
    } 
  } else {
    Gia_ManForEachObj( pGia, pObj, i ) {
      if ((mark == 0 && pObj->fMark0) || (mark == 1 && pObj->fMark1)) {
        if (fToggle == 0) Gia_ObjSetValue( pObj, val );
        else Gia_ObjSetValue( pObj, Gia_ObjValue(pObj) ^ val );
      } else if ((mark == 0 && pObj->fMark0 == 0) || (mark == 1 && pObj->fMark1 == 0)) {
        if (fToggle == 0) Gia_ObjSetValue( pObj, -1 );
      }
    }
  }
}
static inline void          Cec_ManGiaMapId( Gia_Man_t* pGia, Vec_Int_t* vMap ) {
  int nid, i;
  Gia_Obj_t* pObj;
  if (Vec_IntSize(vMap) == 0) {
    Vec_IntFill( vMap, Gia_ManObjNum(pGia), -1 );
    Gia_ManForEachAnd( pGia, pObj, i ) {
      nid = Gia_ObjId(pGia, pObj);
      if (pObj->Value == -1) {
        Vec_IntWriteEntry( vMap, nid, -1 );
      } else {
        Vec_IntWriteEntry( vMap, nid, Abc_Lit2Var(pObj->Value) );
      }
    }
    Vec_IntWriteEntry( vMap, 0, 0 ); // CONST0
  } else {
    Vec_IntForEachEntry( vMap, nid, i ) {
      if (nid < 0 || nid >= Gia_ManObjNum(pGia)) Vec_IntWriteEntry( vMap, i, -1 );
      else if (Gia_ManObj(pGia, nid)->Value == -1) {
          Vec_IntWriteEntry( vMap, i, -1 );
      } else {
          Vec_IntWriteEntry( vMap, i, Abc_Lit2Var(Gia_ManObj(pGia, nid)->Value) );
      }
    }
  }
}
static inline Vec_Int_t*    Cec_ManGetColorNd( Gia_Man_t* pGia, int fGetColor ) {
  Gia_Obj_t* pObj;
  int i, nid;
  Vec_Int_t* vNd = Vec_IntAlloc( Gia_ManObjNum(pGia) );
  assert( pGia->pReprs );
  Gia_ManForEachObj( pGia, pObj, i) {
    nid = Gia_ObjId(pGia, pObj);
    if ( Gia_ObjColors(pGia, nid) == 1 && (fGetColor & 1)) Vec_IntPush( vNd, nid );
    else if ( Gia_ObjColors(pGia, nid) == 2 && (fGetColor & 2)) Vec_IntPush( vNd, nid );
    else if ( Gia_ObjColors(pGia, nid) == 3 && (fGetColor & 4)) Vec_IntPush( vNd, nid );
  }
  return vNd;
}
static inline Vec_Wec_t*    Cec_ManCktFO( Gia_Man_t* pGia ) {
  Vec_Wec_t* vFO = Vec_WecStart( Gia_ManObjNum(pGia) );
  int i, nidbuf, nidself;
  Gia_Obj_t* pObj;
  Gia_ManForEachObj( pGia, pObj, i ) {
    nidself = Gia_ObjId(pGia, pObj);
    if ( Gia_ObjIsCi( pObj ) || Gia_ObjIsConst0( pObj ) ) continue;
    nidbuf = Gia_ObjId(pGia, Gia_ObjFanin0( pObj ));
    Vec_WecPush( vFO, nidbuf, nidself );
    if ( Gia_ObjIsCo( pObj ) ) continue;
    assert( Gia_ObjIsAnd( pObj ) );
    nidbuf = Gia_ObjId(pGia, Gia_ObjFanin1( pObj ));
    Vec_WecPush( vFO, nidbuf, nidself );
  }
  return vFO;
}
static inline Vec_Int_t*    Cec_ManGetTFO( Gia_Man_t* pGia, Vec_Int_t* vNid, int nHeight, int fSetVal ) {
  Vec_Wec_t* vFO = Cec_ManCktFO( pGia );
  Vec_Int_t* vTFO = Vec_IntAlloc( 100 );
  Gia_Obj_t* pObj;
  int cntHeight = 0;
  int i;
  int i_lim;
  int id, id_par, j;

  Vec_IntForEachEntry( vNid, id, i ) {
    Vec_IntForEachEntry( Vec_WecEntry(vFO, id), id_par, j ) {
      if (Vec_IntFind( vNid, id_par ) != -1) continue;
      pObj = Gia_ManObj(pGia, id_par);
      Vec_IntPushUnique( vTFO, id_par );
      if (fSetVal > 0) {
        if ((pObj->Value & fSetVal) != 0) pObj->Value ^= fSetVal;
      } else if (fSetVal < 0) {
        pObj->Value ^= Abc_AbsInt(fSetVal);
      }
    }
  }
  i = 0; i_lim = Vec_IntSize( vTFO );
  while ( i < i_lim ) {
    id = Vec_IntEntry( vTFO, i );
    Vec_IntForEachEntry( Vec_WecEntry(vFO, id), id_par, j ) {
      if (Vec_IntFind( vNid, id_par ) != -1) continue;
      pObj = Gia_ManObj(pGia, id_par);
      Vec_IntPushUnique( vTFO, id_par );
      if (fSetVal > 0) {
        if ((pObj->Value & fSetVal) != 0) pObj->Value ^= fSetVal;
      } else if (fSetVal < 0) {
        pObj->Value ^= Abc_AbsInt(fSetVal);
      }
    }
    i++;
    if ( i == i_lim && (cntHeight < nHeight || nHeight == -1) ) {
      i_lim = Vec_IntSize( vTFO );
      cntHeight++;
      if ( cntHeight == nHeight ) break;
    }
  }
  Vec_WecFree( vFO );
  if (fSetVal != 0) {
    Vec_IntFree( vTFO );
    vTFO = 0;
  }
  return vTFO;
}
// fCover <---> keep the nodes that is in vNid as Sibling 
static inline Vec_Int_t*    Cec_ManGetSibling( Gia_Man_t* pGia_in, Vec_Int_t* vNid, int nHeight, int fCover ) {
    Gia_Man_t* pGia = Gia_ManDup( pGia_in );
    Vec_Int_t* vTFO = Cec_ManGetTFO( pGia, vNid, nHeight, 0 );
    // Vec_IntPrint( vTFO );
    Vec_Int_t* vSibling = Vec_IntAlloc( 100 );
    int i, nidbuf, nidbuf_sib;
    Gia_Obj_t* pObj;
    Gia_ManFillValue( pGia );
    Vec_IntForEachEntry( vNid, nidbuf, i ) {
      Cec_ManSubcircuit( pGia, Gia_ManObj(pGia, nidbuf), 0, 0, 1, 0 );
    }
    Vec_IntForEachEntry( vTFO, nidbuf, i ) {
       pObj = Gia_ManObj( pGia, nidbuf );
       if ( Gia_ObjIsAnd( pObj ) ) {
          nidbuf_sib = Gia_ObjId(pGia, Gia_ObjFanin0( pObj ));
          if ( Vec_IntFind( vTFO, nidbuf_sib ) == -1 && Vec_IntFind( vNid, nidbuf_sib ) == -1 ) {
              if ( fCover == 0 && Gia_ObjValue( Gia_ManObj(pGia, nidbuf_sib) ) != -1) continue;
              Vec_IntPushUnique( vSibling, nidbuf_sib );
              Cec_ManSubcircuit( pGia, Gia_ManObj(pGia, nidbuf_sib), 0, 0, 1, 0 );
          }
          nidbuf_sib = Gia_ObjId(pGia, Gia_ObjFanin1( pObj ));
          if ( Vec_IntFind( vTFO, nidbuf_sib ) == -1 && Vec_IntFind( vNid, nidbuf_sib ) == -1 ) {
              if ( fCover == 0 && Gia_ObjValue( Gia_ManObj(pGia, nidbuf_sib) ) != -1) continue;
              Vec_IntPush( vSibling, nidbuf_sib );
              Cec_ManSubcircuit( pGia, Gia_ManObj(pGia, nidbuf_sib), 0, 0, 1, 0 );
          }
       }
    }
    Vec_IntFree( vTFO );
    return vSibling;
}
// 0 -- size, 1 -- level, 2 -- dist
static inline Vec_Int_t*    Cec_ManGetMFFCost( Gia_Man_t* pGia, int fCost ) {
  int nid, nidSup, buf1, buf2, i, j;
  int** dist;
  Vec_Int_t* vCost = Vec_IntStart( Gia_ManObjNum(pGia) );
  Vec_Int_t* vSup = Vec_IntAlloc( 100 );
  Gia_Obj_t* pObjbuf;
  if (pGia->pRefs == 0) Gia_ManCreateRefs( pGia );
  dist = Cec_ManDist(pGia, fCost & 1);
  Vec_IntFill( vCost, Gia_ManObjNum(pGia), -1 );
  Gia_ManForEachAnd( pGia, pObjbuf, i ) {
      nid = Gia_ObjId( pGia, pObjbuf );
      if ((nid == Gia_ManCoDriverId(pGia, 0)) && (Gia_ManCoNum(pGia) == 1)) continue;
      buf2 = Gia_NodeMffcSizeSupp( pGia, pObjbuf, vSup );
      switch (fCost) {
          case 0: // size
              Vec_IntWriteEntry( vCost, nid, buf2 );
              break;
          case 1: // level
              buf2 = -1;
              Vec_IntForEachEntry( vSup, nidSup, j ) {
                  if ( buf2 == -1 || buf2 < dist[nid][nidSup] ) {
                      buf2 = dist[nid][nidSup];
                  }
              }
              Vec_IntWriteEntry( vCost, nid, buf2 );
              break;
          case 2: // dist
              buf2 = -1;
              Vec_IntForEachEntry( vSup, nidSup, j ) {
                  if ( buf2 == -1 || buf2 > dist[nid][nidSup] ) {
                      buf2 = dist[nid][nidSup];
                  }
              }
              Vec_IntWriteEntry( vCost, nid, buf2 );
              break;
          default:
              assert(0);
      }
  }
  for (i = 0; i < Gia_ManObjNum(pGia); i++) ABC_FREE( dist[i] );
  ABC_FREE( dist );
  return vCost;
}
// contain vFO itself
static inline int           Cec_ManMFFCMul( Gia_Man_t* pGia, Vec_Int_t* vFO, Vec_Int_t* vMFFC ) {
    int i, entry;
    int cnt = 0;
    Gia_Obj_t* pObj;  
    Vec_Int_t* vVal = Cec_ManDumpValue( pGia );
    Vec_Int_t* vIntBuff = Vec_IntAlloc( 100 );
    Gia_ManFillValue( pGia );
    Gia_ManForEachCo( pGia, pObj, i ) {
        Cec_ManSubcircuit( pGia, pObj, vFO, 0, 1, 0 );
    }
    vIntBuff = Vec_IntDup( vFO );
    Vec_IntSort( vIntBuff, 0 );
    Vec_IntForEachEntry( vIntBuff, entry, i ) {
        Gia_ManObj(pGia, entry)->Value ^= 1;
        Cec_ManSubcircuit( pGia, Gia_ManObj(pGia, entry), 0, 0, 2, 0 );
    }
    Gia_ManForEachObj( pGia, pObj, i ) {
        if ( ~Gia_ObjValue( pObj ) != 2 ) continue;
        if (vMFFC) Vec_IntPush( vMFFC, Gia_ObjId(pGia, pObj) );
        cnt++;
    }
    Cec_ManLoadValue( pGia, vVal );
    Vec_IntFree( vVal );
    return cnt;
}
// get the connect nodes of the mark0, which save as mark1
static inline void          Cec_ManBFSMark( Gia_Man_t* pGia, int nIte ) {
    Gia_Obj_t* pObj, *pObj0, *pObj1;
    Vec_Int_t* vMark1;
    int i;
    if (nIte == 0) return;
    Gia_ManForEachObj( pGia, pObj, i ) {
        if (Gia_ObjIsConst0(pObj)) continue;
        if (Gia_ObjIsCi(pObj)) continue;
        pObj0 = Gia_ObjFanin0(pObj);
        pObj1 = Gia_ObjIsAnd(pObj) ? Gia_ObjFanin1(pObj) : NULL;
        if (pObj0->fMark0) pObj->fMark1 = 1;
        if (pObj1 && pObj1->fMark0) pObj->fMark1 = 1;
        if (pObj->fMark0) {
          pObj0->fMark1 = 1;
          if (pObj1) pObj1->fMark1 = 1;
        }
    }
    vMark1 = Cec_ManDumpMark( pGia, 1 );
    Cec_ManLoadMark( pGia, vMark1, 0, 0 );
    Cec_ManBFSMark( pGia, nIte - 1 );
    return;
}
// please save MFFC as mark0, the leaks would be in mark1
static inline void          Cec_ManLeakMark( Gia_Man_t* pGia, int fFO ) {
    Gia_Obj_t* pObj, *pObj0, *pObj1;
    if (pGia->vFanout == NULL) Gia_ManStaticFanoutStart( pGia );
    int i, nid, j, flag;
    Gia_ManForEachObj( pGia, pObj, i ) {
        flag = 0;
        if (pObj->fMark0) continue;
        // Vec_IntForEachEntry( Vec_WecEntry(vFO, Gia_ObjId(pGia, pObj)), nid, j ) {
        Gia_ObjForEachFanoutStaticId( pGia, Gia_ObjId(pGia, pObj), nid, j ) {
          if (Gia_ManObj(pGia, nid)->fMark0) flag |= 1;
          else flag |= 2;
        }
        if (flag == 3) {
          // pObj->fMark1 = 1;
          if (fFO) {
            Gia_ObjForEachFanoutStaticId( pGia, Gia_ObjId(pGia, pObj), nid, j ) {
              if (Gia_ManObj(pGia, nid)->fMark0 == 0) Gia_ManObj(pGia, nid)->fMark1 = 1;
            }
          } else {
            pObj->fMark1 = 1;
          }
        }
    }
}
static inline Vec_Wec_t*    Cec_ManFOcut( Gia_Man_t* pGia, int nid, int limBranch, Vec_Int_t* vTraverse ) {
  assert(limBranch > 0);
  if (pGia->vFanout == NULL) Gia_ManStaticFanoutStart( pGia );
  // Vec_Wec_t* vFO = vFOmem ? vFOmem : Cec_ManCktFO( pGia );
  Vec_Wec_t* vFOcut = Vec_WecAlloc( 1 );
  Vec_Wec_t* vFObranch;
  Vec_Wec_t* vWecBuff;
  Vec_Int_t* vIntBuff;
  // Vec_Int_t* vFOnid = Gia_ObjFanoutId()
  int nFO = Gia_ObjFanoutNumId(pGia, nid);
  int travNid, i;
  if (vTraverse) Vec_IntPush( vTraverse, nid );
  if (nFO > limBranch) Vec_IntPush( Vec_WecPushLevel( vFOcut ), nid );
  else if (nFO == 1)  {
    Vec_WecFree( vFOcut );
    vFOcut = Cec_ManFOcut( pGia, Gia_ObjFanoutId(pGia, nid, 0), limBranch, vTraverse );
  } else {
    Gia_ObjForEachFanoutStaticId( pGia, nid, travNid, i ) {
      vFObranch = Cec_ManFOcut( pGia, travNid, limBranch - nFO + 1, vTraverse );
      vFOcut = Vec_WecCrossSet( vWecBuff = vFOcut, vFObranch );
      Vec_WecFree( vFObranch );
      Vec_WecFree( vWecBuff );
    }
    Vec_IntPush( Vec_WecPushLevel( vFOcut ), nid );
  }

  // if (vFOmem == 0) Vec_WecFree( vFO );
  return vFOcut;

}
/* get the top of the marked node. If a node is CI and not marked, it is not considered. please mark on 0
   fMark = 0: pick the node cover marked; fMark = 1: pick the node that is marked and not covered */
static inline Vec_Int_t*    Cec_ManGetTopMark( Gia_Man_t* pGia, int fMark ) {
  int i, j, nid;
  Gia_Obj_t* pObjbuf;
  Vec_Int_t* vTop = Vec_IntAlloc( 100 );
  Vec_Wec_t* vFO = Cec_ManCktFO( pGia );
  if (fMark) {
    Gia_ManForEachObj( pGia, pObjbuf, i ) {
      if ( pObjbuf->fMark0 == 0 ) continue;
      if (Vec_IntSize( Vec_WecEntry(vFO, Gia_ObjId(pGia, pObjbuf)) ) == 0) 
        Vec_IntPush( vTop, Gia_ObjId(pGia, pObjbuf) );
      else {
        Vec_IntForEachEntry( Vec_WecEntry( vFO, Gia_ObjId(pGia, pObjbuf) ), nid, j ) {
          if ( Gia_ManObj(pGia, nid)->fMark0 == 0 ) {
            Vec_IntPush( vTop, Gia_ObjId(pGia, pObjbuf) );
            break;
          }
        }
      }
    }
  } else {
    Gia_ManForEachObj( pGia, pObjbuf, i ) {
      if ( pObjbuf->fMark0 == 1 ) continue;
      if ( Gia_ObjIsConst0( pObjbuf ) ) continue;
      if ( Gia_ObjIsCi( pObjbuf ) ) continue;
      if ( Gia_ObjIsCo( pObjbuf ) ) {
        if ( Gia_ObjFanin0( pObjbuf )->fMark0 == 0 ) continue;
      } 
      if ( Gia_ObjIsAnd( pObjbuf ) ) {
        if ( Gia_ObjFanin0( pObjbuf )->fMark0 == 0 && Gia_ObjFanin1( pObjbuf )->fMark0 == 0 ) continue;
      }
      Vec_IntPush( vTop, Gia_ObjId(pGia, pObjbuf) ); // won't double push
    }
  }
  Vec_WecFree( vFO );
  return vTop;
}
static inline Vec_Int_t*    Cec_ManGetTop( Gia_Man_t* pGia ) {
  int i;
  Gia_Obj_t* pObj;
  Vec_Int_t* vTop;
  Vec_Int_t* vMark;
  vMark = Cec_ManDumpMark( pGia, 0 );
  Gia_ManForEachObj( pGia, pObj, i ) {
    pObj->fMark0 = 0;
  }
  Gia_ManForEachObj( pGia, pObj, i ) {
    if ( Gia_ObjIsConst0( pObj ) ) continue;
    if ( Gia_ObjIsCi( pObj ) ) continue;
    Gia_ObjFanin0( pObj )->fMark0 = 1;
    if ( Gia_ObjIsCo( pObj ) ) continue;
    Gia_ObjFanin1( pObj )->fMark0 = 1;
  }
  vTop = Cec_ManGetTopMark( pGia, 0 );
  Cec_ManLoadMark( pGia, vMark, 0, 0 );
  return vTop;
}
static inline Vec_Int_t*    Cec_ManCountVal( Gia_Man_t* pGia, Vec_Int_t* vNd ) {
  int i, nidbuf, buf, val;
  Vec_Int_t* vCnt = Vec_IntAlloc( 100 );
  Vec_IntForEachEntry( vNd, nidbuf, i ) {
    buf = Gia_ObjValue( Gia_ManObj(pGia, nidbuf) );
    if (buf < 0) continue;
    if (buf >= Vec_IntSize(vCnt)) {
      Vec_IntFillExtra( vCnt, buf + 1, 0 );
    }
    Vec_IntAddToEntry( vCnt, buf, 1 );
  }
  return vCnt;
}

static inline float         Cec_ManMergeCompare( Vec_Int_t* vCount1, Vec_Int_t* vCount2 ) {
  int i, val;
  int cnt1, cnt2;
  float eval1, eval2;
  float sum1, sum2;
  Vec_Int_t* vCount = Vec_IntOp( vCount1, vCount2, 0 );
  cnt1 = 0; eval1 = 0.0; cnt2 = 0; eval2 = 0.0; sum1 = 0.0; sum2 = 0.0;
  Vec_IntForEachEntry( vCount1, val, i ) {
    if (val > 0) {
      eval1 += 1.0 * val * i * i;
      sum1 += 1.0 * val * i;
      cnt1 += val;
    }
  }
  Vec_IntForEachEntry( vCount2, val, i ) {
    if (val > 0) {
      eval2 += 1.0 * val * i * i;
      sum2 += 1.0 * val * i;
      cnt2 += val;
    }
  }
  printf("nCI_org = %d, nCI_new = %d\n", cnt1, cnt2);
  printf("SqMean(all): ckt_org = %f, ckt_new = %f\n", cnt1 == 0 ? -1.0 : sqrt(eval1 / cnt1), cnt2 == 0 ? -1.0 : sqrt(eval2 / cnt2));
  printf("max(all): lv_org = %d, lv_new = %d\n", Vec_IntSize(vCount1), Vec_IntSize(vCount2));
  printf("avg(all): avg_org = %f, avg_new = %f\n", cnt1 == 0 ? -1.0 : sum1 / cnt1, cnt2 == 0 ? -1.0 : sum2 / cnt2);
  printf("std(all): std_org = %f, std_new = %f\n", sqrt((eval1 - sum1 * sum1 / cnt1) / cnt1), sqrt((eval2 - sum2 * sum2 / cnt2) / cnt2));
  printf("#frt (new - org) in the level\n--->");
  cnt1 = 0; eval1 = 0.0; cnt2 = 0; eval2 = 0.0; sum1 = 0.0; sum2 = 0.0;
  Vec_IntForEachEntry( vCount, val, i ) {
    if (val > 0) {
      eval1 += 1.0 * val * i * i;
      sum1 += 1.0 * val * i;
      cnt1 += val;
    } else if (val < 0) {
      eval2 += -1.0 * val * i * i;
      sum2 += -1.0 * val * i;
      cnt2 += -val;
    }

    if (val != 0) {
      printf("(%d: %d),", i, -val);
    }
  }
  printf("\n");
  printf("SqMean(changed): ckt_org = %f, ckt_new = %f\n", cnt1 == 0 ? -1.0 : sqrt(eval1 / cnt1), cnt2 == 0 ? -1.0 : sqrt(eval2 / cnt2));
  printf("avg(changed): avg_org = %f, avg_new = %f\n", cnt1 == 0 ? -1.0 : sum1 / cnt1, cnt2 == 0 ? -1.0 : sum2 / cnt2);
  Vec_IntFree( vCount );

  if (cnt2 == 0) return -1.0;
  else if (cnt1 == 0) return sqrt(eval2 / cnt2);
  else return sqrt(eval2 / cnt2) - sqrt(eval1 / cnt1);

}
static inline float         Cec_ManMergeUniformality( Vec_Int_t* vCount, int nBin ) {
    if (Vec_IntSum(vCount) == 0) return 0.0;

    int K = nBin > 0 ? nBin : Vec_IntSize(vCount);
    int N = Vec_IntSum(vCount);
    int i, val;
    Vec_Int_t* vIntBuff = Vec_IntCumSum( vCount, 1 );
    float lorenz_area = 1.0 * Vec_IntSum( vIntBuff ) / (Vec_IntEntryLast(vIntBuff) * Vec_IntSize(vCount));
    float gini = 1.0 - 2.0 * lorenz_area + 1.0 / Vec_IntSize(vCount);
    Vec_IntFree(vIntBuff);
    float tv = 0.0;
    float sq = 0.0;
    float buf;
    Vec_IntForEachEntry( vCount, val, i ) {
        if (i < K) {
          buf = 1.0 * val / N - 1.0 * (1.0 * i / N + 1.0) / K;
          tv += Abc_AbsFloat( buf );
          sq += buf >= 0 ? buf * buf : 0.0;
        } else {
          buf = 1.0 * val / N;
          tv += Abc_AbsFloat( buf );
          sq += buf * buf;
        }
    }
    printf("gini = %f, tv = %f, sq = %f\n", gini, tv, sq);
    return gini;
}
// < 0 iff vChange1 < vChange2
static inline int           Cec_ManMergeChangeCompare( Vec_Int_t* vChange1, Vec_Int_t* vChange2, int fMin ) {
  // assert( Vec_IntSize(vChange1) == Vec_IntSize(vChange2) );
  int i, lv, val1, val2;
  int n = vChange2 == 0 ? Vec_IntSize(vChange1) : Abc_MaxInt( Vec_IntSize(vChange1), Vec_IntSize(vChange2) );
  for (i = 0; i < n; i++) {
    lv = fMin ? i : n - i - 1;
    val1 = lv >= Vec_IntSize(vChange1) ? 0 : Vec_IntEntry( vChange1, lv );
    val2 = (vChange2 == 0 || lv >= Vec_IntSize(vChange2)) ? 0 : Vec_IntEntry( vChange2, lv );
    if (val1 == val2) continue;
    else if (val1 > val2) return lv + 1;
    else if (val1 < val2) return - (lv + 1);
  }
  return 0;
}
static inline Vec_Int_t*    Cec_ManMergeAnal( Gia_Man_t* pTemp, Vec_Int_t* vCnt1, Vec_Int_t* vCnt2, int fReplace ) {
    Vec_Int_t *vIntBuff1, *vChange, *vCntBuf;
    Vec_Int_t *vMerge = Vec_IntAlloc( 100 );
    Vec_Int_t *vOut = Vec_IntAlloc( 4 );
    Gia_Obj_t *pObj1, *pObj2, *pObj;
    int buf, i;
    vIntBuff1 = Cec_ManCheckMiter( pTemp );
    pObj1 = Gia_ManObj( pTemp, Vec_IntEntry(vIntBuff1, 0) );
    pObj2 = Gia_ManObj( pTemp, Vec_IntEntry(vIntBuff1, 1) );
    Vec_IntClear( vMerge );
    Cec_ManGetMerge_old( pTemp, pObj1, pObj2, vMerge );
    Cec_ManDistEasy( pTemp, vMerge, Gia_ObjId(pTemp, pObj1), 1 );
    vCntBuf = Cec_ManCountVal( pTemp, vMerge );
    if (Vec_IntSize(vCnt1) > 0) {
      // printf("ckt1 old:\n");
      Vec_IntPrint( vCntBuf );
      vChange = Vec_IntOp( vCntBuf, vCnt1, 0 );
      buf = Cec_ManMergeChangeCompare( vChange, 0, 0 );
      // printf("cover changes 1 (maxlv) = %d\n", buf);
      Vec_IntPush( vOut, buf );
      buf = Cec_ManMergeChangeCompare( vChange, 0, 1 );
      // printf("cover changes 1 (minlv) = %d\n", buf);
      Vec_IntPush( vOut, buf );
      // Vec_IntPrint( vChange );
      Vec_IntFree( vChange );
    }
    if (fReplace) {
        Vec_IntClear( vCnt1 );
        Vec_IntForEachEntry( vCntBuf, buf, i ) {
            Vec_IntPush( vCnt1, buf );
        }
        Vec_IntPush( vOut, -1 );
        Vec_IntPush( vOut, -1 );
    }
    Vec_IntFree( vCntBuf );


    Cec_ManDistEasy( pTemp, vMerge, Gia_ObjId(pTemp, pObj2), 1 );
    vCntBuf = Cec_ManCountVal( pTemp, vMerge );
    if (Vec_IntSize(vCnt2) > 0) {
      // printf("ckt2 old:\n");
      Vec_IntPrint( vCntBuf );
      vChange = Vec_IntOp( vCntBuf, vCnt2, 0 );
      buf = Cec_ManMergeChangeCompare( vChange, 0, 0 );
      // printf("cover changes 2 (maxlv)= %d\n", buf);
      Vec_IntPush( vOut, buf );
      buf = Cec_ManMergeChangeCompare( vChange, 0, 1 );
      // printf("cover changes 2 (minlv)= %d\n", buf);
      Vec_IntPush( vOut, buf );
      // Vec_IntPrint( vChange );
      Vec_IntFree( vChange );
    }
    if (fReplace) {
        Vec_IntClear( vCnt2 );
        Vec_IntForEachEntry( vCntBuf, buf, i ) {
            Vec_IntPush( vCnt2, buf );
        }
        Vec_IntPush( vOut, -1 );
        Vec_IntPush( vOut, -1 );
    }
    Vec_IntFree( vCntBuf );
    Vec_IntFree( vMerge );
    Vec_IntFree( vIntBuff1 );
    buf = 0;
    Gia_ManForEachObj( pTemp, pObj, i ) {
      if (Gia_ObjColors(pTemp, Gia_ObjId(pTemp, pObj)) == 1 || Gia_ObjColors(pTemp, Gia_ObjId(pTemp, pObj)) == 2) buf++;
    }
    Vec_IntPush( vOut, buf );
    printf("size of abs ckt = %d\n", buf);
    return vOut;
}

// fOldVal = 0: Copy(pNew, Val(pOld)) --> Copy(pOld, Val(pNew)) (probably you would require fill value first)
// fOldVal = 1: Copy(pOrg, Val(pOld)), Copy(pOld, Val(pNew)) --> Copy(pOrg, Val(pNew))
static inline void          Cec_ManMapCopy( Gia_Man_t* pNew, Gia_Man_t* pOld, int fOldVal ) {
    Gia_Obj_t* pObjbuf;
    int i, val;
    int debug = 0;
    // Gia_ManFillValue( pNew );
    if (fOldVal == 0) {
        Gia_ManForEachObj( pOld, pObjbuf, i ) {
            val = Gia_ObjValue(pObjbuf);
            if (val >= 0 && Abc_Lit2Var(val) < Gia_ManObjNum(pNew)) {
                Gia_ObjSetValue( Gia_ObjCopy(pNew, pObjbuf), Abc_Var2Lit(Gia_ObjId(pOld, pObjbuf), 0) );
            } else if (debug) {
                printf("Warning: Obj %d in old ckt has invalid copy value %d!\n", Gia_ObjId(pOld, pObjbuf), val);
            }
        }
        Gia_ManForEachCo( pNew, pObjbuf, i ) {
            val = Gia_ObjValue(Gia_ObjFanin0(pObjbuf));
            // if (Gia_ObjValue(pObjbuf) > 0 && Gia_ObjValue(pObjbuf) != Abc_LitNotCond( val, Gia_ObjFaninC0(pObjbuf) )) assert(0);
            if (val != -1) {
                Gia_ObjSetValue( pObjbuf, Abc_LitNotCond( val, Gia_ObjFaninC0(pObjbuf) ) );
            }
        }
    } else if (fOldVal == 1) {
        Gia_ManForEachObj( pNew, pObjbuf, i ) {
            val = Gia_ObjValue(pObjbuf);
            if (val >= 0 && Abc_Lit2Var(val) < Gia_ManObjNum(pOld)) {
                Gia_ObjSetValue( pObjbuf, Gia_ObjValue( Gia_ObjCopy(pOld, pObjbuf) ) );
            } else if (debug) {
                printf("Warning: Obj %d in new ckt has invalid copy value %d!\n", Gia_ObjId(pNew, pObjbuf), val);
            }
        }
        Gia_ManForEachCo( pNew, pObjbuf, i ) {
            val = Gia_ObjValue(Gia_ObjFanin0(pObjbuf));
            // if (Gia_ObjValue(pObjbuf) > 0 && Gia_ObjValue(pObjbuf) != Abc_LitNotCond( val, Gia_ObjFaninC0(pObjbuf) )) assert(0);
            if (val != -1) {
                Gia_ObjSetValue( pObjbuf, Abc_LitNotCond( val, Gia_ObjFaninC0(pObjbuf) ) );
            }
        }
    }
    
}
static inline void          Cec_ManPrintMapCopy( Gia_Man_t* p ) {
  Vec_Int_t* vMap;
  vMap = Cec_ManDumpValue( p );
  printf("Mapping info:\n");
  int i, buf;
  for ( i = 0; i < Vec_IntSize(vMap); i++ ) {
      buf = Vec_IntEntry( vMap, i );
      printf( "^%d->_%d,", i, buf < 0 ? buf : Abc_Lit2Var(buf) );
      if ( i % 10 == 9 ) printf( "\n" );
  }
  if (i % 10 != 0) printf( "\n" );
  Vec_IntFree( vMap );
}
static inline int           Cec_ManFdMapIdSingle( Cec_ManFd_t* pMan, int nid, int fToAbs ) {
  Vec_Int_t* vMap = fToAbs ? pMan->vAbsMap : pMan->vGiaMap;
  // int fIsAbsCi = fToAbs ? Gia_ManObjIsCi(pMan->pGia, nid) : 0;
  if ( nid >= 0 && nid < Vec_IntSize(vMap) ) return Vec_IntEntry( vMap, nid );
  else return -1;
  // if (fIsAbsCi)
  
}
static inline void          Cec_ManFdMapId( Cec_ManFd_t* pMan, Vec_Int_t* vIn, int fToAbs ) { 
  int nid, i;
  Vec_IntForEachEntry( vIn, nid, i ) {
    Vec_IntWriteEntry( vIn, i, Cec_ManFdMapIdSingle( pMan, nid, fToAbs ) );
  }
  // Vec_IntMapId( vMap, vIn );
}
// Gia Var <-> SAT Var
static inline int           Cec_ManFdMapVarSingle( Cec_ManFd_t* pMan, int val, int fToVar ) {
  assert(val >= 0);
  Vec_Int_t* vMap = fToVar ? pMan->vVarMap : pMan->vNdMap;
  int biasIn = fToVar ? Gia_ManObjNum(pMan->pGia) : pMan->nVars;
  int biasOut = fToVar ? pMan->nVars : Gia_ManObjNum(pMan->pGia);
  int bias, buf1, buf2;
  buf1 = val % biasIn; // remove bias
  bias = val / biasIn;
  if (pMan->pPars->fAbsItp) {
    if (fToVar) {
      // Nid
      buf2 = Cec_ManFdMapIdSingle( pMan, buf1, 1 ); // absNid
      if (buf2 < 0) return -1;
      buf1 = Vec_IntEntry( vMap, buf2 ); // var
      if (buf1 < 0) return -1;
      buf2 = buf1 + bias * biasOut; // biased var
    } else {
      // Var
      buf2 = Vec_IntEntry( vMap, buf1 ); // absNid
      assert( buf2 >= 0 );
      buf1 = Cec_ManFdMapIdSingle( pMan, buf2, 0 ); // Nid
      assert( buf1 >= 0 );
      buf2 = buf1 + bias * biasOut; // biased Nid
    }
    return buf2;
  } else {
    buf2 = Vec_IntEntry( vMap, buf1 );
    if (buf2 < 0) {
      if (fToVar) return -1;
      else assert(0);
    }
    buf1 = buf2 + bias * biasOut; // bias
    return buf1;
  }
}
static inline void          Cec_ManFdMapVar( Cec_ManFd_t* pMan, Vec_Int_t* vIn, int fToVar ) {
  int i, val;
  Vec_IntForEachEntry( vIn, val, i ) {
    Vec_IntWriteEntry( vIn, i, Cec_ManFdMapVarSingle( pMan, val, fToVar ) );
  }
}
static inline int           Cec_ManFdMapLitSingle( Cec_ManFd_t* pMan, int val, int fToLit ) {
  int in, neg, buf, out;
  if (fToLit) {
    in = abs(val);
    neg = val < 0;
    out = Cec_ManFdMapVarSingle( pMan, in, 1 );
    return out < 0 ? -1 : Abc_Var2Lit( out, neg );
  } else {
    in = Abc_Lit2Var(val);
    neg = Abc_LitIsCompl(val);
    out = Cec_ManFdMapVarSingle( pMan, in, 0 );
    return out * (neg ? -1 : 1);
  }
}
static inline void          Cec_ManFdMapLit( Cec_ManFd_t* pMan, Vec_Int_t* vIn, int fToLit ) {
  int i, val;
  Vec_IntForEachEntry( vIn, val, i ) {
    // printf("lit mapping:\n");
    // Vec_IntPrint(vIn);
    Vec_IntWriteEntry( vIn, i, Cec_ManFdMapLitSingle( pMan, val, fToLit ) );
    // Vec_IntPrint(vIn);
  }
}
static inline Vec_Int_t*    Cec_ManFdMapAttr( Cec_ManFd_t* pMan, Vec_Int_t* vIn, int fToAbs ) {
  Vec_Int_t* vMap = fToAbs ? pMan->vAbsMap : pMan->vGiaMap;
  Vec_Int_t* vNew = Vec_IntAlloc( Vec_IntSize(vIn) );
  Vec_IntRemapArray( vMap, vIn, vNew, fToAbs ? Gia_ManObjNum(pMan->pAbs) : Gia_ManObjNum(pMan->pGia) );
  return vNew;
}
static inline void          Cec_ManFdMapObj( Cec_ManFd_t* pMan, Vec_Ptr_t* vIn, int fToAbs ) {
  Gia_Man_t* pGia = fToAbs ? pMan->pGia : pMan->pAbs;
  Gia_Man_t* pToGia = fToAbs ? pMan->pAbs : pMan->pGia;
  Vec_Int_t* vIdBuff;
  Gia_Obj_t* pObj;
  int nid, i;
  if (Vec_PtrSize(vIn) > 0) {
    vIdBuff = Vec_IntAlloc( Vec_PtrSize(vIn) );
    Vec_PtrForEachEntry( Gia_Obj_t*, vIn, pObj, i ) {
      assert( pGia->pObjs <= pObj && pObj < pGia->pObjs + pGia->nObjs );
      Vec_IntPush(vIdBuff, Gia_ObjId(pGia, pObj));
    }
  } else {
    vIdBuff = Vec_IntAlloc( Gia_ManObjNum(pGia) );
    Vec_PtrGrow( vIn, Gia_ManObjNum(pGia) );
    vIn->nSize = Gia_ManObjNum(pGia);
    Gia_ManForEachObj( pGia, pObj, i ) {
      Vec_IntPush(vIdBuff, Gia_ObjId(pGia, pObj));
    }
  }
  Cec_ManFdMapId( pMan, vIdBuff, fToAbs );
  Vec_IntForEachEntry( vIdBuff, nid, i ) {
    Vec_PtrWriteEntry( vIn, i, Gia_ManObj(pToGia, nid) );
  }

}
static inline Vec_Int_t*    Cec_ManFdUnsatId( Cec_ManFd_t* pMan, int level, int fHuge ) {
  int stat, nid, i, j;
  Vec_Wec_t* ordUnsat = Vec_WecStart( 1 );
  Vec_Int_t* vUnsat = Vec_IntAlloc( 100 );
  Vec_Int_t* vIntBuff;
  Vec_IntForEachEntry( pMan->vStat, stat, nid ) {
      if (stat > 0) { // stat count from 1
          if (stat > Vec_WecSize(ordUnsat)) { // we push to stat-1
              Vec_WecGrow( ordUnsat, stat );
              ordUnsat->nSize = stat;
          }
          Vec_WecPush( ordUnsat, stat - 1, nid );
      }
  }
  // Vec_WecPrint( ordUnsat, 0 );
  Vec_WecForEachLevel( ordUnsat, vIntBuff, i ) {
      if (i < level) continue;
      if (level == -1 && i < (pMan->iter - 1)) continue;
      Vec_IntForEachEntry( vIntBuff, nid, j ) {
          Vec_IntPush( vUnsat, nid );
      }
  }
  if (fHuge) {
    Vec_IntForEachEntry( pMan->vStat, stat, nid ) {
      if (stat == CEC_FD_HUGE) {
        Vec_IntPush( vUnsat, nid );
      }
    }
  }
  
  return vUnsat;
}
static inline Vec_Int_t*    Cec_ManFdSolved( Cec_ManFd_t* pMan, int fRev, int fIgnore ) {
  int stat, nid, i, j;
  Vec_Int_t* vOut = Vec_IntAlloc( 100 );
  Vec_IntForEachEntry( pMan->vStat, stat, nid ) {
      if (stat > 0 || stat == CEC_FD_HUGE || stat == CEC_FD_SAT) { // stat count from 1
          if (fRev == 0) Vec_IntPush( vOut, nid ); 
      } else if (stat == CEC_FD_UNKNOWN || stat == CEC_FD_UNSOLVE || 
                stat == CEC_FD_TRIVIAL || stat == CEC_FD_UNSHRINKABLE) {
          if (fRev == 1) Vec_IntPush( vOut, nid );
      } else {
          if (fIgnore == 1) Vec_IntPush( vOut, nid );
      }
  }
  return vOut;
}
static inline int           Cec_ManFdToSolve( Cec_ManFd_t* pMan, int nid ) {
  int stat = Vec_IntEntry(pMan->vStat, nid);
  return (stat == CEC_FD_UNKNOWN || stat == CEC_FD_TRIVIAL);
}
static inline void          Cec_ManFdSetPars( Cec_ManFd_t* pMan, Cec_ParFd_t* pPars ) {
    pMan->pPars = pPars;
}
static inline void          Cec_ManFdSetLevelType( Cec_ManFd_t* pMan, int fShort, int fFI, int fNdMin, int fNorm) {
    Cec_ParFdSetLevelType( pMan->pPars, fShort, fFI, fNdMin, fNorm );
}
// type: 0: for Anal, 1: fshortdict, 2: fFI, 3: fndMin, 4: fNorm, 5: ftype
static inline int           Cec_ManFdGetLevelType( Cec_ManFd_t* pMan, int type ) {
  assert(type < 6 && type >= 0);
  int level_type = pMan->pPars->levelType;
  int fNorm = (level_type >> CEC_FD_LEVELBITNORM) & 1;
  int fshortdict = (level_type >> CEC_FD_LEVELBITPATH) & 1;
  int fFI = (level_type >> CEC_FD_LEVELBITFIO) & 1;
  int fndMin = (level_type >> CEC_FD_LEVELBITNDMM) & 1;
  int ftype = (fNorm ? 8 : 0) + (fshortdict ? 4 : 0) + (fFI ? 2 : 0) + (fndMin ? 1 : 0);
  if (type == 0) return ftype & 7;
  else if (type == 1) return fshortdict;
  else if (type == 2) return fFI;
  else if (type == 3) return fndMin;
  else if (type == 4) return fNorm;
  else if (type == 5) return ftype;
  else return -1;
}
static inline void          Cec_ManFdPrintPars( Cec_ManFd_t* pMan ) {
  int fshortdict = Cec_ManFdGetLevelType( pMan, 1 );
  int fFI = Cec_ManFdGetLevelType( pMan, 2 );
  int fndMin = Cec_ManFdGetLevelType( pMan, 3 );
  int fNorm = Cec_ManFdGetLevelType( pMan, 4 );
  printf("Level\n");
  printf("....level is computed as %s\n", fNorm ? "normed" : (fFI ? "FI" : "FO"));
  if (fNorm) {
    printf("....use shortest path as dist for FI part = %d\n", fshortdict);
    printf("....pick min of dist as level = %d\n", fndMin);
    printf("....use shortest path as dist for FO part = %d\n", (fFI ^ fshortdict) );
  } else {
    printf("....use shortest path as dist = %d\n", fshortdict);
    printf("....pick min of dist as level = %d\n", fndMin);
  }
  printf("....Level is computed on pAbs = %d\n", pMan->pPars->fAbs);
  printf("Itp\n");
  printf("....Itp is computed with incremental sat solver = %d\n", pMan->pPars->fInc);
  printf("....Itp is computed on pAbs = %d\n", pMan->pPars->fAbsItp);
  printf("....G support contain merge frontier = %d\n", pMan->pPars->GType & 1);
  printf("....G supprot contain not same merge frontier support = %d\n", pMan->pPars->GType >> 1);
  printf("....cost eval of the patch is %d\n", pMan->pPars->costType);
  printf("....coef of cost eval = %f\n", pMan->pPars->coefPatch);
  printf("Shrink\n");
  printf("....keep special case %d in Gsupport first\n", pMan->pPars->fPickType);
  printf("....shrink base on level of %s\n", pMan->pPars->fLocalShrink ? "Gsupport" : (pMan->pPars->fAbs ? "pAbs" : "pGia"));
  printf("FD Rewrite\n");
  printf("....synthesis before replacement = %d\n", pMan->pPars->fTrim);
  printf("....synthesis after rewriting = %d\n", pMan->pPars->fSyn);
}
static inline int           Cec_ManFdGetPO( Cec_ManFd_t* pMan, int nid, int fAbs, int fSelf ) {
  int ckt = fSelf ? Gia_ObjColors( pMan->pGia, nid ) : Gia_ObjColors( pMan->pGia, nid ) ^ 3;
  if (ckt == 1) {
    return fAbs ? Gia_ObjId(pMan->pAbs, pMan->pObjAbs1) : Gia_ObjId(pMan->pGia, pMan->pObj1);
  } else if (ckt == 2) {
    return fAbs ? Gia_ObjId(pMan->pAbs, pMan->pObjAbs2) : Gia_ObjId(pMan->pGia, pMan->pObj2);
  } else {
    printf("%d is not in single circuit", nid);
    return -1;
  }
}


/*=== cecPat.c ============================================================*/
extern void                 Cec_ManPatSavePattern( Cec_ManPat_t *  pPat, Cec_ManSat_t *  p, Gia_Obj_t * pObj );
extern void                 Cec_ManPatSavePatternCSat( Cec_ManPat_t * pMan, Vec_Int_t * vPat );
extern Vec_Ptr_t *          Cec_ManPatCollectPatterns( Cec_ManPat_t *  pMan, int nInputs, int nWords );
extern Vec_Ptr_t *          Cec_ManPatPackPatterns( Vec_Int_t * vCexStore, int nInputs, int nRegs, int nWordsInit );
/*=== cecSeq.c ============================================================*/
extern int                  Cec_ManSeqResimulate( Cec_ManSim_t * p, Vec_Ptr_t * vInfo );
extern int                  Cec_ManSeqResimulateInfo( Gia_Man_t * pAig, Vec_Ptr_t * vSimInfo, Abc_Cex_t * pBestState, int fCheckMiter );
extern void                 Cec_ManSeqDeriveInfoInitRandom( Vec_Ptr_t * vInfo, Gia_Man_t * pAig, Abc_Cex_t * pCex );
extern int                  Cec_ManCountNonConstOutputs( Gia_Man_t * pAig );
extern int                  Cec_ManCheckNonTrivialCands( Gia_Man_t * pAig );
/*=== cecSolve.c ============================================================*/
extern int                  Cec_ObjSatVarValue( Cec_ManSat_t * p, Gia_Obj_t * pObj );
extern void                 Cec_ManSatSolve( Cec_ManPat_t * pPat, Gia_Man_t * pAig, Cec_ParSat_t * pPars, Vec_Int_t * vIdsOrig, Vec_Int_t * vMiterPairs, Vec_Int_t * vEquivPairs, int f0Proved );
extern void                 Cec_ManSatSolveCSat( Cec_ManPat_t * pPat, Gia_Man_t * pAig, Cec_ParSat_t * pPars );
extern Vec_Str_t *          Cec_ManSatSolveSeq( Vec_Ptr_t * vPatts, Gia_Man_t * pAig, Cec_ParSat_t * pPars, int nRegs, int * pnPats );
extern Vec_Int_t *          Cec_ManSatSolveMiter( Gia_Man_t * pAig, Cec_ParSat_t * pPars, Vec_Str_t ** pvStatus );
extern int                  Cec_ManSatCheckNode( Cec_ManSat_t * p, Gia_Obj_t * pObj );
extern int                  Cec_ManSatCheckNodeTwo( Cec_ManSat_t * p, Gia_Obj_t * pObj1, Gia_Obj_t * pObj2 );
extern void                 Cec_ManSavePattern( Cec_ManSat_t * p, Gia_Obj_t * pObj1, Gia_Obj_t * pObj2 );
extern Vec_Int_t *          Cec_ManSatReadCex( Cec_ManSat_t * p );
/*=== cecSolveG.c ============================================================*/
extern void                 CecG_ManSatSolve( Cec_ManPat_t * pPat, Gia_Man_t * pAig, Cec_ParSat_t * pPars, int f0Proved );
/*=== ceFraeep.c ============================================================*/
extern Gia_Man_t *          Cec_ManFraSpecReduction( Cec_ManFra_t * p );
extern int                  Cec_ManFraClassesUpdate( Cec_ManFra_t * p, Cec_ManSim_t * pSim, Cec_ManPat_t * pPat, Gia_Man_t * pNew );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

