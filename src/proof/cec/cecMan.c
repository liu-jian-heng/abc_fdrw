/**CFile****************************************************************

  FileName    [cecMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Combinational equivalence checking.]

  Synopsis    [Manager procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cecMan.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cecInt.h"

ABC_NAMESPACE_IMPL_START
extern Vec_Wec_t * Gia_ManExploreCuts( Gia_Man_t * pGia, int nCutSize0, int nCuts0, int fVerbose0 );
extern void Gia_WriteDotAigSimple( Gia_Man_t * p, char * pFileName, Vec_Int_t * vRwNd );
extern void Gia_SelfDefShow( Gia_Man_t * p, char * pFileName, Vec_Int_t * vLabel0, Vec_Int_t * vLabel1, Vec_Wec_t* vRelate);

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cec_ManSat_t * Cec_ManSatCreate( Gia_Man_t * pAig, Cec_ParSat_t * pPars )
{
    Cec_ManSat_t * p;
    // create interpolation manager
    p = ABC_ALLOC( Cec_ManSat_t, 1 );
    memset( p, 0, sizeof(Cec_ManSat_t) );
    p->pPars        = pPars;
    p->pAig         = pAig;
    // SAT solving
    p->nSatVars     = 1;
    p->pSatVars     = ABC_CALLOC( int, Gia_ManObjNum(pAig) );
    p->vUsedNodes   = Vec_PtrAlloc( 1000 );
    p->vFanins      = Vec_PtrAlloc( 100 );
    p->vCex         = Vec_IntAlloc( 100 );
    p->vVisits      = Vec_IntAlloc( 100 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Prints statistics of the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManSatPrintStats( Cec_ManSat_t * p )
{
    printf( "SAT solver statistics:\n" );
    Abc_Print( 1, "CO = %8d  ", Gia_ManCoNum(p->pAig) );
    Abc_Print( 1, "AND = %8d  ", Gia_ManAndNum(p->pAig) );
    Abc_Print( 1, "Conf = %5d  ", p->pPars->nBTLimit );
    Abc_Print( 1, "MinVar = %5d  ", p->pPars->nSatVarMax );
    Abc_Print( 1, "MinCalls = %5d\n", p->pPars->nCallsRecycle );
    Abc_Print( 1, "Unsat calls %6d  (%6.2f %%)   Ave conf = %8.1f   ", 
        p->nSatUnsat, p->nSatTotal? 100.0*p->nSatUnsat/p->nSatTotal : 0.0, p->nSatUnsat? 1.0*p->nConfUnsat/p->nSatUnsat :0.0 );
    Abc_PrintTimeP( 1, "Time", p->timeSatUnsat, p->timeTotal );
    Abc_Print( 1, "Sat   calls %6d  (%6.2f %%)   Ave conf = %8.1f   ", 
        p->nSatSat,   p->nSatTotal? 100.0*p->nSatSat/p->nSatTotal : 0.0,   p->nSatSat? 1.0*p->nConfSat/p->nSatSat : 0.0 );
    Abc_PrintTimeP( 1, "Time", p->timeSatSat,   p->timeTotal );
    Abc_Print( 1, "Undef calls %6d  (%6.2f %%)   Ave conf = %8.1f   ", 
        p->nSatUndec, p->nSatTotal? 100.0*p->nSatUndec/p->nSatTotal : 0.0, p->nSatUndec? 1.0*p->nConfUndec/p->nSatUndec : 0.0 );
    Abc_PrintTimeP( 1, "Time", p->timeSatUndec, p->timeTotal );
    Abc_PrintTime( 1, "Total time", p->timeTotal );
}

/**Function*************************************************************

  Synopsis    [Frees the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManSatStop( Cec_ManSat_t * p )
{
    if ( p->pSat )
        sat_solver_delete( p->pSat );
    Vec_IntFree( p->vCex );
    Vec_IntFree( p->vVisits );
    Vec_PtrFree( p->vUsedNodes );
    Vec_PtrFree( p->vFanins );
    ABC_FREE( p->pSatVars );
    ABC_FREE( p );
}



/**Function*************************************************************

  Synopsis    [Creates AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cec_ManPat_t * Cec_ManPatStart()  
{ 
    Cec_ManPat_t * p;
    p = ABC_CALLOC( Cec_ManPat_t, 1 );
    p->vStorage  = Vec_StrAlloc( 1<<20 );
    p->vPattern1 = Vec_IntAlloc( 1000 );
    p->vPattern2 = Vec_IntAlloc( 1000 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManPatPrintStats( Cec_ManPat_t * p )  
{ 
    Abc_Print( 1, "Latest: P = %8d.  L = %10d.  Lm = %10d. Ave = %6.1f. MEM =%6.2f MB\n", 
        p->nPats, p->nPatLits, p->nPatLitsMin, 1.0 * p->nPatLitsMin/p->nPats, 
        1.0*(Vec_StrSize(p->vStorage)-p->iStart)/(1<<20) );
    Abc_Print( 1, "Total:  P = %8d.  L = %10d.  Lm = %10d. Ave = %6.1f. MEM =%6.2f MB\n", 
        p->nPatsAll, p->nPatLitsAll, p->nPatLitsMinAll, 1.0 * p->nPatLitsMinAll/p->nPatsAll, 
        1.0*Vec_StrSize(p->vStorage)/(1<<20) );
    Abc_PrintTimeP( 1, "Finding  ", p->timeFind,   p->timeTotal );
    Abc_PrintTimeP( 1, "Shrinking", p->timeShrink, p->timeTotal );
    Abc_PrintTimeP( 1, "Verifying", p->timeVerify, p->timeTotal );
    Abc_PrintTimeP( 1, "Sorting  ", p->timeSort,   p->timeTotal );
    Abc_PrintTimeP( 1, "Packing  ", p->timePack,   p->timeTotal );
    Abc_PrintTime( 1, "TOTAL    ",  p->timeTotal );
}

/**Function*************************************************************

  Synopsis    [Deletes AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManPatStop( Cec_ManPat_t * p )  
{
    Vec_StrFree( p->vStorage );
    Vec_IntFree( p->vPattern1 );
    Vec_IntFree( p->vPattern2 );
    ABC_FREE( p );
}



/**Function*************************************************************

  Synopsis    [Creates AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cec_ManSim_t * Cec_ManSimStart( Gia_Man_t * pAig, Cec_ParSim_t *  pPars )  
{ 
    Cec_ManSim_t * p;
    p = ABC_ALLOC( Cec_ManSim_t, 1 );
    memset( p, 0, sizeof(Cec_ManSim_t) );
    p->pAig  = pAig;
    p->pPars = pPars;
    p->nWords = pPars->nWords;
    p->pSimInfo = ABC_CALLOC( int, Gia_ManObjNum(pAig) );
    p->vClassOld  = Vec_IntAlloc( 1000 );
    p->vClassNew  = Vec_IntAlloc( 1000 );
    p->vClassTemp = Vec_IntAlloc( 1000 );
    p->vRefinedC  = Vec_IntAlloc( 10000 );
    p->vCiSimInfo = Vec_PtrAllocSimInfo( Gia_ManCiNum(p->pAig), pPars->nWords );
    if ( pPars->fCheckMiter || Gia_ManRegNum(p->pAig) )
    {
        p->vCoSimInfo = Vec_PtrAllocSimInfo( Gia_ManCoNum(p->pAig), pPars->nWords );
        Vec_PtrCleanSimInfo( p->vCoSimInfo, 0, pPars->nWords );
    }
    p->iOut = -1;
    return p;
}

/**Function*************************************************************

  Synopsis    [Deletes AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManSimStop( Cec_ManSim_t * p )  
{
    Vec_IntFree( p->vClassOld );
    Vec_IntFree( p->vClassNew );
    Vec_IntFree( p->vClassTemp );
    Vec_IntFree( p->vRefinedC );
    if ( p->vCiSimInfo ) 
        Vec_PtrFree( p->vCiSimInfo );
    if ( p->vCoSimInfo ) 
        Vec_PtrFree( p->vCoSimInfo );
    ABC_FREE( p->pScores );
    ABC_FREE( p->pCexComb );
    ABC_FREE( p->pCexes );
    ABC_FREE( p->pMems );
    ABC_FREE( p->pSimInfo );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Creates AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cec_ManFra_t * Cec_ManFraStart( Gia_Man_t * pAig, Cec_ParFra_t *  pPars )  
{ 
    Cec_ManFra_t * p;
    p = ABC_ALLOC( Cec_ManFra_t, 1 );
    memset( p, 0, sizeof(Cec_ManFra_t) );
    p->pAig  = pAig;
    p->pPars = pPars;
    p->vXorNodes  = Vec_IntAlloc( 1000 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Deletes AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManFraStop( Cec_ManFra_t * p )  
{
    Vec_IntFree( p->vXorNodes );
    ABC_FREE( p );
}


// Affine equivalence


// Some util functions
void Cec_ManSetColor( Gia_Man_t * pGia, Gia_Obj_t * pObj1, Gia_Obj_t * pObj2 ){
    extern Gia_ManEquivSetColor_rec( Gia_Man_t * p, Gia_Obj_t * pObj, int fOdds );
    int fVerbose = 0;
    Gia_Obj_t * pObj;
    int i, nNodes[2], nShare;
    // assert( (Gia_ManPoNum(pGia) & 1) == 0 );
    Gia_ManForEachObj( pGia, pObj, i ) 
        Gia_ObjUnsetColor( pGia, i );
    
    // const0 and PIs have color A, B
    Gia_ObjSetColors( pGia, 0 );
    Gia_ManForEachPi( pGia, pObj, i )
        Gia_ObjSetColors( pGia, Gia_ObjId(pGia,pObj) );
    // nNodes[0] and nNodes[1] store the amount of nodes in A, B color
    nNodes[0] = nNodes[1] = Gia_ManPiNum(pGia);
    // iteratively mark the nodes
    // Gia_ManForEachPo( pGia, pObj, i )
    nNodes[0] += Gia_ManEquivSetColor_rec( pGia, pObj1, 0 );
    nNodes[1] += Gia_ManEquivSetColor_rec( pGia, pObj2, 1 );
    nShare = 0; // Gia_ManPiNum( pGia ) + 1;
    Gia_ManForEachObj( pGia, pObj, i ) 
      if ( Gia_ObjColors(pGia, i) == 3 ) nShare++;
      
    


    if ( fVerbose )
    {
        Abc_Print( 1, "CI = %7d  AND = %7d  A = %7d  B = %7d  Ad = %7d  Bd = %7d  AB = %7d.\n",
            Gia_ManPiNum(pGia), nNodes[0] + nNodes[1] - nShare, nNodes[0], nNodes[1], nNodes[0] - nShare, nNodes[1] - nShare,
            nShare );
    }
    // return (nDiffs[0] + nDiffs[1]) / 2;
}

// dist[i][j] = dist from node j to node i
int** Cec_ManDist( Gia_Man_t* pGia, int fMax ) {
    Gia_Obj_t *pObj, *pObj_child1, *pObj_child2, *pCo;
    Vec_Ptr_t *vNodes;
    int i, j, k, nid_cur, nid_child1, nid_child2, dist_cur, dist_child1, dist_child2;
    int nObj = Gia_ManObjNum(pGia);

    // Vec_Wec_t* dist = Vec_WecStart( nObj );
    int **dist = ABC_ALLOC( int*, nObj );
    // int (*dist)[nObj] = ABC_ALLOC( int, nObj*nObj );
    for (i = 0; i < nObj; i++) {
        // printf("alloc %d\n", i);
        dist[i] = ABC_ALLOC( int, nObj );
        for (j = 0; j < nObj; j++)
            dist[i][j] = -1;
    }
        

    
    // for (i = 0; i < nObj; i++) {
    //     for (j = 0; j < nObj; j++)
    //         printf("%d ", dist[i][j]);
    //     printf("\n");
    // }

    Gia_ManForEachObj( pGia, pObj, i ) {
        // Vec_WecWriteEntry( dist, nid_child1, nid_child1, 0 );
        // dist[nid_child1][nid_child1] = 0;
            nid_cur = Gia_ObjId(pGia, pObj);
            // printf("%d\n",nid_cur);
            dist[nid_cur][nid_cur] = 0;

            // if( Gia_ObjIsAnd(pObj) == 0 ) continue;
            // printf("%d\n", Gia_ObjId(pGia, pObj));
            if ( Gia_ObjIsAnd(pObj) || Gia_ObjIsCo(pObj) ) {
                // pObj_child1 = Gia_ObjFanin0(pObj);
                nid_child1 = Gia_ObjId(pGia, Gia_ObjFanin0(pObj));
            } else {
                nid_child1 = -1;
            }
            
            // dist[nid_cur][nid_child1] = 1;
            if ( Gia_ObjIsAnd(pObj) ) {
                nid_child2 = Gia_ObjId(pGia, Gia_ObjFanin1(pObj));
            } else {
                nid_child2 = -1;
            }
            // dist[nid_cur][nid_child2] = 1;

            for (k = 0; k < nid_cur; k++) {
                dist_child1 = nid_child1 < 0 ? -1 : (Gia_ManObj(pGia, nid_child1)->fMark0 && k != nid_child1) ? -1 : dist[nid_child1][k];
                dist_child2 = nid_child2 < 0 ? -1 : (Gia_ManObj(pGia, nid_child2)->fMark0 && k != nid_child2) ? -1 : dist[nid_child2][k];
                if (dist_child1 == -1 && dist_child2 == -1) {
                    dist[nid_cur][k] = -1;
                }
                else if (dist_child1 == -1 && dist_child2 != -1) {
                    dist[nid_cur][k] = dist_child2 + 1;
                }
                else if (dist_child2 == -1 && dist_child1 != -1) {
                    // printf("label2\n");
                    dist[nid_cur][k] = dist_child1 + 1;
                    // continue;
                }
                else {
                    dist[nid_cur][k] = fMax ? Abc_MaxInt(dist_child1 + 1, dist_child2 + 1) : Abc_MinInt(dist_child1 + 1, dist_child2 + 1);
                }

                // if (dist_child1 == -1 || dist_child2 == -1) {
                //     continue;
                // }
                // printf("%d %d\n", Vec_WecEntryEntry( dist, k, nid_child1 ), Vec_WecEntryEntry( dist, k, nid_child2 ));
            }
        
    }
    
    
    return dist;
}

void Cec_ManDistEasy( Gia_Man_t* pGia, Vec_Int_t* vFI, int nidFo, int fMax ) {
    int entry, i;
    Gia_Obj_t* pObjbuf, *pObjFi;
    Vec_Int_t* nidStack;
    if (vFI) {
        Vec_IntForEachEntry( vFI, entry, i ) {
            Gia_ManObj( pGia, entry )->fMark0 = 1;
        }
    }
    Gia_ManForEachCi( pGia, pObjbuf, i ) {
        pObjbuf->fMark0 = 1;
    }

    Gia_ManFillValue( pGia );
    Gia_ObjSetValue( Gia_ManObj( pGia, nidFo ), 0 );
    Gia_ManObj( pGia, nidFo )->fMark0 = 1;
    Gia_ManForEachAndReverse( pGia, pObjbuf, i ) { // make sure all FanOut is computed
        if ( Gia_ObjId(pGia, pObjbuf) > nidFo ) continue;
        if ( Gia_ObjValue(pObjbuf) == -1 ) continue;
        if (pObjbuf->fMark0 == 0) continue;
        pObjFi = Gia_ObjFanin0( pObjbuf );
        if ( Gia_ObjValue(pObjFi) == -1 ) {
            Gia_ObjSetValue( pObjFi, Gia_ObjValue( pObjbuf ) + 1 );
            pObjFi->fMark0 ^= 1;
        } else {
            if (fMax) {
                if (Gia_ObjValue(pObjFi) < Gia_ObjValue( pObjbuf ) + 1)
                    Gia_ObjSetValue( pObjFi, Gia_ObjValue( pObjbuf ) + 1 );
            } else {
                if (Gia_ObjValue(pObjFi) > Gia_ObjValue( pObjbuf ) + 1)
                    Gia_ObjSetValue( pObjFi, Gia_ObjValue( pObjbuf ) + 1 );
            }
        }
        pObjFi = Gia_ObjFanin1( pObjbuf );
        if ( Gia_ObjValue(pObjFi) == -1 ) {
            Gia_ObjSetValue( pObjFi, Gia_ObjValue( pObjbuf ) + 1 );
            pObjFi->fMark0 ^= 1;
        } else {
            if (fMax) {
                if (Gia_ObjValue(pObjFi) < Gia_ObjValue( pObjbuf ) + 1)
                    Gia_ObjSetValue( pObjFi, Gia_ObjValue( pObjbuf ) + 1 );
            } else {
                if (Gia_ObjValue(pObjFi) > Gia_ObjValue( pObjbuf ) + 1)
                    Gia_ObjSetValue( pObjFi, Gia_ObjValue( pObjbuf ) + 1 );
            }
        }
    }


    Gia_ManForEachObj( pGia, pObjbuf, i ) {
        pObjbuf->fMark0 = 0;
    }

}
Vec_Wec_t* Cec_ManDistAnalyze( Gia_Man_t* pGia, Vec_Int_t* targets, Vec_Int_t* vFI, Vec_Int_t* vFO, int** dist_long, int** dist_short) {
    Vec_Wec_t* vAnal;
    Gia_Obj_t* pObjbuf;
    Vec_Int_t* vNdBuff, *vFIBuff, *vFOBuff;
    int nidbuf1, nidbuf, nidbuf2, i, j, buf, buf1, buf2;
    int max_long_FI, min_long_FI, max_long_FO, min_long_FO, max_short_FO, min_short_FO, max_short_FI, min_short_FI;
    max_long_FI = -1; min_long_FI = Gia_ManObjNum(pGia);
    max_long_FO = -1; min_long_FO = Gia_ManObjNum(pGia);
    max_short_FO = -1; min_short_FO = Gia_ManObjNum(pGia);
    max_short_FI = -1; min_short_FI = Gia_ManObjNum(pGia);

    if (targets == NULL) {
        vNdBuff = Vec_IntAlloc( Gia_ManObjNum(pGia) );
        Gia_ManForEachObj( pGia, pObjbuf, i )
            Vec_IntPush( vNdBuff, Gia_ObjId(pGia, pObjbuf) );
    } else vNdBuff = targets; // why dup?
    vAnal = Vec_WecStart( Vec_IntSize(vNdBuff) + 1 );

    if (vFI == NULL) {
        vFIBuff = Cec_ManGetFIO( pGia, 0, 0 );
    } else vFIBuff = vFI;

    if (vFO == NULL) {
        vFOBuff = Cec_ManGetFIO( pGia, 0, 1 );
    } else vFOBuff = vFO;
    
    Vec_IntForEachEntry( vNdBuff, nidbuf, i ) {
        // printf("%d\n", nidbuf);
        if (nidbuf == -1) continue;
        Vec_WecPush( vAnal, i, nidbuf);
        Vec_WecPush( vAnal, i, pGia->pReprs ? Gia_ObjColors(pGia, nidbuf) : 0);
        if (dist_long) {
            buf1 = -1;
            buf2 = Gia_ManObjNum(pGia);
            if (vFOBuff) {
                Vec_IntForEachEntry( vFOBuff, nidbuf1, j ) {
                    if (dist_long[nidbuf1][nidbuf] == -1) continue; // not connected
                    if (buf1 < dist_long[nidbuf1][nidbuf]) buf1 = dist_long[nidbuf1][nidbuf];
                    if (buf2 > dist_long[nidbuf1][nidbuf]) buf2 = dist_long[nidbuf1][nidbuf]; 
                }
            }
            Vec_WecPush( vAnal, i, buf1 );
            Vec_WecPush( vAnal, i, (buf2 == Gia_ManObjNum(pGia)) ? -1 : buf2 );
            if (max_long_FO < buf1) max_long_FO = buf1;
            if (buf2 != -1 && min_long_FO > buf2) min_long_FO = buf2;

            buf1 = -1;
            buf2 = Gia_ManObjNum(pGia);
            if (vFIBuff) {
                Vec_IntForEachEntry( vFIBuff, nidbuf2, j ) {
                    if (dist_long[nidbuf][nidbuf2] == -1) continue;
                    if (buf1 < dist_long[nidbuf][nidbuf2]) buf1 = dist_long[nidbuf][nidbuf2];
                    if (buf2 > dist_long[nidbuf][nidbuf2]) buf2 = dist_long[nidbuf][nidbuf2]; 
                }
            }
            
            Vec_WecPush( vAnal, i, buf1 );
            Vec_WecPush( vAnal, i, (buf2 == Gia_ManObjNum(pGia)) ? -1 : buf2 );
            if (max_long_FI < buf1) max_long_FI = buf1;
            if (buf2 != -1 && min_long_FI > buf2) min_long_FI = buf2;
        }

        if (dist_short) {
            buf1 = -1;
            buf2 = Gia_ManObjNum(pGia);
            Vec_IntForEachEntry( vFOBuff, nidbuf1, j ) {
                if (dist_short[nidbuf1][nidbuf] == -1) continue; // not connected
                if (buf1 < dist_short[nidbuf1][nidbuf]) buf1 = dist_short[nidbuf1][nidbuf];
                if (buf2 > dist_short[nidbuf1][nidbuf]) buf2 = dist_short[nidbuf1][nidbuf]; 
            }
            Vec_WecPush( vAnal, i, buf1 );
            Vec_WecPush( vAnal, i, (buf2 == Gia_ManObjNum(pGia)) ? -1 : buf2 );
            if (max_short_FO < buf1) max_short_FO = buf1;
            if (buf2 != -1 && min_short_FO > buf2) min_short_FO = buf2;

            buf1 = -1;
            buf2 = Gia_ManObjNum(pGia);
            Vec_IntForEachEntry( vFIBuff, nidbuf2, j ) {
                if (dist_short[nidbuf][nidbuf2] == -1) continue;
                if (buf1 < dist_short[nidbuf][nidbuf2]) buf1 = dist_short[nidbuf][nidbuf2];
                if (buf2 > dist_short[nidbuf][nidbuf2]) buf2 = dist_short[nidbuf][nidbuf2]; 
            }
            
            Vec_WecPush( vAnal, i, buf1 );
            Vec_WecPush( vAnal, i, (buf2 == Gia_ManObjNum(pGia)) ? -1 : buf2 );
            if (max_short_FI < buf1) max_short_FI = buf1;
            if (buf2 != -1 && min_short_FI > buf2) min_short_FI = buf2;
        }
    }
    Vec_WecPush( vAnal, Vec_IntSize(vNdBuff), max_long_FO );
    Vec_WecPush( vAnal, Vec_IntSize(vNdBuff), min_long_FO );
    Vec_WecPush( vAnal, Vec_IntSize(vNdBuff), max_long_FI );
    Vec_WecPush( vAnal, Vec_IntSize(vNdBuff), min_long_FI );
    Vec_WecPush( vAnal, Vec_IntSize(vNdBuff), max_short_FO );
    Vec_WecPush( vAnal, Vec_IntSize(vNdBuff), min_short_FO );
    Vec_WecPush( vAnal, Vec_IntSize(vNdBuff), max_short_FI );
    Vec_WecPush( vAnal, Vec_IntSize(vNdBuff), min_short_FI );
    if (vFO == NULL) Vec_IntFree( vFOBuff );
    if (vFI == NULL) Vec_IntFree( vFIBuff );
    if (targets == NULL) Vec_IntFree( vNdBuff );
    return vAnal;
}
Vec_Int_t* Cec_ManGetFIO( Gia_Man_t* pGia, Vec_Int_t* targets, int fPickFO ) {
    Vec_Int_t* vOut = Vec_IntAlloc( 100 );
    Gia_Obj_t* pObj;
    int nid, j;
    Gia_ManFillValue(pGia);
    if (targets == NULL) {
        targets = Vec_IntAlloc( 100 );
        if (fPickFO == 1) {
            Gia_ManForEachCo( pGia, pObj, j ) {
                Vec_IntPush( vOut, Gia_ObjId(pGia, pObj) );
            }
        } else {
            Gia_ManForEachCi( pGia, pObj, j ) {
                Vec_IntPush( vOut, Gia_ObjId(pGia, pObj) );
            }
        }
    } else {
        Vec_IntForEachEntry( targets, nid, j ) {
            if (nid == -1) continue;
            Gia_ObjSetValue(Gia_ManObj(pGia, nid), 0);
        }
        Vec_IntForEachEntry( targets, nid, j ) {
            if (nid == -1) continue;
            if (Gia_ObjFanin0(Gia_ManObj(pGia, nid))->Value == 0) Gia_ObjSetValue(Gia_ObjFanin0(Gia_ManObj(pGia, nid)), 1);
            if (Gia_ObjFanin1(Gia_ManObj(pGia, nid))->Value == 0) Gia_ObjSetValue(Gia_ObjFanin1(Gia_ManObj(pGia, nid)), 1);
        }
        Vec_IntForEachEntry( targets, nid, j ) {
            if (nid == -1) continue;
            if (fPickFO == 1 && Gia_ObjValue(Gia_ManObj(pGia, nid)) == 0) Vec_IntPush(vOut, nid);
            if (fPickFO == 0 && (Gia_ObjFanin0(Gia_ManObj(pGia, nid))->Value == -1 || Gia_ObjFanin1(Gia_ManObj(pGia, nid))->Value == -1)) Vec_IntPush(vOut, nid);
        }
    }
    
    return vOut;
}

/* 
  fSetVal = 0: not set value, > 0: add value fSetVal, < 0: add abs(fSetVal) but not jump
  notice val is ~0 initially, thus '0' means in the set
*/

void Cec_ManSubcircuit_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Ptr_t * vNodes, int fSetVal ) {   
    Gia_Obj_t* pNd; int dummy;
    if ( Gia_ObjIsTravIdCurrent(p, pObj) ) return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    // if (vNodes->pArray == NULL) return;
    if ( fSetVal == 0 && pObj->Value != ~0 ) return; // org: fSetVal >= 0
    else if ( fSetVal > 0 ) {
        if ((pObj->Value & fSetVal) == 0) return;
        pObj->Value ^= fSetVal;
    }
    else if ( fSetVal < 0 ) {
        pObj->Value ^= Abc_AbsInt(fSetVal);
    }
    // if ( fSetVal != 0 ) {
    //     pObj->Value ^= Abc_AbsInt(fSetVal);
    // }
    // printf("%d\n", Gia_ObjId(p, pObj) );

    if ( pObj->fMark0 ) return;
    
    if ( Gia_ObjIsAnd(pObj) ) {
        Cec_ManSubcircuit_rec( p, Gia_ObjFanin0(pObj), vNodes, fSetVal );
        Cec_ManSubcircuit_rec( p, Gia_ObjFanin1(pObj), vNodes, fSetVal );
        // printf("%d\n", vNodes);
        if (Gia_ObjFanin0(pObj)->fMark1 || Gia_ObjFanin1(pObj)->fMark1) {
            pObj->fMark1 = 1;
            if (vNodes != NULL) Vec_PtrPush( vNodes, pObj );
        }
    }
    else if ( Gia_ObjIsCo(pObj) ) {
        Cec_ManSubcircuit_rec( p, Gia_ObjFanin0(pObj), vNodes, fSetVal );
        // if (vNodes != NULL) Vec_PtrPush( vNodes, pObj );
        if (Gia_ObjFanin0(pObj)->fMark1) {
            pObj->fMark1 = 1;
            if (vNodes != NULL) Vec_PtrPush( vNodes, pObj );
        }
    }
    else if ( Gia_ObjIsCi(pObj) ) {
        printf("Value: %d\n", pObj->Value);
        assert( 0 );
    } else {
        assert(0);
    }
}

Vec_Ptr_t* Cec_ManSubcircuit( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vLeaveId, Vec_Int_t * vCheckTFO, int fSetVal, int fGetNodes ) {
    // printf("in subcircuit\n");
    int i, entry;
    Gia_Obj_t* pObjbuf;
    Vec_Ptr_t* vNodes = fGetNodes ? Vec_PtrAlloc( 100 ) : NULL;
    // printf("j\n");
    // Vec_Int_t* vMark0 = Cec_ManDumpMark( p, 0 );
    // Vec_Int_t* vMark1 = Cec_ManDumpMark( p, 1 );
    // printf("i\n");
    // printf("%d\n", vNodes->pArray);
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrent( p, Gia_ManConst0(p) );

    Gia_ManForEachObj( p, pObjbuf, i ) {
        pObjbuf->fMark0 = 0;
        pObjbuf->fMark1 = 0;
    }

    if (vLeaveId) 
        Vec_IntForEachEntry( vLeaveId, entry, i ) Gia_ManObj( p, entry )->fMark0 = 1;
    Gia_ManForEachCi( p, pObjbuf, i ) pObjbuf->fMark0 = 1;
    
    if (vCheckTFO) {
        Vec_IntForEachEntry( vCheckTFO, entry, i ) {
            Gia_ManObj( p, entry )->fMark1 = 1;
        }
    } else {
        Gia_ManForEachObj( p, pObjbuf, i ) {
            if (fSetVal >= 0 && pObjbuf->Value != ~0) pObjbuf->fMark1 = 1;
            else pObjbuf->fMark1 = pObjbuf->fMark0;
        }
    }
    
    Cec_ManSubcircuit_rec( p, pObj, vNodes, fSetVal );
    // Cec_ManLoadMark( p, vMark0, 0, 0 );
    // Cec_ManLoadMark( p, vMark1, 1, 0 );
    // printf("out\n");
    return vNodes;

}

unsigned int Cec_ManPatch( Gia_Man_t* pNew, Vec_Ptr_t* cone ) {
    int dummy;
    Gia_Obj_t *pObj;
    assert( Vec_PtrSize(cone) > 0 );
    Vec_PtrForEachEntry( Gia_Obj_t *, cone, pObj, dummy ) {
      if ( Gia_ObjIsXor(pObj) )
          pObj->Value = Gia_ManAppendXor( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
      else if ( Gia_ObjIsCo(pObj) ) {
          pObj->Value = Gia_ObjFanin0Copy(pObj);
      }
      else {
          assert( Gia_ObjIsAnd(pObj) );
          pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
      }
      // Gia_ManSetValue( Gia_ManObj(pNew, Abc_Lit2Var(pObj->Value)), -1 );
      // printf("%d\n", pObj->Value);
    }
    return pObj->Value;
}
Gia_Man_t*           Cec_ManGetSubcircuit( Gia_Man_t* pGia, Vec_Int_t* vFI, Vec_Int_t* vFO, int fRed ) {
    Gia_Man_t* pSub, *pGia_cp;
    int k, nid;
    Vec_Ptr_t* coneNdbuff;
    Vec_Int_t* vIntBuff;
    int val;
    Gia_Obj_t* pObjbuf;
    // char* pName = ABC_ALLOC(char, 100);

    // Gia_SelfDefShow( pGia, "test.dot", 0, 0, 0 );
    pGia_cp = Gia_ManDup( pGia );
    pSub = Gia_ManStart( Gia_ManObjNum(pGia_cp) );
    // printf("1\n");
    Gia_ManFillValue( pGia_cp );
    Vec_IntForEachEntry( vFO, nid, k ) {
        pObjbuf = Gia_ManObj( pGia_cp, nid );
        Cec_ManSubcircuit( pGia_cp, pObjbuf, vFI, 0, 1, 0 );
    }
    if (fRed) {
        Vec_IntForEachEntry( vFI, nid, k ) {
            pObjbuf = Gia_ManObj( pGia_cp, nid );
            if (Gia_ObjValue(pObjbuf) == -1) Vec_IntWriteEntry( vFI, k, -1 );
        }
        Vec_IntRemoveAll( vFI, -1, 0 );
    }

    Gia_ManFillValue( pGia_cp );
    Gia_ManConst0(pGia_cp)->Value = 0;
    Vec_IntForEachEntry( vFI, nid, k ) Gia_ObjSetValue(Gia_ManObj(pGia_cp, nid), Gia_ManAppendCi( pSub ));
    Vec_IntForEachEntry( vFO, nid, k ) {
        pObjbuf = Gia_ManObj( pGia_cp, nid );
        // printf("Output: %d\n", nid);
        coneNdbuff = Cec_ManSubcircuit( pGia_cp, pObjbuf, vFI, 0, 0, 1 );
        if (Vec_PtrSize(coneNdbuff) > 0) val = Cec_ManPatch( pSub, coneNdbuff );
        else val = Gia_ObjValue( pObjbuf );
        Vec_PtrFree( coneNdbuff );
        if (fRed == 0 && val != -1) Gia_ManAppendCo( pSub, val );
        // else Vec_IntWriteEntry( vFO, k, -1 );
    }
    Gia_ManForEachCi( pGia_cp, pObjbuf, k ) {
        if (Gia_ObjValue(pObjbuf) != -1 && Vec_IntFind(vFI, Gia_ObjId(pGia_cp, pObjbuf)) == -1) assert(0);
    }
    
    if (fRed) {
        vIntBuff = Cec_ManGetTop( pSub );
        Vec_IntForEachEntry( vIntBuff, val, k ) 
            Gia_ManAppendCo( pSub, Abc_Var2Lit(val, 0) );
        Vec_IntFree( vIntBuff );
    }

    Cec_ManMapCopy( pSub, pGia_cp, 0 );
    
    Gia_ManStop( pGia_cp );
    return pSub;
}
// Gia_Man_t* Cec_ManGetSubcircuitMarked( Gia_Man_t* pGia ) {
//     Gia_Obj_t* pObj;
//     Gia_Man_t* pSub, *pGia_cp;
//     int i;
//     pGia_cp = Gia_ManDup( pGia );
//     pSub = Gia_ManStart( Gia_ManObjNum(pGia) );
//     Gia_ManForEachCi( pGia, pObj, i ) {
//         if (pObj->fMark0) Gia_ObjSetValue(pObj, Gia_ManAppendCi( pSub ));
//     }
//     Gia_ManForEachAnd( pGia, pObj, i ) {
//         if (pObj->fMark0 && Gia_Obj) Gia_ObjSetValue(pObj, Gia_ManAppendAnd( pSub, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) ));
//     }

// }
Gia_Man_t* Cec_ManGetTFI( Gia_Man_t* pGia, Vec_Ptr_t* vObj, int fRedCi ) {
    Gia_Man_t* pSub;
    int k, nid;
    Vec_Ptr_t* coneNdbuff;
    unsigned int val;
    Vec_Int_t* vPI = Vec_IntAlloc( 100 );
    Vec_Int_t* vIntBuff;
    Vec_Ptr_t* vTar;
    Gia_Obj_t* pObjbuf;
    char* pName = ABC_ALLOC(char, 100);

    // Gia_SelfDefShow( pGia, "test.dot", 0, 0, 0 );
    pSub = Gia_ManStart( Gia_ManObjNum(pGia) );
    // printf("1\n");
    Gia_ManFillValue( pGia );
    if (vObj) vTar = vObj;
    else {
        vTar = Vec_PtrAlloc( Gia_ManCoNum( pGia ) );
        Gia_ManForEachCo( pGia, pObjbuf, k) 
            Vec_PtrPush( vTar, pObjbuf );
        
    }
        
    
    Vec_PtrForEachEntry( Gia_Obj_t *, vTar, pObjbuf, k ) {
        Cec_ManSubcircuit( pGia, pObjbuf, 0, 0, 1, 0 );
    }
    
    Gia_ManForEachCi( pGia, pObjbuf, k ) {
        if (Gia_ObjValue(pObjbuf) != -1) Vec_IntPush( vPI, Gia_ObjId(pGia, pObjbuf) );
        else if (fRedCi == 0) Vec_IntPush( vPI, Gia_ObjId(pGia, pObjbuf) );
    }

    Gia_ManFillValue( pGia );
    Gia_ManConst0(pGia)->Value = 0;
    Vec_IntForEachEntry( vPI, nid, k ) {
        Gia_ObjSetValue(Gia_ManObj(pGia, nid), Gia_ManAppendCi( pSub ));
    }
    Vec_PtrForEachEntry( Gia_Obj_t *, vTar, pObjbuf, k ) {
        coneNdbuff = Cec_ManSubcircuit( pGia, pObjbuf, 0, 0, 0, 1 );
        if (Vec_PtrSize(coneNdbuff) > 0) Cec_ManPatch( pSub, coneNdbuff );
    }
    Cec_ManMapCopy( pSub, pGia, 0 );
    vIntBuff = Cec_ManGetTop( pSub );
    Vec_IntForEachEntry( vIntBuff, val, k ) {
        Gia_ManAppendCo( pSub, Abc_Var2Lit( val, 0 ) );
    }
    Vec_IntFree( vIntBuff );
    
    Vec_PtrFree( coneNdbuff );
    Vec_IntFree( vPI );
    return pSub;
}
Gia_Man_t* Cec_ManGetAbs( Gia_Man_t* pGia, Gia_Obj_t* pObj1, Gia_Obj_t* pObj2, Vec_Int_t* vMerge, int fVerbose ) {
    Gia_Man_t *pSub;
    Gia_Obj_t *pObjbuf, *pObjbuf1, *pObjbuf2;
    // Vec_Int_t *vMerge = vFI ? Vec_IntDup(vFI) : NULL;
    Vec_Ptr_t *vNdCone;
    int buf, buf1, buf2, nid, i;

    if (Vec_IntSize(vMerge) == 0) {
        Cec_ManGetMerge_old( pGia, pObj1, pObj2, vMerge );
    }
    pSub = Gia_ManStart( Gia_ManObjNum(pGia) );
    
    Gia_ManFillValue(pGia);
    Gia_ManConst0(pGia)->Value = 0;
    Vec_IntForEachEntry( vMerge, nid, i ) 
        Gia_ObjSetValue(Gia_ManObj(pGia, nid), Gia_ManAppendCi( pSub ));
    // Vec_IntPrint( vMerge );

    vNdCone = Cec_ManSubcircuit( pGia, pObj1, vMerge, 0, 0, 1 );
    if (Vec_PtrSize(vNdCone) == 0) assert(0);
    else buf1 = Cec_ManPatch( pSub, vNdCone );
    Vec_PtrFree( vNdCone );
    Gia_ManAppendCo( pSub, buf1 );
    
    vNdCone = Cec_ManSubcircuit( pGia, pObj2, vMerge, 0, 0, 1 );
    if (Vec_PtrSize(vNdCone) == 0) assert(0);
    else buf2 = Cec_ManPatch( pSub, vNdCone );
    Vec_PtrFree( vNdCone );
    Gia_ManAppendCo( pSub, buf2 );

    if (pSub->pReprs == NULL) {
        pSub->pReprs = ABC_CALLOC( Gia_Rpr_t, Gia_ManObjNum(pSub) );
        Gia_ManForEachObj( pSub, pObjbuf, buf2 )
            Gia_ObjSetRepr( pSub, buf2, Gia_ObjIsAnd(pObjbuf) ? 0 : GIA_VOID );
    }
    Cec_ManSetColor( pSub, Gia_ObjFanin0(Gia_ManCo(pSub, 0)), Gia_ObjFanin0(Gia_ManCo(pSub, 1)) );
    return pSub;
}
void Cec_ManGetMerge_old( Gia_Man_t* pGia, Gia_Obj_t* pObj1, Gia_Obj_t* pObj2, Vec_Int_t* vMerge ) {
    Gia_Obj_t* pObj, *pObjbuf1, *pObjbuf2;
    Vec_Int_t* vIntBuff;
    int buf, i, cnt, max1, max2;
    float eval1, eval2, avg1, avg2;

    Vec_IntClear( vMerge );
    if (pGia->pReprs == NULL) {
        pGia->pReprs = ABC_CALLOC( Gia_Rpr_t, Gia_ManObjNum(pGia) );
        Gia_ManForEachObj( pGia, pObj, buf )
            Gia_ObjSetRepr( pGia, buf, Gia_ObjIsAnd(pObj) ? 0 : GIA_VOID );
    }
    Cec_ManSetColor( pGia, pObj1, pObj2 );

    Gia_ManForEachObj( pGia, pObj, i ) {
        if ( Gia_ObjIsAnd(pObj) == 0 ) continue;
        if ( Gia_ObjColors(pGia, Gia_ObjId(pGia, pObj)) == 3 ) {
            continue;
        }
        pObjbuf1 = Gia_ObjFanin0( pObj );
        pObjbuf2 = Gia_ObjFanin1( pObj );
        if ( Gia_ObjColors(pGia, Gia_ObjId(pGia,pObjbuf1)) == 3 ) Vec_IntPushUnique( vMerge, Gia_ObjId(pGia,pObjbuf1) );
        if ( Gia_ObjColors(pGia, Gia_ObjId(pGia,pObjbuf2)) == 3 ) Vec_IntPushUnique( vMerge, Gia_ObjId(pGia,pObjbuf2) );
    }

    vIntBuff = Vec_IntAlloc(1);
    Gia_ManLevelNum(pGia);
    Vec_IntForEachEntry( vMerge, buf, i ) {
        Vec_IntPush( vIntBuff, Gia_ObjLevelId(pGia, buf) );
    }
    Vec_IntSelectSortCost2Reverse( vMerge->pArray, Vec_IntSize(vMerge), vIntBuff->pArray );

}
/* fLow iff vMerge must have path to vNid1 and vNid2 in abstracted circuit
   fSep iff vMerge must seperate PO from PI thoroughly*/
void Cec_ManGetMerge( Gia_Man_t* pGia, Vec_Int_t* vNid1, Vec_Int_t* vNid2, Vec_Int_t* vMerge, int fLow, int fSep ) {
    // TO TEST
    int debug = 0;
    int entry, i, flag, sig, nid, buf1, buf2;
    Gia_Obj_t* pObjbuf, *pObjbuf1, *pObjbuf2;
    Vec_Int_t* vIntBuff1, *vIntBuff2, *vIntBuff;

    Vec_IntClear( vMerge );
    
    if (vNid1) vIntBuff1 = Vec_IntDup(vNid1);
    else {
        vIntBuff = Cec_ManGetMFFCost( pGia, 0 );
        nid = Vec_IntArgMax( vIntBuff );
        vIntBuff1 = Vec_IntAlloc(1);
        Vec_IntPush( vIntBuff1, nid );
        Vec_IntFree( vIntBuff );
    }
    if (debug) Vec_IntPrint( vIntBuff1 );

    if (vNid2) vIntBuff2 = Vec_IntDup(vNid2);
    else vIntBuff2 = Cec_ManGetSibling( pGia, vIntBuff1, -1, 0 );
    if (debug) Vec_IntPrint( vIntBuff2 );
    
    Gia_ManFillValue( pGia );
    Vec_IntForEachEntry( vIntBuff1, entry, i )
        Cec_ManSubcircuit( pGia, Gia_ManObj(pGia, entry), 0, 0, 1, 0 );
    Vec_IntForEachEntry( vIntBuff2, entry, i )
        Cec_ManSubcircuit( pGia, Gia_ManObj(pGia, entry), 0, 0, 2, 0 );
    Vec_IntFree( vIntBuff1 );
    Vec_IntFree( vIntBuff2 );

    if (debug) {
        vIntBuff = Vec_IntAlloc(1);
        vIntBuff2 = Vec_IntAlloc(1);
        Gia_ManForEachObj( pGia, pObjbuf, i ) {
            if ( ~Gia_ObjValue(pObjbuf) & 1 ) 
                Vec_IntPush( vIntBuff, Gia_ObjId(pGia, pObjbuf) );
            if ( ~Gia_ObjValue(pObjbuf) & 2 )
                Vec_IntPush( vIntBuff2, Gia_ObjId(pGia, pObjbuf) );
        }
        Gia_SelfDefShow( pGia, "color.dot", vIntBuff, vIntBuff2, 0 );
        Vec_IntFree( vIntBuff );
        Vec_IntFree( vIntBuff2 );
    }

    // real vMerge
    Gia_ManForEachObj( pGia, pObjbuf, i ) {
        if ( Gia_ObjIsConst0(pObjbuf) ) continue;
        if ( Gia_ObjIsCi(pObjbuf) ) continue;
        sig = ~Gia_ObjValue(pObjbuf);
        if ( (sig & 3) == 3 || (sig & 3) == 0 ) {
            continue;
        }
        pObjbuf1 = Gia_ObjFanin0( pObjbuf );
        sig = ~Gia_ObjValue(pObjbuf1);
        if ( (sig & 7) == 3 ) pObjbuf1->Value ^= 4;
        if ( Gia_ObjIsCo(pObjbuf) ) continue;
        pObjbuf2 = Gia_ObjFanin1( pObjbuf );
        sig = ~Gia_ObjValue(pObjbuf2);
        if ( (sig & 7) == 3 ) pObjbuf2->Value ^= 4;
    }
    vIntBuff = Vec_IntAlloc(1);
    Gia_ManForEachObj( pGia, pObjbuf, i )
        if ( ~Gia_ObjValue(pObjbuf) & 4 ) 
            Vec_IntPush( vIntBuff, Gia_ObjId(pGia, pObjbuf) );
    // to make vMerge can block PO from PI thoroughly
    if (fSep) {
        Cec_ManGetTFO( pGia, vIntBuff, -1, 16 );
        Vec_IntFree( vIntBuff );
        Gia_ManForEachObj( pGia, pObjbuf, i ) {
            if ( Gia_ObjIsConst0(pObjbuf) ) continue;
            if ( Gia_ObjIsCi(pObjbuf) ) continue;
            sig = ~Gia_ObjValue(pObjbuf);
            if ( (sig & 16) == 0 ) {
                continue;
            }
            pObjbuf1 = Gia_ObjFanin0( pObjbuf );
            sig = ~Gia_ObjValue(pObjbuf1);
            if ( (sig & 20) == 0 ) pObjbuf1->Value ^= 4;
            if ( Gia_ObjIsCo(pObjbuf) ) continue;
            pObjbuf2 = Gia_ObjFanin1( pObjbuf );
            sig = ~Gia_ObjValue(pObjbuf2);
            if ( (sig & 20) == 0 ) pObjbuf2->Value ^= 4;
        }
    }
    if (debug) {
        vIntBuff = Vec_IntAlloc(1);
        vIntBuff2 = Vec_IntAlloc(1);
        Gia_ManForEachObj( pGia, pObjbuf, i ) {
            if ( ~Gia_ObjValue(pObjbuf) & 4 ) 
                Vec_IntPush( vIntBuff, Gia_ObjId(pGia, pObjbuf) );
            if ( ~Gia_ObjValue(pObjbuf) & 3 == 3 )
                Vec_IntPush( vIntBuff2, Gia_ObjId(pGia, pObjbuf) );
        }
        Gia_SelfDefShow( pGia, "merge_high.dot", vIntBuff, vIntBuff2, 0 );
        Vec_IntFree( vIntBuff );
        Vec_IntFree( vIntBuff2 );
    }

    if (fLow == 0) {
        Gia_ManForEachObj( pGia, pObjbuf, i ) 
            if ( ~Gia_ObjValue(pObjbuf) & 4 ) 
                Vec_IntPush( vMerge, Gia_ObjId(pGia, pObjbuf) );
        return;
    } else {
        vIntBuff = Vec_IntAlloc(1);
        Gia_ManForEachObj( pGia, pObjbuf, i )
            if ( ~Gia_ObjValue(pObjbuf) & 4 ) 
                Vec_IntPush( vIntBuff, Gia_ObjId(pGia, pObjbuf) );
        Vec_IntForEachEntry( vIntBuff, entry, i ) {
            if ( ~Gia_ObjValue(Gia_ManObj(pGia, entry)) & 8 ) continue;
            vIntBuff2 = Vec_IntAlloc(1);
            Vec_IntPush( vIntBuff2, entry );
            Cec_ManGetTFO( pGia, vIntBuff2, -1, 8 );
            Vec_IntFree( vIntBuff2 );
        }
        Vec_IntFree( vIntBuff );
        
        if (debug) {
            vIntBuff = Vec_IntAlloc(1);
            Gia_ManForEachObj( pGia, pObjbuf, i ) 
                if ( ~Gia_ObjValue(pObjbuf) & 8 ) 
                    Vec_IntPush( vIntBuff, Gia_ObjId(pGia, pObjbuf) );
            Gia_SelfDefShow( pGia, "merge_bend.dot", vIntBuff, 0, 0 );
            Vec_IntFree( vIntBuff );
        }
        Gia_ManForEachObj( pGia, pObjbuf, i ) {
            if ( (~Gia_ObjValue(pObjbuf) & 8) ) {
                pObjbuf1 = Gia_ObjFanin0( pObjbuf );
                if ( (~Gia_ObjValue(pObjbuf1) & 8) == 0 ) Vec_IntPushUnique( vMerge, Gia_ObjId(pGia,pObjbuf1) );
                pObjbuf2 = Gia_ObjFanin1( pObjbuf );
                if ( (~Gia_ObjValue(pObjbuf2) & 8) == 0 ) Vec_IntPushUnique( vMerge, Gia_ObjId(pGia,pObjbuf2) );
            }
        }
        if (debug) Gia_SelfDefShow( pGia, "merge_low.dot", vMerge, 0, 0 );
    }

}
Gia_Man_t* Cec_ManSurrounding( Gia_Man_t* pGia, Vec_Int_t* vNd, int nHeight, int fMerge ) {
    assert( vNd && Vec_IntSize(vNd) > 0 );
    Gia_Man_t* pGia_cp = Gia_ManDup( pGia );
    Gia_Man_t* pOut;
    Vec_Int_t* vFI;
    Vec_Int_t* vFO;
    Vec_Int_t* vIntBuff;
    Vec_Ptr_t* vObjBuff;
    int i, nidbuf;
    Gia_Obj_t* pSub;


    vFO = Cec_ManGetTFO( pGia_cp, vNd, nHeight, 0 );
    // Vec_IntFree( vIntBuff );
    switch (fMerge)
    {
        case 0: // Down to PI
            // Vec_IntFree( vFI );
            vFI = Vec_IntDup( pGia_cp->vCis );
            break;
        case 1: case 2: // Down to merge, 1 is low 2 is high
            vFI = Vec_IntAlloc(100);
            vIntBuff = Cec_ManGetSibling( pGia_cp, vNd, nHeight, 0 );
            Cec_ManGetMerge( pGia_cp, vNd, vIntBuff, vFI, fMerge == 1 ? 1 : 0, 0 );
            Vec_IntFree( vIntBuff );
            break;
        default:
            break;
    }
    // keep the copy node Id in returned pOut
    pOut = Cec_ManGetSubcircuit( pGia_cp, vFI, vFO, 1 );
    Gia_ManStop( pGia_cp );
    return pOut;

}
Vec_Flt_t*           Cec_ManRecPathLen( Gia_Man_t* pGia_in, Vec_Int_t* vF, Vec_Int_t* vG, int fMax, int fType, int fDownward ) {
    // to modify: buggy, also consider not one PO circuit
    int debug = 0;
    int i, j, nid, nid1, nid2, lim;
    float distF, distG, distLocal, dist, distOpt;
    Gia_Man_t* pGia = Gia_ManDup( pGia_in );
    int** vDist;
    Vec_Flt_t *vDistGlobal;

    Cec_ManLoadMark( pGia, vF, 0, 0 );
    vDist = Cec_ManDist( pGia, fMax );

    if (debug) {
        Vec_IntPrint( vF );
        Vec_IntPrint( vG );
    }


    vDistGlobal = Vec_IntAlloc( Vec_IntSize(vG) );
    Vec_IntFill( vDistGlobal, Vec_IntSize(vG), -1 );
    if (debug) {
        Gia_SelfDefShow( pGia, "test_TwoPath.dot", vF, vG, 0 );
    }
    Vec_IntForEachEntry( vG, nid2, j ) {
        distOpt = -1;
        Vec_IntForEachEntry( vF, nid1, i ) {
            // if (debug) printf("nid1 = %d, nid2 = %d, dist = %d\n", nid1, nid2, vDist[nid1][nid2]);
            if (vDist[nid1][nid2] == -1) continue;
            nid = fDownward ? Abc_MinInt(nid1, nid2) - 1 : Gia_ManObjNum(pGia) - 1;
            lim = fDownward ? Abc_MaxInt(nid1, nid2) : -1;
            for (; nid > lim; nid--) {
                if (fDownward == 0 && vDist[nid][nid1] <= 0 && vDist[nid][nid2] <= 0) continue;
                if (fDownward == 1 && vDist[nid1][nid] <= 0 && vDist[nid2][nid] <= 0) continue;
                
                distF = fDownward ? 1.0 * vDist[nid1][nid] : 1.0 * vDist[nid][nid1];
                distG = fDownward ? 1.0 * vDist[nid2][nid] : 1.0 * vDist[nid][nid2];
                distLocal = 1.0 * vDist[nid1][nid2];
                if (debug) {
                    printf("nid = %d, nid1 = %d, nid2 = %d\n", nid, nid1, nid2);
                    printf("distF = %f, distG = %f, distLocal = %f\n", distF, distG, distLocal);
                }
                switch (fType)
                {
                    case 0: // p1-norm
                        dist = 1.0 * (distF + distLocal + distG);
                        break;
                    case 1: // p2-norm
                        dist = 1.0 * (distF + distLocal) * (distF + distLocal) 
                                + 1.0 * distG * distG;
                        break;
                    default:
                        break;
                }
                if (distOpt > dist || distOpt == -1) distOpt = dist;
                          
            }
        }
        Vec_FltWriteEntry( vDistGlobal, j, distOpt );
    }

    for (i = 0; i < Gia_ManObjNum(pGia); i++) 
        ABC_FREE( vDist[i] );
    ABC_FREE( vDist );
    Gia_ManStop( pGia );

    return vDistGlobal;
}

Gia_Man_t* Cec_ManPatchCleanSupport( Gia_Man_t* pGia, Vec_Int_t* vSupport ) {
    Gia_Obj_t* pObj;
    Gia_Man_t* patchClean;
    int i, nid;
    Gia_ManFillValue( pGia );
    Gia_ManForEachAnd( pGia, pObj, i ) {
      Gia_ObjSetValue( Gia_ObjFanin0(pObj), 0 );
      Gia_ObjSetValue( Gia_ObjFanin1(pObj), 0 );
    }
    Gia_ManForEachCo( pGia, pObj, i ){
        Gia_ObjSetValue( Gia_ObjFanin0(pObj), 0 );
    }
    Gia_ManForEachCi( pGia, pObj, i ) {
      if (Gia_ObjValue(pObj) == -1) Vec_IntWriteEntry( vSupport, i, -1 );
    }
    Vec_IntRemoveAll( vSupport, -1, 0 );
    patchClean = Cec_ManGetTFI( pGia, 0, 1 );
    return patchClean;
}
// fType = 0: XOR miter, fType = 1: two output
Gia_Man_t*  Cec_ManToMiter( Gia_Man_t* pGia, int fType ) {
    Gia_Man_t *pMiter = Gia_ManStart( Gia_ManObjNum(pGia) + 4 );
    Gia_Obj_t *pObjbuf, *pObj1, *pObj2;
    Vec_Ptr_t* coneNdbuff;
    int k, nid, val1, val2, val, fPhase;

    // Gia_SelfDefShow( pGia, "org.dot", 0, 0, 0 );

    if (Gia_ManCoNum(pGia) == 4) {
        pObj1 = Gia_ObjFanin0( Gia_ManCo( pGia, 0 ) );
        pObj2 = Gia_ObjFanin0( Gia_ManCo( pGia, 2 ) );
        fPhase = Gia_ObjFaninC0( Gia_ManCo( pGia, 0 ) ) ^ Gia_ObjFaninC0( Gia_ManCo( pGia, 2 ) );
        assert(pObj1 != pObj2);
    } else if (Gia_ManCoNum(pGia) == 2) {
        pObj1 = Gia_ObjFanin0( Gia_ManCo( pGia, 0 ) );
        pObj2 = Gia_ObjFanin0( Gia_ManCo( pGia, 1 ) );
        fPhase = Gia_ObjFaninC0( Gia_ManCo( pGia, 0 ) ) ^ Gia_ObjFaninC0( Gia_ManCo( pGia, 1 ) );
    } else if (Gia_ManCoNum(pGia) == 1) {
        pObj1 = Gia_ObjFanin0( Gia_ManCo( pGia, 0 ) );
        pObj2 = Gia_ObjFanin0( Gia_ManCo( pGia, 0 ) );
        // two must be same node
    } else {
        assert(0);
    }

    if (pObj1 == pObj2) { // assume it is a XOR or XNOR
        assert(Gia_ObjRecognizeExor( pObj1, 0, 0 ));
        // printf("%d, %d\n", Gia_ObjFaninC0( Gia_ManCo( pMan->pGia, 0 )) , Gia_ObjRecognizeExor( pMan->pObj1, 0, 0 ));
        // does PO inv and does PO connect to XNOR
        fPhase = Gia_ObjFaninC0( Gia_ManCo( pGia, 0 )) ^ (Gia_ObjFaninC0( Gia_ObjFanin0( pObj1 ) ) != Gia_ObjFaninC1( Gia_ObjFanin0( pObj1 ) ));
        pObj1 = Gia_ObjFanin0( Gia_ObjFanin0( pObj1 ) );
        pObj2 = Gia_ObjFanin1( Gia_ObjFanin0( pObj2 ) );
    }

    Gia_ManFillValue( pGia );
    Gia_ManConst0(pGia)->Value = 0;
    Gia_ManForEachCi( pGia, pObjbuf, k ) {
        Gia_ObjSetValue(pObjbuf, Gia_ManAppendCi( pMiter ));
    }
    coneNdbuff = Cec_ManSubcircuit( pGia, pObj1, 0, 0, 0, 1 );
    val1 = Cec_ManPatch( pMiter, coneNdbuff );
    Vec_PtrFree( coneNdbuff );
    coneNdbuff = Cec_ManSubcircuit( pGia, pObj2, 0, 0, 0, 1 );
    val2 = Cec_ManPatch( pMiter, coneNdbuff );
    Vec_PtrFree( coneNdbuff );

    if (fType == 0) {
        // val1 = Abc_LitNotCond( val1, fPhase );
        val = Gia_ManAppendXor( pMiter, Abc_LitNotCond( val1, fPhase ), val2 );
        Gia_ManAppendCo( pMiter, val );
    } else if (fType == 1) {
        Gia_ManAppendCo( pMiter, Abc_LitNotCond( val1, fPhase ) );
        Gia_ManAppendCo( pMiter, val2 );
    } else {
        assert(0);
    }

    // Gia_SelfDefShow( pMiter, "miter.dot", 0, 0, 0 );
    return pMiter;
}

void  Cec_ManMatchFixEquiv( Gia_Man_t* pGia, Gia_Man_t* pRef, int id, int idRef ) {
    // TODO
}

// mapping old nodes to new nodes
void  Cec_ManMatch( Gia_Man_t* pGia, Gia_Man_t* pRef ) {
    // Value just be the id, lit is not required
    Gia_Obj_t* pObj, *pObj0, *pObj1, *pObjRef0, *pObjRef1, *pObjRef;
    Vec_Int_t* vIntBuff, *vRemain;
    Vec_Wec_t* conf;
    int i, j, k, buf, flag, litRef0, litRef1, idRef, id, debug;
    assert(Gia_ManCoNum(pGia) == Gia_ManCoNum(pRef));
    assert(Gia_ManCiNum(pGia) == Gia_ManCiNum(pRef));
    Gia_ManFillValue( pGia );
    if (pRef->vFanout == NULL) Gia_ManStaticFanoutStart( pRef );

    debug = 0;
    // match from PI, assume strashed
    Gia_ManForEachCi( pGia, pObj, i ) {
        Gia_ObjSetValue(pObj, Abc_Var2Lit(Gia_ObjId(pRef, Gia_ManCi(pRef, i)), 0));
        // printf("map %d to %d\n", Gia_ObjId(pGia, pObj), Gia_ObjId(pRef, Gia_ManCi(pRef, i)));
    }
    Gia_ManForEachAnd( pGia, pObj, i ) {
        if (Gia_ObjFanin0(pObj)->Value == -1) continue;
        if (Gia_ObjFanin1(pObj)->Value == -1) continue;
        litRef0 = Gia_ObjFanin0Copy(pObj);
        litRef1 = Gia_ObjFanin1Copy(pObj);
        Gia_ObjForEachFanoutStaticId( pRef, Abc_Lit2Var(litRef0), idRef, j ) {
            if ((Gia_ObjFaninLit0(Gia_ManObj(pRef, idRef), idRef) == litRef0 
                && Gia_ObjFaninLit1(Gia_ManObj(pRef, idRef), idRef) == litRef1) ||
                (Gia_ObjFaninLit0(Gia_ManObj(pRef, idRef), idRef) == litRef1 
                && Gia_ObjFaninLit1(Gia_ManObj(pRef, idRef), idRef) == litRef0)) {
                    Gia_ObjSetValue(pObj, Abc_Var2Lit(idRef,0));
                    // printf("map %d to %d\n", Gia_ObjId(pGia, pObj), idRef);
                    break;
            }
        }
    }

    vRemain = Vec_IntAlloc(100);
    Gia_ManForEachAndReverse( pGia, pObj, i ) {
        if (Gia_ObjValue(pObj) < 0) 
            Vec_IntPush( vRemain, Gia_ObjId( pGia, pObj ) );
    }

    // match from PO, might conflict
    // map all the node, keep conflict nodes as -1
    // then pick best pattern for conflict ones
    conf = Vec_WecAlloc( 100 );
    Gia_ManForEachCo( pGia, pObj, i ) { // map CO
        pObjRef = Gia_ManCo(pRef, i);
        Gia_ObjSetValue(pObj, Abc_Var2Lit(Gia_ObjId(pRef, pObjRef), 0));
        // printf("map %d to %d\n", Gia_ObjId(pGia, pObj), Gia_ObjId(pRef, pObjRef));
        if (Gia_ObjFaninC0(pObj) == Gia_ObjFaninC0(pObjRef)) {
            Gia_ObjSetValue( Gia_ObjFanin0(pObj), Abc_Var2Lit(Gia_ObjId(pRef, Gia_ObjFanin0(pObjRef)), 0));
            // printf("map %d to %d\n", Gia_ObjId(pGia, Gia_ObjFanin0(pObj)), Gia_ObjId(pRef, Gia_ObjFanin0(pObjRef)));
        }
    }

    Vec_IntForEachEntry( vRemain, id, i ) {
        pObj = Gia_ManObj(pGia, id);
        if (Gia_ObjValue(pObj) < 0) continue;
        pObj0 = Gia_ObjFanin0(pObj);
        pObj1 = Gia_ObjFanin1(pObj);
        pObjRef = Gia_ObjCopy( pRef, pObj );
        pObjRef0 = NULL; // NULL is just as a flag
        if (Gia_ObjFaninC0(pObj) != Gia_ObjFaninC1(pObj)) {
            // match by complement
            if (Gia_ObjFaninC0(pObj) == Gia_ObjFaninC0(pObjRef) && Gia_ObjFaninC1(pObj) == Gia_ObjFaninC1(pObjRef)) {
                pObjRef0 = Gia_ObjFanin0(pObjRef);
                pObjRef1 = Gia_ObjFanin1(pObjRef);
            } else if (Gia_ObjFaninC0(pObj) == Gia_ObjFaninC1(pObjRef) && Gia_ObjFaninC1(pObj) == Gia_ObjFaninC0(pObjRef)) {
                pObjRef0 = Gia_ObjFanin1(pObjRef);
                pObjRef1 = Gia_ObjFanin0(pObjRef);
            } else {
                // not matchable
                continue;
            }
        } else {
            if (Gia_ObjFaninC0(pObj) != Gia_ObjFaninC0(pObjRef)) continue;
            else if (Gia_ObjFaninC0(pObjRef) != Gia_ObjFaninC1(pObjRef)) continue;
            else { // all fanin c are the same, check whether FI copy is confirmed
                if (Gia_ObjValue(pObj0) >= 0) {
                    if (Gia_ObjCopy(pRef, pObj0) == Gia_ObjFanin0(pObjRef)) {
                        pObjRef0 = Gia_ObjFanin0(pObjRef);
                        pObjRef1 = Gia_ObjFanin1(pObjRef);
                    } else if (Gia_ObjCopy(pRef, pObj0) == Gia_ObjFanin1(pObjRef)) {
                        pObjRef0 = Gia_ObjFanin1(pObjRef);
                        pObjRef1 = Gia_ObjFanin0(pObjRef);
                    }
                }
                if (Gia_ObjValue(pObj1) >= 0 && pObjRef0 == NULL) {
                    if (Gia_ObjCopy(pRef, pObj1) == Gia_ObjFanin0(pObjRef)) {
                        pObjRef0 = Gia_ObjFanin1(pObjRef);
                        pObjRef1 = Gia_ObjFanin0(pObjRef);
                    } else if (Gia_ObjCopy(pRef, pObj1) == Gia_ObjFanin1(pObjRef)) {
                        pObjRef0 = Gia_ObjFanin0(pObjRef);
                        pObjRef1 = Gia_ObjFanin1(pObjRef);
                    }
                } 
                if (pObjRef0 == NULL) {
                    // No sufficient info to match, then just guess
                    pObjRef0 = Gia_ObjFanin0(pObjRef);
                    pObjRef1 = Gia_ObjFanin1(pObjRef);
                }
            }
        }
        assert(pObjRef0 != NULL);
        // printf("pObj: %d, pObjRef0: %d, pObjRef1: %d\n", Gia_ObjId(pGia, pObj), Gia_ObjId(pRef, pObjRef0), Gia_ObjId(pRef, pObjRef1));


        if (Gia_ObjValue(pObj0) >= 0) {
            if (Gia_ObjCopy(pRef, pObj0) != pObjRef0) { // conflict in matching
                // printf("pObj: %d, pObj0(Ref): %d, pObjRef: %d, pObjRef0: %d\n", Gia_ObjId(pGia, pObj), Gia_ObjId( pRef, Gia_ObjCopy(pRef, pObj0) ), Gia_ObjId(pRef, pObjRef), Gia_ObjId(pRef, pObjRef0));
                vIntBuff = Vec_WecPushLevel( conf );
                Vec_IntPush( vIntBuff, Gia_ObjId( pGia, pObj0 ) );
                Vec_IntPush( vIntBuff, Gia_ObjId( pRef, Gia_ObjCopy(pRef, pObj0) ) );
                Vec_IntPush( vIntBuff, Gia_ObjId( pRef, pObjRef0 ) );
                Gia_ObjSetValue( pObj0, -2 );
            }
        } else if (Gia_ObjValue(pObj0) == -2) { // already conflict, store the info
            Vec_WecForEachLevel( conf, vIntBuff, j ) {
                if (Vec_IntEntry(vIntBuff, 0) == Gia_ObjId(pGia, pObj)) {
                    buf = Vec_IntCountEntry( vIntBuff, Gia_ObjId( pRef, pObjRef0 ) );
                    if (buf == 0 || (buf == 1 && Vec_IntEntry(vIntBuff,0) == Gia_ObjId( pRef, pObjRef0 ))) 
                        Vec_IntPush( vIntBuff, Gia_ObjId( pRef, pObjRef0 ) );
                    break;
                }
            }
        } else { // no conflict
            assert( Gia_ObjValue(pObj0) == -1 );
            Gia_ObjSetValue( pObj0, Abc_Var2Lit(Gia_ObjId( pRef, pObjRef0 ), 0) );
            // printf("map %d to %d\n", Gia_ObjId(pGia, pObj0), Gia_ObjId(pRef, pObjRef0));
        }
        

        // printf("-1->%d\n", Gia_ObjValue(pObj1));
        if (Gia_ObjValue(pObj1) >= 0) {
            if (Gia_ObjCopy(pRef, pObj1) != pObjRef1) {
                vIntBuff = Vec_WecPushLevel( conf );
                Vec_IntPush( vIntBuff, Gia_ObjId( pGia, pObj1 ) );
                Vec_IntPush( vIntBuff, Gia_ObjId( pRef, Gia_ObjCopy(pRef, pObj1) ) );
                Vec_IntPush( vIntBuff, Gia_ObjId( pRef, pObjRef1 ) );
                Gia_ObjSetValue( pObj1, -2 );
            }
        } else if (Gia_ObjValue(pObj1) == -2) {
            Vec_WecForEachLevel( conf, vIntBuff, j ) {
                if (Vec_IntEntry(vIntBuff, 0) == Gia_ObjId(pGia, pObj)) {
                    buf = Vec_IntCountEntry( vIntBuff, Gia_ObjId( pRef, pObjRef1 ) );
                    if (buf == 0 || (buf == 1 && Vec_IntEntry(vIntBuff,0) == Gia_ObjId( pRef, pObjRef1 ))) 
                        Vec_IntPush( vIntBuff, Gia_ObjId( pRef, pObjRef1 ) );
                    break;
                }
            }
        } else {
            assert( Gia_ObjValue(pObj1) == -1 );
            Gia_ObjSetValue( pObj1, Abc_Var2Lit(Gia_ObjId( pRef, pObjRef1 ),0) );
            // printf("map %d to %d\n", Gia_ObjId(pGia, pObj1), Gia_ObjId(pRef, pObjRef1));
        }
    }

    if (debug) {
        printf("equiv nodes:\n");
        Vec_WecPrint( conf, 0 );
    }
    
    Vec_WecForEachLevel( conf, vIntBuff, i ) {
        assert( Vec_IntSize(vIntBuff) > 2 );
        id = Vec_IntEntry( vIntBuff, 0 );
        if (Vec_IntFind( vRemain, id ) == -1) { // i.e. can be computed by PI
            Gia_ObjSetValue( Gia_ManObj(pGia, id), Vec_IntEntry( vIntBuff, 1 ) );
            // printf("map %d to %d\n", id, Vec_IntEntry( vIntBuff, 1 ));
        } else {
            // TODO: Cec_ManMatchFixEquiv
            continue;
        }
    }

    Gia_ManForEachObj( pGia, pObj, j ) 
        if (Gia_ObjValue(pObj) < 0) pObj->Value = -1;

    if (debug) {
        vIntBuff = Vec_IntAlloc(100);
        Gia_ManForEachAnd( pGia, pObj, i ) {
            if (Gia_ObjValue(pObj) < 0) continue;
            else Vec_IntPush( vIntBuff, Gia_ObjId( pGia, pObj ) );
        }
        Gia_SelfDefShow( pGia, "match_PO.dot", vIntBuff, 0, 0 );
    }
    
    Vec_IntFree( vIntBuff );
    Vec_IntFree( vRemain );
    Vec_WecFree( conf );

}
/* |vFrt| <= limNum && one reduction >= limMffc
 currently limNum is not working, and greedyly */
void  Cec_ManReduceFrontier( Gia_Man_t* pGia, Vec_Int_t* vFrt, int limMffc, int limNum ) {
    if (limMffc == -1 && limNum == -1) return;
    int maxMffc = Cec_ManMFFCMul( pGia, vFrt, 0 );
    int limRed = Abc_MaxInt( 1, Abc_MinInt( limMffc, (int) (maxMffc / Vec_IntSize(vFrt)) ));
    int i, buf, buf2, mffc, flag;
    Vec_Int_t* vCost = Vec_IntStart( Vec_IntSize(vFrt) );
    Vec_Int_t* vIntBuff;
    // one pop test
    for (i = 0; i < Vec_IntSize(vFrt); i++) {
        buf = Vec_IntEntry( vFrt, 0 );
        Vec_IntDrop( vFrt, 0 );
        mffc = Cec_ManMFFCMul( pGia, vFrt, 0 );
        Vec_IntWriteEntry( vCost, i, maxMffc - mffc );
        Vec_IntPush( vFrt, buf );
    }
    Vec_IntSelectSortCost2( vFrt->pArray, Vec_IntSize(vFrt), vCost->pArray );
    printf("maxMffc = %d, max reduce = %d\n", maxMffc, Vec_IntEntry( vCost, Vec_IntSize(vFrt) - 1 ));
    Vec_IntFree( vCost );
    // mul pop greedy test
    buf2 = Vec_IntSize(vFrt);
    for (i = 0; i < buf2; i++) {
        buf = Vec_IntEntry( vFrt, 0 );
        Vec_IntDrop( vFrt, 0 );
        mffc = Cec_ManMFFCMul( pGia, vFrt, 0 );
        if (maxMffc - mffc < limRed) {
            printf("dump %d, new mffc = %d\n", buf, mffc);
            maxMffc = mffc;
        } else {
            Vec_IntPush( vFrt, buf );
        }
    }
    
    // flag = 1;
    // while (flag) {
    //     if (limNum >= 0) {
    //         if (Vec_IntEntry( vCost, Vec_IntSize(vFrt) - 1 - limNum )) limRed = Vec_IntEntry( vCost, Vec_IntSize(vFrt) - 1 - limNum );

    //     } 
    //     limRed = Vec_IntEntry( vCost, Vec_IntSize(vFrt) - 1 - limNum );
    //     limRed = Abc_MaxInt( limRed, limMffc );
    // }
    
    



}

Gia_Man_t*  Cec_ManReplacePatch( Gia_Man_t* pGia, Gia_Man_t* pPatch, Vec_Int_t* vFO_in, Vec_Int_t* vFI_in, int fStage, int fVal ) {
    assert( vFI_in ? Vec_IntSize(vFI_in) == Gia_ManCiNum(pPatch) : Gia_ManCiNum(pPatch) == Gia_ManCiNum(pGia) );
    assert( vFO_in ? Vec_IntSize(vFO_in) == Gia_ManCoNum(pPatch) : Gia_ManCoNum(pGia) == Gia_ManCoNum(pPatch) );
    assert( fVal == 0 || fVal == 1 );
    
    Gia_Man_t* pOut, *pTemp;
    Gia_Man_t* pGia_forCp = Gia_ManDup( pGia );
    Gia_Obj_t* pObjbuf;
    Vec_Int_t* vFOmap = Vec_IntAlloc( 100 );
    Vec_Int_t* vFImap = Vec_IntAlloc( 100 );
    Vec_Int_t* vFI = vFI_in ? Vec_IntDup( vFI_in ) : Vec_IntDup( pGia->vCis );
    // Vec_Int_t* vFIPatch;
    Vec_Int_t* vFO = vFO_in ? Vec_IntDup( vFO_in ) : Vec_IntDup( pGia->vCos );
    Vec_Ptr_t* vNdCone;
    int i, nid, buf;
    int debug = 0;
    if (debug)
        Vec_IntForEachEntry( vFI, nid, i ) assert( Vec_IntFind( vFO, nid ) == -1 );
    
    Gia_ManFillValue( pPatch );
    Gia_ManForEachCo( pPatch, pObjbuf, i )
        Cec_ManSubcircuit( pPatch, pObjbuf, 0, 0, 1, 0);
    Vec_IntForEachEntry( vFI, nid, i ) 
        if (Gia_ManCi(pPatch, i)->Value == -1) Vec_IntWriteEntry( vFI, i, -1 );
    
    if (debug) {
        Vec_IntPrint( vFI );
        Vec_IntPrint( vFO );
    }

    pOut = Gia_ManStart( Gia_ManObjNum(pGia) + Gia_ManObjNum(pPatch) );
    Gia_ManFillValue( pGia_forCp );
    Gia_ManConst0(pGia_forCp)->Value = 0;
    Gia_ManForEachCi( pGia_forCp, pObjbuf, i ) {
        Gia_ObjSetValue(pObjbuf, Gia_ManAppendCi( pOut ));
    }
    Gia_ManFillValue( pPatch );
    Gia_ManConst0( pPatch )->Value = 0;

    Gia_ManForEachCi( pPatch, pObjbuf, i ) {
        nid = Vec_IntEntry( vFI, i );
        if (nid < 0) continue;
        assert( nid >= 0 );
        vNdCone = Cec_ManSubcircuit( pGia_forCp, Gia_ManObj(pGia_forCp, nid), 0, 0, 0, 1 );
        if (Vec_PtrSize(vNdCone) == 0) buf = Gia_ManObj(pGia_forCp, nid)->Value;
        else buf = Cec_ManPatch( pOut, vNdCone );
        Gia_ObjSetValue(pObjbuf, buf);
        if (fStage == 1) Gia_ManAppendCo( pOut, buf );
        // Vec_IntPush( vFImap, Abc_Lit2Var(buf) );
        Vec_PtrFree( vNdCone );
    }
    if (debug) printf("Aig size: %d\n", Gia_ManObjNum(pOut));
    if (fStage == 1) {
        Cec_ManMapCopy( pOut, pGia_forCp, 0 );
        Gia_ManStop( pGia_forCp );
        return pOut;
    }

    Gia_ManForEachCo( pPatch, pObjbuf, i ) {
        nid = Vec_IntEntry( vFO, i );
        assert( nid >= 0 );
        vNdCone = Cec_ManSubcircuit( pPatch, Gia_ManCo(pPatch, i), 0, 0, 0, 1 );
        if (Vec_PtrSize(vNdCone) == 0) buf = Gia_ManCo(pPatch, i)->Value;
        else buf = Cec_ManPatch( pOut, vNdCone );
        Gia_ObjSetValue(Gia_ManObj(pGia_forCp, nid), buf);
        if (fStage == 2) Gia_ManAppendCo( pOut, buf );
        // Vec_IntPush( vFOmap, Abc_Lit2Var(buf) );
        Vec_PtrFree( vNdCone );
    }
    if (debug) printf("Aig size: %d\n", Gia_ManObjNum(pOut));
    if (fStage == 2) {
        Cec_ManMapCopy( pOut, pGia_forCp, 0 );
        Gia_ManStop( pGia_forCp );
        return pOut;
    }
    Gia_ManForEachCo( pGia_forCp, pObjbuf, i ) {
        vNdCone = Cec_ManSubcircuit( pGia_forCp, Gia_ManCo(pGia_forCp, i), 0, 0, 0, 1 );
        if (Vec_PtrSize(vNdCone) == 0) buf = Gia_ManCo(pGia_forCp, i)->Value;
        else buf = Cec_ManPatch( pOut, vNdCone );
        Gia_ObjSetValue(pObjbuf, buf);
        Gia_ManAppendCo( pOut, buf );
        Vec_PtrFree( vNdCone );
    }
    if (debug) printf("Aig size: %d\n", Gia_ManObjNum(pOut));
    if (fStage == 3) {
        Cec_ManMapCopy( pOut, pGia_forCp, 0 );
        Gia_ManStop( pGia_forCp );
        return pOut;
    }

    pTemp = pOut;
    Gia_ManFillValue( pTemp );
    pOut = Gia_ManRehash( pTemp, 0 );
    Gia_ManFillValue( pOut );
    // pOut = Gia_ManDup( pTemp );
    
    if (debug) {
        // Gia_ManForEachObj( pGia_forCp, pObjbuf, i ) {
        //     printf("pGia_forCp[%d] = %d\n", i, Gia_ObjValue(pObjbuf));
        // }
        // Gia_ManForEachObj( pTemp, pObjbuf, i ) {
        //     printf("pTemp[%d] = %d\n", i, Gia_ObjValue(pObjbuf));
        // }
        // Gia_ManForEachObj( pPatch, pObjbuf, i ) {
        //     printf("pPatch[%d] = %d\n", i, Gia_ObjValue(pObjbuf));
        // }
        Gia_SelfDefShow( pGia_forCp, "replace_patch_org.dot", 0, 0, 0 );
        printf("pGia size %d\n", Gia_ManObjNum(pGia_forCp));
        Cec_ManPrintMapCopy( pGia_forCp );
        Gia_SelfDefShow( pPatch, "replace_patch_patch.dot", 0, 0, 0 );
        printf("pPatch size %d\n", Gia_ManObjNum(pPatch));
        Cec_ManPrintMapCopy( pPatch );
        Gia_SelfDefShow( pTemp, "replace_patch_before.dot", 0, 0, 0 );
        printf("pTemp size %d\n", Gia_ManObjNum(pTemp));
        Cec_ManPrintMapCopy( pTemp );
        Gia_SelfDefShow( pOut, "replace_patch_after.dot", 0, 0, 0 );
        printf("pOut size %d\n", Gia_ManObjNum(pOut));
        Cec_ManPrintMapCopy( pOut );
    }

    // Copy(pTemp, lit(pPatch)), Copy(pTemp, lit(pGia_forCp)), Copy(pOut, lit(pTemp))
    buf = 0;
    Gia_ManForEachObj( pPatch, pObjbuf, i ) {
        if (pObjbuf->Value != -1) buf++;
    }
    if (debug) printf("#Aig copied from patch = %d\n", buf);
    buf = 0;
    Gia_ManForEachObj( pGia_forCp, pObjbuf, i ) {
        if (pObjbuf->Value != -1) buf++;
    }
    if (debug) printf("#Aig copied from Gia_forCp = %d\n", buf);
    buf = 0;
    Gia_ManForEachObj( pTemp, pObjbuf, i ) {
        if (pObjbuf->Value != -1) buf++;
    }
    if (debug) printf("#Aig copied from temp = %d\n", buf);
    buf = 0;
    Gia_ManForEachObj( pOut, pObjbuf, i ) {
        if (pObjbuf->Value != -1) buf++;
    }
    if (debug) printf("#Aig copied from out = %d\n", buf);
    Cec_ManMapCopy( pPatch, pTemp, 1 ); // Obj = Copy(pOut, lit(pPatch))
    Cec_ManMapCopy( pOut, pTemp, 0 ); // Obj = Copy(pTemp, lit(pOut))
    // printf("pOut PO %d is mapped to pTemp %d\n", Gia_ObjId(pOut, Gia_ManPo(pOut, 0)), Gia_ManPo(pOut, 0)->Value);
    Gia_ManFillValue( pTemp );
    Cec_ManMapCopy( pTemp, pGia_forCp, 0 ); // Obj = Copy(pGia_forCp, lit(pTemp))
    // printf("pTemp PO %d is mapped to pGia_forCp %d\n", Gia_ObjId(pTemp, Gia_ManPo(pTemp, 0)), Gia_ManPo(pTemp, 0)->Value);
    Cec_ManMapCopy( pOut, pTemp, 1 ); // Obj = Copy(pGia_forCp, lit(pOut))
    // printf("pOut PO %d is mapped to pGia_forCp %d\n", Gia_ObjId(pOut, Gia_ManPo(pOut, 0)), Gia_ManPo(pOut, 0)->Value);
    // printf("POs: %d %d %d\n", Gia_ObjId(pOut, Gia_ManPo(pOut, 0)), Gia_ObjId(pTemp, Gia_ManPo(pTemp, 0)), Gia_ObjId(pGia_forCp, Gia_ManPo(pGia_forCp, 0)));
    if (debug) {
        buf = 0;
        Gia_ManForEachObj( pOut, pObjbuf, i ) {
            if (pObjbuf->Value != -1) buf++;
        }
        printf("#Aig copied = %d\n", buf);
    }
    
    Gia_ManStop( pTemp );
    Gia_ManStop( pGia_forCp );
    return pOut;
}
Gia_Man_t*  Cec_ManItp( Gia_Man_t* pGia, int f, Vec_Int_t* vG, int nBTLimit, int coef_check, Vec_Int_t* stat ) {
    Gia_Man_t* pGia_forCp = Gia_ManDup( pGia );
    Gia_Man_t *pFdMiter, *pFdPatch, *pTemp;
    Gia_Obj_t* pObjbuf;
    Vec_Ptr_t* vNdCone;
    int i, j, nid, buf;
    // for coef_check
    int costOrg, costNew;
    Vec_Int_t* vMerge = Vec_IntAlloc(100);
    Vec_Int_t* vF = Vec_IntAlloc(1);
    Vec_IntPush( vF, f );
    Vec_Int_t* vFNew = Vec_IntAlloc(1);
    Vec_Int_t* vGNew = Vec_IntAlloc( Vec_IntSize(vG) );
    Vec_Int_t* vIntBuff;
    Gia_Man_t* pNew;
    pFdMiter = Gia_ManStart( Gia_ManObjNum(pGia_forCp) );
    Gia_ManFillValue(pGia_forCp);
    Gia_ManConst0(pGia_forCp)->Value = 0;
    Gia_ManForEachCi( pGia_forCp, pObjbuf, j ) {
        Gia_ObjSetValue(pObjbuf, Gia_ManAppendCi( pFdMiter ));
    }
    // printf("label1\n");
    Vec_IntForEachEntry( vG, nid, j ) {
        if (nid == -1) continue;
        vNdCone = Cec_ManSubcircuit( pGia_forCp, Gia_ManObj(pGia_forCp, nid), 0, 0, 0, 1 );
        if (Vec_PtrSize(vNdCone) == 0) buf = Gia_ManObj(pGia_forCp, nid)->Value;
        else buf = Cec_ManPatch( pFdMiter, vNdCone );
        Gia_ManAppendCo( pFdMiter, buf );
        Vec_PtrFree( vNdCone );
    }
    vNdCone = Cec_ManSubcircuit( pGia_forCp, Gia_ManObj(pGia_forCp, f), 0, 0, 0, 1 );
    if (Vec_PtrSize(vNdCone) == 0) buf = Gia_ObjValue(Gia_ManObj(pGia_forCp, f)); // assert(0);
    else buf = Cec_ManPatch( pFdMiter, vNdCone );
    Gia_ManAppendCo( pFdMiter, buf );
    Vec_PtrFree( vNdCone );
    pFdPatch = Int2_ManFdSimp( pFdMiter, nBTLimit, &buf );
    // printf("c\n");
    if (stat) {
        Vec_IntPush( stat, buf );
    }
    Gia_ManStop( pFdMiter );
    // printf("d\n");
    if ( pFdPatch == NULL || pFdPatch == 1 ) return pFdPatch;
     // get mapping

    pNew = Cec_ManReplacePatch( pGia_forCp, pFdPatch, vF, vG, 0, 1 );
    // Lit might be less than 0

    switch (coef_check) {
        case 0: case 1: case 2: // level should not be extended // 1: fMax, 2: fMin
            
            pFdPatch = Cec_ManSimpSyn( pTemp = pFdPatch, 0, 0, -1 );
            Gia_ManStop( pTemp );
            vIntBuff = Cec_ManGetMFFCost( pGia_forCp, coef_check );
            costOrg = Vec_IntFindMax( vIntBuff );
            Vec_IntFree( vIntBuff );
            vIntBuff = Cec_ManGetMFFCost( pNew, coef_check );
            costNew = Vec_IntFindMax( vIntBuff );
            Vec_IntFree( vIntBuff );
            if (costNew > costOrg) pFdPatch = 2;
            break;
        default:
            break;
    }
    // if (pFdPatch != 2) Cec_ManLoadValue( pFdPatch, vIntBuff );
    Gia_ManStop( pNew );
    Vec_IntFree( vMerge );
    Vec_IntFree( vF );
    Vec_IntFree( vFNew );
    Vec_IntFree( vGNew );
    return pFdPatch;
}
int         Cec_ManCompPatch( Gia_Man_t* pOpt, Gia_Man_t* pNew, int coef_cost ) {
    // return 1 if pNew is better
    Gia_Obj_t* pObjbuf;
    int i, nid, retVal;
    int debug = 0;
    float costOpt, costNew;
    Vec_Int_t* vFOpt = Vec_IntAlloc( 100 );
    Vec_Int_t* vFNew = Vec_IntAlloc( 100 );
    Vec_Int_t* vGOpt = Vec_IntAlloc( 100 );
    Vec_Int_t* vGNew = Vec_IntAlloc( 100 );
    Vec_Int_t* vOptVal = Cec_ManDumpValue( pOpt );
    Vec_Int_t* vNewVal = Cec_ManDumpValue( pNew );
    Vec_Flt_t* vGCostOpt;
    Vec_Flt_t* vGCostNew;
    Vec_Flt_t* vFltBuff;
    Gia_ManForEachObj( pOpt, pObjbuf, i ) {
        if ( pObjbuf->fMark0 ) Vec_IntPush( vFOpt, Gia_ObjId(pOpt, pObjbuf) );
        if ( pObjbuf->fMark1 ) Vec_IntPush( vGOpt, Gia_ObjId(pOpt, pObjbuf) );
    }
    Gia_ManForEachObj( pNew, pObjbuf, i ) {
        if ( pObjbuf->fMark0 ) Vec_IntPush( vFNew, Gia_ObjId(pNew, pObjbuf) );
        if ( pObjbuf->fMark1 ) Vec_IntPush( vGNew, Gia_ObjId(pNew, pObjbuf) );
    }
    if (debug) {
        printf("FOpt: ");
        Vec_IntPrint( vFOpt );
        printf("GOpt: ");
        Vec_IntPrint( vGOpt );
        printf("FNew: ");
        Vec_IntPrint( vFNew );
        printf("GNew: ");
        Vec_IntPrint( vGNew );
        Gia_SelfDefShow( pOpt, "pOpt.dot", vFOpt, vGOpt, 0);
        Gia_SelfDefShow( pNew, "pNew.dot", vFNew, vGNew, 0);
    }
    assert( Vec_IntSize(vFOpt) != 0 );
    assert( Vec_IntSize(vFNew) != 0 );
    // assert( Vec_IntSize(vGOpt) != 0 );
    // assert( Vec_IntSize(vGNew) != 0 );
    if (Vec_IntSize(vGOpt) == 0) retVal = 0;
    else if (Vec_IntSize(vGNew) == 0) retVal = 1;
    else {
        switch (coef_cost)
        {
            case 1: case 2: case 3: case 4:// X1: fMax, X0: fMin, 1X: MaxCost, 0X: MinCost // p1-norm
                vGCostOpt = Cec_ManRecPathLen( pOpt, vFOpt, vGOpt, coef_cost & 1, 0, 0);
                vGCostNew = Cec_ManRecPathLen( pNew, vFNew, vGNew, coef_cost & 1, 0, 0);
                vFltBuff = Vec_FltDup( vGCostNew );
                Vec_FltRemoveAll( vGCostOpt, 0, -1 );
                Vec_FltRemoveAll( vGCostNew, 0, -1 );
                if (Vec_FltSize(vGCostNew) == 0)  {
                    retVal = 0;
                    break;
                } else if (Vec_FltSize(vGCostOpt) == 0) {
                    retVal = 1;
                    break;
                }

                costOpt = (coef_cost & 2) ? Vec_FltFindMax( vGCostOpt ) : Vec_FltFindMin( vGCostOpt );
                costNew = (coef_cost & 2) ? Vec_FltFindMax( vGCostNew ) : Vec_FltFindMin( vGCostNew );
                retVal = (costOpt > costNew);
                if (debug) printf("costOpt = %f, costNew = %f\n", costOpt, costNew);
                if (retVal && debug == 0) {
                    printf("Better patch found: cost %f -> %f\n", costOpt, costNew);
                }
                break;
            default:
                assert(0);
                break;
        }
    }
    
    // refine the value
    Cec_ManLoadMark( pOpt, vFOpt, 0, 0 );
    Cec_ManLoadMark( pNew, vFNew, 0, 0 );
    Cec_ManLoadMark( pOpt, vGOpt, 1, 0 );
    Cec_ManLoadMark( pNew, vGNew, 1, 0 );
    Vec_IntFree( vOptVal );
    Vec_IntFree( vNewVal );
    Vec_IntFree( vFOpt );
    Vec_IntFree( vFNew );
    Vec_IntFree( vGOpt );
    Vec_IntFree( vGNew );
    Vec_FltFree( vGCostOpt );
    Vec_FltFree( vGCostNew );
    Vec_FltFree( vFltBuff );
    return retVal;
}
Gia_Man_t*  Cec_ManFuncDepReplace( Gia_Man_t* pGia, Vec_Int_t* vF, Vec_Int_t* vG, Vec_Int_t* vMerge, Vec_Int_t* vCoef ) {
    assert( Vec_IntSize(vCoef) == 4);
    Gia_Man_t *pOpt, *pTemp, *pItp, *pItpStore;
    Gia_Obj_t *pObjbuf;
    Vec_Int_t *vIntBuff = Vec_IntAlloc( 1 );
    Vec_Int_t *vG_all, *vG_in;
    int nidbuf, nid, i, j, k, flag;
    int coef_cost = Vec_IntEntry( vCoef, 0 );
    int coef_check = Vec_IntEntry( vCoef, 1 );
    int fShrink = Vec_IntEntry( vCoef, 2 );
    int nBTLimit = Vec_IntEntry( vCoef, 3 );
    int debug = 0;
    char* str = ABC_ALLOC( char, 100 );

    pOpt = NULL;
    pItpStore = NULL;
    Vec_IntForEachEntry( vF, nid, i ) {
        if (debug) printf("test nid = %d\n", nid);
        vG_all = Vec_IntDup( vG );
        Gia_ManFillValue(pGia);
        if (vMerge) {
            Cec_ManSubcircuit( pGia, Gia_ManObj(pGia, nid), vMerge, 0, 1, 0 );
            Vec_IntForEachEntry( vMerge, nidbuf, j ) {
                if (Gia_ManObj(pGia, nidbuf)->Value != -1) Vec_IntPush( vG_all, nidbuf );
            }
        } else {
            Vec_IntPush( vG_all, -1 ); // just to run with no frontier
        }
        Vec_IntForEachEntryStart( vG_all, nidbuf, j, Vec_IntSize(vG) ) {
            // printf("dumped frontier node: %d\n", nidbuf);
            vG_in = Vec_IntDup( vG_all );
            Vec_IntDrop( vG_in, j );
            pItp = Cec_ManItp( pGia, nid, vG_in, nBTLimit, coef_check, 0 );
            if (pItp == NULL || pItp == 1 || pItp == 2) continue;
            Vec_IntClear( vIntBuff );
            Vec_IntPush( vIntBuff, nid );
            // printf("label 1\n");
            pTemp = Cec_ManReplacePatch( pGia, pItp, vIntBuff, vG_in, 0, 1 );
            // printf("label 2\n");

            if (debug) printf("G_node: ");
            Gia_ManForEachCi( pItp, pObjbuf, k ) {
                if (Gia_ObjValue(pObjbuf) != -1 && k < Vec_IntSize(vG))  {
                    // printf("Mark %d (%d)\n", Vec_IntEntry(vG_in, k), Abc_Lit2Var(Gia_ObjValue(pObjbuf)));
                    if (debug) printf("%d ", Vec_IntEntry(vG_in, k));
                    Gia_ObjCopy(pTemp, pObjbuf)->fMark1 = 1;
                }
            }
            if (debug) printf("\n");
            // printf("label 3\n");
            Gia_ManForEachCo( pItp, pObjbuf, k )
                if (Gia_ObjValue(pObjbuf) != -1)
                    Gia_ObjCopy(pTemp, pObjbuf)->fMark0 = 1;
            if (pOpt == 0 || Cec_ManCompPatch( pOpt, pTemp, coef_cost )) {
                if (pOpt) Gia_ManStop( pOpt );
                pOpt = pTemp;
                if (pItpStore) Gia_ManStop( pItpStore );
                pItpStore = Gia_ManDup( pItp );
            } else Gia_ManStop( pTemp );
            // printf("label 4\n");
            Gia_ManStop( pItp );
            Vec_IntFree( vG_in );
        }
        Vec_IntFree( vG_all );

        if (fShrink) { 
            // TODO
        }
    }
    printf("Final optimized patch\n");
    if (pOpt && debug) {
        Gia_ManForEachObj( pOpt, pObjbuf, i ) {
            if ( pObjbuf->fMark0 ) nid = Gia_ObjId(pOpt, pObjbuf);
            if ( pObjbuf->fMark1 ) printf("G node: %d\n", Gia_ObjId(pOpt, pObjbuf));
        }
        sprintf( str, "opt_patch(%d).dot", nid);
        Gia_SelfDefShow( pItpStore, str, 0, 0, 0 );
    }
    return pOpt;

}

Gia_Man_t*  Cec_ManSyn_rec( Gia_Man_t* pGia, Vec_Int_t* vCoef ) {
    /* new functions: 
        Cec_ManSubCircuit, Cec_ManGetMerge, Cec_ManGetSibling(TFO), 
        Cec_ManFuncDepReplace, Cec_ManItp, Cec_ManCompPatch,
        Int2_ManFdSimp, Cec_ManReplacePatch, Cec_ManRecPathLen,
        Cec_ManGetMaxMFFC, Cec_ManDist
    */
    // assert( Gia_ManCoNum( pGia ) == 1 );

    // print replaced node and its G in fdreplace ()
    // print subcircuit for next rec in pOut_FdReplace ()
    // print MFFC for in and out ckt
    // write function for coloring node with RGB ()
    // consider reconvergence path with considering lower node
    // write a simply replacement (not rec), with listing all patch and its info

    Gia_Man_t* pTemp, *pSep, *pOut, *pInt, *pSub;
    Gia_Obj_t* pObj;
    Vec_Ptr_t* vNdCone, *vObjBuff;
    Vec_Int_t* vSep = Vec_IntAlloc(100);
    Vec_Int_t* vIntBuff, *vIntBuff2;
    Vec_Int_t* vF = Vec_IntAlloc(100);
    Vec_Int_t* vReplaced = Vec_IntAlloc(100);
    Vec_Int_t *vG = Vec_IntAlloc(100);
    int i, nidbuf, val;
    int nidCkt1, nMffc_max, nMffc_prev;
    int coef_check = Vec_IntEntry( vCoef, 1 );
    int nUpLim = Vec_IntEntry( vCoef, 4 );
    int nBTLimit = Vec_IntEntry( vCoef, 3 );
    int debug = 1;
    int nIters = 0;
    int nMaxIters = Vec_IntEntry( vCoef, 5 );
    int fStage = Vec_IntEntry( vCoef, 6 );
    int giaName;
    char* str = ABC_ALLOC(char, 100);

    Cec_ParFra_t ParsFra, * pParsFra = &ParsFra;
    Cec_ManFraSetDefaultParams( pParsFra );
    pParsFra->nBTLimit = nBTLimit;

    pOut = Gia_ManDup( pGia );
    giaName = Gia_ManObjNum(pOut);
    nMffc_prev = -1;
    while (nMaxIters == 0 || nIters < nMaxIters) {
        nIters++;
        Cec_ManGetMerge( pOut, 0, 0, vSep, 1, 1 );
        if (debug) {
            printf("init %d for %d iter...\n", giaName, nIters);
            vIntBuff = Vec_IntAlloc(1);
            Cec_ManGetMerge( pOut, 0, 0, vIntBuff, 0, 1 );
            printf("merge frontier: ");
            Vec_IntPrint(vIntBuff);
            sprintf(str, "%d_pOut_init.dot", giaName);
            Gia_SelfDefShow( pOut, str, vSep, vIntBuff, 0 );
            sprintf(str, "%d_pOut_init.aig", giaName);
            Gia_AigerWrite( pOut, str, 0, 0, 0 );
            Vec_IntFree( vIntBuff );
        }
        if (Vec_IntSize(vSep) == 0) {
            printf("no sep part\n");
            break;
        }
        pOut = Cec_ManPartSyn( pTemp = pOut, vSep, 0, -1, 0 ); // dc2 the sep part
        Gia_ManStop( pTemp );
        pOut = Cec_ManSatSweeping( pTemp = pOut, pParsFra, 0 );

        // compute MFFC
        vIntBuff = Cec_ManGetMFFCost( pOut, coef_check );
        nidCkt1 = Vec_IntArgMax( vIntBuff );
        nMffc_max = Vec_IntFindMax( vIntBuff );
        printf("nMffc_max = %d, nidCkt1 = %d\n", nMffc_max, nidCkt1);
        if (nMaxIters == 0 && nMffc_max > nMffc_prev && nMffc_prev != -1) {
            printf("no more improve\n");
            break;
        }
        else nMffc_prev = nMffc_max;
        Vec_IntFree( vIntBuff );

        // get F and G
        vIntBuff = Vec_IntAlloc(1);
        Vec_IntPush( vIntBuff, nidCkt1 );
        Cec_ManGetMerge( pOut, vIntBuff, 0, vSep, 1, 1 ); // here, vSep for merge frontier ref
        Vec_IntFree( vIntBuff );
        Vec_IntClear( vF );
        Vec_IntClear( vG );
        if (pOut->pRefs == NULL) Gia_ManCreateRefs( pOut );
        Gia_NodeMffcSizeMark( pOut, Gia_ManObj(pOut, nidCkt1) );
        Gia_ManForEachAnd( pOut, pObj, i ) 
            if (Gia_ObjIsTravIdCurrent(pOut, pObj)) Vec_IntPush( vF, Gia_ObjId(pOut, pObj) );
        Gia_ManFillValue( pOut );
        Cec_ManSubcircuit( pOut, Gia_ManObj(pOut, nidCkt1), 0, 0, 1, 0 );
        vIntBuff = Vec_IntAlloc(1);
        Vec_IntPush( vIntBuff, nidCkt1 );
        Cec_ManGetTFO( pOut, vIntBuff, -1, 1 );
        Gia_ManForEachAnd( pOut, pObj, i ) {
            if (Gia_ObjValue(pObj) == -1) Vec_IntPushUnique( vG, Gia_ObjId(pOut, pObj) );
        }
        Vec_IntFree( vIntBuff );
        if (debug) {
            printf("get F and G...\n");
            sprintf(str, "%d_pOut_SimpSyn.dot", giaName);
            // printf("%s", str);
            Gia_SelfDefShow( pOut, str, vF, vG, 0 );
            sprintf(str, "%d_pOut_SimpSyn.aig", giaName);
            Gia_AigerWrite( pOut, str, 0, 0, 0 );
            Vec_IntPrint(vF);
            Vec_IntPrint(vG);
        }
        // get patch and replace
        vIntBuff = Vec_IntDup( vCoef );
        Vec_IntShrink( vIntBuff, 4 );
        pOut = Cec_ManFuncDepReplace( pTemp = pOut, vF, vG, vSep, vIntBuff );
        // pOut = Cec_ManFuncDepReplace( pTemp = pOut, vF, vG, 0, vIntBuff );
        Vec_IntFree( vIntBuff );

        if (pOut == NULL) {
            printf("FD replacement have no solution\n");
            pOut = pTemp;
            break;
        } else {
            Gia_ManStop( pTemp );
        }

        if (debug) {
            printf("get replaced...\n");
            vIntBuff = Vec_IntAlloc(1);
            vIntBuff2 = Vec_IntAlloc(1);
            Gia_ManForEachObj( pOut, pObj, i ) {
                if ( pObj->fMark0 ) Vec_IntPush( vIntBuff, Gia_ObjId(pOut, pObj) );
                if ( pObj->fMark1 ) Vec_IntPush( vIntBuff2, Gia_ObjId(pOut, pObj) );
            }
            sprintf(str, "%d_pOut_FdReplace.dot", giaName);
            Gia_SelfDefShow( pOut, str, vIntBuff, vIntBuff2, 0 );
            sprintf(str, "%d_pOut_FdReplace.aig", giaName);
            Gia_AigerWrite( pOut, str, 0, 0, 0 );
            Vec_IntFree( vIntBuff );
            Vec_IntFree( vIntBuff2 );
        }

        vIntBuff = Cec_ManGetMFFCost( pOut, coef_check );
        nidCkt1 = Vec_IntArgMax( vIntBuff );
        nMffc_max = Vec_IntFindMax( vIntBuff );
        printf("After FDRW: nMffc_max = %d, nidCkt1 = %d\n", nMffc_max, nidCkt1);
        Vec_IntFree( vIntBuff );

        // get pSub
        vReplaced = Cec_ManDumpMark( pOut, 0 );
        vReplaced = Cec_ManGetTFO( pOut, vIntBuff = vReplaced, nUpLim, 0 );
        // Vec_IntFree( vIntBuff );
        vObjBuff = Vec_PtrAlloc( 100 );
        Vec_IntForEachEntry( vReplaced, nidbuf, i ) {
            Vec_PtrPush( vObjBuff, Gia_ManObj(pOut, nidbuf) );
        }
        pSub = Cec_ManGetTFI( pOut, vObjBuff, 0 );
        Vec_PtrFree( vObjBuff );

        Vec_IntClear( vReplaced );
        Gia_ManForEachCo( pSub, pObj, i ) {
            printf("Replace Driver of Co %d: %d\n", i, Abc_Lit2Var(Gia_ObjFanin0Copy(pObj)));
            Vec_IntPush( vReplaced, Abc_Lit2Var(Gia_ObjFanin0Copy(pObj)) );
        }
        Vec_IntPrint(vReplaced);
        if (debug) {
            Gia_ManFillValue( pOut );
            Vec_IntForEachEntry( vReplaced, nidbuf, i ) {
                Cec_ManSubcircuit( pOut, Gia_ManObj(pOut, nidbuf), 0, 0, 1, 0 );
            }
            Vec_IntForEachEntry( vIntBuff, nidbuf, i ) {
                Cec_ManSubcircuit( pOut, Gia_ManObj(pOut, nidbuf), 0, 0, 2, 0 );
            }
            vIntBuff = Vec_IntAlloc(1);
            vIntBuff2 = Vec_IntAlloc(1);
            Gia_ManForEachObj( pOut, pObj, i ) {
                if ( ~pObj->Value & 1 ) Vec_IntPush( vIntBuff, Gia_ObjId(pOut, pObj) );
                if ( ~pObj->Value & 2 ) Vec_IntPush( vIntBuff2, Gia_ObjId(pOut, pObj) );
            }
            sprintf(str, "%d_pOut_getSub.dot", giaName);
            Gia_SelfDefShow( pOut, str, vIntBuff, vIntBuff2, 0 );
            Vec_IntFree( vIntBuff );
            Vec_IntFree( vIntBuff2 );
        }
        pSub = Cec_ManSyn_rec( pTemp = pSub, vCoef );
        Gia_ManStop( pTemp );

        // replace back
        pOut = Cec_ManReplacePatch( pTemp = pOut, pSub, vReplaced, 0, 0, 0 );
        Vec_IntClear( vReplaced );
        Gia_ManForEachCo( pSub, pObj, i ) {
            // printf("value is %d\n", Gia_ObjFanin0Copy(pObj));
            printf("Replaced node is %d\n", Abc_Lit2Var(Gia_ObjFanin0Copy(pObj)));
            Vec_IntPush( vReplaced, Abc_Lit2Var(Gia_ObjFanin0Copy(pObj)) );
        }
        Vec_IntPrint(vReplaced);

        vIntBuff = Cec_ManGetMFFCost( pOut, coef_check );
        nidCkt1 = Vec_IntArgMax( vIntBuff );
        nMffc_max = Vec_IntFindMax( vIntBuff );
        printf("After rec: nMffc_max = %d, nidCkt1 = %d\n", nMffc_max, nidCkt1);
        Vec_IntFree( vIntBuff );
        if (debug) {
            printf("replace back...\n");
            Gia_ManFillValue( pOut );
            Vec_IntForEachEntry( vReplaced, nidbuf, i ) {
                Cec_ManSubcircuit( pOut, Gia_ManObj(pOut, nidbuf), 0, 0, 1, 0 );
            }
            vIntBuff = Vec_IntAlloc(1);
            Gia_ManForEachObj( pOut, pObj, i ) {
                if ( ~pObj->Value & 1 ) Vec_IntPush( vIntBuff, Gia_ObjId(pOut, pObj) );
            }
            sprintf(str, "%d_pOut_replace_back.dot", giaName);
            Gia_SelfDefShow( pOut, str, vIntBuff, 0, 0 );
            Gia_SelfDefShow( pOut, str, 0, 0, 0 );
            sprintf(str, "%d_pOut_replace_back.aig", giaName);
            Gia_AigerWrite( pOut, str, 0, 0, 0 );
            Vec_IntFree( vIntBuff );
        }

        printf("done replacement...\n");
        Gia_ManStop( pTemp );
        Vec_IntFree( vReplaced );
    }
    Cec_ManGetMerge( pOut, 0, 0, vSep, 1, 1 );
    pOut = Cec_ManPartSyn( pTemp = pOut, vSep, 0, -1, 0 );
    Gia_ManStop( pTemp );
    if (debug) {
        sprintf(str, "%d_final_pOut.dot", giaName);
        Gia_SelfDefShow( pOut, str, 0, 0, 0 );
        sprintf(str, "%d_final_pOut.aig", giaName);
        Gia_AigerWrite( pOut, str, 0, 0, 0 );
    }
    printf("end %d...\n", giaName);
    return pOut;
}
Gia_Man_t*  Cec_ManSyn( Gia_Man_t* pGia, Vec_Int_t* vCoef ) {
    return Cec_ManSyn_rec( pGia, vCoef );
}
Gia_Man_t*  Cec_ManSimpSyn( Gia_Man_t* pGia, int fSat, int fVerbose, int nRuns ) {
    Gia_Man_t *pTemp, *pTemp2;
    // Gps_Par_t ParsGps, * pParsGps = &ParsGps;
    abctime clk = Abc_Clock();
    int i;

    pTemp = Gia_ManDup( pGia );
    
    // pTemp = Gia_ManCompress2( pTemp2 = pTemp, 1, 0 );
    if (nRuns == -1) {
        while (true) {
            pTemp = Gia_ManSelfSyn( pTemp2 = pTemp, 1, fSat, fSat, 0 );
            if (Gia_ManAndNum(pTemp) >= Gia_ManAndNum(pTemp2)) {
                Gia_ManStop( pTemp );
                pTemp = pTemp2;
                break;
            } else {
                Gia_ManStop( pTemp2 );
            }
        }
    } else {
        for (i = 0; i < nRuns; i++) {
            pTemp = Gia_ManSelfSyn( pTemp2 = pTemp, 1, fSat, fSat, 0 );
            Gia_ManStop( pTemp2 );
        }
    }
    
    if (fVerbose) {
        printf("trim: %d->%d\n", Gia_ManAndNum(pGia), Gia_ManAndNum(pTemp));
        // Abc_PrintTime(-1, "Time: ", Abc_Clock() - clk);
        Abc_Print(1, "Trim time: %9.6f sec\n", 1.0*((double)(Abc_Clock() - clk))/((double)((__clock_t) 1000000)));

    }
    return pTemp;
}
Gia_Man_t*  Cec_ManPartSyn( Gia_Man_t* pGia, Vec_Int_t* vFI_in, Vec_Int_t* vFO_in, int nDc2, int fVerbose ) {
    // pOut = Cec_ManFdReplaceMulti(pMan, vFI);
    // pOut = Cec_ManPartSyn( pTemp = pOut, vFI, vFO );
    // Cec_ManMapCopy( pOut, pTemp, 1 );
    Gia_Man_t *pOut, *pPart, *pTemp, *pGia_cp;
    Vec_Int_t *vFI = vFI_in ? Vec_IntDup( vFI_in ) : Vec_IntDup( pGia->vCis );
    Vec_Int_t *vFO = vFO_in ? Vec_IntDup( vFO_in ) : Vec_IntDup( pGia->vCos );
    Vec_Ptr_t *vNdCone;
    int nid, i, val;
    Gia_Obj_t *pObj;

    pGia_cp = Gia_ManDup( pGia );
    if (fVerbose) Gia_SelfDefShow( pGia_cp, "synPart.dot", vFI, vFO, 0 );
    // checking, can be removed
    Gia_ManFillValue( pGia_cp );
    Gia_ManConst0(pGia_cp)->Value = 0;
    Gia_ManForEachCo( pGia_cp, pObj, i ) {
        Cec_ManSubcircuit( pGia_cp, pObj, vFO, 0, 1, 0 );
    }
    Vec_IntForEachEntry( vFI, nid, i ) {
        if (Gia_ObjValue( Gia_ManObj(pGia_cp, nid) ) != -1) printf("nid %d is not covered by high\n", nid);
    }
    Vec_IntForEachEntry( vFO, nid, i ) {
        Gia_ObjSetValue( Gia_ManObj(pGia_cp, nid), -1 );
        Cec_ManSubcircuit( pGia_cp, Gia_ManObj(pGia_cp, nid), vFI, 0, 2, 0 );
    }
    Gia_ManForEachCi( pGia_cp, pObj, i ) {
        if ( Gia_ObjValue(pObj) != -1 && Vec_IntFind( vFI, Gia_ObjId(pGia_cp, pObj) ) == -1 ) printf("ci %d is not covered by low\n", Gia_ObjId(pGia_cp, pObj));
    }

    Gia_ManFillValue( pGia_cp );
    Gia_ManConst0(pGia_cp)->Value = 0;
    pPart = Gia_ManStart( Gia_ManObjNum(pGia_cp) );
    Vec_IntForEachEntry( vFI, nid, i ) {
        Gia_ObjSetValue(Gia_ManObj(pGia_cp, nid), Gia_ManAppendCi( pPart ));
    }
    Vec_IntForEachEntry( vFO, nid, i ) {
        vNdCone = Cec_ManSubcircuit( pGia_cp, Gia_ManObj(pGia_cp, nid), vFI, 0, 0, 1 );
        if (Vec_PtrSize(vNdCone) != 0) {
            val = Cec_ManPatch( pPart, vNdCone );
        } else {
            val = Gia_ObjValue( Gia_ManObj(pGia_cp, nid) );
        }
        // val = Cec_ManPatch( pPart, vNdCone );
        Gia_ManAppendCo( pPart, val );
        Vec_PtrFree( vNdCone );
    }
    // Gia_SelfDefShow( pPart, "Part_before_simp.dot", 0,0,0);
    pPart = Cec_ManSimpSyn( pTemp = pPart, 1, fVerbose, nDc2 );
    Gia_ManStop( pTemp );
    
    // Gia_SelfDefShow( pPart, "Part_after_simp.dot", 0,0,0);
    // Vec_IntPrint(vFI);
    // Vec_IntPrint(vFO);
    pOut = Cec_ManReplacePatch( pGia_cp, pPart, vFO, vFI, 0, 1 );
    // Gia_ManForEachObj( pPart, pObj, i ) {
    //     // val = Abc_Lit2Var(Gia_ObjValue(pObj));
    //     if (Gia_ObjValue(pObj) == -1) printf("nid %d is not copied\n", Gia_ObjId(pPart, pObj) );
    //     else {
    //         val = Abc_Lit2Var(Gia_ObjValue(pObj));
    //         printf("nid %d is copied to %d\n", Gia_ObjId(pPart, pObj), val );
    //         printf("double check: pOut node %d have value %d\n", val, Gia_ObjValue(Gia_ManObj(pOut, val)) );
    //     }
        
    // }
    // if (fVerbose) Cec_ManShowValNode( pOut, "synPart_after.dot" );
    Gia_ManStop( pPart );
    Gia_ManStop( pGia_cp );
    return pOut;
    
}
Gia_Man_t*  Cec_ManFraigSyn_naive( Gia_Man_t* pGia, int nConflict ) {
    Vec_Int_t *vIntBuff;
    Vec_Ptr_t *vObj;
    Gia_Man_t *pMiter, *pTemp;
    Gia_Obj_t *pObjbuf;
    Cec_ParFra_t ParsFra, * pParsFra = &ParsFra;
    int i, val1, val2;
    Cec_ManFraSetDefaultParams( pParsFra );
    pParsFra->nBTLimit = nConflict;
    vIntBuff = Cec_ManCheckMiter( pGia );

    vObj = Vec_PtrAlloc( 2 );
    Vec_PtrPush( vObj, Gia_ManObj( pGia, Vec_IntEntry( vIntBuff, 0 ) ) );
    Vec_PtrPush( vObj, Gia_ManObj( pGia, Vec_IntEntry( vIntBuff, 1 ) ) );
    pMiter = Cec_ManGetTFI( pGia, vObj, 0 ); // why there would be 216 co????
    Vec_PtrFree( vObj );
    if (Vec_IntEntry( vIntBuff, 2 )) Gia_ObjFlipFaninC0( Gia_ManCo( pMiter, 0 ) );
    pMiter = Cec_ManSatSweeping( pTemp = pMiter, pParsFra, 0 );

    pTemp = pMiter;
    pMiter = Gia_ManStart( Gia_ManObjNum(pTemp) + 4 );
    Gia_ManFillValue( pTemp );
    Gia_ManConst0(pTemp)->Value = 0;
    Gia_ManForEachCi( pTemp, pObjbuf, i ) {
        Gia_ObjSetValue(pObjbuf, Gia_ManAppendCi( pMiter ));
    }
    vObj = Cec_ManSubcircuit( pTemp, Gia_ManCo(pTemp, 0), 0, 0, 0, 1 );
    val1 = Cec_ManPatch( pMiter, vObj );
    Vec_PtrFree( vObj );
    vObj = Cec_ManSubcircuit( pTemp, Gia_ManCo(pTemp, 1), 0, 0, 0, 1 );
    val2 = Cec_ManPatch( pMiter, vObj );
    Vec_PtrFree( vObj );


    // val1 = Abc_LitNotCond( val1, fPhase );
    Gia_ManAppendCo( pMiter, Gia_ManAppendXor( pMiter, val1, val2 ));
    Vec_IntFree( vIntBuff );
    Gia_ManStop( pTemp );
    return pMiter;

}
Gia_Man_t*  Cec_ManFraigSyn( Gia_Man_t* pGia, Gia_Obj_t* pObj1, Gia_Obj_t* pObj2, int fComp, int fVerbose, int nConflict, int fSimp ) {
    // deal with the miter adding XOR or XNOR!
    extern Aig_Man_t * Fra_FraigCecSyn( Aig_Man_t ** ppAig, int fFirst, int fVerbose );
    Gia_Man_t *pCkt1, *pCkt2, *pMiter, *pTemp;
    Vec_Ptr_t *vObj;
    Aig_Man_t * pMiterCec, *pTempCec;
    Cec_ParFra_t ParsFra, * pParsFra = &ParsFra;
    abctime clk = Abc_Clock();
    int RetValue, i;
    Cec_ManFraSetDefaultParams( pParsFra );
    pParsFra->nBTLimit = nConflict;
    
    vObj = Vec_PtrAlloc( 2 );
    Vec_PtrPush( vObj, pObj1 );
    Vec_PtrPush( vObj, pObj2 );
    pMiter = Cec_ManGetTFI( pGia, vObj, 0 );
    if (fComp) Gia_ObjFlipFaninC0( Gia_ManCo( pMiter, 0 ) );

    pMiter = Cec_ManSatSweeping( pTemp = pMiter, pParsFra, 0 );
    Gia_ManStop( pTemp );
    if (fSimp) {
        printf("only sweep is applied\n");
        pMiter = Cec_ManToMiter( pTemp = pMiter, 0 );
        Gia_ManStop( pTemp );
        return pMiter;
    }

    for (i = 0; i <= 6; i++ ) {
        pMiterCec = Gia_ManToAig( pMiter, 0 );
        Gia_ManStop( pMiter );
        pTempCec = Fra_FraigCecSyn( &pMiterCec, i, fVerbose );
        // Aig_ManStop( pMiterCec );
        pTemp = Gia_ManFromAig( pTempCec );
        Aig_ManStop( pTempCec );
        if (i < 6) {
            pMiter = Cec_ManSatSweeping( pTemp, pParsFra, 0 );
            Gia_ManStop( pTemp );
        } else {
            pMiter = pTemp;
        }
        
    }
    pMiter = Cec_ManToMiter( pTemp = pMiter, 0 );
    Gia_ManStop( pTemp );
    
    if (fVerbose) {
        printf("Fraig Syn (&cec) \n");
        printf("%d->%d\n", Gia_ManAndNum(pGia), Gia_ManAndNum(pMiter));
        Abc_PrintTime(-1, "Time: ", Abc_Clock() - clk);
        Gia_ManPrintStats(pMiter, 0);
    }

    return pMiter;
}
void Cec_ManShowValNode( Gia_Man_t* pGia, char* pFileName ) {
    Vec_Int_t* vIntBuff = Vec_IntAlloc( 1 );
    Gia_Obj_t* pObj;
    int i;
    Gia_ManForEachObj( pGia, pObj, i ) {
        if ( Gia_ObjValue(pObj) != -1 )
            Vec_IntPush( vIntBuff, Gia_ObjId(pGia, pObj) );
    }
    Gia_SelfDefShow( pGia, pFileName, vIntBuff, 0, 0 );
}
void Cec_ManFdSetLevel_abs( Cec_ManFd_t* pMan, int fCkt ) {
    assert(fCkt == 1 || fCkt == 2);
    // int fFI = Cec_ManFdGetLevelType( pMan, 2 );
    int ftype = Cec_ManFdGetLevelType( pMan, 0 );
    int ftype_below = ftype | (1 << CEC_FD_LEVELBITNDMM);
    int** dict_below = Cec_ManFdGetLevelType( pMan, 1 ) ? pMan->dist_short : pMan->dist_long; // still consider dist defined by user, not short dict
    int nidPO = fCkt == 1 ? Gia_ObjId(pMan->pAbs, pMan->pObjAbs1) : Gia_ObjId(pMan->pAbs, pMan->pObjAbs2);
    int orgnid, nid_buff, int_buff, int_buff2, i, j;
    Vec_Int_t* vPO = Vec_IntStart( 1 );
    Vec_Int_t* vLevel = fCkt == 1 ? pMan->vLevel1 : pMan->vLevel2;
    Vec_Int_t* vIntBuff;
    Vec_Int_t* vFrt = Vec_IntAlloc( 1 ); // not all vMerge connect to both PO in pAbs
    Vec_Wec_t* vDistAnalToPO, *vDistAnalToFrt;
    
    Vec_IntFill( vLevel, Vec_IntSize(vLevel), -1);
    Vec_IntWriteEntry( vPO, 0, nidPO );
    vDistAnalToPO = Cec_ManDistAnalyze( pMan->pAbs, 0, 0, vPO, pMan->dist_long_abs, pMan->dist_short_abs );
    Vec_WecForEachLevel( vDistAnalToPO, vIntBuff, i ) {
        if (i == Vec_WecSize(vDistAnalToPO) - 1) continue; // skip the last level
        orgnid = Cec_ManFdMapIdSingle( pMan, Vec_IntEntry(vIntBuff, 0), 0 );
        int_buff = Vec_IntEntry(vIntBuff, ftype + 2);
        if (orgnid == -1 || int_buff == -1) continue;
        if (Gia_ObjColors( pMan->pGia, orgnid ) & fCkt)
            Vec_IntWriteEntry( vLevel, orgnid, int_buff );
        if (Gia_ObjColors( pMan->pGia, orgnid ) == 3)
            Vec_IntPush( vFrt, orgnid );
    }
    vDistAnalToFrt = Cec_ManDistAnalyze( pMan->pGia, 0, 0, vFrt, pMan->dist_long, pMan->dist_short );
    Vec_WecForEachLevel( vDistAnalToFrt, vIntBuff, i ) {
        if (i == Vec_WecSize(vDistAnalToFrt) - 1) continue; // skip the last level
        if (Vec_IntEntry(vIntBuff, 1) != 3) continue;
        orgnid = Vec_IntEntry(vIntBuff, 0);
        int_buff = Vec_IntEntry(vIntBuff, ftype_below + 2);
        if (Vec_IntFind( vFrt, orgnid ) != -1 || int_buff == -1) continue;
        int_buff2 = -1; // pick min of level that have dist as int_buff from nid to nid_buff
        Vec_IntForEachEntry( vFrt, nid_buff, j ) {
            if (dict_below[nid_buff][orgnid] == int_buff) {
                assert(Vec_IntEntry(vLevel, nid_buff) != -1);
                // printf("orgnid: %d nid_buff: %d int_buff: %d\n", orgnid, nid_buff, int_buff);
                if (int_buff2 == -1 || Vec_IntEntry(vLevel, nid_buff) < int_buff2)
                    int_buff2 = Vec_IntEntry(vLevel, nid_buff);
            }
        }
        Vec_IntWriteEntry( vLevel, orgnid, int_buff + int_buff2 );
    }
}

void Cec_ManFdSetLevel_glob( Cec_ManFd_t* pMan, int fCkt ) {
    assert(fCkt == 1 || fCkt == 2);
    // int fFI = Cec_ManFdGetLevelType( pMan, 2 );
    int ftype = Cec_ManFdGetLevelType( pMan, 0 );
    int orgnid, nid_buff, int_buff, int_buff2, i, j;
    int nidPO = fCkt == 1 ? Gia_ObjId(pMan->pGia, pMan->pObj1) : Gia_ObjId(pMan->pGia, pMan->pObj2);
    Vec_Int_t* vPO = Vec_IntStart( 1 );
    Vec_Int_t* vLevel = fCkt == 1 ? pMan->vLevel1 : pMan->vLevel2;
    Vec_Int_t* vIntBuff;
    Vec_Int_t* vFrt = Vec_IntAlloc( 1 ); // not all vMerge connect to both PO in pAbs
    Vec_Wec_t* vDistAnalToPO;
    
    Vec_IntFill( vLevel, Vec_IntSize(vLevel), -1);
    Vec_IntWriteEntry( vPO, 0, nidPO );
    vDistAnalToPO = Cec_ManDistAnalyze( pMan->pGia, 0, 0, vPO, pMan->dist_long, pMan->dist_short );
    Vec_WecForEachLevel( vDistAnalToPO, vIntBuff, i ) {
        if (i == Vec_WecSize(vDistAnalToPO) - 1) continue; // skip the last level
        orgnid = Vec_IntEntry(vIntBuff, 0);
        int_buff = Vec_IntEntry( vIntBuff, ftype + 2 );
        Vec_IntWriteEntry( vLevel, orgnid, int_buff );
    }
}

void Cec_ManFdSetLevel( Cec_ManFd_t* pMan ) {
    int fAbs = pMan->pPars->fAbs;
    int fFI = Cec_ManFdGetLevelType( pMan, 2 );
    int ftype = Cec_ManFdGetLevelType( pMan, 0 );
    // int ftype_togFI = ftype ^ (1 << CEC_FD_LEVELBITFIO);
    // if (fFI) ftype_togFI ^= (1 << CEC_FD_LEVELBITPATH);
    int fNorm = Cec_ManFdGetLevelType( pMan, 4 );
    int maxval, int_buff, int_buff2, i, nid, absnid, nid1, nid2;

    assert( fNorm == 0 ); // currently ban fNorm

    Vec_Int_t* vIntBuff;
    Vec_Int_t* vIntSum = Vec_IntStart( Gia_ManObjNum( pMan->pGia ) );
    Vec_Wec_t* vDistAnalToPO, *vDistAnalToFrt;
    Gia_Obj_t *pObjbuf;

    // ckt1
    if (fAbs) {
        Cec_ManFdSetLevel_abs( pMan, 1 );
        Cec_ManFdSetLevel_abs( pMan, 2 );
    } else {
        Cec_ManFdSetLevel_glob( pMan, 1 );
        Cec_ManFdSetLevel_glob( pMan, 2 );
    }
    
}

void Cec_ManFdSetCostTh( Cec_ManFd_t* pMan ) {
    int fAbs = pMan->pPars->fAbs;
    int ftype = pMan->pPars->costType;
    float coef = pMan->pPars->coefPatch;
    float val, dist_buf, dist;
    Vec_Flt_t* vCostTh = pMan->vCostTh;
    Vec_Int_t* vLevel, *vBuf;
    Gia_Obj_t* pObjbuf, *pObjbuf2;
    int i, j, nid, nidbuf, nidPO, color, cnt, dist_in, dist_out, dist_sup, check;
    Vec_FltFill( vCostTh, Vec_FltSize(vCostTh), -1.0 );
    switch (ftype)
    {
    case CEC_FD_COSTDUMMY:
        break;
    case CEC_FD_COSTANDNUM:
        Gia_ManForEachAnd( pMan->pAbs, pObjbuf, i ) {
            nid = Gia_ObjId(pMan->pAbs, pObjbuf);
            Vec_FltWriteEntry( vCostTh, Cec_ManFdMapIdSingle(pMan, nid, 0), coef * Gia_ManAndNum(pMan->pAbs) );
        }
        break;
    case CEC_FD_COSTFIXANDNUM:
        Gia_ManForEachAnd( pMan->pAbs, pObjbuf, i ) {
            nid = Gia_ObjId(pMan->pAbs, pObjbuf);
            Vec_FltWriteEntry( vCostTh, Cec_ManFdMapIdSingle(pMan, nid, 0), coef );
        }
        break;
    case CEC_FD_COSTHIERANDNUM:
        Gia_ManForEachAnd( pMan->pAbs, pObjbuf, i ) {
            nid = Gia_ObjId(pMan->pAbs, pObjbuf);
            Gia_ManFillValue( pMan->pAbs );
            Cec_ManSubcircuit( pMan->pAbs, pObjbuf, 0, 0, 1, 0 );
            cnt = 0;
            Gia_ManForEachAnd( pMan->pAbs, pObjbuf2, j ) {
                if (Gia_ObjValue(pObjbuf2) != -1) cnt += 1;
            }
            // printf("costTh: %d, %d, %f, %f\n", Cec_ManFdMapIdSingle(pMan, nid, 0), cnt, coef, coef * cnt);
            Vec_FltWriteEntry( vCostTh, Cec_ManFdMapIdSingle(pMan, nid, 0), coef * cnt );
            // printf("%f(%f)\n", coef * cnt, Vec_FltEntry(vCostTh, Cec_ManFdMapIdSingle(pMan, nid, 0)));
        }
        break;
    case CEC_FD_COSTMFFC:
        Gia_ManForEachAnd( pMan->pAbs, pObjbuf, i ) {
            nid = Gia_ObjId(pMan->pAbs, pObjbuf);
            val = 1.0 * Gia_NodeMffcSize( pMan->pAbs, pObjbuf );
            Vec_FltWriteEntry( vCostTh, Cec_ManFdMapIdSingle(pMan, nid, 0), coef * val );
        }
        break;
    case CEC_FD_COSTHEIGHT:
        Gia_ManForEachAnd( pMan->pAbs, pObjbuf, i ) {
            nid = Gia_ObjId(pMan->pAbs, pObjbuf);
            dist = 0.0;
            for (j = 0; j < Gia_ManObjNum(pMan->pAbs); j++) {
                if (pMan->dist_long_abs[nid][j] > dist) dist = 1.0 * pMan->dist_long_abs[nid][j];
            }
            Vec_FltWriteEntry( vCostTh, Cec_ManFdMapIdSingle(pMan, nid, 0), coef * dist );
        }
        break;
    case CEC_FD_COSTTWOPATHOLD:
        assert(coef <= 1.0);
        assert(coef >= 0.0);
        Gia_ManForEachAnd( pMan->pAbs, pObjbuf, i ) {
            nid = Cec_ManFdMapIdSingle( pMan, Gia_ObjId(pMan->pAbs, pObjbuf), 0 );
            nidPO = Cec_ManFdGetPO( pMan, nid, 0, 1 );
            // printf("nid: %d, pObjbuf2: %d\n", nid, Gia_ObjId(pMan->pGia, pObjbuf2));
            dist = -1.0;
            dist_out = Cec_ManFdGetLevelType(pMan, 1) ? 
                        pMan->dist_short[nidPO][nid] :
                        pMan->dist_long[nidPO][nid];
            if (pMan->pPars->fVerbose) printf("nid: %d\n", nid);
            Vec_IntForEachEntry( Vec_WecEntry( pMan->vMergeSupport, nid ), check, j ) {
                if (check) {
                    nidPO = Cec_ManFdGetPO( pMan, nid, 0, 0 );
                    dist_in = Cec_ManFdGetLevelType(pMan, 1) ? 
                        pMan->dist_short[nid][Vec_IntEntry( pMan->vMerge, j )] :
                        pMan->dist_long[nid][Vec_IntEntry( pMan->vMerge, j )];
                    dist_sup = Cec_ManFdGetLevelType(pMan, 1) ? 
                        pMan->dist_short[nidPO][Vec_IntEntry( pMan->vMerge, j )] :
                        pMan->dist_long[nidPO][Vec_IntEntry( pMan->vMerge, j )];
                    if (pMan->pPars->fVerbose)
                        printf("\tnidsup: %d, dist_in: %d, dist_out: %d, dist_sup: %d\n", Vec_IntEntry( pMan->vMerge, j ), dist_in, dist_out, dist_sup);
                    val = coef * (dist_in + dist_out) + (1.0 - coef) * dist_sup;
                    if (dist < 0) dist = val;
                    else {
                        if (Cec_ManFdGetLevelType(pMan, 3)) { // min
                            dist = Abc_MinFloat( dist, val ); 
                        } else { // max
                            dist = Abc_MaxFloat( dist, val );
                        }
                    }
                }
            }
            
            Vec_FltWriteEntry( vCostTh, nid, dist );
        }
        break;
    case CEC_FD_COSTTWOPATHRMS:
    case CEC_FD_COSTTWOPATHMAX:
    case CEC_FD_COSTTWOPATHMIN:
        Gia_ManForEachAnd( pMan->pAbs, pObjbuf, i ) {
            nid = Cec_ManFdMapIdSingle( pMan, Gia_ObjId(pMan->pAbs, pObjbuf), 0 );
            nidPO = Cec_ManFdGetPO( pMan, nid, 0, 1 );
            // printf("nid: %d, pObjbuf2: %d\n", nid, Gia_ObjId(pMan->pGia, pObjbuf2));
            dist = -1.0;
            dist_out = Gia_ObjColor( pMan->pGia, nid, 0 ) ? 
                        Vec_IntEntry( pMan->vLevel1, nid ) : 
                        Vec_IntEntry( pMan->vLevel2, nid );
            // dist_out = Cec_ManFdGetLevelType(pMan, 1) ? 
            //             pMan->dist_short[nidPO][nid] :
            //             pMan->dist_long[nidPO][nid];
            val = 0.0;
            cnt = 0;
            Vec_IntForEachEntry( Vec_WecEntry( pMan->vMergeSupport, nid ), check, j ) {
                if (check) {
                    nidPO = Cec_ManFdGetPO( pMan, nid, 0, 0 );
                    dist_in = Cec_ManFdGetLevelType(pMan, 1) ? 
                        pMan->dist_short[nid][Vec_IntEntry( pMan->vMerge, j )] :
                        pMan->dist_long[nid][Vec_IntEntry( pMan->vMerge, j )];
                    dist_sup = Gia_ObjColor( pMan->pGia, nid, 0 ) ? 
                                Vec_IntEntry( pMan->vLevel2, Vec_IntEntry( pMan->vMerge, j ) ) : 
                                Vec_IntEntry( pMan->vLevel1, Vec_IntEntry( pMan->vMerge, j ) );
                    dist_buf = 1.0 * (dist_in + dist_out) * (dist_in + dist_out) + 1.0 * dist_sup * dist_sup;
                    if (ftype == CEC_FD_COSTTWOPATHRMS) {
                        val += dist_buf;
                        cnt += 1;
                    } else if (ftype == CEC_FD_COSTTWOPATHMAX) {
                        if (val == 0.0 || dist_buf > val) val = dist_buf;
                        cnt = 1;
                    } else if (ftype == CEC_FD_COSTTWOPATHMIN) {
                        if (val == 0.0 || dist_buf < val) val = dist_buf;
                        cnt = 1;
                    }
                }
            }
            dist = coef * sqrt( val / cnt );
            Vec_FltWriteEntry( vCostTh, nid, dist );
        }
        break;
    
    case CEC_FD_COSTCINUM:
        Gia_ManForEachAnd( pMan->pAbs, pObjbuf, i ) {
            nid = Cec_ManFdMapIdSingle( pMan, Gia_ObjId(pMan->pAbs, pObjbuf), 0 );
            vBuf = Vec_WecEntry( pMan->vMergeSupport, nid );
            val = 1.0 * Vec_IntSum(vBuf);
            Vec_FltWriteEntry( vCostTh, nid, coef * val );
        }
        break;
    case CEC_FD_COSTSUPRMS:
    case CEC_FD_COSTSUPMAX:
    case CEC_FD_COSTSUPMIN:
        Gia_ManForEachAnd( pMan->pAbs, pObjbuf, i ) {
            nid = Cec_ManFdMapIdSingle( pMan, Gia_ObjId(pMan->pAbs, pObjbuf), 0 );
            nidPO = Cec_ManFdGetPO( pMan, nid, 1, 1 );
            // pObjbuf2 = (Gia_ObjColors( pMan->pGia, nid ) == 1) ? pMan->pObjAbs1 : pMan->pObjAbs2;
            val = 0.0;
            cnt = 0;
            Vec_IntForEachEntry( Vec_WecEntry( pMan->vMergeSupport, nid ), check, j ) {
                if (check) {
                    nidbuf = Vec_IntEntry( pMan->vMerge, j );
                    dist_sup = Cec_ManFdGetLevelType(pMan, 1) ? 
                        pMan->dist_short_abs[nidPO][Cec_ManFdMapIdSingle(pMan, nidbuf, 1)] :
                        pMan->dist_long_abs[nidPO][Cec_ManFdMapIdSingle(pMan, nidbuf, 1)];
                    dist_buf = 1.0 * dist_sup * dist_sup;
                    if (ftype == CEC_FD_COSTSUPRMS) {
                        val += dist_buf;
                        cnt += 1;
                    } else if (ftype == CEC_FD_COSTSUPMAX) {
                        if (val == 0.0 || dist_buf > val) val = dist_buf;
                        cnt = 1;
                    } else if (ftype == CEC_FD_COSTSUPMIN) {
                        if (val == 0.0 || dist_buf < val) val = dist_buf;
                        cnt = 1;
                    }
                }
            }
            dist = coef * sqrt(val / cnt);
            Vec_FltWriteEntry( vCostTh, nid, dist );
        }
        break;
    default:
        break;
    }
}
void Cec_ManFdSetGSupportDefaultone( Cec_ManFd_t* pMan, int nid ) {
    Gia_Obj_t* pObjbuff;
    int nidSupport, i, j, val;
    int colorSelf = Gia_ObjColors( pMan->pGia, nid );
    assert( colorSelf == 1 || colorSelf == 2 );
    int gType = pMan->pPars->GType >> 1;
    int maxLevel, minLevel;
    Vec_Int_t *vIntBuff1, *vIntBuff2;
    Vec_Int_t *vLevel = (colorSelf == 1) ? pMan->vLevel2 : pMan->vLevel1;
    Vec_Int_t *vFdSupportBuff = Vec_WecEntry( pMan->vGSupport, nid );
    Vec_Int_t *vNdcktSelf = Cec_ManGetColorNd( pMan->pGia, colorSelf );
    Vec_Int_t *vNdcktOth = Cec_ManGetColorNd( pMan->pGia, colorSelf ^ 3);
    Vec_Int_t *vNdcktShare = Cec_ManGetColorNd( pMan->pGia, 4 );
    Vec_Wec_t *vSupport = pMan->vMergeSupport;

    // add merge frontier
    vIntBuff1 = Vec_WecEntry( vSupport, nid ); // one-hot
    maxLevel = -1;
    Vec_IntForEachEntry( vIntBuff1, val, j) {
        if (val) {
            // TODO deal with merge frontier that might not be in big SAT solver
            maxLevel = Abc_MaxInt( maxLevel, Vec_IntEntry(vLevel, Vec_IntEntry(pMan->vMerge, j)) );
            if (pMan->pPars->GType & 1) Vec_IntPush( vFdSupportBuff, Vec_IntEntry(pMan->vMerge, j) );
        }
    }
    minLevel = Vec_IntEntry((colorSelf == 1) ? pMan->vLevel1 : pMan->vLevel2, nid);

    Vec_IntForEachEntry( vNdcktOth, nidSupport, j ) { // nodes in another circuit
        if (pMan->pPars->fInc && Cec_ManFdMapVarSingle( pMan, nidSupport, 1 ) < 0) continue;

        if (gType == CEC_FD_GABS || gType == CEC_FD_GALL) {
            Vec_IntPush( vFdSupportBuff, nidSupport );
        } else if (gType == CEC_FD_GUPPER || gType == CEC_FD_GALLUPPER) {
            if (Vec_IntEntry(vLevel, nidSupport) <= maxLevel) // upper
                Vec_IntPush( vFdSupportBuff, nidSupport );
        } else if (gType == CEC_FD_GLOWER || gType == CEC_FD_GALLLOWER || gType == CEC_FD_GINITLOWER) {
            if (Vec_IntEntry(vLevel, nidSupport) > minLevel) // lower
                Vec_IntPush( vFdSupportBuff, nidSupport );
        } else {
            vIntBuff2 = Vec_WecEntry( vSupport, nidSupport );
            assert( Vec_IntSize(vIntBuff2) > 0 );
            if ( Vec_IntSize(vIntBuff2) > 0 && Vec_IntCheckSubset( vIntBuff1, vIntBuff2 ) ) 
                Vec_IntPush( vFdSupportBuff, nidSupport );
        }
    }

    Gia_ManFillValue( pMan->pGia );
    // might be trouble here
    Cec_ManSubcircuit( pMan->pGia, Gia_ManObj(pMan->pGia, nid), 0, 0, 1, 0 ); // mark the TFI as 0
    if (gType == CEC_FD_GINITLOWER) {
        Vec_IntForEachEntry( pMan->vNodeInit, nidSupport, j) {
            Gia_ManObj(pMan->pGia, nidSupport)->Value ^= 2;
        }
        Gia_ManForEachAnd( pMan->pGia, pObjbuff, i ) {
            pObjbuff->Value ^= 2;
            // Gia_ObjSetValue( pObjbuff, -Gia_ObjValue( pObjbuff ) ); // not in init would be 1, which is avoid in further
        }
    }
    if (gType == CEC_FD_GALL || gType == CEC_FD_GALLUPPER || gType == CEC_FD_GALLLOWER || gType == CEC_FD_GINITLOWER) { // nodes in shared part
        Vec_IntForEachEntry( vNdcktShare, nidSupport, j) {
            if (pMan->pPars->fInc && Cec_ManFdMapVarSingle( pMan, nidSupport, 1 ) < 0) continue;
            if (Gia_ObjIsAnd( Gia_ManObj(pMan->pGia, nidSupport) ) == 0) continue;
            if (Gia_ObjValue( Gia_ManObj(pMan->pGia, nidSupport) ) != -1) continue; // In TFI
            assert( Gia_ObjColors( pMan->pGia, nidSupport ) == 3 );
            // printf("nidSupport: %d, level: %d\n", nidSupport, Vec_IntEntry(vLevel, nidSupport));
            
            switch (gType)
            {
                case CEC_FD_GALL:
                    Vec_IntPush( vFdSupportBuff, nidSupport );
                    break;
                case CEC_FD_GALLUPPER:
                    if (Vec_IntEntry(vLevel, nidSupport) <= maxLevel) // upper
                        Vec_IntPush( vFdSupportBuff, nidSupport );
                    break;
                case CEC_FD_GALLLOWER: case CEC_FD_GINITLOWER:
                    if (Vec_IntEntry(vLevel, nidSupport) > minLevel) // lower
                        Vec_IntPush( vFdSupportBuff, nidSupport );
                    break;
                default:
                    break;
            }
        }
    }
}
void Cec_ManFdSetGSupportDefault( Cec_ManFd_t* pMan ) {
    // Gia_Obj_t* pObjbuff;
    int nid, i; // , nidSupport, j, val;
    // int gType = pMan->pPars->GType >> 1;
    // int maxLevel;
    Vec_Int_t *vIntBuff1, *vIntBuff2, *vFdSupportBuff;
    Vec_Int_t *vNdckt1 = Cec_ManGetColorNd( pMan->pGia, 1 );
    Vec_Int_t *vNdckt2 = Cec_ManGetColorNd( pMan->pGia, 2 );
    // Vec_Int_t *vNdckt3 = Cec_ManGetColorNd( pMan->pGia, 4 );
    // Vec_Int_t *vMerge = pMan->vMerge;
    // Vec_Wec_t *vSupport = pMan->vMergeSupport;

    Vec_IntForEachEntry( vNdckt1, nid, i ) {
        Cec_ManFdSetGSupportDefaultone( pMan, nid );
    }
    Vec_IntForEachEntry( vNdckt2, nid, i ) {
        Cec_ManFdSetGSupportDefaultone( pMan, nid );
    }

    // TODO: currently those under merge frontier is not consider
    
    
}
void Cec_ManFdReadStat( Cec_ManFd_t* pMan, char* pFileName, int fClear ) {
    assert( pMan->dist_short );
    Vec_Wec_t * vWecBuff;
    Vec_Int_t * vIntBuff; 
    Gia_Obj_t * pObj;
    int nSize, i, val, iBuffer, stat;
    FILE * pFile;
    char *content, *buffer;

    pFile = fopen( pFileName, "r" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for reading.\n", pFileName );
        return NULL;
    }
    fseek( pFile, 0, SEEK_END );
    nSize = ftell( pFile );
    // fclose( pFile );
    if ( nSize == 0 )
    {
        printf( "The input file is empty.\n" );
        fclose( pFile );
        return NULL;
    }
    rewind(pFile);
    content = ABC_ALLOC( char, nSize );
    fread( content, nSize, 1, pFile );
    if (fClear) {
        Vec_IntClear( pMan->vNodeInit );
        Vec_WecClear( pMan->vClauses );
    }

    buffer = ABC_ALLOC( char, 15 );
    stat = 0;
    iBuffer = 0;
    vIntBuff = Vec_IntAlloc( 10 );
    for (i = 0; i < nSize; i++) {
        if (content[i] == ' ' || content[i] == '\n') {
            buffer[iBuffer] = '\0';
            if (iBuffer != 0) {
                Vec_IntPush( vIntBuff, atoi(buffer) );
                iBuffer = 0;
                buffer[0] = '\0';
            } 
            // Vec_IntPrint( p );
            if (content[i] == '\n') {
                switch (stat) {
                    case 1:
                        Vec_IntAppend( pMan->vNodeInit, vIntBuff );
                        break;
                    case 2:
                        Vec_IntAppend( Vec_WecPushLevel( pMan->vClauses ), vIntBuff );
                        break;
                }
                Vec_IntClear( vIntBuff );
                
            }
            
        } else if ((content[i] - '0' >= 0 && content[i] - '0' <= 9) || content[i] == '-') {
            buffer[iBuffer] = content[i];
            iBuffer++;
        } else {
            Vec_IntClear( vIntBuff );
            switch (content[i])
            {
            case 'f':
                stat = 1;
                break;
            case 'c':
                stat = 2;
                break;
            default:
                stat = 0;
                break;
            }
        }
    }

    ABC_FREE( content );
    ABC_FREE( buffer );
    fclose( pFile );

    val = Vec_IntFindMin( pMan->vNodeInit );
    Vec_IntRemoveAll( pMan->vNodeInit, 0, -1 );
    if (val < 0) {
        vWecBuff = Cec_ManDistAnalyze( pMan->pGia, 0, 0, pMan->vMerge, pMan->dist_long, pMan->dist_short );
        Vec_WecPrint( vWecBuff, 0 );
        Vec_WecForEachLevel( vWecBuff, vIntBuff, i) { 
            if (Vec_IntEntry( vIntBuff, 7 ) <= -val) // shortest path
                Vec_IntPush( pMan->vNodeInit, Vec_IntEntry( vIntBuff, 0 ) );
        }
        Vec_WecFree( vWecBuff );
    }
    Vec_IntUniqify( pMan->vNodeInit );
    // Vec_IntPrint( pMan->vNodeInit );

    Cec_ManFdPrepareSolver( pMan );
    
}
void Cec_ManFdDumpStat( Cec_ManFd_t* pMan, char* pFileName ) {
    FILE* pFile;
    Vec_Int_t* vIntBuff;
    int nid, i, j;
    pFile = fopen( pFileName, "w" );
    Vec_IntForEachEntry( pMan->vNodeInit, nid, i ) {
        if (i % 10 == 0) fprintf( pFile, "f ");
        fprintf(pFile, "%d ", nid);
        if (i % 10 == 9 || i == Vec_IntSize(pMan->vNodeInit) - 1) fprintf( pFile, "\n" );
    }
    Vec_WecForEachLevel( pMan->vClauses, vIntBuff, i ) {
        fprintf( pFile, "c " );
        Vec_IntForEachEntry( vIntBuff, nid, j ) {
            fprintf( pFile, "%d ", nid );
        }
        fprintf( pFile, "\n" );
    }
    fclose( pFile );

}
void Cec_ManFdAddClause( Cec_ManFd_t* pMan, sat_solver2* p ) {
    int i, Cid;
    Vec_Int_t* vIntBuff;
    sat_solver2* pSat = p ? p : pMan->pSat;
    int nClause = p ? 0 : pMan->nClauses;
    for (i = nClause; i < Vec_WecSize(pMan->vClauses); i++) {
        vIntBuff = Vec_IntDup(Vec_WecEntry(pMan->vClauses, i));
        // printf("add clause %d\n", i);
        // Vec_IntPrint(vIntBuff);
        Cec_ManFdMapLit( pMan, vIntBuff, 1 );
        // Vec_IntPrint(vIntBuff);
        if( Vec_IntFind( vIntBuff, -1 ) == -1 ) {
            Cid = sat_solver2_addclause( pSat, Vec_IntArray(vIntBuff), Vec_IntLimit(vIntBuff), -1 );
            if (Vec_IntFindMax( vIntBuff ) < 2 * (pMan->nVars + 1)) clause2_set_partA( pSat, Cid, 1 );
            else clause2_set_partA( pSat, Cid, 0 );
        }
            // sat_solver2_addclause( pSat, Vec_IntArray(vIntBuff), Vec_IntLimit(vIntBuff), -1 );
        Vec_IntFree( vIntBuff );
    }
    if (p == NULL) pMan->nClauses = Vec_WecSize(pMan->vClauses);
}
void Cec_ManFdPrepareSolver( Cec_ManFd_t* pMan ) {
    Gia_Man_t *pTemp;
    Gia_Obj_t *pObjbuf, *pObjbuf1, *pObjbuf2;
    Vec_Int_t *vIntBuff;
    int buf, i, j, nid;

    if (pMan->pSat != NULL) Cec_ManFdStop( pMan, 0 );
    // sat-solver
    pTemp = pMan->pPars->fAbsItp ? pMan->pAbs : pMan->pGia;
    pMan->vVarMap = Vec_IntStart( Gia_ManObjNum(pTemp) ); 
    pMan->pSat = Int2_ManSolver( pTemp, pMan->vVarMap );
    pMan->vNdMap = Vec_IntInvert( pMan->vVarMap, -1 );
    // if (Vec_WecSize(pMan->vClauses) > 0) {
    //     // remove those not be var in the node clause
    // }
    pMan->nClauses = 0;
    assert( pMan->pSat->size % 3 == 1 );
    pMan->nVars = (pMan->pSat->size - 1) / 3;
    // printf("nid to var\n");
    // printf("#GIA, #VAR = %d, %d\n", Gia_ManObjNum(pMan->pGia), pMan->nVars);
    // Vec_IntPrintMap( pMan->vVarMap );
    Cec_ManFdAddClause( pMan, 0 );

    // vGSupport
    pMan->vGSupport = Vec_WecStart( Gia_ManObjNum(pMan->pGia) );
    Cec_ManFdSetGSupportDefault( pMan );

    // vCost
    pMan->vCostTh = Vec_FltStart( Gia_ManObjNum(pMan->pGia) );
    Cec_ManFdSetCostTh( pMan );

    // stat
    pMan->vConf = Vec_IntStart( Gia_ManObjNum(pMan->pGia) );
    Vec_IntFill( pMan->vConf, Vec_IntSize(pMan->vConf), -1 );
    pMan->vStat = Vec_IntStart( Gia_ManObjNum(pMan->pGia) );
    Vec_IntFill( pMan->vStat, Vec_IntSize(pMan->vStat), 
        pMan->pPars->GType & 1 ? CEC_FD_TRIVIAL : CEC_FD_UNKNOWN ); // 0 for sat, -1 for unsolve, -2 for unknown 
    Gia_ManForEachObj( pMan->pGia, pObjbuf, i ) {
        if ( Gia_ObjIsAnd(pObjbuf) == 0 ) 
            Vec_IntWriteEntry(pMan->vStat, Gia_ObjId(pMan->pGia,pObjbuf), CEC_FD_IGNORE);
        else if ( Vec_IntSize( Vec_WecEntry(pMan->vGSupport, Gia_ObjId(pMan->pGia,pObjbuf)) ) == 0 ) 
            Vec_IntWriteEntry(pMan->vStat, Gia_ObjId(pMan->pGia,pObjbuf), CEC_FD_IGNORE);
        else if ( pMan->pPars->fInc && Cec_ManFdMapVarSingle(pMan, Gia_ObjId(pMan->pGia,pObjbuf), 1) == -1 )
            Vec_IntWriteEntry(pMan->vStat, Gia_ObjId(pMan->pGia,pObjbuf), CEC_FD_IGNORE);
        
    }

    pMan->vVeryStat = Vec_PtrStart( Gia_ManObjNum(pMan->pGia) );
    Vec_PtrFill( pMan->vVeryStat, Vec_PtrSize(pMan->vVeryStat), NULL );
    pMan->iter = 1;
}
void Cec_ManFdUpdate( Cec_ManFd_t* pMan, Gia_Man_t* p ) {
    Gia_Man_t *pTemp, *pTemp2;
    Gia_Obj_t *pObjbuf, *pObjbuf1, *pObjbuf2;
    Vec_Int_t *vIntBuff;
    int buf, i, j, nid, bnid, bias, flag, neg;
    int debug = 0;

    assert(p);

    if (pMan->pGia != NULL) {
        Gia_ManFillValue( pMan->pGia );
        
        if (pMan->pPars->fVerbose) printf("GiaMap\n");
        Gia_ManForEachObj( p, pObjbuf, i ) {
            if (Gia_ObjValue(pObjbuf) >= 0) {
                Gia_ObjSetValue( Gia_ObjCopy(pMan->pGia, pObjbuf), Abc_Var2Lit(Gia_ObjId(p, pObjbuf), 0) );
                if (pMan->pPars->fVerbose) printf("^%d->_%d\n", 
                    Gia_ObjId(pMan->pGia, Gia_ObjCopy(pMan->pGia, pObjbuf)), Gia_ObjId(p, pObjbuf) );
            }
        }
        
        
        Vec_IntForEachEntry( pMan->vNodeInit, nid, i ) {
            if (Gia_ObjValue(Gia_ManObj(pMan->pGia, nid)) >= 0) {
                Vec_IntWriteEntry( pMan->vNodeInit, i, Gia_ObjId( p, Gia_ObjCopy(p, Gia_ManObj(pMan->pGia, nid))) );
            } else {
                Vec_IntWriteEntry( pMan->vNodeInit, i, -1 );
            }
        }
        Vec_IntRemoveAll( pMan->vNodeInit, -1, 0 );

        // printf("start to update the clauses\n");
        // printf("Giasize: %d -> %d\n", Gia_ManObjNum(pMan->pGia), Gia_ManObjNum(p));
        // Vec_WecPrint( pMan->vClauses, 0 );
        Vec_WecForEachLevel( pMan->vClauses, vIntBuff, i ) {
            flag = 0;
            Vec_IntForEachEntry( vIntBuff, buf, j ) {
                neg = buf < 0 ? -1 : 1;
                bnid = abs(buf);
                nid = bnid % Gia_ManObjNum(pMan->pGia);
                bias = bnid / Gia_ManObjNum(pMan->pGia);
                if (Gia_ObjValue(Gia_ManObj(pMan->pGia, nid)) >= 0) {
                    nid = Gia_ObjId( p, Gia_ObjCopy(p, Gia_ManObj(pMan->pGia, nid)));
                    bnid = bias * Gia_ManObjNum(p) + nid;
                    Vec_IntWriteEntry( vIntBuff, j, neg * bnid );
                } else {
                    flag = 1;
                    break;
                }
            }
            if (flag) {
                Vec_IntClear( vIntBuff );
            }
        }
        Vec_WecPrint( pMan->vClauses, 0 );

        Vec_WecKeepLevels( pMan->vClauses, 0 );
        Cec_ManFdStop( pMan, 1 );
        // write the status back (vClause, vVarMap, vNodeInit)
    } else {
        pMan->vClauses = Vec_WecAlloc( 1000 );
        pMan->vNodeInit = NULL; // to dup vMerge
    }

    if (debug) printf("1\n");
    if (p->vFanout == NULL) Gia_ManStaticFanoutStart( p );
    // pObj1, pObj2
    pMan->pGia = p;
    if (Gia_ManCoNum(pMan->pGia) == 4) {
        pMan->pObj1 = Gia_ObjFanin0( Gia_ManCo( pMan->pGia, 0 ) );
        pMan->pObj2 = Gia_ObjFanin0( Gia_ManCo( pMan->pGia, 2 ) );
        pMan->fPhase = Gia_ObjFaninC0( Gia_ManCo( pMan->pGia, 0 ) ) ^ Gia_ObjFaninC0( Gia_ManCo( pMan->pGia, 2 ) );
        assert(pMan->pObj1 != pMan->pObj2);
    } else if (Gia_ManCoNum(pMan->pGia) == 2) {
        pMan->pObj1 = Gia_ObjFanin0( Gia_ManCo( pMan->pGia, 0 ) );
        pMan->pObj2 = Gia_ObjFanin0( Gia_ManCo( pMan->pGia, 1 ) );
        pMan->fPhase = Gia_ObjFaninC0( Gia_ManCo( pMan->pGia, 0 ) ) ^ Gia_ObjFaninC0( Gia_ManCo( pMan->pGia, 1 ) );
    } else if (Gia_ManCoNum(pMan->pGia) == 1) {
        pMan->pObj1 = Gia_ObjFanin0( Gia_ManCo( pMan->pGia, 0 ) );
        pMan->pObj2 = Gia_ObjFanin0( Gia_ManCo( pMan->pGia, 0 ) );
        // two must be same node
    } else {
        assert(0);
    }
    if (pMan->pObj1 == pMan->pObj2) { // assume it is a XOR or XNOR
        assert(Gia_ObjRecognizeExor( pMan->pObj1, 0, 0 ));
        // printf("%d, %d\n", Gia_ObjFaninC0( Gia_ManCo( pMan->pGia, 0 )) , Gia_ObjRecognizeExor( pMan->pObj1, 0, 0 ));
        // does PO inv and does PO connect to XNOR
        pMan->fPhase = Gia_ObjFaninC0( Gia_ManCo( pMan->pGia, 0 )) ^ (Gia_ObjFaninC0( Gia_ObjFanin0( pMan->pObj1 ) ) != Gia_ObjFaninC1( Gia_ObjFanin0( pMan->pObj1 ) ));
        pMan->pObj1 = Gia_ObjFanin0( Gia_ObjFanin0( pMan->pObj1 ) );
        pMan->pObj2 = Gia_ObjFanin1( Gia_ObjFanin0( pMan->pObj2 ) );
        
    }

    // dist
    pMan->dist_long = Cec_ManDist( pMan->pGia, 1 );
    pMan->dist_short = Cec_ManDist( pMan->pGia, 0 );
    if (debug) printf("2\n");
    pMan->vMerge = Vec_IntAlloc(1);
    Cec_ManGetMerge_old( pMan->pGia, pMan->pObj1, pMan->pObj2, pMan->vMerge );
    if (pMan->vNodeInit == NULL) { // vNodeInit contain all nodes not shared, might contain frt if GType & 1
        if (pMan->pPars->GType & 1) pMan->vNodeInit = Vec_IntDup( pMan->vMerge );
        else pMan->vNodeInit = Vec_IntAlloc(1);
        Gia_ManForEachAnd( pMan->pGia, pObjbuf, i ) {
            // if ( Gia_ObjIsAnd(pObjbuf) == 0 ) continue;
            if ( Gia_ObjColors(pMan->pGia, Gia_ObjId(pMan->pGia, pObjbuf)) == 1 
                 || Gia_ObjColors(pMan->pGia, Gia_ObjId(pMan->pGia, pObjbuf)) == 2 ) {
                    Vec_IntPush( pMan->vNodeInit, Gia_ObjId(pMan->pGia,pObjbuf) );
            }
        }
        // Vec_IntPrint(pMan->vNodeInit);
    }
    if (debug) printf("4\n");

    // pAbs
    pMan->pAbs = Cec_ManGetAbs( pMan->pGia, pMan->pObj1, pMan->pObj2, pMan->vMerge, 0 );
    pMan->vAbsMap = Vec_IntAlloc(1);
    Cec_ManGiaMapId( pMan->pGia, pMan->vAbsMap );
    pMan->vGiaMap = Vec_IntInvert( pMan->vAbsMap, -1 );
    Vec_IntFillExtra( pMan->vGiaMap, Gia_ManObjNum(pMan->pAbs), -1 );
    pMan->pObjAbs1 = Gia_ObjCopy( pMan->pAbs, pMan->pObj1 );
    pMan->pObjAbs2 = Gia_ObjCopy( pMan->pAbs, pMan->pObj2 );
    pMan->dist_long_abs = Cec_ManDist( pMan->pAbs, 1 );
    pMan->dist_short_abs = Cec_ManDist( pMan->pAbs, 0 );
    if (debug) printf("5\n");
    // vMergeSupport
    pMan->vMergeSupport = Vec_WecStart( Gia_ManObjNum(pMan->pGia) );
        Gia_ManForEachObj( pMan->pGia, pObjbuf, i ) {
        if ( Gia_ObjIsAnd(pObjbuf) == 0 ) continue;
        if ( Gia_ObjColors(pMan->pGia, Gia_ObjId(pMan->pGia,pObjbuf)) == 3 ) continue;
        Gia_ManFillValue( pMan->pGia );
        Cec_ManSubcircuit( pMan->pGia, pObjbuf, pMan->vMerge, 0, 1, 0 );
        Vec_IntForEachEntry( pMan->vMerge, nid, j ) {
            Vec_WecPush( pMan->vMergeSupport, Gia_ObjId(pMan->pGia, pObjbuf), (Gia_ManObj(pMan->pGia, nid)->Value != -1) );
        } // change to fMark1
    }
    if (debug) printf("6\n");

    // vLevel1, vLevel2
    pMan->vLevel1 = Vec_IntStart( Gia_ManObjNum(pMan->pGia) );
    pMan->vLevel2 = Vec_IntStart( Gia_ManObjNum(pMan->pGia) );
    Cec_ManFdSetLevel( pMan );
    if (debug) printf("7\n");

    // vCuts
    pMan->vCuts = Gia_ManExploreCuts( pMan->pAbs, 8, -1, 1 );


    Cec_ManFdPrepareSolver( pMan );
}
Cec_ManFd_t* Cec_ManFdStart( Gia_Man_t* p, Cec_ParFd_t* pPars, char* pFileName ) {
    Vec_Int_t *vIntBuff;
    Gia_Obj_t *pObjbuf, *pObjbuf1, *pObjbuf2;
    int i, j, buf, nid;
    Cec_ManFd_t* pMan = ABC_ALLOC( Cec_ManFd_t, 1 );
    pMan->pGia = NULL;
    pMan->pSat = NULL;
    

    Cec_ManFdSetPars( pMan, pPars );
    Cec_ManFdUpdate( pMan, p );
    if (pFileName) {
        Cec_ManFdReadStat( pMan, pFileName, 1 );
    }

    if (pMan->pPars->fGetCkts) Gia_SelfDefShow( pMan->pGia, "pGia.dot", pMan->vMerge, pMan->vNodeInit, 0);
    vIntBuff = Vec_IntDup( pMan->vNodeInit );
    Cec_ManFdMapId( pMan, vIntBuff, 1 );
    Vec_IntRemoveAll( vIntBuff, -1, 0 );
    if (pMan->pPars->fGetCkts) Gia_SelfDefShow( pMan->pAbs, "pAbs.dot", 0, vIntBuff, 0);
    Vec_IntFree( vIntBuff );
    return pMan;
}
void  Cec_ManFdStop( Cec_ManFd_t* pMan, int fRange ) {
    Gia_Man_t* pTemp;
    int i;

    if (fRange >= 0) {
        sat_solver2_delete( pMan->pSat );
        pMan->pSat = NULL;
        Vec_IntFree( pMan->vNdMap );
        Vec_IntFree( pMan->vVarMap );
        // pMan->nClauses = 0;
        
        Vec_WecFree( pMan->vGSupport );
        Vec_FltFree( pMan->vCostTh );

        Vec_IntFree( pMan->vConf );
        Vec_IntFree( pMan->vStat );
        Vec_PtrForEachEntry( Gia_Man_t *, pMan->vVeryStat, pTemp, i )
            if (pTemp != NULL && pTemp != 1 && pTemp != 2) Gia_ManStop( pTemp );
        Vec_PtrFree( pMan->vVeryStat );
        // pMan->iter = 1;   
    }

    if (fRange >= 1) {
        for (i = 0; i < Gia_ManObjNum(pMan->pGia); i++) {
            ABC_FREE( pMan->dist_long[i] );
            ABC_FREE( pMan->dist_short[i] );
        }
        ABC_FREE( pMan->dist_long );
        ABC_FREE( pMan->dist_short );

        Vec_IntFree( pMan->vMerge );
        Vec_IntFree( pMan->vAbsMap );
        Vec_IntFree( pMan->vGiaMap );
        for (i = 0; i < Gia_ManObjNum(pMan->pAbs); i++) {
            ABC_FREE( pMan->dist_long_abs[i] );
            ABC_FREE( pMan->dist_short_abs[i] );
        }
        ABC_FREE( pMan->dist_long_abs );
        ABC_FREE( pMan->dist_short_abs );
        Vec_WecFree( pMan->vMergeSupport );
        Vec_IntFree( pMan->vLevel1 );
        Vec_IntFree( pMan->vLevel2 );
        Vec_WecFree( pMan->vCuts );

        // Gia_ManStop( pMan->pGia );
        Gia_ManStop( pMan->pAbs );
        pMan->pGia = NULL;
    }

    if (fRange >= 2) {
        Vec_IntFree( pMan->vNodeInit );
        Vec_WecFree( pMan->vClauses );
        ABC_FREE( pMan );
    }
    
}


Gia_Man_t* Cec_ManFdGetFd_Inc( Cec_ManFd_t* pMan, int nidGlobal, Vec_Int_t* vFdSupportGlobal ) {
    Gia_Man_t* pFdMiter, *pFdPatch, *pTemp;
    sat_solver2* pSat;
    abctime clk;
    Vec_Int_t *vFdSupport = vFdSupportGlobal ? Vec_IntDup(vFdSupportGlobal) : Vec_IntDup(Vec_WecEntry(pMan->vGSupport, nidGlobal));
    Vec_Int_t *vIntBuff;
    int i;
    Vec_Wec_t *vLearnt = Vec_WecAlloc(100);
    
    pTemp = pMan->pPars->fAbsItp ? pMan->pAbs : pMan->pGia;
    pSat = Int2_ManSolver( pTemp, 0 );
    Cec_ManFdAddClause( pMan, pSat );
    
    Cec_ManFdMapVar( pMan, vFdSupport, 1 );
    Vec_IntRemoveAll( vFdSupport, -1, 0 );
    int varObj = Cec_ManFdMapVarSingle( pMan, nidGlobal, 1 );
    assert(varObj != -1);
    clk = Abc_Clock();
    // char* patchName = ABC_ALLOC( char, 100 );
    // sprintf(patchName, "patch_%d.dimac", nidGlobal);
    // pMan->pSat->verbosity = 1;
    // Sat_Solver2WriteDimacs( pMan->pSat, patchName, 0, 0, 0 );
    printf("GiaSize:%d, VarNum:%d\n", Gia_ManObjNum(pMan->pGia), pMan->nVars);
    printf("learnt clause size (before): %d\n", Vec_WecSize(pMan->vClauses));
    // Vec_WecPrint( pMan->vClauses, 0 );
    pFdPatch = Int2_ManFd( pSat, vFdSupport, varObj, pMan->pPars->nBTLimit, 0 );
    Sat_Solver2Learnts( pSat, vLearnt );
    // Vec_WecPrint( vLearnt, 0 );
    Vec_WecForEachLevel( vLearnt, vIntBuff, i ) {
        // Vec_IntPrint(vIntBuff); 
        Cec_ManFdMapLit( pMan, vIntBuff, 0 );
        // Vec_IntPrint(vIntBuff);
        Vec_IntAppend( Vec_WecPushLevel( pMan->vClauses ), vIntBuff );
    }
    Vec_WecFree( vLearnt );
    printf("learnt clause size (after): %d\n", Vec_WecSize(pMan->vClauses));
    // Vec_WecPrint( pMan->vClauses, 0 );
    
    if (pMan->pPars->fVerbose) {
        if (pFdPatch != NULL && pFdPatch != 1 && pFdPatch != 2) 
            Abc_Print(1, "Find a patch for node %d with size %d with G sized %d require %.6f\n", 
                nidGlobal, Gia_ManAndNum(pFdPatch), Vec_IntSize(vFdSupport), 1.0*((double)(Abc_Clock() - clk))/((double)((__clock_t) 1000000)));
    }

    Vec_IntFree( vFdSupport );
    return pFdPatch;
}
Gia_Man_t* Cec_ManFdGetFd( Cec_ManFd_t* pMan, int nidGlobal, Vec_Int_t* vFdSupportGlobal, Vec_Int_t* stat ) {
    Gia_Man_t *pGia = pMan->pPars->fAbsItp ? pMan->pAbs : pMan->pGia;
    Gia_Man_t *pFdPatch; // * pFdMiter, , *pTemp;
    // Gia_Obj_t* pObjbuf;
    Gia_Obj_t* pObj = pMan->pPars->fAbsItp ? Gia_ManObj(pGia, Cec_ManFdMapIdSingle(pMan, nidGlobal, 1)) : Gia_ManObj(pGia, nidGlobal);
    // Vec_Ptr_t *vNdCone;
    Vec_Int_t *vFdSupport = vFdSupportGlobal ? Vec_IntDup(vFdSupportGlobal) : Vec_IntDup(Vec_WecEntry(pMan->vGSupport, nidGlobal));
    Vec_Int_t *vIntBuff = Vec_IntAlloc( 1 );
    if (pMan->pPars->fAbsItp) Cec_ManFdMapId(pMan, vFdSupport, 1);
    int i, j, k, nid, nidbuf, nidbuf1, nidbuf2, buf, buf1, buf2, status;
    abctime clk;

    clk = Abc_Clock();
    pFdPatch = Cec_ManItp( pGia, Gia_ObjId(pGia, pObj), vFdSupport, pMan->pPars->nBTLimit, -1, vIntBuff );
    if (stat) Vec_IntAppend( stat, vIntBuff );
    // Vec_IntWriteEntry( pMan->vConf, nidGlobal, Vec_IntEntry(vIntBuff, 0) );
    if (pMan->pPars->fVerbose) {
        if (pFdPatch != NULL && pFdPatch != 1) 
            Abc_Print(1, "Find a patch for node %d with size %d and G sized %d require %.6f\n", 
                nidGlobal, Gia_ManAndNum(pFdPatch), Vec_IntSize(vFdSupport), 1.0*((double)(Abc_Clock() - clk))/((double)((__clock_t) 1000000)));
        // if (Vec_IntSize(vFdSupport) == 1) {
        //     Gia_ManFillValue(pFdMiter);
        //     Gia_ManForEachCo( pFdMiter, pObjbuf, i ) {
        //         if (i == Gia_ManCoNum(pFdMiter)-1) break;
        //         Cec_ManSubcircuit( pFdMiter, pObjbuf, 0, 0, 1, 0 );
        //     }
        //     Gia_ManFillValue(pFdMiter);
        //     Cec_ManSubcircuit( pFdMiter, Gia_ManCo(pFdMiter, Gia_ManCoNum(pFdMiter)-1), 0, 0, 1, 0 );

        // }
    }

    Vec_IntFree( vFdSupport );
    return pFdPatch;
}

float Cec_ManFdRawCost( Cec_ManFd_t* pMan, int nid, Vec_Int_t* vFdSupport, Gia_Man_t* patch ) {
    int fAbs = pMan->pPars->fAbs;
    int fDistAbs;
    int cnt, val, dist_out_sup, dist_out, dist_in, dist_in_org, i, nidPO;
    int** dist_lib, **dist_lib_org;
    int ftype = pMan->pPars->costType;
    float coef = pMan->pPars->coefPatch;
    float dist, dist_buf;
    Vec_Int_t* vFdSupportEval = vFdSupport ? vFdSupport : Vec_WecEntry(pMan->vGSupport, nid );
    Gia_Man_t* patchEval = patch ? patch : (Gia_Man_t*) Vec_PtrEntry( pMan->vVeryStat, nid );
    Gia_Obj_t* pObjbuf;
    Vec_Wec_t* vWecBuff;

    if (patchEval == NULL || patchEval == 1 || patchEval == 2) return -1.0;
    switch ( ftype ) {
        case CEC_FD_COSTSUPMAX:
        case CEC_FD_COSTSUPRMS:
            fDistAbs = 1;
            break;
        case CEC_FD_COSTTWOPATHOLD:
        case CEC_FD_COSTTWOPATHRMS:
            fDistAbs = 0;
            break;
        default:
            fDistAbs = fAbs ? 1 : 0;
            break;
    }
    if (Cec_ManFdGetLevelType(pMan, 1)) {
        dist_lib = Cec_ManDist( patchEval, 0 );
        dist_lib_org = fDistAbs ? pMan->dist_short_abs : pMan->dist_short;
    } else {
        dist_lib = Cec_ManDist( patchEval, 1 );
        dist_lib_org = fDistAbs ? pMan->dist_long_abs : pMan->dist_long;
    }
    switch ( ftype ) {
        case CEC_FD_COSTDUMMY:
            dist = 0.0;
            break;
        case CEC_FD_COSTANDNUM:
        case CEC_FD_COSTFIXANDNUM:
        case CEC_FD_COSTHIERANDNUM:
        case CEC_FD_COSTMFFC:
            dist = 1.0 * Gia_ManAndNum(patchEval);
            break;
        
        case CEC_FD_COSTTWOPATHOLD:
            dist = -1.0;
            
            if (pMan->pPars->fVerbose) printf("cost of %d: \n", nid);
            if (Gia_ManCoDriverId(patchEval,0) == 0) { // constant 0
                dist = dist_lib_org[Cec_ManFdGetPO(pMan, nid, 0, 1)][nid];
                dist *= coef;
            }
            else {
                Vec_IntForEachEntry( vFdSupportEval, val, i ) {
                    
                    dist_in = dist_lib[Gia_ManCoDriverId(patchEval,0)][Gia_ManCiIdToId(patchEval, i)];
                    if (dist_in == -1) continue; // 1. if no And between Co and Ci, 2. if the PI doesn't connect to PO
                    dist_out_sup = dist_lib_org[Cec_ManFdGetPO(pMan, nid, 0, 0)][val];
                    dist_out = dist_lib_org[Cec_ManFdGetPO(pMan, nid, 0, 1)][nid];
                    dist_buf = (dist_out_sup == -1) ? 
                                1.0 * (dist_out + dist_in) : 
                                coef * (dist_in + dist_out) + (1.0 - coef) * dist_out_sup;
                    // printf("  %d: out %d, in %d, sup %d, buf %f\n", val, dist_out, dist_in, dist_out_sup, dist_buf);
                    if (pMan->pPars->fVerbose && dist < dist_buf) {
                        printf("%d: out %d, in %d, sup %d, buf %f\n", val, dist_out, dist_in, dist_out_sup, dist_buf);
                    }
                    if (dist < 0) dist = dist_buf;
                    else {
                        if (Cec_ManFdGetLevelType(pMan, 3)) { // min
                            dist = Abc_MinFloat( dist, dist_buf ); // only consider self cone
                        } else { // max
                            dist = Abc_MaxFloat( dist, dist_buf ); // only consider self cone
                        }
                    }
                
                }
            }
            break;
        case CEC_FD_COSTTWOPATHRMS:
        case CEC_FD_COSTTWOPATHMAX:
        case CEC_FD_COSTTWOPATHMIN:
            dist = 0.0;
            cnt = 0;
            if (Gia_ManCoDriverId(patchEval,0) == 0) { // constant 0
                dist = dist_lib_org[Cec_ManFdGetPO(pMan, nid, 0, 1)][nid];
            } else {
                Vec_IntForEachEntry( vFdSupportEval, val, i ) {
                    dist_in = dist_lib[Gia_ManCoDriverId(patchEval,0)][Gia_ManCiIdToId(patchEval, i)];
                    if (dist_in == -1) continue; // 1. if no And between Co and Ci, 2. if the PI doesn't connect to PO
                    dist_out_sup = Gia_ObjColor(pMan->pGia, nid, 0) ? 
                                    Vec_IntEntry(pMan->vLevel2, val) : Vec_IntEntry(pMan->vLevel1, val); // dist_lib_org[Cec_ManFdGetPO(pMan, nid, 0, 0)][val];
                    dist_out = Gia_ObjColor(pMan->pGia, nid, 0) ? 
                                    Vec_IntEntry(pMan->vLevel1, nid) : Vec_IntEntry(pMan->vLevel2, nid); // dist_lib_org[Cec_ManFdGetPO(pMan, nid, 0, 1)][nid];
                    // printf("  %d: out %d, in %d, sup %d\n", val, dist_out, dist_in, dist_out_sup);
                    assert( dist_out_sup >= 0 );
                    dist_buf = 1.0 * (dist_out + dist_in) * (dist_out + dist_in) + 1.0 * (dist_out_sup * dist_out_sup);
                    if (ftype == CEC_FD_COSTTWOPATHMAX) {
                        if (dist == 0.0) dist = dist_buf;
                        else dist = Abc_MaxFloat(dist_buf, dist);
                        cnt = 1; // only max is considered
                    } else if (ftype == CEC_FD_COSTTWOPATHMIN) {
                        if (dist == 0.0) dist = dist_buf;
                        else dist = Abc_MinFloat(dist_buf, dist);
                        cnt = 1; // only min is considered
                    } else if (ftype == CEC_FD_COSTTWOPATHRMS) {
                        dist += dist_buf;
                        cnt += 1;
                    }
                }
                // printf("dist: %f, cnt: %d\n", dist, cnt);
                dist = sqrt(dist / cnt);
            }
            break;
        case CEC_FD_COSTHEIGHT:
            dist = 1.0 * Gia_ManLevelNum(patchEval);
            break;
        case CEC_FD_COSTCINUM:
            Gia_ManFillValue( patchEval );
            Gia_ManForEachAnd( patchEval, pObjbuf, i )
                Gia_ObjFanin0(pObjbuf)->Value = 0;
                Gia_ObjFanin1(pObjbuf)->Value = 0;
            val = 0;
            Gia_ManForEachCi( patchEval, pObjbuf, i )
                val += (pObjbuf->Value == 0);
            dist = 1.0 * val;
            break;
        case CEC_FD_COSTSUPRMS:
        case CEC_FD_COSTSUPMAX:
        case CEC_FD_COSTSUPMIN:
            dist = 0.0;
            cnt = 0;
            if (Gia_ManCoDriverId(patchEval,0) == 0) { // constant 0
                dist = dist_lib_org[Cec_ManFdGetPO(pMan, nid, 1, 1)][Cec_ManFdMapIdSingle(pMan, nid, 1)];
            }
            else {
                Vec_IntForEachEntry( vFdSupportEval, val, i ) {
                    dist_in = dist_lib[Gia_ManCoDriverId(patchEval,0)][Gia_ManCiIdToId(patchEval, i)];
                    if (dist_in == -1) continue; // 1. if no And between Co and Ci, 2. if the PI doesn't connect to PO
                    dist_out_sup = dist_lib_org[Cec_ManFdGetPO(pMan, nid, 1, 1)][Cec_ManFdMapIdSingle(pMan, val, 1)];
                    dist_out = dist_lib_org[Cec_ManFdGetPO(pMan, nid, 1, 1)][Cec_ManFdMapIdSingle(pMan, nid, 1)];
                    dist_buf = dist_out_sup < 0 ? 
                                1.0 * (dist_out + dist_in) : 
                                Cec_ManFdGetLevelType(pMan, 3) ? 
                                    Abc_MinFloat(1.0 * (dist_out + dist_in), 1.0 * dist_out_sup) : 
                                    Abc_MaxFloat(1.0 * (dist_out + dist_in), 1.0 * dist_out_sup);
                    dist_buf = 1.0 * dist_buf * dist_buf;
                    if (ftype == CEC_FD_COSTSUPMAX) {
                        if (dist == 0.0) dist = dist_buf;
                        else dist = Abc_MaxFloat(dist_buf, dist);
                        cnt = 1; // only max is considered
                    } else if (ftype == CEC_FD_COSTSUPMIN) {
                        if (dist == 0.0) dist = dist_buf;
                        else dist = Abc_MinFloat(dist_buf, dist);
                        cnt = 1; // only min is considered
                    }else if (ftype == CEC_FD_COSTSUPRMS) {
                        dist += dist_buf;
                        cnt += 1;
                    }
                }
                if (cnt > 0) dist = sqrt(dist / cnt);
            }
            break;
        default:
            assert(0);
            break;
    }

    for (i = 0; i < Gia_ManObjNum(patchEval); i++) 
        ABC_FREE( dist_lib[i] );
    ABC_FREE( dist_lib );
    return dist;
}

void Cec_ManFdUpdateStatForce( Cec_ManFd_t* pMan, int nid, Vec_Int_t* vFdSupport, Gia_Man_t* patch, float cost ) {
  int nidBuff, i;
  int costType = pMan->pPars->costType;
  int preStat = Vec_IntEntry(pMan->vStat, nid);
  float coef = pMan->pPars->coefPatch;
  // float preCost = Cec_ManFdRawCost( pMan, nid, 0, 0 );
  assert( Vec_IntEntry(pMan->vStat, nid) != CEC_FD_LOCK && Vec_IntEntry(pMan->vStat, nid) != CEC_FD_IGNORE );
  Gia_Man_t* patchOld = (Gia_Man_t*) Vec_PtrEntry( pMan->vVeryStat, nid );
  if (patch == NULL) Vec_IntWriteEntry(pMan->vStat, nid, CEC_FD_SAT);
  else if (patch == 1) Vec_IntWriteEntry(pMan->vStat, nid, CEC_FD_UNSOLVE);
  else if ((Vec_FltEntry(pMan->vCostTh, nid) >= 0) && (cost > Vec_FltEntry(pMan->vCostTh, nid))) Vec_IntWriteEntry(pMan->vStat, nid, CEC_FD_HUGE);
  /// else if (coef != 0.0 && Gia_ManAndNum(patch) > coef * Gia_ManAndNum(pMan->pAbs)) Vec_IntWriteEntry(pMan->vStat, nid, CEC_FD_HUGE);
  else Vec_IntWriteEntry(pMan->vStat, nid, pMan->iter);

  // otherwise, vStat is not changed
  if (vFdSupport) {
    Vec_IntClear( Vec_WecEntry(pMan->vGSupport, nid) );
    Vec_IntForEachEntry( vFdSupport, nidBuff, i ) {
      Vec_WecPush( pMan->vGSupport, nid, nidBuff );
    }
  }
  
  if (patchOld != NULL && patchOld != 1 && patchOld != 2) Gia_ManStop( patchOld );
  Vec_PtrWriteEntry( pMan->vVeryStat, nid, patch );
  if (pMan->pPars->fVerbose) {
    printf("stat of node %d is updated: %d --> %d\n", nid, preStat, Vec_IntEntry(pMan->vStat, nid) );
    printf(" cost to %f\n", cost );
    printf(" threshold = %f\n", Vec_FltEntry(pMan->vCostTh, nid) );
  }
}
int Cec_ManFdUpdateStat( Cec_ManFd_t* pMan, int nid, Vec_Int_t* vFdSupport, Gia_Man_t* patch, int fStrict ) {
    int entry, i;
    float coef = pMan->pPars->coefPatch;
    int stat = Vec_IntEntry(pMan->vStat, nid);
    int statSup;
    float precost = Cec_ManFdRawCost( pMan, nid, 0, 0 );
    float cost = -1;
    float costTh = Vec_FltEntry(pMan->vCostTh, nid);
    Gia_Man_t* patchfin;
    if (stat == CEC_FD_LOCK || stat == CEC_FD_IGNORE) return 0;
    else if (stat > 0 && stat < pMan->iter) return 0; // avoid update those that already replaced
    else if (patch == NULL) { // SAT
      if (stat == CEC_FD_UNKNOWN || stat == CEC_FD_UNSOLVE || stat == CEC_FD_SAT) {
        Cec_ManFdUpdateStatForce( pMan, nid, vFdSupport, patch, cost );
        return 1;
      }
    }
    else if (patch == 1) { // UNSOLVE
      if (stat == CEC_FD_UNKNOWN || stat == CEC_FD_UNSOLVE) {
        Cec_ManFdUpdateStatForce( pMan, nid, vFdSupport, patch, cost );
        return 1;
      }
    }
    else {
      if (pMan->pPars->fTrim) patchfin = Cec_ManSimpSyn( patch, 0, pMan->pPars->fVerbose, -1 );
      else patchfin = Gia_ManDup( patch );
      cost = Cec_ManFdRawCost( pMan, nid, vFdSupport, patchfin );
      assert(cost >= 0);
      if (stat <= 0 && stat != CEC_FD_HUGE) { // no matter cost, a solve is better than no solve
        Cec_ManFdUpdateStatForce( pMan, nid, vFdSupport, patchfin, cost );
        return 1;
      }
      if (stat > 0 && (costTh >= 0) && (cost > costTh)) {
        Gia_ManStop(patchfin);
        return 0; // do not update UNSAT to huge
      }
      if (fStrict == 0 || cost <= precost || precost == -1) { // should only contain UNSAT --> UNSAT and HUGE --> HUGE/UNSAT
        Cec_ManFdUpdateStatForce( pMan, nid, vFdSupport, patchfin, cost );
        return 1;
      }
    }
    return 0;
}
void Cec_ManFdCleanOneUnknown( Cec_ManFd_t* pMan, int nid ) {
  Gia_Man_t *pTemp;
  Vec_Int_t *vIntBuff = Vec_IntAlloc( 1 );
  abctime clk;
  int fStrict = pMan->pPars->costType == CEC_FD_COSTCINUM ? 0 : 1;
  clk = Abc_Clock();
  if (Vec_IntEntry(pMan->vStat, nid) != CEC_FD_UNKNOWN 
    && Vec_IntEntry(pMan->vStat, nid) != CEC_FD_HUGE
    && Vec_IntEntry(pMan->vStat, nid) != CEC_FD_UNSOLVE) return;
  if (pMan->pPars->fVerbose) printf("clean unknown nid: %d\n", nid);
  pTemp = pMan->pPars->fInc ? Cec_ManFdGetFd_Inc( pMan, nid, 0 ) : Cec_ManFdGetFd( pMan, nid, 0, vIntBuff );
  if (Cec_ManFdUpdateStat( pMan, nid, 0, pTemp, fStrict )) {
    Vec_IntWriteEntry( pMan->vConf, nid, Vec_IntEntry(vIntBuff, 0));
  }
          
  if (pMan->pPars->fVerbose) {
      // printf("nid:%d______%s\n", nid, pTemp == NULL ? "SAT" : pTemp == 1 ? "UNSOLVE" : "UNSAT");
      // Abc_PrintTime(-1, "Time: ", Abc_Clock() - clk);
      Abc_Print(1, "Time: %9.6f sec\n", 1.0*((double)(Abc_Clock() - clk))/((double)((__clock_t) 1000000)));
      if (pTemp != NULL && pTemp != 1 && pTemp != 2) printf("patch size = %d\n", Gia_ManAndNum(pTemp)); //Gia_ManPrintStats(pTemp, 0);
  }
  if (pTemp != NULL && pTemp != 1 && pTemp != 2) Gia_ManStop( pTemp );
  Vec_IntFree( vIntBuff );
}
void Cec_ManFdCleanUnknown( Cec_ManFd_t* pMan, int fGetColor ) {
    int i, nid, stat;
    Vec_Int_t* vCare = Cec_ManGetColorNd( pMan->pGia, fGetColor );
    // Vec_IntPrint(vCare);
    Vec_IntForEachEntry( vCare, nid, i ) {
        
        Cec_ManFdCleanOneUnknown( pMan, nid );
    }
}
int Cec_ManFdTraverseUnknownCone( Cec_ManFd_t* pMan, int nid, int nHeight ) {
    Gia_Obj_t* pObj = Gia_ManObj( pMan->pGia, nid );
    int nid1, nid2, buf;
    if (nHeight == 0) return -1;
    if (Gia_ObjIsCi(pObj)) return -1;
    if (Gia_ObjIsConst0(pObj)) return -1;
    nid1 = Gia_ObjFaninId0(pObj, nid);
    if (Cec_ManFdToSolve( pMan, nid1 ))
        Cec_ManFdShrinkSimple( pMan, nid1 );
    if (Vec_IntEntry( pMan->vStat, nid1 ) > 0) return nid1;
    buf = Cec_ManFdTraverseUnknownCone( pMan, nid1, nHeight - 1 );
    if (buf != -1) return buf;
    nid2 = Gia_ObjFaninId1(pObj, nid);
    if (Cec_ManFdToSolve( pMan, nid2 ))
        Cec_ManFdShrinkSimple( pMan, nid2 );
    if (Vec_IntEntry( pMan->vStat, nid2 ) > 0) return nid2;
    buf = Cec_ManFdTraverseUnknownCone( pMan, nid2, nHeight - 1 );
    if (buf != -1) return buf;
    else return -1;

}
void Cec_ManFdTraverseUnknownNeighbor( Cec_ManFd_t* pMan, int nid, int nHeight ) {
    int nidAbs = Cec_ManFdMapIdSingle( pMan, nid, 1 );
    int i, nidAbsBuf, nidGiaBuf, stat;
    Vec_Int_t* vIntBuff;
    Vec_Int_t* vNeighbor = Cec_ManGetSibling( pMan->pAbs, vIntBuff = Vec_IntItoV( nidAbs ), nHeight, 0 );
    Vec_IntFree( vIntBuff );
    vIntBuff = Vec_IntAlloc( 100 );
    Vec_IntForEachEntry( vNeighbor, nidAbsBuf, i ) {
        nidGiaBuf = Cec_ManFdMapIdSingle( pMan, nidAbsBuf, 0 );
        if (Cec_ManFdToSolve( pMan, nidGiaBuf )) 
            Cec_ManFdShrinkSimple( pMan, nidGiaBuf );
        if (Vec_IntEntry( pMan->vStat, nidGiaBuf ) > 0) Vec_IntPush( vIntBuff, nidGiaBuf );
        else {
            nidGiaBuf = Cec_ManFdTraverseUnknownCone( pMan, nidGiaBuf, nHeight - 1 );
            if (nidGiaBuf != -1) Vec_IntPush( vIntBuff, nidGiaBuf );
        }
    }
    Vec_IntForEachEntry( vIntBuff, nidGiaBuf, i ) {
        Cec_ManFdTraverseUnknownNeighbor( pMan, nidGiaBuf, nHeight );
    }
    Vec_IntFree( vNeighbor );
    Vec_IntFree( vIntBuff );
}
int Cec_ManFdTraverseUnknownUpward( Cec_ManFd_t* pMan, int nid, int nHeight ) {
    // Vec_Int_t* vTraverse = Vec_IntAlloc( 8 );
    // Vec_Wec_t* vCuts = Vec_WecAlloc( 100 );
    int step = 3;
    int nidAbs = Cec_ManFdMapIdSingle( pMan, nid, 1 );
    int nidGiaBuf, nidAbsBuf;
    int i, highest;
    Vec_Int_t* vTFO;
    // if (Vec_IntEntry(pMan->vStat, nid) != CEC_FD_UNKNOWN && Vec_IntEntry(pMan->vStat, nid) != CEC_FD_TRIVIAL) return;
    vTFO = Cec_ManGetTFO( pMan->pAbs, nidAbs, nHeight, 0 );
    Vec_IntReverseOrder( vTFO );
    highest = nid;
    Vec_IntForEachEntry( vTFO, nidAbsBuf, i ) {
        nidGiaBuf = Cec_ManFdMapIdSingle( pMan, nidAbsBuf, 0 );
        Cec_ManFdShrinkSimple( pMan, nidGiaBuf );
        if (Vec_IntEntry(pMan->vStat, nidGiaBuf) > 0) {
            highest = Cec_ManFdTraverseUnknownUpward( pMan, nidGiaBuf, step );
            break;
        }
    }

    return highest;
}
void Cec_ManFdTraverseUnknown( Cec_ManFd_t* pMan, int fColor ) {
    // find a start point
    // Cec_ManFOcut
    // If no cut is all UNSAT, pick the node as the Fanin of UNSOLVE (check by vTraverse) (maybe single cut to try?)
    // mark the UNSATs with fMark0
    // find the MFFCLeaks
    // pick one node in MFFCLeaks that make max MFFCmul

    Gia_Obj_t *pObj;
    int i, j, nidAbs, nidGia, buf, buf1, buf2;
    int nHeight = 5;
    int step = 3;
    Vec_Int_t *vSharePI = Vec_IntAlloc( 100 );
    Vec_Int_t *vCandidate = Vec_IntAlloc( 100 );
    Vec_Int_t *vIntBuff, *vIntBuff2, *vMFFC;
    // Vec_Int_t *vCost = Vec_IntAlloc( 100 );
    assert( pMan->pAbs->pReprs );
    Gia_ManFillValue( pMan->pAbs );
    Cec_ManFillMark( pMan->pAbs, 0 );
    Cec_ManSubcircuit( pMan->pAbs, pMan->pObjAbs1, 0, 0, 1, 0 );
    Cec_ManSubcircuit( pMan->pAbs, pMan->pObjAbs2, 0, 0, 2, 0 );
    Gia_ManForEachCi( pMan->pAbs, pObj, i ) {
        if (Gia_ObjValue(pObj) == 3) Vec_IntPush( vSharePI, Gia_ObjId(pMan->pAbs, pObj) );
    }
    Gia_ManLevelNum( pMan->pAbs );
    Gia_ManForEachObj( pMan->pAbs, pObj, i ) {
        nidAbs = Gia_ObjId(pMan->pAbs, pObj);
        nidGia = Cec_ManFdMapIdSingle( pMan, nidAbs, 0 );
        if (Gia_ObjColors(pMan->pGia, nidGia) != fColor) continue;
        if (Gia_ObjLevel(pMan->pAbs, pObj) == 1) {
            buf1 = Gia_ObjFaninId0(pObj, nidAbs);
            buf2 = Gia_ObjFaninId1(pObj, nidAbs);
            if (Vec_IntFind( vSharePI, buf1 ) != -1 || Vec_IntFind( vSharePI, buf2 ) != -1) 
                Vec_IntPush( vCandidate, nidGia );
            
            // if (buf1 == 0) continue;
            // else {
            //     Vec_IntPush( vCandidate, nidAbs );
            //     Vec_IntPush( vCost, (1000 * buf1) / buf2 );
            // }
        }
    }
    // Vec_IntSelectSortCost2Reverse( vCandidate->pArray, Vec_IntSize(vCandidate), vCost->pArray );
    buf = 0;
    for ( ; buf < Vec_IntSize(vCandidate); ) {
        nidGia = Vec_IntEntry( vCandidate, buf );
        nidAbs = Cec_ManFdMapIdSingle( pMan, nidGia, 1 );
        pObj = Gia_ManObj( pMan->pAbs, nidAbs );
        if (pObj->fMark0 == 0 && pObj->fMark1 == 0) {
            buf = Cec_ManFdTraverseUnknownUpward( pMan, nidGia, nHeight );
            Cec_ManFdTraverseUnknownNeighbor( pMan, buf, step );
            vIntBuff = Cec_ManFdUnsatId( pMan, 0, 0 );
            vMFFC = Vec_IntAlloc( 100 );
            Cec_ManFdMapId( pMan, vIntBuff, 1 );
            Cec_ManMFFCMul( pMan->pAbs, vIntBuff, vMFFC );
            Cec_ManLoadMark( pMan->pAbs, vMFFC, 0, 0 );
            // do MFFC based traverse
            // LOCK the MFFC nodes? or just mark? nevertheless, need to set constraint on traverse functions
        }
        buf++;
    }

}
// ---TODO
Vec_Int_t* Cec_ManTFISize( Gia_Man_t* pGia ) {
    Gia_Obj_t* pObj, *pObj_buf;
    Vec_Int_t* vSize = Vec_IntAlloc( 100 );
    int i, j, buf;
    // Gia_ManFillValue( pGia );
    Gia_ManFillValue( pGia );
    Gia_ManForEachObj( pGia, pObj, i ) {
        // bottom up won't work, as we need to consider overlap
        Cec_ManSubcircuit( pGia, pObj, 0, 0, 1, 0 );
        buf = 0;
        Gia_ManForEachAnd( pGia, pObj_buf, j ) {
            if (Gia_ObjValue(pObj_buf) != -1) buf++;
            Gia_ObjSetValue( pObj_buf, -1 );
        }
        Vec_IntPush( vSize, buf );
    }
    return vSize;
}
// traverse higher cost one, In-order
int Cec_ManBuildOrder1( Gia_Man_t* pGia, Vec_Int_t* vCost, int nid, int val ) {
    Gia_Obj_t* pObj = Gia_ManObj( pGia, nid );
    int rwVal, id1, id2, fDebug = 0;
    // printf("nid = %d, val = %d\n", nid, val);
    if (Gia_ObjValue(pObj) == -1) {
        rwVal = val;
        if (Gia_ObjIsCi(pObj)) {
            if (fDebug) printf("nid = %d, rwVal = %d\n", nid, rwVal);
            Gia_ObjSetValue( Gia_ManObj(pGia, nid), rwVal );
            if (fDebug) printf("nid = %d, rwVal = %d\n", nid, rwVal);
            rwVal++;
        } else if (Gia_ObjIsCo(pObj)) {
            if (fDebug) printf("nid = %d, rwVal = %d\n", nid, rwVal);
            rwVal = Cec_ManBuildOrder1( pGia, vCost, Gia_ObjFaninId0(pObj, nid), rwVal );
            Gia_ObjSetValue( Gia_ManObj(pGia, nid), rwVal );
            if (fDebug) printf("nid = %d, rwVal = %d\n", nid, rwVal);
            rwVal++;
        } else if (Gia_ObjIsAnd(pObj)) {
            if (Vec_IntEntry( vCost, Gia_ObjFaninId0(pObj, nid) ) > Vec_IntEntry( vCost, Gia_ObjFaninId1(pObj, nid) )) {
                id1 = Gia_ObjFaninId0(pObj, nid);
                id2 = Gia_ObjFaninId1(pObj, nid);
            } else {
                id1 = Gia_ObjFaninId1(pObj, nid);
                id2 = Gia_ObjFaninId0(pObj, nid);
            }
            if (fDebug) printf("nid = %d, rwVal = %d\n", nid, rwVal);
            rwVal = Cec_ManBuildOrder1( pGia, vCost, id1, rwVal );
            Gia_ObjSetValue( Gia_ManObj(pGia, nid), rwVal );
            if (fDebug) printf("nid = %d, rwVal = %d\n", nid, rwVal);
            rwVal++;
            rwVal = Cec_ManBuildOrder1( pGia, vCost, id2, rwVal );
            if (fDebug) printf("nid = %d, rwVal = %d\n", nid, rwVal);
        } else assert(0);
        return rwVal;
    } else return val;
}

// if fRev, check far one
// IO are both abstract circuit id
int Cec_ManUpWard( Cec_ManFd_t* pMan, Vec_Int_t* vOrder, int nid, int height, int fRev, Vec_Int_t* vTrav ) {
    
    Gia_Man_t* pGia = pMan->pAbs;
    if (pGia->vFanout == NULL) Gia_ManStaticFanoutStart( pGia );

    Gia_Obj_t* pObj = Gia_ManObj( pGia, nid );
    Vec_Wec_t* vTFOCone;
    Vec_Int_t* vIntBuff; 
    Vec_Int_t* vFind = Vec_IntAlloc( 10 );
    Vec_Int_t* vSortOrd = Vec_IntAlloc( 10 );
    int id_nxt, id_buf, id_out, id_globalbuf, i, j, k;
    // int coef = Vec_IntEntry( vCoef, 0 );
    vTFOCone = Vec_WecStart( height + 1 );
    id_out = -1;
    Vec_IntPush( Vec_WecEntry( vTFOCone, 0 ), nid );
    for (int i = 0; i < height; i++) {
        // vIntBuff = Vec_WecEntry( vTFOCone, i );
        Vec_WecPrint( vTFOCone, 0 );
        Vec_IntClear( vSortOrd );
        Vec_IntForEachEntry( Vec_WecEntry( vTFOCone, i ), id_nxt, j ) {

            Gia_ObjForEachFanoutStaticId( pGia, Gia_ObjId(pGia, pObj), id_buf, k ) {
                if (Vec_IntEntry( vOrder, id_buf ) < Vec_IntEntry( vOrder, id_nxt )) continue;
                else if (Vec_IntFind( vFind, id_buf ) != -1) continue;
                Vec_IntPush( vFind, id_buf );
                Vec_IntPush( Vec_WecEntry( vTFOCone, i + 1 ), id_buf );
                Vec_IntPush( vSortOrd, Vec_IntEntry( vOrder, id_buf ) );
            }
            
        }
        if ( Vec_IntSize( Vec_WecEntry( vTFOCone, i + 1 ) ) == 0 ) break;
        if (fRev) Vec_IntSelectSortCost2Reverse( Vec_WecEntry( vTFOCone, i + 1 ), Vec_IntSize( vSortOrd ), vSortOrd );
        else Vec_IntSelectSortCost2( Vec_WecEntry( vTFOCone, i + 1 ), Vec_IntSize( vSortOrd ), vSortOrd );
    }
    Vec_WecPrint( vTFOCone, 0 );
    Vec_WecForEachLevelReverse( vTFOCone, vIntBuff, i ) {
        if (i == 0) continue;
        Vec_IntForEachEntry( vIntBuff, id_nxt, j ) {
            if (vTrav) Vec_IntPush( vTrav, id_nxt );
            if (id_out != -1) continue;
            id_globalbuf = Cec_ManFdMapIdSingle( pMan, id_nxt, 0 );
            if (Cec_ManFdToSolve( pMan, id_globalbuf ) == 0) continue;
            Cec_ManFdShrinkSimple( pMan, id_globalbuf );
            if (Vec_IntEntry( pMan->vStat, id_globalbuf ) > 0) { // find UNSAT node
                id_out = id_nxt;
            }
        }
    }
    Vec_WecFree( vTFOCone );
    return id_out;
}
// IO are both abstract circuit id
int Cec_ManDownWard( Cec_ManFd_t* pMan, Vec_Int_t* vOrder, int nid, int fType, int coef, Vec_Int_t* vCost, Vec_Int_t* vTrav ) {
    // int nid = Cec_ManFdMapIdSingle( pMan, nidGlobal, 1 );
    Gia_Man_t* pGia = pMan->pAbs;
    if (pGia->vFanout == NULL) Gia_ManStaticFanoutStart( pGia );
    Gia_Obj_t* pObj = Gia_ManObj( pGia, nid );
    int id_nxt;
    // int coef = Vec_IntEntry( vCoef, 0 );
    if (vTrav) Vec_IntPush( vTrav, nid );

    if (Gia_ObjIsCi(pObj)) return nid;
    if (Gia_ObjIsCo(pObj)) return Cec_ManDownWard( pMan, vOrder, Gia_ObjFaninId0(pObj, nid), fType, coef, vCost, vTrav );
    if (Gia_ObjIsAnd(pObj)) {
        if (Vec_IntEntry( vOrder, Gia_ObjFaninId0(pObj, nid) ) > Vec_IntEntry( vOrder, Gia_ObjFaninId1(pObj, nid) )) {
            id_nxt = Gia_ObjFaninId0(pObj, nid);
        } else {
            id_nxt = Gia_ObjFaninId1(pObj, nid);
        }
        switch (fType) {
            case 0:
                if (coef <= 0 && Gia_ObjFanoutNum( pGia, pObj ) > 1) return nid;
                else return Cec_ManDownWard( pMan, vOrder, id_nxt, fType, coef - 1, vCost, vTrav );
            case 1:
                if ( Vec_IntEntry( vCost, id_nxt ) < coef && Gia_ObjFanoutNum( pGia, pObj ) > 1) return nid;
                else return Cec_ManDownWard( pMan, vOrder, id_nxt, fType, coef, vCost, vTrav );
            default:
                assert(0);
        }
    } else assert(0);
    
}
Vec_Int_t* Cec_ManFdTraverseUnknownFrt_old( Cec_ManFd_t* pMan, int nStart, int nLimPatch, float nLimFrt ) {
    int fJumpFar = 0;
    Vec_Int_t* vCost, *vTFO;
    Vec_Int_t* vTrav = Vec_IntAlloc( Gia_ManObjNum(pMan->pAbs) );
    Vec_Int_t* vOrder = Vec_IntAlloc( Gia_ManObjNum(pMan->pAbs) );
    Vec_Ptr_t* vNdCone;
    Gia_Obj_t* pCo = Gia_ObjColors( pMan->pGia, nStart ) == 1 ? pMan->pObjAbs1 : pMan->pObjAbs2;
    Gia_Obj_t* pObj;
    int k, nidTrav, nidbuf;
    // if (pMan->pAbs->vFanout == NULL) Gia_ManStaticFanoutStart( pMan->pAbs );
    // vCost = Cec_ManTFISize( pMan->pAbs );
    Gia_ManLevelNum( pMan->pAbs );
    vCost = Vec_IntDup( pMan->pAbs->vLevels );
    // vCost = Vec_IntStart( Gia_ManObjNum(pMan->pAbs) );
    // nidbuf = Gia_ObjId( pMan->pAbs, pCo );
    // for (k = 1; k < Gia_ManObjNum(pMan->pAbs); k++) {
    //     Vec_IntWriteEntry( vCost, k, pMan->dist_long_abs[nidbuf][k] );
    // }
    Gia_ManFillValue( pMan->pAbs );
    // problem: currently use In-order, not pre-order
    Cec_ManBuildOrder1( pMan->pAbs, vCost, Gia_ObjId(pMan->pAbs, pCo), 0 ); 
    vOrder = Cec_ManDumpValue( pMan->pAbs );
    Vec_IntPrintMap( vCost );
    Vec_IntPrintMap( vOrder );

    nidTrav = Cec_ManFdMapIdSingle( pMan, nStart, 1 );
    while (true) {
        nidTrav = Cec_ManDownWard( pMan, vOrder, nidTrav, 0, 3, vCost, vTrav );
        Vec_IntPrint( vTrav );
        nidTrav = Cec_ManUpWard( pMan, vOrder, nidTrav, 3, fJumpFar, vTrav );
        Vec_IntPrint( vTrav );
        if (nidTrav == -1) break;
    }
    return vTrav;

}
// return nidAbs
int Cec_ManFdTraverseUnknownFirst( Cec_ManFd_t* pMan, Vec_Int_t* vOrder, Vec_Int_t* vTrav ) {
    int limSearch = 10;
    int nidStart, nidTrav, nidbuf, nidGlob, i, buf, buf2;
    Vec_Int_t* vIntBuff;
    for (i = 0; limSearch > 0; i++) {
        nidStart = Vec_IntFind( vOrder, i );
        if (Gia_ObjIsAnd( Gia_ManObj( pMan->pAbs, nidStart ) ) == 0) continue;
        limSearch--;
        nidGlob = Cec_ManFdMapIdSingle( pMan, nidStart, 0 );
        if (Cec_ManFdToSolve( pMan, nidGlob )) {
            if (vTrav) Vec_IntPush( vTrav, nidStart );
            Cec_ManFdShrinkSimple( pMan, nidGlob );
            if (Vec_IntEntry( pMan->vStat, nidGlob ) > 0) {
                break;
            }
        }
    }
    if (limSearch == 0) return -1;
    while (Vec_IntEntry( pMan->vStat, nidGlob ) > 0) {
        nidTrav = Cec_ManFdMapIdSingle( pMan, nidGlob, 1 );
        buf = -1;
        buf2 = -1;
        Gia_ObjForEachFanoutStaticId( pMan->pAbs, nidTrav, nidbuf, i ) {
            if (buf == -1 || buf > Vec_IntEntry( vOrder, nidbuf )) {
                buf = Vec_IntEntry( vOrder, nidbuf );
                buf2 = nidbuf;
            }
        }
        assert(buf2 > 0);
        // nidTrav = buf2;
        nidGlob = Cec_ManFdMapIdSingle( pMan, buf2, 0 );
        assert(Cec_ManFdToSolve( pMan, nidGlob ));
        if (vTrav) Vec_IntPush( vTrav, buf2 );
        Cec_ManFdShrinkSimple( pMan, nidGlob );
    }
    return nidTrav;


}
// IO is nidGlob, pTFI is pAbs, assume commited Cover with value be ~2
int Cec_ManFdCntCoverChange( Cec_ManFd_t* pMan, Gia_Man_t* pTFICheck, int nid, int fOrg ) {
    int cnt = 0;
    int nidAbs = Cec_ManFdMapIdSingle( pMan, nid, 1 );
    Gia_Obj_t* pObj;
    if (fOrg) {
        Gia_ManForEachAnd( pTFICheck, pObj, nid ) {
            Cec_ManSubcircuit( pTFICheck, Gia_ManObj( pTFICheck, nidAbs ), 0, 0, 1, 0 );
            if (Gia_ObjIsAnd(pObj) == 0) continue;
            if (~(pObj->Value) == 1) cnt++;
            
        }
        return cnt;
    } else if ( Vec_IntEntry( pMan->vStat, nid ) > 0 ) {
        return Gia_ManAndNum( (Gia_Man_t*) Vec_PtrEntry(pMan->vVeryStat, nid) );
    } else {
        return -1;
    }
    
    return cnt;
}
// IO is nidGlob
int Cec_ManFdCntFrtChange( Cec_ManFd_t* pMan, Vec_Int_t* vFrt, int nid, int fOrg ) {
    // Vec_Int_t* vFrtNid = Vec_IntStart( Vec_IntSize(pMan->vMerge) );
    int i, nidbuf, buf, cnt = 0;
    if (fOrg) {
        Vec_IntForEachEntry( Vec_WecEntry(pMan->vMergeSupport, nid), buf, i ) {
           if (buf == 1 && Vec_IntEntry( vFrt, i ) == 0) {
               Vec_IntWriteEntry( vFrt, i, 1 );
               cnt++;
           }
        }
    } else {
        Vec_IntForEachEntry( Vec_WecEntry(pMan->vGSupport, nid), nidbuf, i ) {
            buf = Vec_IntFind( pMan->vMerge, nidbuf );
            if (buf == -1) continue;
            if (Vec_IntEntry( vFrt, buf ) == 0) {
                Vec_IntWriteEntry( vFrt, buf, 1 );
                cnt++;
            }
        }
    }
    
    return cnt;
}
// IO is nidAbs
int Cec_ManCommit( Gia_Man_t* pTFICheck, Vec_Int_t* vFrt, int fCommit ) {
    Gia_Obj_t* pObj;
    // int nidGlob = Cec_ManFdMapIdSingle( pMan, nid, 0 );
    int i, nidbuf, buf;
    if (fCommit) {
        // Cec_ManSubcircuit( pTFICheck, Gia_ManObj( pTFICheck, nid ), 0, 0, 2, 0 );
        Gia_ManForEachObj( pTFICheck, pObj, nidbuf ) {
            if (~(Gia_ObjValue(pObj)) == 1) {
                Gia_ObjSetValue( pObj, ~2 );
            }
        }
        Vec_IntForEachEntry( vFrt, buf, i ) {
            if (buf == 1) Vec_IntWriteEntry( vFrt, i, 2 );
        }
    } else {
        // uncommit the node with value 1
        Gia_ManForEachObj( pTFICheck, pObj, nidbuf ) {
            if (~(Gia_ObjValue(pObj)) == 1) {
                Gia_ObjSetValue( pObj, ~0 );
            }
        }
        Vec_IntForEachEntry( vFrt, buf, i ) {
            if (buf == 1) Vec_IntWriteEntry( vFrt, i, 0 );
        }
    }
}
// IO is nidAbs
void Cec_ManFdTraverseUnknownCommitNew( Cec_ManFd_t* pMan, Gia_Man_t* pTFICheck, Vec_Int_t* vFrt, int nid ) {
    int nidbuf = Cec_ManFdMapIdSingle( pMan, nid, 0 );
    Cec_ManFdCntCoverChange( pMan, pTFICheck, nidbuf, 1 );
    Cec_ManFdCntFrtChange( pMan, vFrt, nidbuf, 0 );
    Cec_ManCommit( pTFICheck, vFrt, 1 );
}
// IO is nidAbs
void Cec_ManFdTraverseUnknownCommitTest( Cec_ManFd_t* pMan, Gia_Man_t* pTFICheck, Vec_Int_t* vFrt, int nid, int fOrg, int* nTFI, int* nFRT ) {
    int nidbufGlob = Cec_ManFdMapIdSingle( pMan, nid, 0 );
    *nTFI = Cec_ManFdCntCoverChange( pMan, pTFICheck, nidbufGlob, fOrg );
    *nFRT = Cec_ManFdCntFrtChange( pMan, vFrt, nidbufGlob, fOrg );
    Cec_ManCommit( pTFICheck, vFrt, 0 );
}
// nid is nidAbs
int Cec_ManFdTraverseUnknownCheck( Cec_ManFd_t* pMan, Gia_Man_t* pCheckTFI, Vec_Int_t* vUsedFrt, Vec_Flt_t* vCoef, Vec_Int_t* vStat, int nidTrav, int fType ) {
    assert( Vec_FltSize( vCoef ) >= 2 );
    assert( Vec_IntSize( vStat ) == 4 );
    int inc_TFI_org, inc_TFI_new, inc_FRT_org, inc_FRT_new;
    int rec_FRT = Vec_IntEntry( vStat, 0 );
    int rec_FRTp = Vec_IntEntry( vStat, 1 );
    int rec_patch = Vec_IntEntry( vStat, 2 );
    int rec_TFI = Vec_IntEntry( vStat, 3 );
    float nLimPatch = Vec_FltEntry( vCoef, 0 );
    float nLimFrt = Vec_FltEntry( vCoef, 1 );
    float coef1 = Vec_FltSize( vCoef ) > 2 ? Vec_FltEntry( vCoef, 2 ) : -1;
    float coef2 = Vec_FltSize( vCoef ) > 3 ? Vec_FltEntry( vCoef, 3 ) : -1;
    float coef3 = Vec_FltSize( vCoef ) > 4 ? Vec_FltEntry( vCoef, 4 ) : -1;
    float coef4 = Vec_FltSize( vCoef ) > 5 ? Vec_FltEntry( vCoef, 5 ) : -1;
    int nidbufGlob = Cec_ManFdMapIdSingle( pMan, nidTrav, 0 );
    int RetVal;
    float ratio_TFI, ratio_FRT;
    

    if ( Cec_ManFdToSolve( pMan, nidbufGlob ) ) {
        // preTest, if not success, return RetVal = 0
        Cec_ManFdShrinkSimple( pMan, nidbufGlob );
    }

    if ( Vec_IntEntry( pMan->vStat, nidbufGlob ) <= 0 ) {
        RetVal = 0;
    } else {
        Cec_ManFdTraverseUnknownCommitTest( pMan, pCheckTFI, vUsedFrt, nidTrav, 1, &inc_TFI_org, &inc_FRT_org );
        rec_TFI += inc_TFI_org;
        rec_FRT += inc_FRT_org;
        Cec_ManFdTraverseUnknownCommitTest( pMan, pCheckTFI, vUsedFrt, nidTrav, 0, &inc_TFI_new, &inc_FRT_new );
        rec_patch += inc_TFI_new;
        rec_FRTp += inc_FRT_new;
        switch (fType) {
            case 0: // sum_up
                ratio_TFI = 1.0 * (float)(rec_TFI) / (float)(rec_patch);
                ratio_FRT = 1.0 * (float)(rec_FRT) / (float)(rec_FRTp);
                if ( ratio_TFI > nLimPatch || ratio_FRT > nLimFrt ) {
                    RetVal = 0;
                    // bendit
                    if ( coef1 < 0 ) coef1 = 0.8;
                    if ( coef2 < 0 ) coef2 = 0.8;
                    if ( coef3 < 0 ) coef3 = 50.0;
                    if ( coef4 < 0 ) coef4 = 50.0;
                    ratio_TFI = 1.0 * (float)(coef3 + rec_TFI) / (float)(coef3 + rec_patch);
                    ratio_FRT = 1.0 * (float)(coef4 + rec_FRT) / (float)(coef4 + rec_FRTp);
                    nLimPatch = coef1 * nLimPatch + (1 - coef1) * ratio_TFI;
                    nLimFrt = coef2 * nLimFrt + (1 - coef2) * ratio_FRT;
                    coef1 -= 0.2 * coef1 * (1.0 - coef1);
                    coef2 -= 0.2 * coef2 * (1.0 - coef2);
                    Vec_FltWriteEntry( vCoef, 0, nLimPatch );
                    Vec_FltWriteEntry( vCoef, 1, nLimFrt );
                    if (Vec_FltSize( vCoef ) > 2) Vec_FltWriteEntry( vCoef, 2, coef1 );
                    if (Vec_FltSize( vCoef ) > 3) Vec_FltWriteEntry( vCoef, 3, coef2 );
                    
                } else {
                    RetVal = 1;
                    if ( coef1 < 0 ) coef1 = 0.8;
                    if ( coef2 < 0 ) coef2 = 0.8;
                    if ( coef3 < 0 ) coef3 = 50.0;
                    if ( coef4 < 0 ) coef4 = 50.0;
                    ratio_TFI = 1.0 * (float)(coef3 + rec_TFI) / (float)(coef3 + rec_patch);
                    ratio_FRT = 1.0 * (float)(coef4 + rec_FRT) / (float)(coef4 + rec_FRTp);
                    nLimPatch = coef1 * nLimPatch + (1 - coef1) * ratio_TFI;
                    nLimFrt = coef2 * nLimFrt + (1 - coef2) * ratio_FRT;
                    coef1 += (1.0 - coef1) * (1.0 - coef1);
                    coef2 += (1.0 - coef2) * (1.0 - coef2);
                    Vec_FltWriteEntry( vCoef, 0, nLimPatch );
                    Vec_FltWriteEntry( vCoef, 1, nLimFrt );
                    
                    if( Vec_FltSize( vCoef ) > 2 ) Vec_FltWriteEntry( vCoef, 2, coef1 );
                    if( Vec_FltSize( vCoef ) > 3 ) Vec_FltWriteEntry( vCoef, 3, coef2 );
                    
                }
                break;
            case 1:
                if ( 1.0 * (float)(inc_TFI_new) / (float)(inc_TFI_org) > nLimPatch || 
                     1.0 * (float)(inc_FRT_new) / (float)(inc_FRT_org) > nLimFrt ) {
                        RetVal = 0;
                } else {
                    RetVal = 1;
                }
                break;
        }
    }
    
    if ( RetVal == 1 ) {
        Vec_IntWriteEntry( vStat, 0, rec_FRT );
        Vec_IntWriteEntry( vStat, 1, rec_FRTp );
        Vec_IntWriteEntry( vStat, 2, rec_patch );
        Vec_IntWriteEntry( vStat, 3, rec_TFI );
    }
    return RetVal;
    // Cec_ManCommit( pMan, pCheckTFI, vUsedFrt, nidTrav );
}

Vec_Int_t* Cec_ManFdTraverseUnknownFrt( Cec_ManFd_t* pMan, int fColor, int nRange, int fType, Vec_Flt_t* vCoef ) {
    // int fJumpFar = 0;
    assert( fColor == 1 || fColor == 2 );
    // assert( Vec_FltSize( vCoef ) >= 2 );
    Vec_Int_t* vCost, *vTFO, *vCand;
    Vec_Int_t* vTrav = Vec_IntAlloc( Gia_ManObjNum(pMan->pAbs) );
    Vec_Int_t* vOrder = Vec_IntAlloc( Gia_ManObjNum(pMan->pAbs) );
    Vec_Int_t* vUsedFrt = Vec_IntStart( Vec_IntSize(pMan->vMerge) ); // bin-format
    Vec_Int_t* vIntBuff;
    Vec_Int_t* vStat = Vec_IntStart(4);
    Vec_IntFill( vStat, 4, 0 );
    Vec_Ptr_t* vNdCone;
    Gia_Man_t* pCheckTFI = Gia_ManDup( pMan->pAbs );
    Gia_Man_t* pPatch;
    // Gia_Obj_t* pCo = fColor == 1 ? pMan->pObjAbs1 : pMan->pObjAbs2;
    Gia_Obj_t* pObj;
    int lv, cntFail;
    float cost1, cost2;
    int inc_TFI_org, inc_TFI_new, inc_FRT_org, inc_FRT_new;
    // int rec_TFI, rec_FRT, rec_patch, rec_FRTp;
    int k, buf, buf2, nidbuf, nidbufGlob;

    if (pMan->pAbs->vFanout == NULL) Gia_ManStaticFanoutStart( pMan->pAbs );
    Gia_ManLevelNum( pMan->pAbs );
    vCost = Vec_IntDup( pMan->pAbs->vLevels );
    Gia_ManFillValue( pMan->pAbs );
    buf = fColor == 1 ? Gia_ObjId( pMan->pAbs, pMan->pObjAbs1 ) : Gia_ObjId( pMan->pAbs, pMan->pObjAbs2 );
    Cec_ManBuildOrder1( pMan->pAbs, vCost, buf, 0 );
    vOrder = Cec_ManDumpValue( pMan->pAbs );
    // Vec_IntPrintMap( vCost );
    // Vec_IntPrintMap( vOrder );

    buf = Cec_ManFdTraverseUnknownFirst( pMan, vOrder, vTrav );
    lv = Vec_IntEntry( pMan->pAbs->vLevels, buf );
    // currently only consider one level

    Gia_ManFillValue( pCheckTFI );
    Vec_IntFill( vUsedFrt, Vec_IntSize(vUsedFrt), 0 );

    vIntBuff = Cec_ManFdUnsatId( pMan, 0, 0 );
    Cec_ManFdMapId( pMan, vIntBuff, 1 );
    // Vec_IntSelectSortCostReverse( vIntBuff->pArray, Vec_IntSize(vIntBuff), vOrder );
    
    buf = 0; cost1 = -1.0; cost2 = -1.0;
    Vec_IntForEachEntry( vIntBuff, nidbuf, k ) {
        nidbufGlob = Cec_ManFdMapIdSingle( pMan, nidbuf, 0 );
        if (Vec_IntEntry( pMan->vStat, nidbufGlob ) > 0) Vec_IntWriteEntry( pMan->vStat, nidbufGlob, CEC_FD_HUGE );
        else continue;
        Cec_ManFdTraverseUnknownCommitTest( pMan, pCheckTFI, vUsedFrt, nidbuf, 1, &inc_TFI_org, &inc_FRT_org );
        Cec_ManFdTraverseUnknownCommitTest( pMan, pCheckTFI, vUsedFrt, nidbuf, 0, &inc_TFI_new, &inc_FRT_new );
        // currently no consider on FRT
        if (cost1 < 0 || 
            (cost1 > 1.0 * (float)(inc_TFI_new) / (float)(inc_TFI_org))) {
                cost1 = 1.0 * (float)(inc_TFI_new) / (float)(inc_TFI_org);
                cost2 = 1.0 * (float)(inc_FRT_new) / (float)(inc_FRT_org);
                buf = nidbuf;
            }
    }
    nidbufGlob = Cec_ManFdMapIdSingle( pMan, buf, 0 );
    if ( Vec_FltSize( vCoef ) >= 2 ) {
        if (Cec_ManFdTraverseUnknownCheck( pMan, pCheckTFI, vUsedFrt, vCoef, vStat, buf, fType )) {
            Vec_IntWriteEntry( pMan->vStat, nidbufGlob, 1 );
            Cec_ManFdTraverseUnknownCommitNew( pMan, pCheckTFI, vUsedFrt, buf );
        }
    } else {
        Vec_IntWriteEntry( pMan->vStat, nidbufGlob, 1 );
        Cec_ManFdTraverseUnknownCommitNew( pMan, pCheckTFI, vUsedFrt, buf );
        Vec_FltPush( vCoef, cost1 );
        Vec_FltPush( vCoef, cost2 );
    }

    // Cec_ManFdMapId( pMan, vIntBuff, 1 );
    // Vec_IntForEachEntry( vIntBuff, nidbuf, k ) {
    //     Cec_ManSubcircuit( pCheckTFI, Gia_ManObj( pCheckTFI, nidbuf ), 0, 0, 1, 0 );
    // }
    for (buf = lv + nRange; buf > lv - nRange; buf--) {
        vIntBuff = Vec_IntFindAll( pMan->pAbs->vLevels, buf, 0 );
        Vec_IntSelectSortCost( vIntBuff->pArray, Vec_IntSize(vIntBuff), vOrder );
        cntFail = 0;
        Vec_IntForEachEntry( vIntBuff, nidbuf, k ) {
            nidbufGlob = Cec_ManFdMapIdSingle( pMan, nidbuf, 0 );
            if (Cec_ManFdToSolve( pMan, nidbufGlob ) == 0) continue;
            if (Gia_ObjColors( pMan->pGia, nidbufGlob ) != fColor) continue;
            if (Gia_ObjValue( Gia_ManObj( pCheckTFI, nidbuf ) ) != -1) continue;
            if (vTrav) Vec_IntPush( vTrav, nidbuf );
            
            if (Cec_ManFdTraverseUnknownCheck( pMan, pCheckTFI, vUsedFrt, vCoef, vStat, nidbuf, fType )) {
                cntFail = 0;
                printf("nid: %d, result:", nidbuf);
                Cec_ManFdTraverseUnknownCommitNew( pMan, pCheckTFI, vUsedFrt, nidbuf );
            } else {
                if (Vec_IntEntry( pMan->vStat, nidbufGlob ) > 0) {
                    printf("nid %d: not accepted\n", nidbuf);
                    Vec_IntWriteEntry( pMan->vStat, nidbufGlob, CEC_FD_HUGE );
                } else {
                    printf("nid %d: not solved\n", nidbuf);
                }
                // cntFail++;
            }
            printf("stat:");
            Vec_IntPrint( vStat );
            printf("coef:");
            Vec_FltPrint( vCoef );
            // Update
            
            // if (cntFail == 100) break;
            
        }
    }
    

    return vTrav;

}
void Cec_ManFdTraverseUnknownLevelTest( Cec_ManFd_t* pMan, int lv_min, int lv_max, int lv_lim, int fColor ) {
    assert( fColor == 1 || fColor == 2 );
    // assert( Vec_FltSize( vCoef ) >= 2 );
    Vec_Int_t* vCost, *vTFO, *vCand;
    Vec_Int_t* vTrav = Vec_IntAlloc( Gia_ManObjNum(pMan->pAbs) );
    Vec_Int_t* vOrder = Vec_IntAlloc( Gia_ManObjNum(pMan->pAbs) );
    Vec_Int_t* vUsedFrt = Vec_IntStart( Vec_IntSize(pMan->vMerge) ); // bin-format
    Vec_Int_t* vIntBuff;
    Vec_Int_t* vStat = Vec_IntStart(4);
    Vec_IntFill( vStat, 4, 0 );
    Vec_Ptr_t* vNdCone;
    Gia_Man_t* pCheckTFI = Gia_ManDup( pMan->pAbs );
    Gia_Man_t* pPatch;
    // Gia_Obj_t* pCo = fColor == 1 ? pMan->pObjAbs1 : pMan->pObjAbs2;
    Gia_Obj_t* pObj;
    int lv, cntFail;
    float cost1, cost2;
    int inc_TFI_org, inc_TFI_new, inc_FRT_org, inc_FRT_new;
    // int rec_TFI, rec_FRT, rec_patch, rec_FRTp;
    int k, buf, buf2, nidbuf, nidbufGlob;

    if (pMan->pAbs->vFanout == NULL) Gia_ManStaticFanoutStart( pMan->pAbs );
    Gia_ManLevelNum( pMan->pAbs );
    vCost = Cec_ManTFISize( pMan->pAbs );

    for (buf = lv_max; buf > lv_min; buf--) {
        vIntBuff = Vec_IntFindAll( pMan->pAbs->vLevels, buf, 0 );
        Vec_IntForEachEntry( vIntBuff, nidbuf, k ) {
            nidbufGlob = Cec_ManFdMapIdSingle( pMan, nidbuf, 0 );
            if (Gia_ObjColors( pMan->pGia, nidbufGlob ) != fColor) continue;
            Cec_ManFdShrinkLevelTest( pMan, nidbufGlob, lv_lim );
            printf("TFI size: %d\n", Vec_IntEntry( vCost, nidbuf ) );
        }
    }

    vCand = Cec_ManFdUnsatId( pMan, 0, 1 );
    vIntBuff = Vec_IntAlloc(100);
    Vec_IntForEachEntry( vCand, buf, k ) {
        Vec_IntPush( vIntBuff, Gia_ManAndNum( (Gia_Man_t*) Vec_PtrEntry( pMan->vVeryStat, buf) ) );
    }
    Vec_IntSelectSortCost2( vCand->pArray, Vec_IntSize(vCand), vIntBuff->pArray );
    for (k = 0; k < Vec_IntSize(vCand); k++) {
        nidbufGlob = Vec_IntEntry(vCand, k);
        nidbuf = Cec_ManFdMapIdSingle( pMan, nidbufGlob, 1 );
        printf("nid: %d(%d), size: %d, TFI size:%d\n", nidbufGlob, nidbuf, Vec_IntEntry(vIntBuff, k), Vec_IntEntry(vCost, nidbuf) );
    }
}
void Cec_ManFdShrinkLevelTest( Cec_ManFd_t* pMan, int nid, int lv_lim ) {
    assert( Gia_ObjColors( pMan->pGia, nid ) == 1 || Gia_ObjColors( pMan->pGia, nid ) == 2 );
    // if( Cec_ManFdToSolve( pMan, nid ) == 0 ) return;
    int cktOth = Gia_ObjColors( pMan->pGia, nid ) == 1 ? 2 : 1;
    int nidSup, i, buf, flag, nidAbs;
    Vec_Int_t* vIntBuff, *vStat;
    Vec_Int_t* vFdSupport = Vec_WecEntry(pMan->vGSupport, nid);
    Vec_Int_t* vFdSupportNew = Vec_IntAlloc( Vec_IntSize(vFdSupport) );
    Gia_Man_t* patch, *pTemp;// , patchOpt;

    Gia_ManLevelNum( pMan->pAbs );
    Vec_IntForEachEntry( vFdSupport, nidSup, i ) {
        nidAbs = Cec_ManFdMapIdSingle( pMan, nidSup, 1 );
        if ( Gia_ObjLevelId( pMan->pAbs, nidAbs ) >= lv_lim ) Vec_IntPush( vFdSupportNew, nidSup );
    }
    vStat = Vec_IntAlloc(1);
    patch = Cec_ManFdGetFd( pMan, nid, vFdSupportNew, vStat );
    nidAbs = Cec_ManFdMapIdSingle( pMan, nid, 1 );
    printf("nid %d conflict required %d\n", nid, nidAbs, Vec_IntEntry( vStat, 0 ));
    Vec_IntClear( vStat );
    if (patch != NULL && patch != 1 && patch != 2) {
        patch = Cec_ManPatchCleanSupport( pTemp = patch, vFdSupportNew );
        Gia_ManStop(pTemp);
        patch = Cec_ManSimpSyn( pTemp = patch, 1, 0, -1 );
        Gia_ManStop(pTemp);
        printf("nid %d(%d) with shrink lv_lim %d have found patch sized %d\n", nid, nidAbs, lv_lim, Gia_ManAndNum(patch));
        printf("G support: ");
        Cec_ManFdMapId( pMan, vIntBuff = Vec_IntDup( vFdSupportNew ), 1 );
        Vec_IntPrint( vIntBuff );
        Vec_IntFree( vIntBuff );
    } else {
        printf("nid %d(%d) is SAT/UNSOLVED\n", nid, nidAbs);
    }
   
    Cec_ManFdUpdateStatForce( pMan, nid, vFdSupportNew, patch, 0 );
}
// -TODO
void Cec_ManFdShrink( Cec_ManFd_t* pMan, int nid ) {
    printf("Shrinking nid %d\n", nid);
    if (nid > Gia_ManObjNum(pMan->pGia) || nid < 0) {
        printf("nid %d is out of range\n", nid);
        return;
    }
    if (Vec_IntEntry( pMan->vStat, nid ) <= 0 && Vec_IntEntry( pMan->vStat, nid ) != CEC_FD_TRIVIAL) { // && Vec_IntEntry( pMan->vStat, nid ) != CEC_FD_HUGE) {
        printf("nid %d is not available to shrink\n", nid);
        return;
    }
    
    abctime clk;
    Vec_Int_t* vIntBuff, *vSupport, *vStat = Vec_IntAlloc(1);
    Vec_Wec_t *vFdSupportSplit, *vWecBuff;
    int fLocalShrink = pMan->pPars->fLocalShrink;
    int fMin = Cec_ManFdGetLevelType( pMan, 3 );
    int fFI = Cec_ManFdGetLevelType( pMan, 2 );
    int ftype = Cec_ManFdGetLevelType( pMan, 0 );
    // int fStrict = (pMan->pPars->costType == CEC_FD_COSTHEIGHT ||  
    //                 pMan->pPars->costType == CEC_FD_COSTTWOPATHOLD) ? 1 : 0;
    int fStrict = 0;
    float coef = pMan->pPars->coefPatch;
    int toShrink, i, nidbuff, buf, val, entry;
    int** dist_long = pMan->dist_long;
    int** dist_short = pMan->dist_short;
    Gia_Man_t *p = pMan->pGia;
    Gia_Man_t *pTemp;
    Gia_Obj_t *pObj;
    Vec_Int_t* vFdSupportBuff = Vec_WecEntry(pMan->vGSupport, nid);
    Vec_Int_t* vLevel = (Gia_ObjColors( p, nid ) == 1) ? pMan->vLevel2 : pMan->vLevel1;
    
    
    
    vFdSupportSplit = Vec_WecAlloc( 100 );
    Gia_ManFillValue( p );
    if (pMan->pPars->fPickType == 1) {
        Vec_IntForEachEntry( Cec_ManGetFIO( p, vFdSupportBuff, 1 ), nid, i ) {
            Gia_ObjSetValue(Gia_ManObj(p, nid), 0);
        }
    }

    if (fLocalShrink) {
        vWecBuff = Cec_ManDistAnalyze( p, vFdSupportBuff, Cec_ManGetFIO( p, vFdSupportBuff, 0 ), 
                Cec_ManGetFIO( p, vFdSupportBuff, 1 ), dist_long, dist_short );
        Vec_WecForEachLevel( vWecBuff, vIntBuff, i ) {
            if (i == Vec_WecSize(vWecBuff) - 1) continue; // skip the last level
            nidbuff = Vec_IntEntry(vIntBuff, 0);
            
            if (Gia_ObjValue(Gia_ManObj(p, nidbuff)) == 0) val = 0;
            else {
                val = Vec_IntEntry(vIntBuff, ftype + 2);
                if (fFI) val = Vec_IntEntry(Vec_WecEntryLast( vWecBuff ), ftype - fMin) - val;
                val += 1;
            }
            // printf("%d: %d\n", nid, Vec_IntEntry(vIntBuff, 0));
            while (Vec_IntSize(vFdSupportSplit) <= val) Vec_WecPushLevel( vFdSupportSplit );
            assert( val >= 0 ); 
            Vec_WecPush( vFdSupportSplit, val, nidbuff );
        }
        // Vec_WecPrint(vWecBuff, 0);
        Vec_WecFree( vWecBuff );
    } else {
        // Problem: PO level might be far from going to another node
        Vec_IntForEachEntry( vFdSupportBuff, nidbuff, i ) {
            if (nidbuff == -1) continue;
            if (Gia_ObjValue(Gia_ManObj(p, nidbuff)) == 0) val = 0;
            else val = Vec_IntEntry( vLevel, nidbuff ) + 1;
            
            while (Vec_IntSize(vFdSupportSplit) <= val) Vec_WecPushLevel( vFdSupportSplit );
            assert( val >= 0 ); 
            Vec_WecPush( vFdSupportSplit, val, nidbuff );
        }
        
    }
    
    Vec_WecForEachLevel( vFdSupportSplit, vIntBuff, i ) {
        if (Vec_IntSize(vIntBuff) == 0) continue;
        printf("%d: ", i);
        Vec_IntPrint(vIntBuff);
    }

    clk = Abc_Clock(); 
    vIntBuff = Vec_IntAlloc( Vec_IntSize(vFdSupportBuff) );
    val = 0;
    toShrink = 1;
    pTemp = NULL;
    while (toShrink) {
        // if (val == Vec_WecSize(vFdSupportSplit)) {
        //     printf("No shrink is better with size %d\n", Gia_ManAndNum(pTemp));
        //     toShrink = 0;
        //     break;
        // }
        if ( val == Vec_WecSize(vFdSupportSplit) ) {
            printf("val has reached the max level %d\n", val);
            break;
        }
        if (Vec_IntSize(Vec_WecEntry(vFdSupportSplit, val)) == 0) {
            val += 1;
            continue;
        }
        Vec_IntForEachEntry( Vec_WecEntry(vFdSupportSplit, val), nidbuff, i ) {
            Vec_IntPush( vIntBuff, nidbuff );
        }
        if (Vec_IntSize(vIntBuff) == Vec_IntSize(vFdSupportBuff)) {
            printf("No shrink is better for %d\n", nid);
            toShrink = 0;
            break;
        }
        // if (pMan->pPars->fVerbose) printf("support level %d have size of support: %d......\n", val, Vec_IntSize(vIntBuff));
        
        Gia_ManFillValue(p);
        // Vec_IntPrint(vIntBuff);
        Vec_IntClear( vStat );
        pTemp = pMan->pPars->fInc ? Cec_ManFdGetFd_Inc( pMan, nid, vIntBuff ) : Cec_ManFdGetFd( pMan, nid, vIntBuff, vStat );
        if (pTemp != NULL && pTemp != 1 && pTemp != 2) {
            
            vSupport = Vec_IntDup( Vec_WecEntry(pMan->vMergeSupport, nid) );
            // Vec_IntPrint(vIntBuff);
            Vec_IntForEachEntry( vSupport, entry, i ) {
                if (entry == 0) continue;
                nidbuff = Vec_IntEntry( pMan->vMerge, i );
                // printf("nidbuff %d\n", nidbuff);
                if (Vec_IntFind( vIntBuff, nidbuff ) != -1) continue;
                else {
                    printf("UNSAT in shrink level %d\n", val);
                    if (Cec_ManFdUpdateStat( pMan, nid, vIntBuff, pTemp, fStrict )) { 
                        // no need to be smaller, not to be huge is enough
                        Vec_IntWriteEntry( pMan->vConf, nid, Vec_IntEntry(vStat, 0));
                        printf("Found FD with shrink level %d with patch size %d\n", 
                            val, Gia_ManAndNum((Gia_Man_t*) Vec_PtrEntry( pMan->vVeryStat, nid )));
                        if (fStrict == 0 && Vec_IntEntry(pMan->vStat, nid) > 0) toShrink = 0;
                    } else {
                        printf("not better\n");
                    }
                    break;
                }
            }
            if (i == Vec_IntSize(vSupport)) {
                printf("Not shrinked in support\n");
                if (Vec_IntEntry( pMan->vStat, nid ) == CEC_FD_TRIVIAL) 
                    Vec_IntWriteEntry( pMan->vStat, nid, CEC_FD_UNSHRINKABLE );
                toShrink = 0;
            }
            Gia_ManStop(pTemp);

        } else {
            if (pMan->pPars->fVerbose) printf("%s: %d\n", pTemp ? "UNSOLVE in shrink level" : "SAT in shrink level", val);
        }
        val += 1;
    }

    if (pMan->pPars->fVerbose) Abc_PrintTime( -1, "Total Shrink time", Abc_Clock() - clk );
    Vec_IntFree( vFdSupportSplit );
    // Vec_IntFree( vFdSupportBuff );
    Vec_IntFree( vIntBuff );
    Vec_IntFree( vStat );


}
// find a patch with fewest frontier nodes
void Cec_ManFdShrinkSimple( Cec_ManFd_t* pMan, int nid ) {
    assert( Gia_ObjColors( pMan->pGia, nid ) == 1 || Gia_ObjColors( pMan->pGia, nid ) == 2 );
    if( Cec_ManFdToSolve( pMan, nid ) == 0 ) return;
    int cktOth = Gia_ObjColors( pMan->pGia, nid ) == 1 ? 2 : 1;
    int nidSup, i, buf, flag;
    int clean_support = 0;
    Vec_Int_t* vIntBuff;
    Vec_Int_t* vFdSupportNew = Vec_WecEntry(pMan->vGSupport, nid);
    Vec_Int_t* vFdSupportFrt = Vec_IntAlloc( 8 );
    Gia_Man_t* patch;// , patchOpt;

    Vec_IntForEachEntry( vFdSupportNew, nidSup, i ) 
        if ( Gia_ObjColors( pMan->pGia, nidSup ) == 3 ) Vec_IntPush( vFdSupportFrt, nidSup );
    vIntBuff = Vec_IntAlloc( Vec_IntSize(vFdSupportFrt) );
    Vec_IntForEachEntry( vFdSupportFrt, nidSup, i ) 
        Vec_IntPush( vIntBuff, Vec_IntEntry( cktOth == 1 ? pMan->vLevel1 : pMan->vLevel2, nidSup ) );
    Vec_IntSelectSortCost2( vFdSupportFrt->pArray, Vec_IntSize(vFdSupportFrt), vIntBuff->pArray );
    Vec_IntFree( vIntBuff );

    flag = 0;
    Vec_IntForEachEntry( vFdSupportFrt, nidSup, i ) {
        buf = Vec_IntFind( vFdSupportNew, nidSup );
        if (buf != -1) {
            Vec_IntDrop( vFdSupportNew, buf );
            patch = Cec_ManFdGetFd( pMan, nid, vFdSupportNew, 0 );
            if (patch != NULL && patch != 1 && patch != 2) {
                // Gia_ManStop(Cec_ManPatchCleanSupport( patch, vFdSupportNew ));
                // Gia_ManStop(patch);
                flag = 1;
            } else {
                Vec_IntPush( vFdSupportNew, nidSup );
            }
        }
    }

    if (flag == 1 || Vec_IntEntry(pMan->vStat, nid) == CEC_FD_UNKNOWN) {
        Vec_IntWriteEntry( pMan->vStat, nid, CEC_FD_UNKNOWN );
        Cec_ManFdCleanOneUnknown( pMan, nid );
    } else {
        if (Vec_IntEntry(pMan->vStat, nid) == CEC_FD_TRIVIAL) 
            Vec_IntWriteEntry( pMan->vStat, nid, CEC_FD_UNSHRINKABLE );
    }

    if (clean_support == 1) {
        Cec_ManFdCleanSupport( pMan, nid );
    }
}



void Cec_ManFdCleanSupport( Cec_ManFd_t* pMan, int nid ) {

    int stat = Vec_IntEntry(pMan->vStat, nid);
    Gia_Man_t *patch = (Gia_Man_t*) Vec_PtrEntry( pMan->vVeryStat, nid );
    Gia_Man_t *patchClean;
    Gia_Obj_t* pObj;
    int i, val;
    if (stat <= 0) return;
    // printf("size:%d\n", Gia_ManAndNum(patch));
    patchClean = Cec_ManPatchCleanSupport( patch, Vec_WecEntry(pMan->vGSupport, nid) );
    Gia_ManStop( patch );
    Vec_PtrWriteEntry( pMan->vVeryStat, nid, patchClean );
}
void Cec_ManFdIncreIter( Cec_ManFd_t* pMan ) {
    int stat, statSup, nid, nidSup, i;
    // pMan->iter += 1;
    Vec_IntForEachEntry( pMan->vStat, stat, nid ) {
      if (stat == pMan->iter) {
        Cec_ManFdCleanSupport( pMan, nid );
        // TODO ? turn covered nodes to _COVER
        Vec_IntForEachEntry( Vec_WecEntry(pMan->vGSupport, nid), nidSup, i ) { // TODO: only lock the support is not enough
          if (nidSup == -1) continue;
          statSup = Vec_IntEntry( pMan->vStat, nidSup );
          if ( statSup < 0 && statSup != CEC_FD_IGNORE ) Vec_IntWriteEntry( pMan->vStat, nidSup, CEC_FD_LOCK );
        }
      }
      // if (stat <= 0 && stat != CEC_FD_IGNORE) Vec_IntWriteEntry( pMan->vStat, nid, CEC_FD_LOCK );
    } 
    pMan->iter += 1;
}
Gia_Man_t*    Cec_ManFdReplaceOne( Cec_ManFd_t* pMan, int nid ) {
  Vec_Int_t* vIntBuff = Vec_IntAlloc( 1 );
  Gia_Man_t* pOut;
  Vec_IntPush( vIntBuff, nid );
  pOut = Cec_ManFdReplaceMulti(pMan, vIntBuff);
  Vec_IntFree( vIntBuff );
  return pOut;
}
Gia_Man_t*  Cec_ManFdReplaceMulti( Cec_ManFd_t* pMan, Vec_Int_t* vF ) {
    Gia_Man_t *pGia = pMan->pGia;
    Gia_Man_t *pOut, *pTemp, *pTemp2, *pFdPatch, *pGia_forCp;
    Gia_Obj_t *pObjbuf, *pSupportbuf, *pObjCp;
    Vec_Ptr_t *vNdCone;
    Vec_Int_t *vIntBuff;
    Vec_Wec_t *vFdSupport = pMan->vGSupport;
    Cec_ParCec_t ParsCec, * pPars = &ParsCec;
    int i, j, nid, nidSup, buf, fDebug = 0, fVerb = 0, val, nid1, nid2;

    // assert( Vec_PtrSize(vGiaObj) == Vec_WecSize(vFdSupport) );
    pOut = Gia_ManStart( Gia_ManObjNum(pGia) * (Vec_IntSize(vF) + 1 ));
    pGia_forCp = Gia_ManDup( pGia );
    nid1 = Gia_ObjId( pGia, pMan->pObj1 );
    nid2 = Gia_ObjId( pGia, pMan->pObj2 );
    Cec_ManCecSetDefaultParams( pPars );

    // Gia_SelfDefShow( pOut, "test3.dot", 0, 0, 0 );
    // Vec_IntPrint(vF);
    Gia_ManFillValue( pGia_forCp );
    Gia_ManConst0(pGia_forCp)->Value = 0;
    Cec_ManSubcircuit( pGia_forCp, Gia_ManCo(pGia_forCp, 0), vF, 0, 1, 0 );
    if (Gia_ManCoNum(pGia_forCp) == 2)
        Cec_ManSubcircuit( pGia_forCp, Gia_ManCo(pGia_forCp, 1), vF, 0, 1, 0 );
    // Gia_ManForEachAndId( pGia_forCp)
    Vec_IntForEachEntry( vF, nid, i ) {
        if (Gia_ObjValue(Gia_ManObj(pGia_forCp, nid)) == -1) {
            Vec_IntWriteEntry( vF, i, -1 );
        }
    }


    // vPI = Vec_IntAlloc( Gia_ManCiNum(pGia_forCp) );
    // Gia_ManForEachCi( pGia_forCp, pObjbuf, i ) {
    //     Vec_IntPush( vPI, Gia_ObjId(pGia_forCp, pObjbuf) );
    // }

    Gia_ManFillValue( pGia_forCp );
    Gia_ManConst0(pGia_forCp)->Value = 0;
    Gia_ManForEachCi( pGia_forCp, pObjbuf, i ) {
        Gia_ObjSetValue(pObjbuf, Gia_ManAppendCi( pOut ));
    }
    // printf("Replace nodes: ");
    Vec_IntForEachEntry( vF, nid, i ) {
        if (nid < 0) continue;
        // printf("%d, ", nid);
        pObjbuf = Gia_ManObj(pGia, nid);
        pObjCp = Gia_ManObj(pGia_forCp, nid);
        // printf("Replaced nid = %d\n", nid);
        // Vec_IntPrint( Vec_WecEntry(pMan->vMergeSupport, nid) );
        if (pObjCp->Value != -1) {
            printf("node %d is copied\n", nid);
            assert(0);
            // return NULL;
            continue;
        }

        // doing Fd use pGia
        pFdPatch = (Gia_Man_t*) Vec_PtrEntry( pMan->vVeryStat, nid );
        assert(pFdPatch != NULL);

        // pFdPatch = Cec_ManSimpSyn( pTemp = pFdPatch, pMan->pPars->fVerbose ); 
        
        Gia_ManFillValue( pFdPatch );
        Gia_ManConst0(pFdPatch)->Value = 0;
        // vPIpatch = Vec_IntAlloc( Gia_ManCiNum(pFdPatch) );
        if (Vec_IntSize(Vec_WecEntry(vFdSupport, nid)) != Gia_ManCiNum(pFdPatch)) {
            Vec_IntPrint(Vec_WecEntry(vFdSupport, nid));
            printf("error: patch %d has %d inputs, not %d\n", nid, Gia_ManCiNum(pFdPatch), Vec_IntSize(Vec_WecEntry(vFdSupport, nid)));
            assert(0); // problem here
        }
        Gia_ManForEachCi( pFdPatch, pSupportbuf, j ) {
            // Vec_IntPush( vPIpatch, Gia_ObjId(pFdPatch, pSupportbuf) );
            // Gia_ObjSetValue(pSupportbuf, Gia_ManAppendCi( pOut ));
            nidSup = Vec_IntEntry( Vec_WecEntry(vFdSupport, nid), j );
            // printf("Patch Ci nid = %d\n", nidSup);
            // Vec_IntPrint( Vec_WecEntry(pMan->vMergeSupport, nidSup) );
            if (nidSup == -1) continue;
            vNdCone = Cec_ManSubcircuit( pGia_forCp, Gia_ManObj(pGia_forCp, nidSup), 0, 0, 0, 1 );
            // Vec_IntPrint(vNdCone);
            if (Vec_PtrSize(vNdCone) == 0) buf = Gia_ManObj(pGia_forCp, nidSup)->Value;
            else buf = Cec_ManPatch( pOut, vNdCone );
            Gia_ObjSetValue(pSupportbuf, buf);
            // printf("nid = %d\n", nidSup);
            // printf("buf = %d\n", buf);
            Vec_PtrFree( vNdCone );
        }
    
        // Vec_IntForEachEntry( vPIpatch, nid, j ) {
        //     printf("nid = %d\n", nid);
        //     printf("copy lit = %d\n", Gia_ObjValue(Gia_ManObj(pFdPatch, nid)));
        // }
        // Vec_IntPrint(vPIpatch);
        vNdCone = Cec_ManSubcircuit( pFdPatch, Gia_ManCo(pFdPatch, 0), 0, 0, 0, 1 );
        // Vec_IntPrint(vNdCone);
        if (Vec_PtrSize(vNdCone) == 0) buf = Gia_ManCo(pFdPatch, 0)->Value;
        else buf = Cec_ManPatch( pOut, vNdCone );
        Gia_ObjSetValue( pObjCp, buf );
        // printf("nid = %d, copied = %d\n", nid, Abc_Lit2Var(buf));
        // Gia_SelfDefShow( pOut, "test2.dot", 0, 0, 0 );
        // assert(0);
        
        Vec_PtrFree( vNdCone );
        // Vec_IntFree( vPIpatch );
    }
    // printf("\n");
    // printf("done replacement\n");


    vNdCone = Cec_ManSubcircuit( pGia_forCp, Gia_ManCo(pGia_forCp, 0), 0, 0, 0, 1 );
    // printf("1\n");
    if (Vec_PtrSize(vNdCone) == 0) buf = Gia_ManCo(pGia_forCp, 0)->Value;
    else buf = Cec_ManPatch( pOut, vNdCone );
    // printf("2\n");
    Gia_ManAppendCo( pOut, buf );
    // Gia_SelfDefShow( pOut, "fd_fora.dot", 0, 0, 0 );
    Vec_PtrFree( vNdCone );

    // pGia_forCp have value pOut
    if (Gia_ManCoNum(pGia_forCp) == 2 && Gia_ObjId(pGia_forCp, Gia_ObjFanin0(Gia_ManCo(pGia_forCp, 1))) != Gia_ObjId(pGia_forCp, Gia_ObjFanin0(Gia_ManCo(pGia_forCp, 0)))) {
        vNdCone = Cec_ManSubcircuit( pGia_forCp, Gia_ManCo(pGia_forCp, 1), 0, 0, 0, 1 );
        if (Vec_PtrSize(vNdCone) == 0) buf = Gia_ManCo(pGia_forCp, 1)->Value;
        else buf = Cec_ManPatch( pOut, vNdCone );
        Gia_ManAppendCo( pOut, buf );
        Vec_PtrFree( vNdCone );   
    }
    
    // Gia_ManForEachObj( pGia_forCp, pObjCp, nid ) {
    //     if (Gia_ObjValue(pObjCp) == -1) printf("newly node: %d\n", nid);
    // }
    // pGia_forCp have value pTemp, pTemp have value pOut
    pTemp = pOut;
    Gia_ManFillValue( pTemp );
    pOut = Gia_ManRehash( pTemp, 0 );
    
    // try old code here
    // vIntBuff = Vec_IntAlloc( 1 );
    // Gia_ManForEachObj( pGia_forCp, pObjCp, nid ) {
    //     if (Gia_ObjValue(pObjCp) != -1 && Gia_ObjValue(Gia_ObjCopy(pTemp, pObjCp)) != -1) {
    //         Gia_ObjSetValue( Gia_ObjCopy( pOut, Gia_ObjCopy(pTemp, pObjCp) ), Abc_Var2Lit( Gia_ObjId(pGia_forCp, pObjCp), 0 ) );
    //     }
    // }
    // c.f. 
    // print each copy
    Gia_ManFillValue( pOut );
    Cec_ManMapCopy( pOut, pTemp, 0 );
    Gia_ManFillValue( pTemp );
    Cec_ManMapCopy( pTemp, pGia_forCp, 0 );
    Cec_ManMapCopy( pOut, pTemp, 1 );

    Gia_ManForEachObj( pGia_forCp, pObjbuf, nid ) {
        pObjbuf->fMark0 = 0;
    }
    Gia_ManForEachObj( pOut, pObjbuf, nid ) {
        // printf("%d is copied from %d\n", nid, Gia_ObjId(pGia_forCp, Gia_ObjCopy(pGia_forCp, pObjbuf)));
        if (Gia_ObjIsCo(pObjbuf)) continue;
        if (Gia_ObjValue(pObjbuf) != -1) {
            if (Gia_ObjCopy( pGia_forCp, pObjbuf )->fMark0 == 0) Gia_ObjCopy( pGia_forCp, pObjbuf )->fMark0 = 1;
            else printf("nid %d is double copied with val %d\n", nid, Abc_Lit2Var(Gia_ObjValue(pObjbuf)));
            // if (Abc_Lit2Var(Gia_ObjValue(pObjbuf)) == 2269) printf("nid %d\n", nid);
        } else {
            // printf("nid %d is not a copied node\n", nid);
        }
    }
    Gia_ManForEachObj( pGia_forCp, pObjbuf, nid ) {
        pObjbuf->fMark0 = 0;
    }
    
    if (pMan->pPars->fSyn) {
        // pOut = Cec_ManFraigSyn( pTemp = pOut, 
        //             Gia_ObjCopy( pOut, Gia_ManObj( pGia_forCp, nid1)), 
        //             Gia_ObjCopy( pOut, Gia_ManObj( pGia_forCp, nid2) ), 
        //             pMan->fPhase, pMan->pPars->fVerbose, pMan->pPars->nBTLimit, 1 );
        // Gia_ManStop( pTemp );
    }
    Gia_ManStop( pGia_forCp );

    if (fDebug) {
        Gia_ManAppendCo( pOut, buf );
        printf("#AND: %d, %d\n", Gia_ManAndNum(pGia), Gia_ManAndNum(pOut));
        pTemp = Gia_ManMiter( pGia, pOut, 0, 1, 0, 0, 0 );
        if (Cec_ManVerify( pTemp, pPars ) == 0) {
            Vec_IntPrint(vFdSupport);
            Gia_AigerWrite( pGia, "fd_org.aig", 0, 0, 0 );
            Gia_SelfDefShow( pGia, "fd_org.dot", 0, 0, 0 );
            Gia_SelfDefShow( pTemp, "fd_miter.dot", 0, 0, 0 );
            Gia_AigerWrite( pOut, "fd_new.aig", 0, 0, 0 );
            Gia_SelfDefShow( pOut, "fd_new.dot", 0, 0, 0 );
            assert(0);
        }
        Gia_ManStop( pTemp );
        printf("successfully replace\n");
        // assert(0);
    }
    return pOut;
}

void Cec_ManFdCoverFrt( Cec_ManFd_t* pMan, int fCkt ) {
    assert(fCkt == 1 || fCkt == 2);
    // TODO:
    // Find the frt node that is farest (largest level) to the root
    // Find the cut that covers the frt node, or the G support that covers the frt node
    // ---- compute cone in TFO of frt and TFI of ckt (frt should probably only connect to one PO in pAbs)
    // based on picked cut rewrited nodes, find other frt node to cover
    Vec_Int_t *vF, *vG, *vCoverFrt, *vFrtLv, *vCutBuf, *vCutBuf2, *vCoverNd, *vIntBuff;
    Vec_Wec_t *vCut;
    Vec_Wec_t *vGtoF;
    Vec_Ptr_t *vNdCone;
    Gia_Man_t *pTemp;
    Gia_Obj_t *pObj;
    Gia_Obj_t *pPoSelf = fCkt == 1 ? pMan->pObjAbs1 : pMan->pObjAbs2;
    Gia_Obj_t *pPoOth = fCkt == 1 ? pMan->pObjAbs2 : pMan->pObjAbs1;
    int i, j, k, nid, nid2, entry, entry2;
    vF = Vec_IntAlloc( 100 );
    vG = Vec_IntAlloc( 100 );
    vGtoF = Vec_WecAlloc( 100 );

    // record unsat nodes and their G support
    Vec_IntForEachEntry( pMan->vStat, entry, nid ) {
        // printf("nid = %d\n", nid);
        if (entry > 0 && (Gia_ObjColors(pMan->pGia, nid) == fCkt)) { // stat count from 1
            Vec_IntPush( vF, Cec_ManFdMapIdSingle( pMan, nid, 1 ) );
            Vec_IntForEachEntry( Vec_WecEntry(pMan->vGSupport, nid), nid2, i ) {
                // printf("....nid2 = %d\n", nid2);
                j = Vec_IntFind( vG, Cec_ManFdMapIdSingle( pMan, nid2, 1 ) );
                if (j == -1) {
                    Vec_IntPush( vG, Cec_ManFdMapIdSingle( pMan, nid2, 1 ) );
                    Vec_IntPush( Vec_WecPushLevel( vGtoF ), Cec_ManFdMapIdSingle( pMan, nid, 1 ) );
                } else {
                    Vec_WecPush( vGtoF, j, Cec_ManFdMapIdSingle( pMan, nid, 1 ) );
                }
            }
        }
    }

    Gia_ManFillValue( pMan->pAbs );
    Cec_ManSubcircuit( pMan->pAbs, pPoSelf, 0, 0, -1, 0 ); // 0 iff connect to POself in abs
    Cec_ManSubcircuit( pMan->pAbs, pPoSelf, vF, 0, -2, 0 ); // 0 iff connect to POself with vF in abs
    Cec_ManSubcircuit( pMan->pAbs, pPoOth, 0, 0, -4, 0 ); // 0 iff connect to POoth in abs
    Cec_ManSubcircuit( pMan->pAbs, pPoOth, vG, 0, -8, 0 ); // 0 iff connect to POoth with vG in abs (i.e. connect to any node in 2 directly)
    // Vec_IntForEachEntry( vG, nid, i ) {
    //     Cec_ManSubcircuit( pMan->pAbs, Gia_ManObj( pMan->pAbs, nid ), 0, 0, 8, 0 ); // 0 iff in vG's TFI
    // }

    // find frt nodes that are covered, sort by level
    vCoverFrt = Vec_IntAlloc( 100 );
    vFrtLv = Vec_IntAlloc( 100 );
    Gia_ManForEachCi( pMan->pAbs, pObj, i ) {
        nid = Gia_ObjId(pMan->pAbs, pObj);
        printf("frt label = %d\n", ~Gia_ObjValue(pObj));
        if ((~(Gia_ObjValue(pObj)) & 3) == 1) {
            Vec_IntPush( vCoverFrt, nid );
            Vec_IntPush( vFrtLv, Vec_IntEntry( pMan->vLevel1, nid ) );
        }
        if ((~(Gia_ObjValue(pObj)) & 12) == 4) { // use negative value to record nid with 
            Vec_IntPush( vCoverFrt, -nid );
            Vec_IntPush( vFrtLv,  Vec_IntEntry( pMan->vLevel2, nid ) );
        }
    }
    Vec_IntSelectSortCost2Reverse( vCoverFrt->pArray, Vec_IntSize(vCoverFrt), vFrtLv->pArray );

    nid = abs(Vec_IntEntry( vCoverFrt, 0 ));
    if (Vec_IntEntry( vCoverFrt, 0 ) > 0) {
        
    } else {

    }

    // TODO : Find all cuts <= k by FPT
    // line "subcircuit" is problemetic
    // vIntBuff = Vec_IntAlloc( 1 );
    // vCoverNd = Vec_IntAlloc( 100 );
    // vCut = Vec_WecAlloc( 100 );
    // // check cut that with only one node
    // Vec_IntForEachEntry( vCoverFrt, entry, i ) {
    //     nid = abs(entry);
    //     printf("nid = %d\n", nid);
    //     if (entry > 0) {
    //         Gia_ManFillValue( pMan->pAbs );
    //         Vec_IntPush( vIntBuff, nid );
    //         Cec_ManSubcircuit( pMan->pAbs, pPoSelf, 0, vIntBuff, -1, 0 );
    //         Vec_IntForEachEntry( vF, nid2, j ) 
    //             if (Gia_ObjValue(Gia_ManObj(pMan->pAbs, nid2)) != -1) Vec_IntPush( vIntBuff, nid2 );
    //         Vec_IntPrint( vIntBuff );
    //         Gia_ManFillValue( pMan->pAbs );
    //         Cec_ManSubcircuit( pMan->pAbs, pPoSelf, vIntBuff, vIntBuff, 1, 0 );
    //         Vec_IntClear( vIntBuff );
    //         // find one cut only
    //         // vCutBuf = Vec_WecPushLevel( vCut );
    //         Vec_IntForEachEntry( vF, nid2, j ) 
    //             if (Gia_ObjValue(Gia_ManObj(pMan->pAbs, nid2)) != -1) printf("%d ", nid2);
    //         printf("\n");            
    //     } else {
    //         Gia_ManFillValue( pMan->pAbs );
    //         Vec_IntPush( vIntBuff, nid );
    //         Cec_ManSubcircuit( pMan->pAbs, pPoOth, 0, vIntBuff, -1, 0 );
    //         Vec_IntClear( vIntBuff );
    //         Vec_IntForEachEntry( vG, nid2, j ) 
    //             if (Gia_ObjValue(Gia_ManObj(pMan->pAbs, nid2)) != -1) Vec_IntPush( vIntBuff, nid2 );
    //         Vec_IntPrint( vIntBuff );
    //         Gia_ManFillValue( pMan->pAbs );
    //         Cec_ManSubcircuit( pMan->pAbs, pPoOth, vIntBuff, vIntBuff, 1, 0 );
    //         Vec_IntClear( vIntBuff );
    //         // find one cut only
    //         // vCutBuf = Vec_WecPushLevel( vCut );
    //         Vec_IntForEachEntry( vG, nid2, j ) 
    //             if (Gia_ObjValue(Gia_ManObj(pMan->pAbs, nid2)) != -1) {
    //                 printf("----%d\n", nid2);
    //                 Vec_IntPrint( Vec_WecEntry( vGtoF, j ) );
    //             }
    //         printf("\n");
    //     }
    //     Vec_WecClear( vCut );
    // }
}

Gia_Man_t* Cec_ManFdVisSupport( Cec_ManFd_t* pMan, int nid, int fPatch ) {
    Gia_Man_t* pOut = Gia_ManStart( Gia_ManObjNum(pMan->pGia) );
    Gia_Man_t* pFdPatch = (Gia_Man_t*) Vec_PtrEntry( pMan->vVeryStat, nid );
    Vec_Ptr_t* vNdCone;
    Gia_Obj_t* pObj;
    int i, nidbuf, buf;

    if (fPatch && (pFdPatch == NULL || pFdPatch == 1 || pFdPatch == 2)) {
        printf("Node %d has no patch\n", nid);
        return NULL;
    } else {
        Gia_ManFillValue( pFdPatch );
    }

    Gia_ManFillValue( pMan->pGia );
    Gia_ManForEachCi( pMan->pGia, pObj, i ) {
        Gia_ObjSetValue(pObj, Gia_ManAppendCi( pOut ));
    }

    // printf("Support nodes of node %d:\n", nid);
    // Vec_IntPrint( Vec_WecEntry( pMan->vGSupport, nid ) );
    Vec_IntForEachEntry( Vec_WecEntry(pMan->vGSupport, nid), nidbuf, i ) {
        if (nidbuf == -1) continue;
        vNdCone = Cec_ManSubcircuit( pMan->pGia, Gia_ManObj(pMan->pGia, nidbuf), 0, 0, 0, 1 );
        if (Vec_PtrSize(vNdCone) == 0) buf = Gia_ManObj(pMan->pGia, nidbuf)->Value;
        else buf = Cec_ManPatch( pOut, vNdCone );
        if (fPatch) Gia_ObjSetValue( Gia_ManCi(pFdPatch, i), buf );
        Vec_PtrFree( vNdCone );
    }

    if (fPatch) {
        // Gia_SelfDefShow( pFdPatch, "fd_patch.dot", 0, 0, 0 );
        vNdCone = Cec_ManSubcircuit( pFdPatch, Gia_ManCo(pFdPatch, 0), 0, 0, 0, 1 );
        assert(Vec_PtrSize(vNdCone) != 0);
        buf = Cec_ManPatch( pOut, vNdCone );
        buf = Gia_ManAppendCo( pOut, buf );
        Gia_ObjSetValue( Gia_ManObj(pMan->pGia, nid), buf );
    } else {
        vNdCone = Cec_ManSubcircuit( pMan->pGia, Gia_ManObj(pMan->pGia, nid), 0, 0, 0, 1 );
        assert(Vec_PtrSize(vNdCone) != 0);
        buf = Cec_ManPatch( pOut, vNdCone );
        buf = Gia_ManAppendCo( pOut, buf );
        Gia_ObjSetValue( Gia_ManObj(pMan->pGia, nid), buf );
    }
    
    return pOut;
}
// fAbs = 1 iff pGia == pAbs; fVecAbs = 1 iff vColor on Abs; fColor >> 1 is Vec_IntColorize (0 for simply one color); fColor & 1 is Colorize ignore min 
void Cec_ManFdPlot( Cec_ManFd_t* pMan, int fAbs, int fVecAbs, int fColor, char* pFileName, Vec_Int_t* vColor1, Vec_Int_t* vColor2, Vec_Int_t* vColor3 ) {
  Vec_Int_t *vIntBuff1, *vIntBuff2, *vIntBuff3, *vIntBuff;
  Gia_Man_t *pGia;
  if (fAbs == 1) {
    pGia = pMan->pAbs;
    if (fVecAbs != fAbs) {
      if (vColor1) {
        if (fColor) vIntBuff1 = Cec_ManFdMapAttr( pMan, vColor1, 1 );
        else Cec_ManFdMapId( pMan, vIntBuff1 = Vec_IntDup( vColor1 ), 1 );
      }
      if (vColor2) {
        if (fColor) vIntBuff2 = Cec_ManFdMapAttr( pMan, vColor2, 1 );
        else Cec_ManFdMapId( pMan, vIntBuff2 = Vec_IntDup( vColor2 ), 1 );
      }
      if (vColor3) {
        if (fColor) vIntBuff3 = Cec_ManFdMapAttr( pMan, vColor3, 1 );
        else Cec_ManFdMapId( pMan, vIntBuff3 = Vec_IntDup( vColor3 ), 1 );
      }
    } else {
        vIntBuff1 = vColor1 ? Vec_IntDup( vColor1 ) : 0;
        vIntBuff2 = vColor2 ? Vec_IntDup( vColor2 ) : 0;
        vIntBuff3 = vColor3 ? Vec_IntDup( vColor3 ) : 0;
    }
  } else {
    pGia = pMan->pGia;
    if (fVecAbs != fAbs) {
      if (vColor1) {
        if (fColor) vIntBuff1 = Cec_ManFdMapAttr( pMan, vColor1, 0 );
        else Cec_ManFdMapId( pMan, vIntBuff1 = Vec_IntDup( vColor1 ), 0 );
      }
      if (vColor2) {
        if (fColor) vIntBuff2 = Cec_ManFdMapAttr( pMan, vColor2, 0 );
        else Cec_ManFdMapId( pMan, vIntBuff2 = Vec_IntDup( vColor2 ), 0 );
      }
      if (vColor3) {
        if (fColor) vIntBuff3 = Cec_ManFdMapAttr( pMan, vColor3, 0 );
        else Cec_ManFdMapId( pMan, vIntBuff3 = Vec_IntDup( vColor3 ), 0 );
      }
    } else {
        vIntBuff1 = vColor1 ? Vec_IntDup( vColor1 ) : 0;
        vIntBuff2 = vColor2 ? Vec_IntDup( vColor2 ) : 0;
        vIntBuff3 = vColor3 ? Vec_IntDup( vColor3 ) : 0;
    }
  }
  if (fColor) {
    if (vIntBuff1) {
        vIntBuff1 = Vec_IntColorize( vIntBuff = vIntBuff1, fColor >> 1, fColor & 1 ? -1 : 0 );
        Vec_IntFree( vIntBuff );
    }
    if (vIntBuff2) {
        vIntBuff2 = Vec_IntColorize( vIntBuff = vIntBuff2, fColor >> 1, fColor & 1 ? -1 : 0 );
        Vec_IntFree( vIntBuff );
    }
    if (vIntBuff3) {
        vIntBuff3 = Vec_IntColorize( vIntBuff = vIntBuff3, fColor >> 1, fColor & 1 ? -1 : 0 );
        Vec_IntFree( vIntBuff );
    }
  } else {
    if (vIntBuff1) {
        vIntBuff1 = Vec_IntArgToBin( vIntBuff = vIntBuff1, 0 );
        Vec_IntFree( vIntBuff );
        Vec_IntFillExtra( vIntBuff1, Gia_ManObjNum(pGia), 0 );
        Vec_IntReplace( vIntBuff1, 1, 255 );
    }
    if (vIntBuff2) {
        vIntBuff2 = Vec_IntArgToBin( vIntBuff = vIntBuff2, 0 );
        Vec_IntFree( vIntBuff );
        Vec_IntFillExtra( vIntBuff2, Gia_ManObjNum(pGia), 0 );
        Vec_IntReplace( vIntBuff2, 1, 255 );
    }
    if (vIntBuff3) {
        vIntBuff3 = Vec_IntArgToBin( vIntBuff = vIntBuff3, 0 );
        Vec_IntFree( vIntBuff );
        Vec_IntFillExtra( vIntBuff3, Gia_ManObjNum(pGia), 0 );
        Vec_IntReplace( vIntBuff3, 1, 255 );
    }
  }
  Gia_ShowColor( pGia, pFileName, vIntBuff1, vIntBuff2, vIntBuff3 );
  
}
void Cec_ManFdReport( Cec_ManFd_t* pMan ) {
    int nid, i, color, stat, check, nid2, j, is_cec = pMan->pPars->fSyn, cnt_xor, cnt_mux, cnt, buf;
    float val;
    char* str = ABC_ALLOC(char, 100);
    Vec_Int_t * vUnsat, *vUnsat_Abs, *vSat, *vSat_Abs, *vUnsatAll, *vUnsatAll_Abs, *vUnsat_Abs_stat0; 
    Vec_Int_t * vIntBuff, *vIntBuff1, *vIntBuff2, *vIntBuff3, *vIntBuff4;
    Vec_Wec_t * vWecBuff;
    Vec_Ptr_t * vObjBuff;
    Gia_Obj_t * pObj;
    Gia_Man_t * pTemp;
    printf("-------------------- start Report --------------------\n");
    Cec_ManFdPrintPars( pMan );
    pMan->pPars->fSyn = 0;
    printf("origin AIG:\n");
    printf("#AIG = %d, #CI = %d\n", Gia_ManAndNum(pMan->pGia), Gia_ManCiNum(pMan->pGia));
    vWecBuff = Cec_ManDistAnalyze( pMan->pGia, 0, 0, 0, pMan->dist_long, pMan->dist_short);
    printf("pObj1, pObj2 = %d, %d\n", Gia_ObjId(pMan->pGia, pMan->pObj1), Gia_ObjId(pMan->pGia, pMan->pObj2));
    printf("dist from pObj1 to PI: max = %d, min = %d\n", 
        Vec_WecEntryEntry(vWecBuff, Gia_ObjId(pMan->pGia, pMan->pObj1), 4), 
        Vec_WecEntryEntry(vWecBuff, Gia_ObjId(pMan->pGia, pMan->pObj1), 9));
    printf("dist from pObj2 to PI: max = %d, min = %d\n", 
        Vec_WecEntryEntry(vWecBuff, Gia_ObjId(pMan->pGia, pMan->pObj2), 4), 
        Vec_WecEntryEntry(vWecBuff, Gia_ObjId(pMan->pGia, pMan->pObj2), 9));
    printf("dist_analyze (nid, ckt, max_long_FO, min_long_FO, max_long_FI, min_long_FI, max_short_FO, min_short_FO, max_short_FI, min_short_FI):\n");
    Vec_WecForEachLevel( vWecBuff, vIntBuff, i ) 
        if (i != Vec_WecSize(vWecBuff) - 1) Vec_IntPrint(vIntBuff);
    Vec_WecFree( vWecBuff );

    printf("Abstracted AIG:\n");
    printf("merge frt:\n");
    Cec_ManDistEasy( pMan->pGia, pMan->vMerge, Gia_ObjId(pMan->pGia, pMan->pObj1), 1 );
    vIntBuff = Cec_ManCountVal( pMan->pGia, pMan->vMerge );
    Vec_IntForEachEntry( vIntBuff, cnt, j ) {
        printf("vLevel1[%d]=%d\n", j, cnt);
    }
    Cec_ManDistEasy( pMan->pGia, pMan->vMerge, Gia_ObjId(pMan->pGia, pMan->pObj2), 1 );
    vIntBuff2 = Cec_ManCountVal( pMan->pGia, pMan->vMerge );
    Vec_IntForEachEntry( vIntBuff2, cnt, j ) {
        printf("vLevel2[%d]=%d\n", j, cnt);
    }
    Vec_IntFree( vIntBuff );
    Vec_IntFree( vIntBuff2 );
    printf("#AIG = %d, #CI = %d\n", Gia_ManAndNum(pMan->pAbs), Gia_ManCiNum(pMan->pAbs));
    vWecBuff = Cec_ManDistAnalyze( pMan->pAbs, 0, 0, 0, pMan->dist_long_abs, pMan->dist_short_abs);
    printf("pObj1, pObj2 = %d, %d\n", Gia_ObjId(pMan->pAbs, pMan->pObjAbs1), Gia_ObjId(pMan->pAbs, pMan->pObjAbs2));
    printf("dist from pObj1 to PI: max = %d, min = %d\n", 
        Vec_WecEntryEntry(vWecBuff, Gia_ObjId(pMan->pAbs, pMan->pObjAbs1), 4), 
        Vec_WecEntryEntry(vWecBuff, Gia_ObjId(pMan->pAbs, pMan->pObjAbs1), 9));
    printf("dist from pObj2 to PI: max = %d, min = %d\n", 
        Vec_WecEntryEntry(vWecBuff, Gia_ObjId(pMan->pAbs, pMan->pObjAbs2), 4), 
        Vec_WecEntryEntry(vWecBuff, Gia_ObjId(pMan->pAbs, pMan->pObjAbs2), 9));
    printf("dist_analyze:\n");
    printf("(nid, ckt, max_long_FO, min_long_FO, max_long_FI, min_long_FI, max_short_FO, min_short_FO, max_short_FI, min_short_FI):\n");
    Vec_WecForEachLevel( vWecBuff, vIntBuff, i )
        if (i != Vec_WecSize(vWecBuff) - 1) Vec_IntPrint(vIntBuff);
    Vec_WecFree( vWecBuff );

    printf("detail info\n (*nid, _nidabs, color, level_1, level_2, FI1, FI2, MFFC_abs, cost, costTh)\n");
    if (pMan->pAbs->pRefs == NULL) Gia_ManCreateRefs( pMan->pAbs );
    Gia_ManForEachObj( pMan->pGia, pObj, nid ) {
        nid2 = Cec_ManFdMapIdSingle(pMan, nid, 1);
        if (Gia_ObjIsAnd(pObj)) {
            if (nid2 == -1) 
                printf("*%d, --, %d, %d, %d, %d, %d, --, --, --\n", nid, Vec_IntEntry(pMan->vLevel1, nid), Vec_IntEntry(pMan->vLevel2, nid),
                Gia_ObjColors(pMan->pGia, nid), Gia_ObjFaninId0(pObj, nid), Gia_ObjFaninId1(pObj, nid));
            else
                printf("*%d, _%d, %d, %d, %d, %d, %d, %d, %f, %f\n", nid, nid2, Vec_IntEntry(pMan->vLevel1, nid), Vec_IntEntry(pMan->vLevel2, nid), 
                Gia_ObjColors(pMan->pGia, nid), Gia_ObjFaninId0(pObj, nid), Gia_ObjFaninId1(pObj, nid), Gia_NodeMffcSize( pMan->pAbs, Gia_ManObj(pMan->pAbs, nid2) ),
                Cec_ManFdRawCost( pMan, nid, 0, 0 ), Vec_FltEntry(pMan->vCostTh, nid));
        } else if (Gia_ObjIsCi(pObj)) {
            printf("*%d, --, --, %d, %d, --, --, --, --\n", nid, Vec_IntEntry(pMan->vLevel1, nid), Vec_IntEntry(pMan->vLevel2, nid));
        }
    }

    vUnsat = Cec_ManFdUnsatId( pMan, 0, 0 );
    vUnsat_Abs_stat0 = Vec_IntAlloc( 100 );
    printf("UNSAT (rewritable): \n");
    Vec_IntForEachEntry( vUnsat, nid, i ) {
        color = Gia_ObjColors(pMan->pGia, nid);
        assert(color == 1 || color == 2);
        printf("%d (%d): color = %d, level = %d, conflict = %d, cost = %f, costThreshold = %f\n", 
                nid, Cec_ManFdMapIdSingle(pMan, nid, 1), color, 
                color == 1 ? Vec_IntEntry(pMan->vLevel1, nid) : Vec_IntEntry(pMan->vLevel2, nid),
                Vec_IntEntry(pMan->vConf, nid), Cec_ManFdRawCost( pMan, nid, 0, 0 ), Vec_FltEntry(pMan->vCostTh, nid) );

        vIntBuff = (color == 1) ? pMan->vLevel1 : pMan->vLevel2;
        vObjBuff = Vec_PtrAlloc(1);
        Vec_PtrPush( vObjBuff, Gia_ManObj(pMan->pGia, nid) );
        Cec_ManFdMapObj(pMan, vObjBuff, 1);
        pTemp = Cec_ManGetTFI( pMan->pAbs, vObjBuff, 0 );
        Cec_ManDistEasy( pTemp, 0, Gia_ObjId(pTemp, Gia_ObjFanin0(Gia_ManCo(pTemp, 0))), 1 );
        Gia_ManCountMuxXor(pTemp, &cnt_xor, &cnt_mux);
        cnt = 0;
        Vec_IntForEachEntry( Vec_WecEntry(pMan->vMergeSupport, nid), check, j ) {
            if (check) { cnt+=1; }
        }
        printf("absSize = %d, absHeight = %d, #Ci = %d, #absXor = %d, #absMux = %d\n", 
            Gia_ManAndNum(pTemp), Gia_ManLevelNum(pTemp), cnt, cnt_xor, cnt_mux);
        printf("absSupport: \n");
        Vec_IntForEachEntry( Vec_WecEntry(pMan->vMergeSupport, nid), check, j ) {
            if (check) {
                nid2 = Vec_IntEntry(pMan->vMerge, j);
                printf( "%d (%d): max_dist = %d, level_self = %d\n", nid2, Cec_ManFdMapIdSingle(pMan, nid2, 1), Gia_ObjValue(Gia_ManCi(pTemp, j)),Vec_IntEntry(vIntBuff, nid2) );
            }
        }
        Gia_ManStop(pTemp);

        vIntBuff = (color == 1) ? pMan->vLevel2 : pMan->vLevel1;
        pTemp = (Gia_Man_t*) Vec_PtrEntry(pMan->vVeryStat, nid);
        Cec_ManDistEasy( pTemp, 0, Gia_ObjId(pTemp, Gia_ObjFanin0(Gia_ManCo(pTemp, 0))), 1 );
        Gia_ManCountMuxXor(pTemp, &cnt_xor, &cnt_mux);
        printf("patchSize = %d, patchHeight = %d, #Ci = %d, #patchXor = %d, #patchMux = %d\n", 
            Gia_ManAndNum(pTemp), Gia_ManLevelNum(pTemp), Gia_ManCiNum(pTemp), cnt_xor, cnt_mux );
        Vec_IntForEachEntry( Vec_WecEntry(pMan->vGSupport, nid), nid2, j ) {
            // nid2 = Vec_IntEntry(pMan->vMerge, j);
            printf( "%d (%d): max_dist = %d, level_oth = %d\n", nid2, Cec_ManFdMapIdSingle(pMan, nid2, 1), Gia_ObjValue(Gia_ManCi(pTemp, j)), Vec_IntEntry(vIntBuff, nid2) );
        }
        // Vec_IntPrint( Vec_WecEntry(pMan->vGSupport, nid) );
        if (Vec_IntEntry(pMan->vStat, nid) == 1) Vec_IntPush( vUnsat_Abs_stat0, nid );
        Vec_PtrFree( vObjBuff );
    }
    vUnsat_Abs = Vec_IntDup( vUnsat );
    Cec_ManFdMapId( pMan, vUnsat_Abs, 1 );
    Cec_ManFdMapId( pMan, vUnsat_Abs_stat0, 1 );
    vSat = Vec_IntAlloc( 100 );
    Vec_IntForEachEntry( pMan->vStat, stat, nid ) {
        // map to Abs after printing vSat
        if (stat == CEC_FD_SAT) Vec_IntPush( vSat, nid );
    }
    printf("SAT: \n");
    Vec_IntPrint( vSat );
    vSat_Abs = Vec_IntDup( vSat );
    Cec_ManFdMapId( pMan, vSat_Abs, 1 );
    printf("Huge: \n");
    vUnsatAll = Cec_ManFdUnsatId( pMan, 0, 1 );
    Vec_IntForEachEntry( vUnsatAll, nid, i ) {
        color = Gia_ObjColors(pMan->pGia, nid);
        assert(color == 1 || color == 2);
        if (Vec_IntEntry(pMan->vStat, nid) == CEC_FD_HUGE) 
            printf("%d (%d): color = %d, level = %d, cost = %f, costThreshold = %f\n", 
                nid, Cec_ManFdMapIdSingle(pMan, nid, 1), color, 
                color == 1 ? Vec_IntEntry(pMan->vLevel1, nid) : Vec_IntEntry(pMan->vLevel2, nid),
                Cec_ManFdRawCost( pMan, nid, 0, 0 ), Vec_FltEntry(pMan->vCostTh, nid) );
    }
    vUnsatAll_Abs = Vec_IntDup( vUnsatAll );
    Cec_ManFdMapId( pMan, vUnsatAll_Abs, 1 );

    if (pMan->pPars->fGetCkts) {

        Gia_SelfDefShow( pMan->pAbs, "pAbs_status.dot", vUnsatAll_Abs, vSat_Abs, 0);

        // vIntBuff3 = Vec_IntDup( pMan->vConf );
        // Vec_IntForEachEntry( pMan->vStat, stat, nid ) {
        //     if (stat == CEC_FD_UNSOLVE) Vec_IntWriteEntry( vIntBuff3, nid, -1 );
        // }
        // vIntBuff3 = Cec_ManFdMapAttr( pMan, vIntBuff = vIntBuff3, 1 );
        // Vec_IntFree( vIntBuff );
        // // Vec_IntReplace( vIntBuff, -1, 2 * pMan->pPars->nBTLimit );
        // Vec_IntColorize( vIntBuff3, 0, -1 );
        
        // vIntBuff4 = Vec_IntAlloc( 100 );
        // Vec_PtrForEachEntry( Gia_Man_t*, pMan->vVeryStat, pTemp, i ) {
        //     if (pTemp == NULL || pTemp == 1) Vec_IntPush(vIntBuff4, -1);
        //     else Vec_IntPush(vIntBuff4, Gia_ManAndNum(pTemp));
        // }
        // vIntBuff4 = Cec_ManFdMapAttr( pMan, vIntBuff = vIntBuff4, 1 );
        // Vec_IntFree( vIntBuff );
        // // buf = Vec_IntFindMax( vIntBuff );
        // // Vec_IntReplace( vIntBuff, -1, 2 * buf );
        // Vec_IntColorize( vIntBuff4, 0, -1 );

        // vIntBuff1 = Vec_IntColorize( vIntBuff3, 2, -1 );
        // Gia_ShowColor( pMan->pAbs, "pAbs_conf_ord.dot", vIntBuff1, 0, 0);
        // vIntBuff2 = Vec_IntColorize( vIntBuff4, 2, -1 );
        // Gia_ShowColor( pMan->pAbs, "pAbs_patch_ord.dot", 0, vIntBuff2, 0);
        // Gia_ShowColor( pMan->pAbs, "pAbs_c2p_ord.dot", vIntBuff1, vIntBuff2, 0);
        // Vec_IntFree( vIntBuff1 );
        // Vec_IntFree( vIntBuff2 );
        // vIntBuff1 = Vec_IntColorize( vIntBuff3, 1, -1 );
        // Gia_ShowColor( pMan->pAbs, "pAbs_conf_lin.dot", vIntBuff1, 0, 0);
        // vIntBuff2 = Vec_IntColorize( vIntBuff4, 1, -1 );
        // Gia_ShowColor( pMan->pAbs, "pAbs_patch_lin.dot", 0, vIntBuff2, 0);
        // Gia_ShowColor( pMan->pAbs, "pAbs_c2p_lin.dot", vIntBuff1, vIntBuff2, 0);
        // Vec_IntFree( vIntBuff1 );
        // Vec_IntFree( vIntBuff2 );
        // vIntBuff1 = Vec_IntColorize( vIntBuff3, 3, -1 );
        // Gia_ShowColor( pMan->pAbs, "pAbs_conf_std.dot", vIntBuff1, 0, 0);
        // vIntBuff2 = Vec_IntColorize( vIntBuff4, 3, -1 );
        // Gia_ShowColor( pMan->pAbs, "pAbs_patch_std.dot", 0, vIntBuff2, 0);
        // Gia_ShowColor( pMan->pAbs, "pAbs_c2p_std.dot", vIntBuff1, vIntBuff2, 0);
        // Vec_IntFree( vIntBuff1 );
        // Vec_IntFree( vIntBuff2 );
        // vIntBuff1 = Vec_IntColorize( vIntBuff3, 4, -1 );
        // Gia_ShowColor( pMan->pAbs, "pAbs_conf_log.dot", vIntBuff1, 0, 0);
        // vIntBuff2 = Vec_IntColorize( vIntBuff4, 4, -1 );
        // Gia_ShowColor( pMan->pAbs, "pAbs_patch_log.dot", 0, vIntBuff2, 0);
        // Gia_ShowColor( pMan->pAbs, "pAbs_c2p_log.dot", vIntBuff1, vIntBuff2, 0);
        // Vec_IntFree( vIntBuff1 );
        // Vec_IntFree( vIntBuff2 );

        // Vec_IntFree( vIntBuff3 );
        // Vec_IntFree( vIntBuff4 );

        // Gia_ShowColor();
        Gia_SelfDefShow( pMan->pGia, "pGia_UNSAT.dot", vUnsatAll, pMan->vMerge, 0);
        Gia_ColorMap("colorMap.dot");

        if (pMan->pPars->costType != CEC_FD_COSTDUMMY) Gia_SelfDefShow( pMan->pAbs, "pAbs_allUnsat.dot", vUnsat_Abs, vUnsatAll_Abs, 0);

    }
    pMan->pPars->fSyn = is_cec;
    printf("-------------------- done Report --------------------\n");
    return;
}
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

