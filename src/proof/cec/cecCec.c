/**CFile****************************************************************

  FileName    [cecCec.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Combinational equivalence checking.]

  Synopsis    [Integrated combinatinal equivalence checker.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cecCec.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cecInt.h"
#include "proof/fra/fra.h"
#include "aig/gia/giaAig.h"
#include "misc/extra/extra.h"
#include "sat/cnf/cnf.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
extern Vec_Wec_t * Gia_ManExploreCuts( Gia_Man_t * pGia, int nCutSize0, int nCuts0, int fVerbose0 );
extern void Gia_WriteDotAigSimple( Gia_Man_t * p, char * pFileName, Vec_Int_t * vRwNd );
extern void Gia_SelfDefShow( Gia_Man_t * p, char * pFileName, Vec_Int_t * vLabel0, Vec_Int_t * vLabel1, Vec_Wec_t* vRelate);
// extern Gia_Man_t * Gia_ManInterTest( Gia_Man_t * p );
////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Saves the input pattern with the given number.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManTransformPattern( Gia_Man_t * p, int iOut, int * pValues )
{
    int i;
    assert( p->pCexComb == NULL );
    p->pCexComb = Abc_CexAlloc( 0, Gia_ManCiNum(p), 1 );
    p->pCexComb->iPo = iOut;
    for ( i = 0; i < Gia_ManCiNum(p); i++ )
        if ( pValues && pValues[i] )
            Abc_InfoSetBit( p->pCexComb->pData, i );
}

/**Function*************************************************************

  Synopsis    [Interface to the old CEC engine]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec_ManVerifyOld( Gia_Man_t * pMiter, int fVerbose, int * piOutFail, abctime clkTotal, int fSilent )
{
//    extern int Fra_FraigCec( Aig_Man_t ** ppAig, int nConfLimit, int fVerbose );
    extern int Ssw_SecCexResimulate( Aig_Man_t * p, int * pModel, int * pnOutputs );
    Gia_Man_t * pTemp = Gia_ManTransformMiter( pMiter );
    Aig_Man_t * pMiterCec = Gia_ManToAig( pTemp, 0 ); // change env
    int RetValue, iOut, nOuts;
    if ( piOutFail )
        *piOutFail = -1;
    Gia_ManStop( pTemp );
    // run CEC on this miter
    RetValue = Fra_FraigCec( &pMiterCec, 10000000, fVerbose ); // Old CEC
    // RetValue = Fra_FraigCec( &pMiterCec, 100000, fVerbose );
    // report the miter
    if ( RetValue == 1 )
    {
        if ( !fSilent )
        {
            Abc_Print( 1, "Networks are equivalent.  " );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clkTotal );
        }
    }
    else if ( RetValue == 0 )
    {
        if ( !fSilent )
        {
            Abc_Print( 1, "Networks are NOT EQUIVALENT.  " );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clkTotal );
        }
        if ( pMiterCec->pData == NULL )
            Abc_Print( 1, "Counter-example is not available.\n" );
        else
        {
            iOut = Ssw_SecCexResimulate( pMiterCec, (int *)pMiterCec->pData, &nOuts );
            if ( iOut == -1 )
                Abc_Print( 1, "Counter-example verification has failed.\n" );
            else 
            {
                if ( !fSilent )
                {
                    Abc_Print( 1, "Primary output %d has failed", iOut );
                    if ( nOuts-1 >= 0 )
                        Abc_Print( 1, ", along with other %d incorrect outputs", nOuts-1 );
                    Abc_Print( 1, ".\n" );
                }
                if ( piOutFail )
                    *piOutFail = iOut;
            }
            Cec_ManTransformPattern( pMiter, iOut, (int *)pMiterCec->pData );
        }
    }
    else if ( !fSilent )
    {
        Abc_Print( 1, "Networks are UNDECIDED.  " );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clkTotal );
    }
    fflush( stdout );
    Aig_ManStop( pMiterCec );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec_ManHandleSpecialCases( Gia_Man_t * p, Cec_ParCec_t * pPars )
{
    Gia_Obj_t * pObj1, * pObj2;
    Gia_Obj_t * pDri1, * pDri2;
    int i;
    abctime clk = Abc_Clock();
    Gia_ManSetPhase( p );
    Gia_ManForEachPo( p, pObj1, i )
    {
        pObj2 = Gia_ManPo( p, ++i );
        // check if they different on all-0 pattern
        // (for example, when they have the same driver but complemented)
        if ( Gia_ObjPhase(pObj1) != Gia_ObjPhase(pObj2) )
        {
            if ( !pPars->fSilent )
            {
            Abc_Print( 1, "Networks are NOT EQUIVALENT. Output %d trivially differs (different phase).  ", i/2 );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
            }
            pPars->iOutFail = i/2;
            Cec_ManTransformPattern( p, i/2, NULL );
            return 0;
        }
        // get the drivers
        pDri1 = Gia_ObjFanin0(pObj1);
        pDri2 = Gia_ObjFanin0(pObj2);
        // drivers are different PIs
        if ( Gia_ObjIsPi(p, pDri1) && Gia_ObjIsPi(p, pDri2) && pDri1 != pDri2 )
        {
            if ( !pPars->fSilent )
            {
            Abc_Print( 1, "Networks are NOT EQUIVALENT. Output %d trivially differs (different PIs).  ", i/2 );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
            }
            pPars->iOutFail = i/2;
            Cec_ManTransformPattern( p, i/2, NULL );
            // if their compl attributes are the same - one should be complemented
            assert( Gia_ObjFaninC0(pObj1) == Gia_ObjFaninC0(pObj2) );
            Abc_InfoSetBit( p->pCexComb->pData, Gia_ObjCioId(pDri1) );
            return 0;
        }
        // one of the drivers is a PI; another is a constant 0
        if ( (Gia_ObjIsPi(p, pDri1) && Gia_ObjIsConst0(pDri2)) || 
             (Gia_ObjIsPi(p, pDri2) && Gia_ObjIsConst0(pDri1)) )
        {
            if ( !pPars->fSilent )
            {
            Abc_Print( 1, "Networks are NOT EQUIVALENT. Output %d trivially differs (PI vs. constant).  ", i/2 );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
            }
            pPars->iOutFail = i/2;
            Cec_ManTransformPattern( p, i/2, NULL );
            // the compl attributes are the same - the PI should be complemented
            assert( Gia_ObjFaninC0(pObj1) == Gia_ObjFaninC0(pObj2) );
            if ( Gia_ObjIsPi(p, pDri1) )
                Abc_InfoSetBit( p->pCexComb->pData, Gia_ObjCioId(pDri1) );
            else
                Abc_InfoSetBit( p->pCexComb->pData, Gia_ObjCioId(pDri2) );
            return 0;
        }
    }
    if ( Gia_ManAndNum(p) == 0 )
    {
        if ( !pPars->fSilent )
        {
        Abc_Print( 1, "Networks are equivalent.  " );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
        }
        return 1;
    }
    return -1;
}

/**Function*************************************************************

  Synopsis    [Performs naive checking.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec_ManVerifyNaive( Gia_Man_t * p, Cec_ParCec_t * pPars )
{
    Cnf_Dat_t * pCnf = (Cnf_Dat_t *)Mf_ManGenerateCnf( p, 8, 0, 0, 0, 0 );
    sat_solver * pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    Gia_Obj_t * pObj0, * pObj1;
    abctime clkStart = Abc_Clock();
    int nPairs = Gia_ManPoNum(p)/2;
    int nUnsats = 0, nSats = 0, nUndecs = 0, nTrivs = 0;
    int i, iVar0, iVar1, pLits[2], status, RetValue;
    ProgressBar * pProgress = Extra_ProgressBarStart( stdout, nPairs );
    assert( Gia_ManPoNum(p) % 2 == 0 );
    for ( i = 0; i < nPairs; i++ )
    {
        if ( (i & 0xFF) == 0 )
            Extra_ProgressBarUpdate( pProgress, i, NULL );
        pObj0 = Gia_ManPo(p, 2*i);
        pObj1 = Gia_ManPo(p, 2*i+1);
        if ( Gia_ObjChild0(pObj0) == Gia_ObjChild0(pObj1) )
        {
            nUnsats++;
            nTrivs++;
            continue;
        }
        if ( pPars->TimeLimit && (Abc_Clock() - clkStart)/CLOCKS_PER_SEC >= pPars->TimeLimit )
        {
            printf( "Timeout (%d sec) is reached.\n", pPars->TimeLimit );
            nUndecs = nPairs - nUnsats - nSats;
            break;
        }
        iVar0 = pCnf->pVarNums[ Gia_ObjId(p, pObj0) ];
        iVar1 = pCnf->pVarNums[ Gia_ObjId(p, pObj1) ];
        assert( iVar0 >= 0 && iVar1 >= 0 );
        pLits[0] = Abc_Var2Lit( iVar0, 0 );
        pLits[1] = Abc_Var2Lit( iVar1, 0 );
        // check direct
        pLits[0] = lit_neg(pLits[0]);
        status = sat_solver_solve( pSat, pLits, pLits + 2, pPars->nBTLimit, 0, 0, 0 );
        if ( status == l_False )
        {
            pLits[0] = lit_neg( pLits[0] );
            pLits[1] = lit_neg( pLits[1] );
            RetValue = sat_solver_addclause( pSat, pLits, pLits + 2 );
            assert( RetValue );
        }
        else if ( status == l_True )
        {
            printf( "Output %d is SAT.\n", i );
            nSats++;
            continue;
        }
        else
        {
            nUndecs++;
            continue;
        }
        // check inverse
        status = sat_solver_solve( pSat, pLits, pLits + 2, pPars->nBTLimit, 0, 0, 0 );
        if ( status == l_False )
        {
            pLits[0] = lit_neg( pLits[0] );
            pLits[1] = lit_neg( pLits[1] );
            RetValue = sat_solver_addclause( pSat, pLits, pLits + 2 );
            assert( RetValue );
        }
        else if ( status == l_True )
        {
            printf( "Output %d is SAT.\n", i );
            nSats++;
            continue;
        }
        else
        {
            nUndecs++;
            continue;
        }
        nUnsats++;
    }
    Extra_ProgressBarStop( pProgress );
    printf( "UNSAT = %6d.  SAT = %6d.   UNDEC = %6d.  Trivial = %6d.  ", nUnsats, nSats, nUndecs, nTrivs );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clkStart );
    Cnf_DataFree( pCnf );
    sat_solver_delete( pSat );
    if ( nSats )
        return 0;
    if ( nUndecs )
        return -1;
    return 1;
}

/**Function*************************************************************

  Synopsis    [New CEC engine.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec_ManVerify( Gia_Man_t * pInit, Cec_ParCec_t * pPars )
{   // extern void Gia_WriteDotAigSimple( Gia_Man_t * p, char * pFileName, Vec_Int_t * vRwNd );
    int fDumpUndecided = 0;
    Cec_ParFra_t ParsFra, * pParsFra = &ParsFra;
    Gia_Man_t * p, * pNew;
    int RetValue;
    abctime clk = Abc_Clock();
    abctime clkTotal = Abc_Clock();
    // consider special cases:
    // 1) (SAT) a pair of POs have different value under all-0 pattern
    // 2) (SAT) a pair of POs has different PI/Const drivers
    // 3) (UNSAT) 1-2 do not hold and there is no nodes
    RetValue = Cec_ManHandleSpecialCases( pInit, pPars );
    if ( RetValue == 0 || RetValue == 1 ) {
        printf("THIS IS SPECIAL CASE\n");
        return RetValue;
    }
    // preprocess
    Abc_PrintTime( 1, "Start EquivFixOutputPairs", Abc_Clock() - clk );
    p = Gia_ManDup( pInit );
    Gia_ManEquivFixOutputPairs( p );
    // Gia_WriteDotAigSimple( p, "miter_init.dot",0 );
    p = Gia_ManCleanup( pNew = p );
    Gia_ManStop( pNew );
    Abc_PrintTime( 1, "End EquivFixOutputPairs", Abc_Clock() - clk );
    if ( pPars->fNaive ) // naive check
    {
        printf("label 17\n");
        RetValue = Cec_ManVerifyNaive( p, pPars );
        Gia_ManStop( p );
        return RetValue;
    }
    if ( pInit->vSimsPi )
    {
        printf("label 18\n");
        p->vSimsPi = Vec_WrdDup(pInit->vSimsPi); // copy sim info (by copy int array)
        p->nSimWords = pInit->nSimWords;
    }
    // sweep for equivalences
    Abc_PrintTime( 1, "Start ManSatSweeping", Abc_Clock() - clk );
    Cec_ManFraSetDefaultParams( pParsFra ); // set param
    pParsFra->nItersMax    = 1000; // used in sweeping
    pParsFra->nBTLimit     = pPars->nBTLimit;
    pParsFra->TimeLimit    = pPars->TimeLimit;
    pParsFra->fVerbose     = pPars->fVerbose;
    pParsFra->fVeryVerbose = pPars->fVeryVerbose;
    pParsFra->fCheckMiter  = 1;
    pParsFra->fDualOut     = 1;
    pParsFra->fRetainData  = pPars->fUseAffine;
    pNew = Cec_ManSatSweeping( p, pParsFra, pPars->fSilent ); // sweeping
    // Gia_WriteDotAigSimple( pNew, "miter_sweep.dot",0 );
    pPars->iOutFail = pParsFra->iOutFail;
    pInit->pCexComb = p->pCexComb; p->pCexComb = NULL;
    Gia_ManStop( p );
    Abc_PrintTime( 1, "End ManSatSweeping", Abc_Clock() - clk );
    p = pInit;
    if ( pNew == NULL )
    {
        Abc_PrintTime( 1, "Check Noneq in ManVerify", Abc_Clock() - clk );
        if ( p->pCexComb != NULL )
        {
            printf("label 20\n");
            if ( p->pCexComb && !Gia_ManVerifyCex( p, p->pCexComb, 1 ) )
                Abc_Print( 1, "Counter-example simulation has failed.\n" );
            if ( !pPars->fSilent )
            {
            Abc_Print( 1, "Networks are NOT EQUIVALENT.  " );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
            }
            return 0;
        }
        p = Gia_ManDup( pInit );
        Gia_ManEquivFixOutputPairs( p );
        p = Gia_ManCleanup( pNew = p );
        Gia_ManStop( pNew );
        pNew = p;
        Abc_PrintTime( 1, "End Check Noneq", Abc_Clock() - clk );
    }
    //.... from below, it means new CEC engine can't solve it
    if ( pPars->fVerbose )
    {
        Abc_Print( 1, "Networks are UNDECIDED after the new CEC engine.  " );
        if (pPars->fUseAffine) Gia_ManEquivPrintClasses( pNew, 1, 0 );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }
    if ( fDumpUndecided )
    {
        ABC_FREE( pNew->pReprs );
        ABC_FREE( pNew->pNexts );
        Gia_AigerWrite( pNew, "gia_cec_undecided.aig", 0, 0, 0 ); // writeout aig
        Abc_Print( 1, "The result is written into file \"%s\".\n", "gia_cec_undecided.aig" );
    }
    if ( pPars->TimeLimit && (Abc_Clock() - clkTotal)/CLOCKS_PER_SEC >= pPars->TimeLimit )
    {
        Gia_ManStop( pNew );
        return -1;
    }

    if (pPars->fUseAffine) {
        // Call Output Match Solver
        if ( pPars->fVerbose )
            Abc_Print( 1, "Calling the Output Matching CEC engine.\n" );
        pPars->TimeLimit -= (Abc_Clock() - clkTotal)/CLOCKS_PER_SEC;
        RetValue = Cec_ManVerifyWithAffine( pNew, pPars );
    }

    // call other solver
    if ( pPars->fVerbose )
        Abc_Print( 1, "Calling the old CEC engine.\n" );
    fflush( stdout );
    Abc_PrintTime( 1, "Start Old Verify", Abc_Clock() - clk );
    RetValue = Cec_ManVerifyOld( pNew, pPars->fVerbose, &pPars->iOutFail, clkTotal, pPars->fSilent );
    Abc_PrintTime( 1, "End Old Verify", Abc_Clock() - clk );
    p->pCexComb = pNew->pCexComb; pNew->pCexComb = NULL;
    if ( p->pCexComb && !Gia_ManVerifyCex( p, p->pCexComb, 1 ) )
        Abc_Print( 1, "Counter-example simulation has failed.\n" );
    Gia_ManStop( pNew );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [CEC engine with Affine.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

Gia_Man_t* Cec_ManFdRewrite( Gia_Man_t * p, Cec_ParFd_t * pPars, Vec_Int_t* vRwNd, char* pStat ) {
    FILE* pFile;
    Vec_Ptr_t *vObj;
    Vec_Int_t *vUnsat, *vMerge, *vSolved;
    Vec_Int_t *vIntBuff, *vIntBuff1, *vIntBuff2, *vIntBuff3, *vIntBuff4, *vIntBuff5, *vIntBuff6;
    Vec_Int_t *vCnt, *vCntInit1, *vCntInit2, *vCnt1, *vCnt2, *vCnt3, *vCnt4, *vCnt5, *vCnt6, *vChange;
    Vec_Wec_t *vWecBuff;
    Vec_Flt_t *vCost;
    Gia_Man_t *pOut, *pTemp, *pTemp2, *pItp;
    Gia_Obj_t *pObj, *pObj1, *pObj2, *pObjbuf;
    int i, j, k, flag, buf, buf1, buf2, buf3, buf4, buf5, buf6, cnt1, cnt2, nodeId, nid, nid2, fFdnid = 3;
    int frac, up, bot, rwCkt, frtMin;
    int** dist;
    float cost_buf, cost_buf1, cost_buf2, cost_buf3, cost_buf4;
    char* str = ABC_ALLOC(char, 100);

    // Cec_ManFd_t* pMan = 0;
    Cec_ManFd_t* pMan_buf;
    Cec_ManFd_t* pMan = Cec_ManFdStart( p, pPars, pStat );
    Cec_ManFdPrintPars( pMan );

    if (vRwNd != NULL) {
        assert( Vec_IntSize(vRwNd) > 0 );
        Vec_IntForEachEntry( vRwNd, nodeId, i ) {
            if (nodeId < 0) {
                if (nodeId == -1) {
                    // report
                    if (pPars->fVerbose) Cec_ManFdReport( pMan );
                    Cec_ManFdStop( pMan, 2 );
                    return 0;
                } else if (nodeId == -2) {
                    // only fraig
                    pOut = Cec_ManFraigSyn( pMan->pGia, pMan->pObj1, pMan->pObj2, pMan->fPhase, 1, pMan->pPars->nBTLimit, (pMan->pPars->fSyn == 0));
                    Cec_ManMatch( pOut, pMan->pGia );
                    Cec_ManFdUpdate( pMan, pOut );
                    Cec_ManFdDumpStat( pMan, "synstat.log" );
                    Gia_AigerWrite( pOut, "syn.aig", 0, 0, 0 );
                    Cec_ManFdStop( pMan, 2 );
                    return pOut;
                } else if (nodeId == -3) {
                    // 0: no resyn, 1: resyn upper, 2: resyn lower, 3: resyn upper(low frt), 4: resyn lower(low frt), 5: resyn all
                    buf = Vec_IntSize(vRwNd) > 1 ? Vec_IntEntry( vRwNd, 1 ) : 0; 
                    pTemp = pOut;
                    vIntBuff = Cec_ManCheckMiter( pTemp );
                    Vec_IntPop( vIntBuff );
                    switch (buf)
                    {
                        case 0:
                            vMerge = 0;
                            break;
                        case 1:
                            vMerge = Vec_IntAlloc(100);
                            Cec_ManGetMerge( pTemp, Vec_IntItoV( Vec_IntEntry(vIntBuff, 0) ), Vec_IntItoV( Vec_IntEntry(vIntBuff, 1) ), vMerge, 0, 1 ); 
                            pTemp = Cec_ManPartSyn( pTemp2 = pTemp, vMerge, vIntBuff, -1, 0 );
                            Gia_ManStop( pTemp2 );
                            break;
                        case 2:
                            vMerge = Vec_IntAlloc(100);
                            Cec_ManGetMerge( pTemp, Vec_IntItoV( Vec_IntEntry(vIntBuff, 0) ), Vec_IntItoV( Vec_IntEntry(vIntBuff, 1) ), vMerge, 0, 1 ); 
                            pTemp = Cec_ManPartSyn( pTemp2 = pTemp, 0, vMerge, -1, 0 );
                            Gia_ManStop( pTemp2 );
                            break;
                        case 3:
                            vMerge = Vec_IntAlloc(100);
                            Cec_ManGetMerge( pTemp, Vec_IntItoV( Vec_IntEntry(vIntBuff, 0) ), Vec_IntItoV( Vec_IntEntry(vIntBuff, 1) ), vMerge, 1, 1 ); 
                            pTemp = Cec_ManPartSyn( pTemp2 = pTemp, vMerge, vIntBuff, -1, 0 );
                            Gia_ManStop( pTemp2 );
                            break;
                        case 4:
                            vMerge = Vec_IntAlloc(100);
                            Cec_ManGetMerge( pTemp, Vec_IntItoV( Vec_IntEntry(vIntBuff, 0) ), Vec_IntItoV( Vec_IntEntry(vIntBuff, 1) ), vMerge, 1, 1 ); 
                            pTemp = Cec_ManPartSyn( pTemp2 = pTemp, 0, vMerge, -1, 0 );
                            Gia_ManStop( pTemp2 );
                            break;
                        case 5:
                            pTemp = Cec_ManPartSyn( pTemp2 = pTemp, 0, vIntBuff, -1, 0 );
                            Gia_ManStop( pTemp2 );
                            break;
                        default:
                            break;
                    }

                    Vec_IntFree( vIntBuff );
                    pTemp = Gia_ManRehash( pTemp2 = pTemp, 0 );
                    Gia_ManStop( pTemp2 );
                    if (vMerge) Vec_IntFree( vMerge );
                    
                    
                    pOut = Cec_ManFraigSyn_naive( pTemp, 10000 );
                    
                    Cec_ManMatch( pOut, pMan->pGia );
                    Cec_ManFdUpdate( pMan, pOut );
                    Cec_ManFdDumpStat( pMan, "synstat.log" );
                    Gia_AigerWrite( pOut, "syn.aig", 0, 0, 0 );
                    Cec_ManFdStop( pMan, 2 );

                    return pOut;
                } else if (nodeId == -4) {
                    // save all single rewrite
                    Cec_ManFdIncreIter( pMan );
                    Cec_ManFdReport( pMan );
                    Vec_IntForEachEntry( vRwNd, nodeId, i ) {
                        if (nodeId > 0) {
                            nid = Cec_ManFdMapIdSingle( pMan, nodeId, 1 );
                            if (nid < 0) assert(0);
                            pTemp = Cec_ManFdReplaceOne(pMan, nodeId);
                            sprintf(str, "%d(%d)_Rewrited.aig", nodeId, nid);
                            Gia_AigerWriteSimple( pTemp, str );
                        }
                    }
                    Cec_ManFdStop( pMan, 2 );
                    return 0;
                } else if (nodeId == -5) {
                    // same as cec
                    nid = Gia_ObjId( pMan->pGia, pMan->pObj1 );
                    Cec_ManFdCleanOneUnknown( pMan, nid );
                    Cec_ManFdShrink( pMan, nid );
                    nid = Gia_ObjId( pMan->pGia, pMan->pObj2 );
                    Cec_ManFdCleanOneUnknown( pMan, nid );
                    Cec_ManFdShrink( pMan, nid );
                    Cec_ManFdIncreIter( pMan );
                    Cec_ManFdReport( pMan );
                    Cec_ManFdStop( pMan, 2 );
                    return 0;
                } else if (nodeId == -6) {
                    vIntBuff = Vec_IntDup( vRwNd );
                    Vec_IntRemove( vIntBuff, -6 );
                    pOut = Cec_ManSyn( p, vIntBuff );
                    Gia_AigerWrite( pOut, "syn_fin.aig", 0, 0, 0 );
                    Gia_SelfDefShow( pOut, "syn_fin.dot", NULL, NULL, NULL );
                    return pOut;
                } else if (nodeId == -7) {
                    // -7, coef_Patch, coef_Frt, partial_method
                    flag = 1; // print detail mapping
                    rwCkt = Vec_IntEntry(vRwNd, 1);
                    cost_buf1 = Vec_IntSize(vRwNd) > 2 ? 1.0 * Vec_IntEntry(vRwNd, 2) / 100.0 : 2.0;
                    cost_buf2 = Vec_IntSize(vRwNd) > 3 ? 1.0 * Vec_IntEntry(vRwNd, 3) / 100.0 : 1.0;
                    Gia_SelfDefShow( pMan->pGia, "pGia.dot", 0, 0, 0 );
                    assert( rwCkt == 2 || rwCkt == 1 );
                    vCost = Vec_FltAlloc( 6 );
                    Vec_FltPush(vCost, cost_buf1);
                    Vec_FltPush(vCost, cost_buf2);
                    vIntBuff = Cec_ManFdTraverseUnknownFrt( pMan, rwCkt, 2, 1, vCost );
                    vIntBuff1 = Cec_ManFdUnsatId( pMan, 0, 0 );
                    Cec_ManFdMapId( pMan, vIntBuff3 = Vec_IntDup( vIntBuff1 ), 1 );

                    Gia_ManForEachAnd( pMan->pGia, pObj, j ) {
                        nid = Gia_ObjId( pMan->pGia, pObj );
                        buf = Gia_ObjColors( pMan->pGia, nid );
                        if (buf == 1 || buf == 2) {
                            if (Cec_ManFdToSolve( pMan, nid )) Cec_ManFdShrinkSimple( pMan, nid );
                        }
                    }
                    vUnsat = Cec_ManFdUnsatId( pMan, 0, 1 );
                    vSolved = Cec_ManFdSolved( pMan, 0, 0 );
                    Cec_ManFdMapId( pMan, vIntBuff2 = Vec_IntDup( vUnsat ), 1 );
                    Gia_SelfDefShow( pMan->pAbs, "picked_onFrt.dot", vIntBuff, vIntBuff3, NULL );
                    Gia_SelfDefShow( pMan->pAbs, "traverse.dot", vIntBuff2, vIntBuff, NULL );
                    Vec_IntFree( vIntBuff2 );
                    Vec_IntFree( vIntBuff3 );
                    Vec_IntFree( vIntBuff );
                    
                    buf = rwCkt == 1 ? Gia_ObjId(pMan->pGia, pMan->pObj2) : Gia_ObjId(pMan->pGia, pMan->pObj1);
                    if ( Vec_IntEntry( pMan->vStat, buf ) > 0 || Vec_IntEntry( pMan->vStat, buf ) == CEC_FD_HUGE ) {
                        printf("PO patch is found\n");
                        return 0;
                    }

                    if (flag) {
                        printf("mapped node info:\n");
                        vIntBuff2 = Vec_IntAlloc(100);
                        vIntBuff3 = Vec_IntAlloc(100);
                        Vec_IntForEachEntry( pMan->vNodeInit, nid, j ) {
                            // printf("colors: %d ", Gia_ObjColors( pMan->pGia, nid ) );
                            if (Gia_ObjColors( pMan->pGia, nid ) != 1 && 
                                Gia_ObjColors( pMan->pGia, nid ) != 2) continue;
                            buf = Cec_ManFdMapIdSingle( pMan, nid, 1 );
                            if ( Vec_IntFind( vSolved, nid ) >= 0 ) {
                                printf("node %d(%d): solved\n", nid, buf );
                                Vec_IntPush( vIntBuff2, buf );
                            } else {
                                printf("node %d(%d): unsolved\n", nid, buf );
                                Vec_IntPush( vIntBuff3, buf );
                            }
                        }
                        Gia_SelfDefShow( pMan->pAbs, "mapped_vNodeInit.dot", vIntBuff2, vIntBuff3, NULL );
                    }

                    printf("start replacement\n");
                    pTemp = Cec_ManFdReplaceMulti( pMan, vIntBuff1 ); // pTemp --> pGia
                    Gia_AigerWrite( pTemp, "_replaced.aig", 0, 0, 0 );
                    // Gia_SelfDefShow( pTemp, "replaced.dot", 0, 0, 0 );
                    Gia_ManFillValue( pMan->pGia );
                    Cec_ManMapCopy( pMan->pGia, pTemp, 0 ); // pGia --> pTemp
                    Cec_ManGiaMapId( pMan->pGia, vIntBuff2 = Vec_IntDup(pMan->vMerge) );
                    Vec_IntRemoveAll( vIntBuff2, -1, 0 );
                    Cec_ManGiaMapId( pMan->pGia, vIntBuff3 = Vec_IntDup(vSolved) );
                    Vec_IntRemoveAll( vIntBuff3, -1, 0 );
                    Cec_ManGiaMapId( pMan->pGia, vIntBuff4 = Vec_IntDup(vIntBuff1) );
                    Vec_IntRemoveAll( vIntBuff4, -1, 0 );

                    Gia_ManFillValue( pTemp );
                    buf3 = Vec_IntSize( vRwNd ) > 4 ? Vec_IntEntry( vRwNd, 4 ) : 0;
                    if (buf3 == 0) {
                        Vec_IntForEachEntry( vIntBuff3, nid, j ) {
                            Cec_ManSubcircuit( pTemp, Gia_ManObj( pTemp, nid ), 0, 0, 1, 0 );
                        }
                    } else if (buf3 == 1) {
                        pObjbuf = rwCkt == 1 ? Gia_ObjCopy( pTemp, pMan->pObj1 ) : Gia_ObjCopy( pTemp, pMan->pObj2 );
                        Cec_ManSubcircuit( pTemp, pObjbuf, 0, 0, 1, 0 );
                        pObjbuf->Value = -1;
                        Vec_IntForEachEntry( vIntBuff3, nid, j ) {
                            Cec_ManSubcircuit( pTemp, Gia_ManObj( pTemp, nid ), 0, 0, 1, 0 );
                        }
                    } else if (buf3 == 2) {
                        Vec_IntForEachEntry( vIntBuff4, nid, j ) {
                            Cec_ManSubcircuit( pTemp, Gia_ManObj( pTemp, nid ), 0, 0, 1, 0 );
                        }
                    }
                    // Vec_IntForEachEntry( vIntBuff2, nid, j ) {
                    //     Cec_ManSubcircuit( pTemp, Gia_ManObj( pTemp, nid ), 0, 0, 1, 0 );
                    // }
                    // Cec_ManMark2Val( pTemp, ~0 ^ 1, 0, 1, 0 );
                    // vMerge = Cec_ManGetTopMark( pTemp, 1 );
                    Cec_ManMark2Val( pTemp, ~0 ^ 1, 0, 1, 0 );
                    vMerge = Cec_ManGetTopMark( pTemp, 1 );
                    Vec_IntForEachEntry( vIntBuff2, nid, j ) {
                        Vec_IntPush( vMerge, nid );
                    }
                    Vec_IntFree( vIntBuff3 );
                    Vec_IntFree( vIntBuff4 );

                    // printf("merges to be done:\n");
                    // Vec_IntPrint( vMerge );
                    pTemp = Cec_ManPartSyn( pTemp2 = pTemp, 0, vMerge, -1, 1 ); // why circuit size not changed
                    printf("after partsyn\n");
                    if (flag) Cec_ManPrintMapCopy( pTemp );
                    Vec_IntFree( vMerge );
                    Gia_ManFillValue( pTemp2 );
                    Cec_ManMapCopy( pTemp2, pMan->pGia, 0 ); // pTemp2 --> pGia
                    Cec_ManMapCopy( pTemp, pTemp2, 1 ); // pTemp --> pGia
                    Gia_ManFillValue( pMan->pGia );
                    Cec_ManMapCopy( pMan->pGia, pTemp, 0 ); // pGia --> pTemp
                    Cec_ManGiaMapId( pMan->pGia, vUnsat );
                    Cec_ManGiaMapId( pMan->pGia, vSolved );
                    Cec_ManGiaMapId( pMan->pGia, vIntBuff4 = Vec_IntDup(vIntBuff1) );
                    Cec_ManFdMapId( pMan, vIntBuff5 = Vec_IntDup(vIntBuff1), 1 );
                    printf("map of replaced nodes (pGia):\n");
                    Vec_IntForEachEntryTwo( vIntBuff1, vIntBuff4, buf1, buf2, j ) {
                        printf("map %d to %d\n", buf1, buf2);
                    }
                    Vec_IntRemoveAll( vUnsat, -1, 0 );
                    Vec_IntRemoveAll( vSolved, -1, 0 );
                    // Gia_SelfDefShow( pTemp, "partSyn.dot", 0, 0, 0 );
                    // return 0;

                    // Gia_AigerWrite( pTemp, "partSyn.aig", 0, 0, 0 );
                    // Gia_SelfDefShow( pTemp, "partSyn.dot", 0, 0, 0 );

                    pOut = pTemp;

                    Cec_ManFdUpdate( pMan, pOut );
                    Cec_ManFdMapId( pMan, vIntBuff4, 1 );
                    printf("map of replaced nodes (pAbs):\n");
                    Vec_IntForEachEntryTwo( vIntBuff5, vIntBuff4, buf1, buf2, j ) {
                        printf("map (%d) to (%d)\n", buf1, buf2);
                    }
                    Cec_ManFdDumpStat( pMan, "partSyn.stat" );
                    pFile = fopen( "partSyn_solved.stat", "w" );
                    buf = 0;
                    Vec_IntForEachEntry( pMan->vNodeInit, nid, j ) {
                        if (Gia_ObjColors( pMan->pGia, nid ) == 3) continue;
                        if (Vec_IntFind( vSolved, nid ) == -1) continue; 
                        if (buf % 10 == 0) fprintf( pFile, "f ");
                        fprintf(pFile, "%d ", nid);
                        if (buf % 10 == 9) fprintf( pFile, "\n" );
                        buf++;
                    }
                    if (buf % 10 != 0) fprintf( pFile, "\n" );
                    fclose( pFile );
                    pFile = fopen( "partSyn_unsolved.stat", "w" );
                    buf = 0;
                    Vec_IntForEachEntry( pMan->vNodeInit, nid, j ) {
                        if (Gia_ObjColors( pMan->pGia, nid ) == 3) continue;
                        if (Vec_IntFind( vSolved, nid ) != -1) continue; 
                        if (buf % 10 == 0) fprintf( pFile, "f ");
                        fprintf(pFile, "%d ", nid);
                        if (buf % 10 == 9) fprintf( pFile, "\n" );
                        buf++;
                    }
                    if (buf % 10 != 0) fprintf( pFile, "\n" );
                    fclose( pFile );
                    Gia_AigerWrite( pMan->pGia, "_partSyn.aig", 0, 0, 0 );
                    Cec_ManFdMapId( pMan, vSolved, 1 );
                    Cec_ManFdMapId( pMan, vIntBuff5 = Vec_IntDup( pMan->vNodeInit ), 1 );
                    Vec_IntRemoveAll( vIntBuff5, -1, 0 );
                    Vec_IntRemoveAll( vSolved, -1, 0 );
                    Gia_SelfDefShow( pMan->pAbs, "partSyn.dot", vSolved, vIntBuff5, 0 );
                    Cec_ManFdStop( pMan, 2 );
                    
                    return 0;

                } else if (nodeId == -8) {
                    Cec_ManFdCleanUnknown( pMan, 3 ); // Try to get conflicts, not just by printing. Maybe idata? pName?
                    vIntBuff1 = Cec_ManFdUnsatId( pMan, 0, 0 );
                    vIntBuff3 = Cec_ManFdSolved( pMan, 0, 0 );
                    Cec_ManFdMapId( pMan, vIntBuff3, 1 );
                    pPars->nBTLimit *= 5;
                    Cec_ManFdCleanUnknown( pMan, 3 );
                    vIntBuff2 = Cec_ManFdUnsatId( pMan, 0, 0 ); // all under nBTLimit, contain those in 1
                    // Vec_IntSort( vIntBuff2, 0 );
                    vIntBuff4 = Cec_ManFdSolved( pMan, 0, 0 );
                    Cec_ManFdMapId( pMan, vIntBuff4, 1 );
                    Gia_SelfDefShow( pMan->pAbs, "pAbs.dot", vIntBuff3, vIntBuff4, 0);
                    Vec_IntFree( vIntBuff3 );
                    Vec_IntFree( vIntBuff4 );
                    // Vec_IntPrint( pMan->vConf );
                    Vec_IntReplace( pMan->vConf, 0, 1 );

                    Vec_IntPrint( vIntBuff1 );
                    vCnt1 = Vec_IntAlloc( 100 );
                    vCnt2 = Vec_IntAlloc( 100 );
                    vCnt3 = Vec_IntAlloc( 100 );
                    vCnt4 = Vec_IntAlloc( 100 );
                    buf = Vec_IntEntry( vRwNd, 2 );
                    Vec_IntForEachEntry( vIntBuff1, nid, j ) {
                        if (nid < buf) continue;
                        vIntBuff5 = Cec_ManGetTFO( pMan->pGia, Vec_IntItoV(nid), -1, 0 );
                        pItp = (Gia_Man_t*) Vec_PtrEntry( pMan->vVeryStat, nid );
                        pItp = Cec_ManSimpSyn( pTemp2 = pItp, 1, 0, -1 );
                        Cec_ManFdUpdateStat( pMan, nid, 0, pItp, 0 );
                        Cec_ManFdCleanSupport( pMan, nid ); // pItp is stopped
                        pItp = (Gia_Man_t*) Vec_PtrEntry( pMan->vVeryStat, nid );
                        pTemp = Cec_ManFdReplaceOne( pMan, nid );
                        
                        if (Vec_IntEntry( vRwNd, 1 ) == 1) {
                            Cec_ManMapCopy( pMan->pGia, pTemp, 0 ); // pGia --> pTemp (pTemp2)
                            Gia_ManFillValue( pTemp );
                            Vec_IntForEachEntry( vIntBuff1, nid2, k ) {
                                pObj = Gia_ManObj(pMan->pGia, nid2);
                                if (pObj->Value != -1)
                                    Cec_ManSubcircuit( pTemp, Gia_ObjCopy(pTemp, pObj), 0, 0, 1, 0 );
                            }
                            Gia_ManForEachObj( pTemp, pObj, i ) {
                                if (pObj->Value != -1) pObj->fMark0 = 1;
                            }
                            vMerge = Cec_ManGetTopMark( pTemp, 0 );
                            pTemp = Cec_ManPartSyn( pTemp2 = pTemp, 0, vMerge, -1, 0 ); // pTemp --> pTemp2
                            Cec_ManMapCopy( pTemp2, pMan->pGia, 0 ); // pTemp2 --> pGia
                            Cec_ManMapCopy( pTemp, pTemp2, 1 ); // pTemp --> pGia
                            Gia_ManStop( pTemp2 );
                        }
                        
                        vIntBuff = Vec_IntAlloc( Gia_ManObjNum(pTemp) );
                        Cec_ManGiaMapId( pTemp, vIntBuff ); // pTemp --> pGia
                        pMan_buf = Cec_ManFdStart( pTemp, pMan->pPars, NULL );
                        Cec_ManFdCleanUnknown( pMan_buf, 3 );
                        Vec_IntRemapArray( vIntBuff, pMan_buf->vConf, vIntBuff4 = Vec_IntAlloc( Vec_IntSize(pMan->vConf) ), Vec_IntSize(pMan->vConf) );
                        Vec_IntReplace( vIntBuff4, 0, -1 );
                        buf1 = 0;
                        buf2 = 0;
                        buf5 = 0;
                        buf6 = 0;
                        cost_buf1 = 0.0;
                        cnt1 = 0;
                        cost_buf2 = 0.0;
                        cnt2 = 0;
                        // vCnt1 = Vec_IntAlloc( 100 );
                        // vCnt2 = Vec_IntAlloc( 100 );
                        Vec_IntForEachEntryTwo( pMan->vConf, vIntBuff4, buf3, buf4, nid2 ) {
                            if (buf3 == -1 || buf4 == -1) {
                                // printf("%d is not copied\n", nid2);
                                continue;
                            } else if (buf3 > pPars->nBTLimit && buf4 > pPars->nBTLimit) {
                                // printf("%d is still not solvable\n", nid2);
                                // buf5++;
                                continue; 
                            } else {
                                if (buf3 > buf4) {
                                    // printf("%d is solved faster...%d, %d\n", nid2, buf3, buf4);
                                    // Vec_IntPush( vCnt1, nid2 );
                                    if (buf3 > pPars->nBTLimit) {
                                        buf1++;
                                        if (Vec_IntFind( vIntBuff5, nid2 ) != -1) {
                                            buf5++;
                                            printf("newly solved %d with level is %d\n", nid2, 
                                                Gia_ObjColors(pMan->pGia, nid2) == 1 ? Vec_IntEntry( pMan->vLevel1, nid2 ) : Vec_IntEntry( pMan->vLevel2, nid2 ) );
                                        }
                                    }
                                    else if (buf3 < 0.2 * pPars->nBTLimit) continue;
                                    else {
                                        cost_buf = 1.0 * (buf3 - buf4) / buf3;
                                        cost_buf1 += cost_buf * cost_buf;
                                        cnt1++;
                                    }
                                } else if (buf3 < buf4) {
                                    // printf("%d is solved slower...%d, %d\n", nid2, buf3, buf4);
                                    // Vec_IntPush( vCnt2, nid2 );
                                    if (buf4 > pPars->nBTLimit) {
                                        buf2++;
                                        if (Vec_IntFind( vIntBuff5, nid2 ) != -1) {
                                            buf6++;
                                            printf("fail solved %d with level is %d\n", nid2, 
                                                Gia_ObjColors(pMan->pGia, nid2) == 1 ? Vec_IntEntry( pMan->vLevel1, nid2 ) : Vec_IntEntry( pMan->vLevel2, nid2 ) );
                                        }
                                    }
                                    else if (buf4 < 0.2 * pPars->nBTLimit) continue;
                                    else {
                                        cost_buf = 1.0 * (buf4 - buf3) / buf4;
                                        cost_buf2 += cost_buf * cost_buf;
                                        cnt2++;
                                    }
                                }

                            }
                        }
                        // cost_buf1 /= (1.0 * cnt1);
                        // cost_buf2 /= (1.0 * cnt2);
                        printf("> > > replacing %d new solve %d(%d), less solve %d(%d), conflict change ratio (square) : -%f, %f\n", nid, buf1, buf5, buf2, buf6, cost_buf1, cost_buf2);
                        // cost_buf1 *= (1.0 * cnt1);
                        // cost_buf2 *= (1.0 * cnt2);
                        cost_buf1 += 1.0 * buf1;
                        cost_buf2 += 1.0 * buf2;
                        // cost_buf1 /= (1.0 * (cnt1 + buf1));
                        // cost_buf2 /= (1.0 * (cnt2 + buf2));

                        if (buf1 > buf2) {
                            Vec_IntPush( vCnt1, nid );
                        } else {
                            Vec_IntPush( vCnt2, nid );
                        }
                        if (buf5 > buf6) {
                            Vec_IntPush( vCnt3, nid );
                        } else {
                            Vec_IntPush( vCnt4, nid );
                        }
                        // Cec_ManFdMapId( pMan, vCnt1, 1 );
                        // Cec_ManFdMapId( pMan, vCnt2, 1 );
                        // Gia_SelfDefShow( pMan->pAbs, "pAbs_comp.dot", vCnt1, vCnt2, 0);
                        
                        /*
                            vIntBuff3 = Cec_ManFdUnsatId( pMan_buf, 0, 0 );
                            vCnt = Vec_IntDup( vIntBuff3 );
                            Cec_ManFdMapId( pMan_buf, vCnt, 1 );
                            Gia_SelfDefShow( pMan_buf->pAbs, "pAbs_rep.dot", vCnt, NULL, 0);
                            Vec_IntPrint( vIntBuff3 );
                            Vec_IntPrint( vIntBuff2 );
                            Vec_IntMap( vIntBuff, vIntBuff3 );
                            Vec_IntRemoveAll( vIntBuff3, -1, 0 ); // problem: replaced patch is also recorded
                            vIntBuff = Vec_IntInvert( vIntBuff4 = vIntBuff, -1 );
                            Vec_IntFree( vIntBuff4 );
                            Vec_IntPrintMap( vIntBuff );
                            Vec_IntMap( vIntBuff, vIntBuff3 );
                            vIntBuff4 = Vec_IntDup( vIntBuff2 );
                            Vec_IntMap( vIntBuff, vIntBuff4 );
                            Vec_IntRemoveAll( vIntBuff4, -1, 0 );
                            Vec_IntSort( vIntBuff3, 0 );
                            Vec_IntSort( vIntBuff4, 0 );
                            Vec_IntTwoRemove1( vIntBuff4, vIntBuff3 );
                            // vIntBuff2 is not on pMan_buf, wrong values
                            Vec_IntTwoRemove1( vIntBuff3, vIntBuff2 );
                            printf("nid %d have gain %d nodes\n", nid, Vec_IntSize(vIntBuff3) - Vec_IntSize(vIntBuff4));
                            printf("new computed nodes:");
                            Vec_IntPrint( vIntBuff3 );
                            printf("not computable nodes:");
                            Vec_IntPrint( vIntBuff4 );
                            vCnt1 = Vec_IntDup( vIntBuff3 );
                            vCnt2 = Vec_IntDup( vIntBuff4 );
                            // both have some nodes not in pAbs...
                            Cec_ManFdMapId( pMan_buf, vCnt1, 1 );
                            Cec_ManFdMapId( pMan_buf, vCnt2, 1 );
                            Gia_SelfDefShow( pMan_buf->pAbs, "pAbs_comp.dot", vCnt1, vCnt2, 0);
                        */
                        
                        Vec_IntFree( vIntBuff );
                        // Vec_IntFree( vIntBuff3 );
                        Vec_IntFree( vIntBuff4 );
                        Cec_ManFdStop( pMan_buf, 2 );
                    }
                    Cec_ManFdMapId( pMan, vCnt1, 1 );
                    Cec_ManFdMapId( pMan, vCnt2, 1 );
                    Gia_SelfDefShow( pMan->pAbs, "pAbs_cand.dot", vCnt1, vCnt2, 0);
                    Gia_SelfDefShow( pMan->pAbs, "pAbs_cand_TFO.dot", vCnt3, vCnt4, 0);
                    Cec_ManFdStop( pMan, 2 );
                    return 0;
                } else if (nodeId == -9) {
                    vMerge = Vec_IntAlloc(100);
                    vCnt1 = Vec_IntAlloc(100);
                    vCnt2 = Vec_IntAlloc(100);
                    vIntBuff4 = Vec_IntAlloc(100);
                    vIntBuff3 = Vec_IntAlloc(100); // serve as series of best result: decrease ratio, decrease node, pAbs size
                    Vec_IntFill( vIntBuff3, 4, -1 );
                    vCost = Vec_FltAlloc(100);
                    Vec_FltFill( vCost, 4, -1.0 );
                    printf("pGia, pAbs = %d, %d\n", Gia_ManObjNum(pMan->pGia), Gia_ManObjNum(pMan->pAbs));
                    Gia_ManForEachObj( pMan->pGia, pObj, i ) {
                        nid = Gia_ObjId(pMan->pGia, pObj);
                        if (Gia_ObjColors(pMan->pGia, nid) == 1 || Gia_ObjColors(pMan->pGia, nid) == 2) {
                            if (Vec_IntSize(Vec_WecEntry(pMan->vGSupport, nid)) == 0) continue;
                            pItp = Cec_ManFdGetFd( pMan, nid, 0, 0 );
                            if (pItp == NULL || pItp == 1) continue;
                            Vec_IntPush( vCnt1, Cec_ManFdMapIdSingle( pMan, nid, 1 ) );
                            printf("nid: %d\n", nid);
                            printf("size of itp: %d\n", Gia_ManAndNum(pItp));
                            if (Gia_ManAndNum(pItp) > 200) continue;
                            Vec_IntPush( vIntBuff4, Cec_ManFdMapIdSingle( pMan, nid, 1 ) );
                            pItp = Cec_ManPatchCleanSupport( pTemp = pItp, Vec_WecEntry(pMan->vGSupport, nid) );
                            buf1 = Gia_ManAndNum( pItp );
                            Gia_ManStop( pTemp );
                            
                            vIntBuff1 = Vec_IntItoV( nid );
                            pTemp = Cec_ManReplacePatch( pMan->pGia, pItp, vIntBuff1, Vec_WecEntry(pMan->vGSupport, nid), 0, 0 );
                            printf("size of ckt with patch: %d\n", Gia_ManAndNum(pTemp));
                            Vec_IntFree( vIntBuff1 );

                            // ---- surrounding test
                            vIntBuff1 = Vec_IntItoV( nid );
                            vIntBuff1 = Cec_ManGetTFO( pMan->pGia, vIntBuff2 = vIntBuff1, 3, 0 );
                            Vec_IntFree( vIntBuff2 );
                            Gia_ManFillValue( pMan->pGia );
                            Vec_IntForEachEntry( vIntBuff1, nid2, i ) {
                                Cec_ManSubcircuit( pMan->pGia, Gia_ManObj( pMan->pGia, nid2 ), 0, 0, 1, 0 );
                            }
                            Vec_IntFree( vIntBuff1 );
                            buf3 = 0;
                            Gia_ManForEachObj( pMan->pGia, pObj2, i ) {
                                if (Gia_ObjColors(pMan->pGia, Gia_ObjId(pMan->pGia, pObj2)) != 3 && pObj2->Value != ~0) buf3++;
                            }
                            printf("size of surrounding: %d\n", buf3);
                            Gia_ManFillValue( pMan->pGia );
                            Vec_IntForEachEntry( Vec_WecEntry(pMan->vGSupport, nid), nid2, i ) {
                                Cec_ManSubcircuit( pMan->pGia, Gia_ManObj( pMan->pGia, nid2 ), 0, 0, 1, 0 );
                            }
                            buf4 = 0;
                            Gia_ManForEachObj( pMan->pGia, pObj2, i ) {
                                if (Gia_ObjColors(pMan->pGia, Gia_ObjId(pMan->pGia, pObj2)) != 3 && pObj2->Value != ~0) buf4++;
                            }
                            printf("size of G support: %d\n", buf4);

                            // with patch
                            vIntBuff1 = Vec_IntItoV( nid );
                            vIntBuff1 = Cec_ManGetTFO( pMan->pGia, vIntBuff2 = vIntBuff1, 3, 0 );
                            vIntBuff1 = Vec_IntDup( pItp->vCos );
                            Cec_ManGiaMapId( pItp, vIntBuff1 );
                            Gia_ManStop( pItp );
                            pItp = Cec_ManSurrounding( pTemp, vIntBuff1, 3, 1 );
                            Vec_IntFree( vIntBuff1 );
                            Gia_SelfDefShow( pItp, "fd_replaced_surrounding.dot", NULL, NULL, NULL );
                            buf2 = Gia_ManAndNum(pItp);
                            pItp = Cec_ManSimpSyn( pTemp2 = pItp, 1, 0, -1 );
                            Gia_ManStop( pTemp2 );
                            buf = Gia_ManAndNum(pItp);
                            printf("syn ratio with patch: %f\n", (float)buf / (float)buf2);
                            printf("approx amount of reduced nodes: %f\n", (1.0 - ((float)buf / (float)buf2)) * (float)(buf2 - buf1));
                            printf("reduce leverage: %f\n", (float)(buf2 - buf)/(float)buf1);
                            if (Vec_FltEntry(vCost, 0) == -1.0 || Vec_FltEntry(vCost, 0) < (1.0 - ((float)buf / (float)buf2)) * (float)(buf2 - buf1)) {
                                Vec_FltWriteEntry(vCost, 0, (1.0 - ((float)buf / (float)buf2)) * (float)(buf2 - buf1));
                                Vec_IntWriteEntry(vIntBuff3, 0, nid);
                            }
                            if (Vec_FltEntry(vCost, 1) == -1.0 || Vec_FltEntry(vCost, 1) > (float)buf / (float)buf2) {
                                Vec_FltWriteEntry(vCost, 1, (float)buf / (float)buf2);
                                Vec_IntWriteEntry(vIntBuff3, 1, nid);
                            }
                            if (Vec_FltEntry(vCost, 2) == -1.0 || Vec_FltEntry(vCost, 2) < (float)(buf2 - buf)/(float)buf1) {
                                Vec_FltWriteEntry(vCost, 2, (float)(buf2 - buf)/(float)buf1);
                                Vec_IntWriteEntry(vIntBuff3, 2, nid);
                            }

                            vIntBuff1 = Vec_IntItoV( nid );
                            vIntBuff1 = Cec_ManGetTFO( pMan->pGia, vIntBuff2 = vIntBuff1, 3, 0 );
                            Vec_IntFree( vIntBuff2 );
                            Cec_ManGetMerge( pMan->pGia, vIntBuff1, Vec_WecEntry(pMan->vGSupport, nid), vMerge, 1, 0 );
                            Vec_IntAppend( vIntBuff1, Vec_WecEntry(pMan->vGSupport, nid) );
                            pItp = Cec_ManGetSubcircuit( pMan->pGia, vMerge, vIntBuff1, 1 );
                            Gia_SelfDefShow( pItp, "fd_origin_surrounding.dot", NULL, NULL, NULL );
                            Vec_IntFree( vIntBuff1 );
                            buf2 = Gia_ManAndNum(pItp);
                            pItp = Cec_ManSimpSyn( pTemp2 = pItp, 1, 0, -1 );
                            Gia_ManStop( pTemp2 );
                            buf = Gia_ManAndNum(pItp);
                            printf("syn ratio without patch: %f\n", (float)buf / (float)buf2);
                            printf("amount of reduced nodes: %d\n", buf2 - buf);
                            // ---- old (replace partial)
                            // if (Vec_IntEntry( vRwNd, 1 ) == 1) {
                            //     vIntBuff1 = Vec_IntDup( pItp->vCos );
                            //     Cec_ManGiaMapId( pItp, vIntBuff1 );
                            //     Gia_ManStop( pItp );
                            //     printf("replace the patch\n");
                            //     pItp = Cec_ManSurrounding( pTemp, vIntBuff1, 3, 1 );
                            //     if (Gia_ManCoNum(pItp) > 31) {
                            //         printf("too many co\n");
                            //         Gia_ManStop( pItp );
                            //         Gia_ManStop( pTemp );
                            //         Vec_IntFree( vIntBuff1 );
                            //         continue;
                            //     }
                            //     Vec_IntFree( vIntBuff1 );
                            //     vIntBuff1 = Vec_IntDup( pItp->vCos );
                            //     Cec_ManGiaMapId( pItp, vIntBuff1 );
                            //     vIntBuff2 = Vec_IntDup( pItp->vCis );
                            //     Cec_ManGiaMapId( pItp, vIntBuff2 );
                            //     printf("replace nid = ");
                            //     Vec_IntPrint(vIntBuff1);
                            //     printf("Support");
                            //     Vec_IntPrint(vIntBuff2);
                            //     Gia_ManFillValue( pItp );
                            //     buf = ~0;
                            //     Gia_ManForEachCo( pItp, pObjbuf, j ) {
                            //         Cec_ManSubcircuit( pItp, pObjbuf, 0, 0, 1 << j, 0 );
                            //         buf ^= (1 << j);
                            //     }
                            //     vIntBuff3 = Vec_IntAlloc(100);
                            //     Gia_ManForEachObj( pItp, pObjbuf, j ) {
                            //         if (Gia_ObjValue(pObjbuf) == buf) Vec_IntPush( vIntBuff3, Gia_ObjId( pItp, pObjbuf ) );
                            //     }
                            //     Gia_SelfDefShow( pItp, "fd_surrounding.dot", vIntBuff3, 0, NULL );
                            //     Gia_SelfDefShow( pTemp, "fd_replace.dot", vIntBuff1, vIntBuff2, NULL );
                            //     Gia_AigerWrite( pTemp, "fd_replace.aig", 0, 0, 0 );
                            //     printf("start syn surrounding\n");
                            //     pItp = Cec_ManSimpSyn( pTemp2 = pItp, 1, 0, -1 );
                            //     Gia_ManStop( pTemp2 );
                            //     buf = Gia_ManAndNum(pItp);
                            //     Gia_ManFillValue( pItp );
                            //     buf = ~0;
                            //     Gia_ManForEachCo( pItp, pObjbuf, j ) {
                            //         Cec_ManSubcircuit( pItp, pObjbuf, 0, 0, 1 << j, 0 );
                            //         buf ^= (1 << i);
                            //     }
                            //     vIntBuff3 = Vec_IntAlloc(100);
                            //     Gia_ManForEachObj( pItp, pObjbuf, j ) {
                            //         if (Gia_ObjValue(pObjbuf) == buf) Vec_IntPush( vIntBuff3, Gia_ObjId( pItp, pObjbuf ) );
                            //     }
                            //     Gia_SelfDefShow( pItp, "fd_synSurrounding.dot", vIntBuff3, 0, NULL );
                            //     pTemp = Cec_ManReplacePatch( pTemp2 = pTemp, pItp, vIntBuff1, vIntBuff2, 0, 0);
                            //     Gia_ManStop( pTemp2 );
                            //     Vec_IntFree( vIntBuff1 );
                            //     Vec_IntFree( vIntBuff2 );
                            //     Vec_IntFree( vIntBuff3 );
                            //     vIntBuff1 = Vec_IntDup( pItp->vCos );
                            //     Cec_ManGiaMapId( pItp, vIntBuff1 );
                            //     vIntBuff2 = Vec_IntDup( pItp->vCis );
                            //     Cec_ManGiaMapId( pItp, vIntBuff2 );
                            //     printf("syn: nid = ");
                            //     Vec_IntPrint(vIntBuff1);
                            //     printf("Support");
                            //     Vec_IntPrint(vIntBuff2);
                            //     Vec_IntRemoveAll( vIntBuff2, -1, 0 );
                            //     Gia_SelfDefShow( pTemp, "fd_syn.dot", vIntBuff1, vIntBuff2, NULL );
                            //     Vec_IntFree( vIntBuff1 );
                            //     Vec_IntFree( vIntBuff2 );
                            // }
                            
                            vIntBuff1 = Cec_ManCheckMiter( pTemp );
                            Vec_IntPop( vIntBuff1 );
                            // to modify
                            if (Vec_IntEntry( vRwNd, 1 ) == 0) {
                                vIntBuff2 = Vec_IntDup( pTemp->vCis );
                            } else if (Vec_IntEntry( vRwNd, 2 ) == 1) {
                                vIntBuff2 = Vec_IntAlloc(100);
                                vIntBuff = Cec_ManCheckMiter( pTemp );
                                Cec_ManGetMerge( pTemp, Vec_IntItoV( Vec_IntEntry( vIntBuff, 0 ) ), Vec_IntItoV( Vec_IntEntry( vIntBuff, 1 ) ), vIntBuff2, 0, 1 );
                                Vec_IntFree( vIntBuff );
                            } else {
                                vIntBuff2 = Vec_IntAlloc(100);
                                vIntBuff = Cec_ManCheckMiter( pTemp );
                                Cec_ManGetMerge( pTemp, Vec_IntItoV( Vec_IntEntry( vIntBuff, 0 ) ), Vec_IntItoV( Vec_IntEntry( vIntBuff, 1 ) ), vIntBuff2, 1, 1 );
                                Vec_IntFree( vIntBuff );
                            }
                            
                            pTemp = Cec_ManPartSyn( pTemp2 = pTemp, vIntBuff2, vIntBuff1, -1, 0 );
                            Vec_IntFree( vIntBuff1 );
                            Vec_IntFree( vIntBuff2 );
                            vIntBuff = Cec_ManCheckMiter( pTemp );
                            pOut = Cec_ManFraigSyn( pTemp, 
                                                    Gia_ManObj( pTemp, Vec_IntEntry(vIntBuff, 0)), Gia_ManObj( pTemp, Vec_IntEntry(vIntBuff, 1)), 
                                                    Vec_IntEntry(vIntBuff, 2), 0, pMan->pPars->nBTLimit, 1);
                            Gia_ManStop(pTemp);
                            vIntBuff1 = Cec_ManCheckMiter( pOut );
                            Cec_ManGetMerge_old( pOut, Gia_ManObj( pOut, Vec_IntEntry(vIntBuff1, 0)), Gia_ManObj( pOut, Vec_IntEntry(vIntBuff1, 1)), vMerge );
                            pTemp = Cec_ManGetAbs( pOut, Gia_ManObj( pOut, Vec_IntEntry(vIntBuff1, 0)), Gia_ManObj( pOut, Vec_IntEntry(vIntBuff1, 1)), vMerge, 0 );
                            printf("pOut, pTemp: %d, %d\n", Gia_ManAndNum(pOut), Gia_ManAndNum(pTemp));
                            printf("size change of pAbs = %f\n", (float)Gia_ManAndNum(pTemp) / (float)Gia_ManAndNum(pMan->pAbs) - 1.0);
                            if (Vec_FltEntry(vCost, 3) == -1.0 || Vec_FltEntry(vCost, 3) > (float)Gia_ManAndNum(pTemp) / (float)Gia_ManAndNum(pMan->pAbs) - 1.0) {
                                Vec_FltWriteEntry( vCost, 3, (float)Gia_ManAndNum(pTemp) / (float)Gia_ManAndNum(pMan->pAbs) - 1.0 );
                                Vec_IntWriteEntry( vIntBuff3, 3, nid );
                            }
                            Gia_ManStop( pTemp );
                            Vec_IntFree( vIntBuff );
                            Gia_ManStop( pItp );
                            Gia_ManStop( pOut );
                        }
                    }
                    Vec_FltPrint( vCost );
                    Vec_IntPrint( vIntBuff3 );
                    Vec_IntFree( vIntBuff3 );
                    Vec_FltFree( vCost );
                    Gia_SelfDefShow( pMan->pAbs, "pAbs.dot", vIntBuff4, vCnt1, NULL );
                    Vec_IntFree( vIntBuff4 );
                    Cec_ManFdStop( pMan, 2 );
                    return 0;
                } else if (nodeId == -12) {
                    rwCkt = Vec_IntEntry(vRwNd, 1);
                    // Gia_SelfDefShow( pMan->pGia, "pGia.dot", 0, 0, 0 );
                    // assert( rwCkt == 2 || rwCkt == 1 );
                    if (rwCkt == 1 || rwCkt == 2) {
                        buf1 = Vec_IntEntry(vRwNd, 2);
                        buf2 = Vec_IntEntry(vRwNd, 3);
                        buf3 = Vec_IntEntry(vRwNd, 4);
                        Gia_ManLevelNum( pMan->pAbs );
                        Cec_ManFdTraverseUnknownLevelTest( pMan, buf2, buf3, buf1, rwCkt );
                    } else {
                        buf1 = Vec_IntEntry(vRwNd, 2);
                        Vec_IntForEachEntry( vRwNd, nid, j ) {
                            if (j < 2) continue;
                            else if (j == 2) {
                                buf1 = nid;
                                continue;
                            } 
                            else {
                                Cec_ManFdShrinkLevelTest( pMan, nid, buf1 );
                            }
                        }
                    }
                    
                    vIntBuff1 = Cec_ManFdUnsatId( pMan, 0, 0 );
                    vUnsat = Cec_ManFdUnsatId( pMan, 0, 1 );
                    vSolved = Cec_ManFdSolved( pMan, 0, 0 );
                    Cec_ManFdMapId( pMan, vIntBuff2 = Vec_IntDup( vUnsat ), 1 );
                    Cec_ManFdMapId( pMan, vIntBuff4 = Vec_IntDup( vSolved ), 1 );
                    Gia_SelfDefShow( pMan->pAbs, "nodeStat.dot", vIntBuff2, vIntBuff4, NULL );
                    Vec_IntFree( vIntBuff2 );
                    Vec_IntFree( vIntBuff4 );

                    if (rwCkt == 3) {
                        printf("start checking node init\n");
                        if (pStat) {
                            Vec_IntForEachEntry( pMan->vNodeInit, nid, i ) {
                                if (Gia_ObjColors( pMan->pGia, nid ) != 1 && Gia_ObjColors( pMan->pGia, nid ) != 2) continue;
                                Cec_ManFdShrinkLevelTest( pMan, nid, buf1 );

                            }
                        }
                        return 0;
                    }

                    printf("mapped node info:\n");
                    vIntBuff2 = Vec_IntAlloc(100);
                    vIntBuff3 = Vec_IntAlloc(100);
                    
                    printf("start replacement\n");
                    pTemp = Cec_ManFdReplaceMulti( pMan, vIntBuff1 ); // pTemp --> pGia
                    Gia_AigerWrite( pTemp, "_replaced.aig", 0, 0, 0 );
                    Gia_ManFillValue( pMan->pGia );
                    Cec_ManMapCopy( pMan->pGia, pTemp, 0 ); // pGia --> pTemp
                    Cec_ManGiaMapId( pMan->pGia, vIntBuff2 = Vec_IntDup(pMan->vMerge) );
                    Vec_IntRemoveAll( vIntBuff2, -1, 0 );
                    Cec_ManGiaMapId( pMan->pGia, vIntBuff3 = Vec_IntDup(vSolved) );
                    Vec_IntRemoveAll( vIntBuff3, -1, 0 );
                    Cec_ManGiaMapId( pMan->pGia, vIntBuff4 = Vec_IntDup(vIntBuff1) );
                    Vec_IntRemoveAll( vIntBuff4, -1, 0 );

                    Gia_ManFillValue( pTemp );
                    Vec_IntForEachEntry( vIntBuff4, nid, j ) {
                        Cec_ManSubcircuit( pTemp, Gia_ManObj( pTemp, nid ), 0, 0, 1, 0 );
                    }
                    Cec_ManMark2Val( pTemp, ~0 ^ 1, 0, 1, 0 );
                    vMerge = Cec_ManGetTopMark( pTemp, 1 );
                    Vec_IntForEachEntry( vIntBuff2, nid, j ) {
                        Vec_IntPush( vMerge, nid );
                    }
                    Vec_IntFree( vIntBuff3 );
                    Vec_IntFree( vIntBuff4 );

                    // printf("merges to be done:\n");
                    // Vec_IntPrint( vMerge );
                    pTemp = Cec_ManPartSyn( pTemp2 = pTemp, 0, vMerge, -1, 1 ); // why circuit size not changed
                    printf("after partsyn\n");
                    Cec_ManPrintMapCopy( pTemp );
                    Vec_IntFree( vMerge );
                    Gia_ManFillValue( pTemp2 );
                    Cec_ManMapCopy( pTemp2, pMan->pGia, 0 ); // pTemp2 --> pGia
                    Cec_ManMapCopy( pTemp, pTemp2, 1 ); // pTemp --> pGia
                    Gia_ManFillValue( pMan->pGia );
                    Cec_ManMapCopy( pMan->pGia, pTemp, 0 ); // pGia --> pTemp
                    Cec_ManGiaMapId( pMan->pGia, vUnsat );
                    Cec_ManGiaMapId( pMan->pGia, vSolved );
                    Cec_ManGiaMapId( pMan->pGia, vIntBuff4 = Vec_IntDup(vIntBuff1) );
                    Cec_ManFdMapId( pMan, vIntBuff5 = Vec_IntDup(vIntBuff1), 1 );
                    printf("map of replaced nodes (pGia):\n");
                    Vec_IntForEachEntryTwo( vIntBuff1, vIntBuff4, buf1, buf2, j ) {
                        printf("map %d to %d\n", buf1, buf2);
                    }
                    Vec_IntRemoveAll( vUnsat, -1, 0 );
                    Vec_IntRemoveAll( vSolved, -1, 0 );


                    pOut = pTemp;
                    Cec_ManFdUpdate( pMan, pOut );
                    Cec_ManFdMapId( pMan, vIntBuff4, 1 );
                    printf("map of replaced nodes (pAbs):\n");
                    Vec_IntForEachEntryTwo( vIntBuff5, vIntBuff4, buf1, buf2, j ) {
                        printf("map (%d) to (%d)\n", buf1, buf2);
                    }

                    

                    Cec_ManFdDumpStat( pMan, "partSyn.stat" );
                    Gia_AigerWrite( pMan->pGia, "_partSyn.aig", 0, 0, 0 );
                    Cec_ManFdStop( pMan, 2 );

                    return 0;
                } else if (nodeId == -13) {
                    rwCkt = Vec_IntEntry(vRwNd, 1);
                    assert( rwCkt == 2 || rwCkt == 1 );

                    vIntBuff2 = Vec_IntAlloc( 100 );
                    Gia_ManForEachAnd( pMan->pGia, pObj, j ) {
                        nid = Gia_ObjId( pMan->pGia, pObj );
                        buf = Gia_ObjColors( pMan->pGia, nid );
                        if (buf == 1 || buf == 2) {
                            printf("nid = %d, color = %d\n", nid, buf);
                            if (Cec_ManFdToSolve( pMan, nid )) Cec_ManFdShrinkSimple( pMan, nid );
                            printf("label\n");
                            if (buf != rwCkt && Vec_IntEntry( pMan->vStat, nid ) <= 0) {
                                Vec_IntPush( vIntBuff2, nid );
                            }
                        }
                    }
                    printf("done\n");
                    vUnsat = Cec_ManFdUnsatId( pMan, 0, 1 );
                    
                    // vIntBuff = Vec_IntStart( Gia_ManObjNum( pMan->pGia ) );
                    // Gia_ManForEachObj( pMan->pGia, pObj, j ) {
                    //     if (Gia_ObjColors( pMan->pGia, Gia_ObjId( pMan->pGia, pObj ) ) == 3) 
                    //         Vec_IntWriteEntry( vIntBuff, Gia_ObjId( pMan->pGia, pObj ), -1 );
                    //     else if (Vec_IntEntry( pMan->vStat, Gia_ObjId( pMan->pGia, pObj ) ) <= 0)
                    //         Vec_IntWriteEntry( vIntBuff, Gia_ObjId( pMan->pGia, pObj ), -1 );
                    //     else {
                    //         buf = Vec_FltEntry( pMan->vCostTh, Gia_ObjId( pMan->pGia, pObj ) ); // Gia_ManObjNum( (Gia_Man_t*) Vec_PtrEntry( pMan->vVeryStat, Gia_ObjId( pMan->pGia, pObj ) ) );
                    //         // Vec_IntForEachEntry( Vec_WecEntry( pMan->vGSupport, Gia_ObjId( pMan->pGia, pObj ) ), nid, k ) {
                    //         //     if (Gia_ObjColors( pMan->pGia, nid ) == 3) continue;
                    //         //     buf += Vec_FltEntry( pMan->vCostTh, nid );
                    //         // }
                    //         // buf = Abc_MaxInt( 0, buf - 5 );
                    //         buf = 20000 * Gia_ManObjNum( (Gia_Man_t*) Vec_PtrEntry( pMan->vVeryStat, Gia_ObjId( pMan->pGia, pObj ) ) ) / buf;
                    //         Vec_IntWriteEntry( vIntBuff, Gia_ObjId( pMan->pGia, pObj ), buf );
                    //         printf("nid = %d, cost = %d\n", Cec_ManFdMapIdSingle( pMan, Gia_ObjId( pMan->pGia, pObj ), 1), buf );
                    //     }
                    // }
                    // Vec_IntSetCeil( vIntBuff, 100, -1 );
                    // Vec_IntSetCeil( vIntBuff, 100, 2 );
                    // Cec_ManFdPlot( pMan, 1, 0, 5, "cost.dot", vIntBuff, 0, 0 );
                    
                    buf = rwCkt == 1 ? Gia_ObjId(pMan->pGia, pMan->pObj2) : Gia_ObjId(pMan->pGia, pMan->pObj1);
                    if ( Vec_IntEntry( pMan->vStat, buf ) > 0 || Vec_IntEntry( pMan->vStat, buf ) == CEC_FD_HUGE ) {
                        printf("PO patch is found\n");
                        return 0;
                    }
                    vUnsat = Cec_ManFdUnsatId( pMan, 0, 0 );
                    printf("initial #Unsat: %d\n", Vec_IntSize( vUnsat ) );

                    // Cec_ManFdPlot( pMan, 1, 0, 0, "pickNode.dot", 0, vUnsat, 0 );
                    // return 0;

                    buf1 = 0; buf2 = 0;
                    Vec_IntForEachEntry( pMan->vNodeInit, nid, j ) {
                        if ( Gia_ObjColors( pMan->pGia, nid ) != 3 ) buf1++;
                        if ( Vec_IntFind( vIntBuff2, nid ) >= 0 ) buf2++;
                    }
                    printf("there are %d initial node to go\n", buf1 );
                    printf("there are %d node still UNSAT in ckt %d\n", buf2, rwCkt );
                    printf("start replace node finding\n");
                    
                    Gia_SelfDefShow( pMan->pGia, "init.dot", NULL, NULL, NULL );
                    Gia_ManFillValue( pMan->pGia );
                    Cec_ManSubcircuit( pMan->pGia, Gia_ManCo( pMan->pGia, 0 ), vUnsat, 0, 1, 0 );
                    vIntBuff1 = Vec_IntAlloc( Vec_IntSize( vUnsat ) );
                    Vec_IntForEachEntry( vUnsat, nid, j ) {
                        buf = Gia_ObjColors( pMan->pGia, nid );
                        if (buf == rwCkt && Gia_ManObj( pMan->pGia, nid )->Value != -1) Vec_IntPush( vIntBuff1, nid );
                    }
                    Cec_ManReduceFrontier( pMan->pGia, vIntBuff1, 5, 0 );
                    if (Vec_IntSize( vIntBuff1 ) == 0) {
                        printf("no patch is found\n");
                        return 0;
                    }
                    Cec_ManFdPlot( pMan, 1, 0, 0, "pickNode.dot", vUnsat, vIntBuff1, vIntBuff2 );
                    Vec_IntForEachEntry( vIntBuff1, nid, j ) {
                        printf("nid = %d, patch size = %d\n", nid, Gia_ManAndNum( (Gia_Man_t*) Vec_PtrEntry( pMan->vVeryStat, nid ) ) );
                    }


                    printf("start replacement\n");
                    pTemp = Cec_ManFdReplaceMulti( pMan, vIntBuff1 );
                    buf = 0;
                    Gia_ManForEachObj( pTemp, pObj, j ) {
                        if (pObj->Value != -1) buf++;
                    }
                    Gia_AigerWrite( pTemp, "replaced.aig", 0, 0, 0 );
                    Gia_SelfDefShow( pTemp, "replaced.dot", NULL, NULL, NULL );
                    pOut = Gia_ManDup( pTemp );
                    Gia_ManFillValue( pMan->pGia );
                    Cec_ManMapCopy( pMan->pGia, pTemp, 0 );
                    Cec_ManGiaMapId( pMan->pGia, vIntBuff3 = Vec_IntDup(vUnsat) );
                    Vec_IntRemoveAll( vIntBuff3, -1, 0 );
                    Gia_ManFillValue( pTemp );
                    if (Vec_IntEntry( vRwNd, 2 ) == 0) {
                        Vec_IntForEachEntry( vIntBuff3, nid, j ) {
                            Cec_ManSubcircuit( pTemp, Gia_ManObj( pTemp, nid ), 0, 0, 1, 0 );
                        }
                        Vec_IntForEachEntry( pMan->vMerge, nid, j ) {
                            Cec_ManSubcircuit( pTemp, Gia_ManObj( pTemp, nid ), 0, 0, 1, 0 );
                        }
                    } else if (Vec_IntEntry( vRwNd, 2 ) == 1) {
                        pObjbuf = rwCkt == 1 ? Gia_ObjCopy( pTemp, pMan->pObj1 ) : Gia_ObjCopy( pTemp, pMan->pObj2 );
                        Cec_ManSubcircuit( pTemp, pObjbuf, 0, 0, 1, 0 );
                        pObjbuf->Value = -1;
                        Vec_IntForEachEntry( vIntBuff3, nid, j ) {
                            Cec_ManSubcircuit( pTemp, Gia_ManObj( pTemp, nid ), 0, 0, 1, 0 );
                        }
                        Vec_IntForEachEntry( pMan->vMerge, nid, j ) {
                            Cec_ManSubcircuit( pTemp, Gia_ManObj( pTemp, nid ), 0, 0, 1, 0 );
                        }
                    }
                    Vec_IntFree( vIntBuff3 );
                    Cec_ManMark2Val( pTemp, ~0 ^ 1, 0, 1, 0 );
                    vMerge = Cec_ManGetTopMark( pTemp, 0 );

                    pTemp = Cec_ManPartSyn( pTemp2 = pTemp, 0, vMerge, -1, 1 ); // why circuit size not changed
                    Vec_IntFree( vMerge );
                    Gia_ManFillValue( pTemp2 );
                    Cec_ManMapCopy( pTemp2, pMan->pGia, 0 ); // pTemp2 --> pGia
                    Cec_ManMapCopy( pTemp, pTemp2, 1 ); // pTemp --> pGia
                    Gia_ManFillValue( pMan->pGia );
                    Cec_ManMapCopy( pMan->pGia, pTemp, 0 ); // pGia --> pTemp2

                    pTemp = Cec_ManFraigSyn_naive( pTemp2 = pTemp, pMan->pPars->nBTLimit );
                    if (Gia_ManAndNum( pTemp ) == Gia_ManAndNum( pTemp2 )) {
                        Gia_ManStop( pTemp );
                        pTemp = pTemp2;
                        Gia_ManFillValue( pTemp );
                        Cec_ManMapCopy( pTemp, pMan->pGia, 0 ); // pTemp --> pGia
                    } else {
                        printf("found equivalence node....forcing fraig\n");
                        Gia_ManStop( pTemp2 );
                        Cec_ManMatch( pTemp, pMan->pGia ); // pTemp --> pGia
                        Gia_ManForEachObj( pTemp, pObj, j ) {
                            if (Gia_ObjValue(pObj) < 0) {
                                // if (Gia_ObjValue(pObj) != -1) printf("Nid: %d\n", Gia_ObjId(pTemp, pObj));
                                pObj->Value = -1;
                            }
                        }
                        Gia_ManFillValue( pMan->pGia );
                        Cec_ManMapCopy( pMan->pGia, pTemp, 0 ); // pGia --> pTemp
                    }

                    Gia_AigerWrite( pTemp, "partSyn.aig", 0, 0, 0 );
                    Gia_SelfDefShow( pTemp, "partSyn.dot", 0, 0, 0 );
                    pFile = fopen( "partSyn.stat", "w" );
                    buf = 0;
                    Vec_IntForEachEntry( pMan->vNodeInit, nid, j ) {
                        if (Gia_ObjValue(Gia_ManObj(pMan->pGia, nid)) == -1) continue;
                        if (buf % 10 == 0) fprintf( pFile, "f ");
                        fprintf(pFile, "%d ", Abc_Lit2Var(Gia_ObjValue(Gia_ManObj(pMan->pGia, nid))) );
                        if (buf % 10 == 9) fprintf( pFile, "\n" );
                        buf++;
                    }
                    if (buf % 10 != 0) fprintf( pFile, "\n" );
                    fclose( pFile );
                    // buf = 0;
                    // Gia_ManForEachObj( pTemp, pObj, j ) {
                    //     if (pObj->Value != -1) buf++;
                    // }
                    // printf("#Aig copied = %d\n", buf);
                    vIntBuff4 = Cec_ManDumpValue( pTemp );

                    printf("start conflict test\n");

                    // pMan->pPars->fVerbose = 0;
                    pMan_buf = Cec_ManFdStart( pTemp, pMan->pPars, NULL );
                    Gia_ManForEachAnd( pMan_buf->pGia, pObj, j ) {
                        nid = Gia_ObjId( pMan_buf->pGia, pObj );
                        if (Vec_IntEntry( vIntBuff4, nid ) == -1) continue;
                        if (Vec_IntEntry( vIntBuff4, nid ) < 0) {
                            printf("nid %d have value %d\n", nid, Vec_IntEntry( vIntBuff4, nid ));
                            continue;
                        }
                        // printf("nid %d have value %d\n", nid, Vec_IntEntry( vIntBuff4, nid ));
                        buf = Gia_ObjColors( pMan_buf->pGia, nid );
                        if (buf != 1 && buf != 2) continue;
                        buf = Abc_Lit2Var(Vec_IntEntry( vIntBuff4, nid ));
                        if (Vec_IntFind( vIntBuff2, buf ) != -1) {
                            // printf("nid %d can be mapped to %d\n", nid, Abc_Lit2Var( Vec_IntEntry( vIntBuff4, nid )));
                            Cec_ManFdShrinkSimple( pMan_buf, nid );
                        }
                    }
                    printf("lable conflict\n");
                    buf = 0;
                    Vec_IntForEachEntry( vIntBuff4, buf, j ) {
                        // printf("nid %d have value %d\n", j, buf);
                        if (buf != -1) {
                            if (buf < 0) {
                                printf("buf: %d\n", buf);
                                continue;
                            }
                            if (Abc_Lit2Var( buf ) >= Vec_IntSize( pMan->vStat )) printf("nid %d is copied to not exists node %d\n", j, Abc_Lit2Var( buf ));
                            else if (Vec_IntEntry( pMan->vStat, Abc_Lit2Var( buf ) ) > 0 && Vec_IntEntry( pMan_buf->vStat, j ) > 0) {
                                printf("nid = %d(%d) is double find\n", Abc_Lit2Var( buf ), j);
                            }
                        }
                    }
                    printf("start final\n");

                    // pMan->pPars->fVerbose = 1;
                    vIntBuff5 = Cec_ManFdUnsatId( pMan_buf, 0, 0 );
                    printf("newly find #UNSAT: %d\n", Vec_IntSize(vIntBuff5));
                    vIntBuff6 = Vec_IntDup( vIntBuff5 );
                    Cec_ManLoadValue( pTemp, vIntBuff4 );
                    Cec_ManGiaMapId( pTemp, vIntBuff6 );
                    Cec_ManFdPlot( pMan, 1, 0, 0, "final.dot", vUnsat, vIntBuff6, 0 );

                    Vec_IntForEachEntryTwo( vIntBuff5, vIntBuff6, buf1, buf2, j ) {
                        printf( "nid = %d(%d)\n", buf2, buf1 );
                        printf( "conflict = %d\n", Vec_IntEntry( pMan_buf->vConf, buf1 ) );
                        printf( "patch = %d\n", Gia_ManAndNum( (Gia_Man_t*) Vec_PtrEntry( pMan_buf->vVeryStat, buf1 ) ) );
                        if (Vec_IntFind( pMan->vNodeInit, buf2 ) != -1) printf( "node is in initial circuit\n");
                        else printf( "node is an added node\n" );
                        if (buf2 > 0) printf( "org status = %d\n", Vec_IntEntry( pMan->vStat, buf2 ) );
                        else printf( "somehow not a copied nid\n" );
                    }
                    Cec_ManFdStop( pMan_buf, 2 );
                    if (Vec_IntSize(vIntBuff5) == 0) {
                        printf("conflict test failed\n");
                        return 0;
                    }

                    // final syn
                    pTemp = pOut;
                    vIntBuff = Cec_ManCheckMiter( pTemp );
                    Vec_IntPop( vIntBuff );
                    pTemp = Cec_ManPartSyn( pTemp2 = pTemp, 0, vIntBuff, -1, 0 );
                    Gia_ManStop( pTemp2 );
                    Vec_IntFree( vIntBuff );
                    pOut = Cec_ManFraigSyn_naive( pTemp, 10000 );
                    
                    Cec_ManMatch( pOut, pMan->pGia );
                    Cec_ManFdUpdate( pMan, pOut );
                    Cec_ManFdDumpStat( pMan, "final.log" );
                    Gia_AigerWrite( pOut, "final.aig", 0, 0, 0 );
                    Cec_ManFdStop( pMan, 2 );
                    return pOut;

                } else if (nodeId == -14) {
                    // vIntBuff = Vec_IntAlloc( 100 );
                    vIntBuff2 = Vec_IntAlloc( 100 );
                    vIntBuff3 = Cec_ManTFISize( pMan->pAbs );
                    rwCkt = Vec_IntEntry( vRwNd, 1 );
                    buf1 = Vec_IntEntry( vRwNd, 2 );
                    buf2 = Vec_IntEntry( vRwNd, 3 );
                    buf4 = Vec_IntEntry( vRwNd, 4 );
                    buf3 = Gia_ManLevelNum(pMan->pAbs);
                    
                    Gia_ManForEachAnd( pMan->pAbs, pObj, i ) {
                        if (Gia_ObjLevel( pMan->pAbs, pObj ) < buf1) continue;
                        if (Gia_ObjLevel( pMan->pAbs, pObj ) > buf2) continue;
                        nid = Cec_ManFdMapIdSingle( pMan, i, 0 );
                        if (Gia_ObjColors( pMan->pGia, nid ) != rwCkt) continue;
                        Cec_ManFdShrinkSimple( pMan, nid );
                        if (Vec_IntEntry( pMan->vStat, nid ) > 0) {
                            vWecBuff = Vec_WecStart( buf3 );
                            Vec_IntForEachEntry( Vec_WecEntry( pMan->vGSupport, nid), buf, j ) {
                                Vec_WecPush( vWecBuff, Gia_ObjLevelId( pMan->pAbs, Cec_ManFdMapIdSingle( pMan, buf, 1 ) ), buf );
                            }
                            
                            for (buf = 1; buf < buf3; buf++) {
                                vIntBuff = Vec_IntDup( Vec_WecEntry( vWecBuff, 0 ) );
                                Vec_WecForEachLevel( vWecBuff, vIntBuff1, j ) {
                                    if (j < buf) continue;
                                    else Vec_IntAppend( vIntBuff, vIntBuff1 );
                                }
                                pTemp = Cec_ManFdGetFd( pMan, nid, vIntBuff, 0 );
                                if (pTemp != NULL && pTemp != 1) {
                                    Cec_ManFdUpdateStat( pMan, nid, vIntBuff, pTemp, 0 );
                                } else break;
                                Vec_IntFree( vIntBuff );
                            }

                            buf6 = Gia_ManAndNum( (Gia_Man_t*) Vec_PtrEntry( pMan->vVeryStat, nid ) );
                            buf5 = Vec_IntEntry( vIntBuff3, i );
                            printf("size of circuit is %d, TFI is %d\n", buf6, buf5);
                            if (buf4 == -1 || buf6 < buf4 * (buf5 + 5)) Vec_IntPush( vIntBuff2, nid );
                        }
                        
                    }

                    Gia_ManFillValue( pMan->pGia );
                    Vec_IntForEachEntry( vIntBuff2, nid, i ) {
                        pObj = Gia_ManObj(pMan->pGia, nid);
                        if (Gia_ObjValue( pObj) != -1) continue;
                        Cec_ManSubcircuit( pMan->pGia, Gia_ManObj(pMan->pGia, nid), 0, 0, 1, 0 );
                        Gia_ObjSetValue( pObj, -1 );
                    }
                    Vec_IntForEachEntry( vIntBuff2, nid, i ) {
                        pObj = Gia_ManObj(pMan->pGia, nid);
                        if (Gia_ObjValue( pObj ) == -1) continue;
                        else Vec_IntWriteEntry( vIntBuff2, i, -1 );
                    }
                    Vec_IntRemoveAll( vIntBuff2, -1, 0 );
                    if (Vec_IntSize( vIntBuff2 ) > 0) {
                        pTemp = Cec_ManFdReplaceMulti( pMan, vIntBuff2 );
                        pTemp = Cec_ManSimpSyn( pTemp2 = pTemp, 1, 0, -1);
                        Gia_ManStop( pTemp2 );
                        pTemp = Cec_ManFraigSyn_naive( pTemp2 = pTemp, pMan->pPars->nBTLimit );
                        Gia_ManStop( pTemp2 );
                        Gia_AigerWrite( pTemp, "replaced.aig", 0, 0, 0 );
                    }
                    
                    return 0;
                } else if (nodeId == -15) {
                    vIntBuff2 = Vec_IntAlloc( 100 );
                    vIntBuff3 = Cec_ManTFISize( pMan->pAbs );
                    rwCkt = Vec_IntEntry( vRwNd, 1 );
                    
                    vIntBuff4 = Vec_IntAlloc( 100 );
                    vIntBuff5 = Vec_IntAlloc( 100 );
                    vCnt1 = Vec_IntAlloc( 100 );
                    vCnt2 = Vec_IntAlloc( 100 );
                    vCnt3 = Vec_IntAlloc( 100 );
                    vCnt4 = Vec_IntAlloc( 100 );
                    
                    Cec_ManMergeAnal( pMan->pGia, vIntBuff4, vIntBuff5, 1 );
                    vCnt1 = Vec_IntDup( vIntBuff4 );
                    vCnt2 = Vec_IntDup( vIntBuff5 );
                    pOut = NULL;
                    // Vec_IntFill( vCnt1, Vec_IntSize(vIntBuff4), 0 );
                    // Vec_IntFill( vCnt2, Vec_IntSize(vIntBuff5), 0 );
                    buf1 = 0; buf2 = 0; 
                    Gia_ManForEachAnd( pMan->pAbs, pObj, i ) {
                        nid = Cec_ManFdMapIdSingle( pMan, i, 0 );
                        if (Gia_ObjColors( pMan->pGia, nid ) != rwCkt) continue;
                        Cec_ManFdShrink( pMan, nid );
                        if (Vec_IntEntry( pMan->vStat, nid ) > 0) {

                            pTemp = Cec_ManFdReplaceOne( pMan, nid );
                            pTemp = Cec_ManSimpSyn( pTemp2 = pTemp, 1, 0, -1);
                            Gia_ManStop( pTemp2 );
                            pTemp = Cec_ManFraigSyn_naive( pTemp2 = pTemp, pMan->pPars->nBTLimit );
                            Gia_ManStop( pTemp2 );
                            // Vec_IntClear( vCnt3 );
                            // Vec_IntClear( vCnt4 );
                            vCnt3 = Vec_IntAlloc( 100 );
                            vCnt4 = Vec_IntAlloc( 100 );
                            Cec_ManMergeAnal( pTemp, vCnt3, vCnt4, 1 );
                            // Vec_IntPrint( vIntBuff4 );
                            // Vec_IntPrint( vIntBuff5 );
                            Vec_IntPrint(vCnt1);
                            Vec_IntPrint(vCnt2);
                            if (rwCkt == 1) {
                                // buf3 = Cec_ManMergeChangeCompare( vCnt4, vIntBuff5, 0 );
                                buf3 = Cec_ManMergeChangeCompare( vCnt3, vIntBuff4, 0 );
                                buf4 = Cec_ManMergeChangeCompare( vCnt4, vCnt2, 0 );
                                buf5 = Cec_ManMergeChangeCompare( vCnt3, vCnt1, 0 );
                                printf("buf3 = %d, buf4 = %d, buf5 = %d\n", buf3, buf4, buf5);
                                if (buf3 > Vec_IntSize(vIntBuff4) + 2) continue;
                                if (buf4 > 0) continue;
                                if (buf4 == 0 && buf5 > 0) continue;
                                Vec_IntFree( vCnt1 );
                                Vec_IntFree( vCnt2 );
                                vCnt1 = vCnt3;
                                vCnt2 = vCnt4;
                                pOut = Gia_ManDup( pTemp );
                            } else {
                                buf3 = Cec_ManMergeChangeCompare( vCnt4, vIntBuff5, 0 );
                                buf4 = Cec_ManMergeChangeCompare( vCnt3, vCnt1, 0 );
                                buf5 = Cec_ManMergeChangeCompare( vCnt4, vCnt2, 0 );
                                printf("buf3 = %d, buf4 = %d, buf5 = %d\n", buf3, buf4, buf5);
                                if (buf3 > Vec_IntSize(vIntBuff5) + 2) continue;
                                if (buf4 > 0) continue;
                                if (buf4 == 0 && buf5 > 0) continue;
                                Vec_IntFree( vCnt1 );
                                Vec_IntFree( vCnt2 );
                                vCnt1 = vCnt3;
                                vCnt2 = vCnt4;
                                pOut = Gia_ManDup( pTemp );
                            }
                            Vec_IntPrint(vCnt1);
                            Vec_IntPrint(vCnt2);
                            Gia_ManStop( pTemp );
                        }
                    }

                    if (pOut) Gia_AigerWrite( pOut, "replaced.aig", 0, 0, 0 );
                    
                    return 0;
                } else if (nodeId == -16) {
                    vIntBuff = Vec_IntDup( vRwNd );
                    buf = Vec_IntEntry( vRwNd, 1 );
                    Vec_IntDrop( vIntBuff, 0 );
                    Vec_IntDrop( vIntBuff, 0 );
                    vIntBuff1 = Vec_IntAlloc( 100 );
                    vIntBuff2 = Vec_IntAlloc( 100 );
                    buf1 = 0;
                    if (buf == 1) {
                        Vec_IntForEachEntry( vIntBuff, nid, j ) {
                            if (nid >= 0) {
                                if (buf1 == 0) Vec_IntPush( vIntBuff1, nid );
                                else if (buf1 == 1) Vec_IntPush( vIntBuff2, nid );
                            } else buf1++;
                        }
                        // Vec_IntPrint(vIntBuff1);
                        // Vec_IntPrint(vIntBuff2);
                        Cec_ManFdMapId( pMan, vIntBuff1, 1 );
                        Cec_ManFdMapId( pMan, vIntBuff2, 1 );
                        Gia_SelfDefShow( pMan->pAbs, "pAbs_color.dot", vIntBuff1, vIntBuff2, 0);
                    } else if (buf == 0) {
                        Vec_IntForEachEntry( vIntBuff, nid, j ) {
                            if (nid >= 0) {
                                if (buf1 == 0) Vec_IntPush( vIntBuff1, nid );
                                else if (buf1 == 1) Vec_IntPush( vIntBuff2, nid );
                            } else buf1++;
                        }
                        Gia_SelfDefShow( pMan->pGia, "pGia_color.dot", vIntBuff1, vIntBuff2, 0);
                    }
                    return 0;

                } else {
                    Vec_IntPrint(vRwNd);
                    // assert(Vec_IntSize(vRwNd) == 6);
                    frac = Vec_IntEntry( vRwNd, 1 );
                    up = Vec_IntEntry( vRwNd, 2 );
                    bot = Vec_IntEntry( vRwNd, 3 );
                    rwCkt = Vec_IntSize( vRwNd ) > 4 ? Vec_IntEntry( vRwNd, 4 ) : -1;
                    frtMin = Vec_IntSize( vRwNd ) > 5 ? Vec_IntEntry( vRwNd, 5 ) : -1;

                    assert(up < bot); // level decrease as going up
                    buf1 = -1;
                    buf2 = -1;
                    Vec_IntForEachEntry( pMan->vMerge, buf, j) {
                        if (Vec_IntEntry(pMan->vLevel1, buf) > buf1) buf1 = Vec_IntEntry(pMan->vLevel1, buf);
                        if (Vec_IntEntry(pMan->vLevel2, buf) > buf2) buf2 = Vec_IntEntry(pMan->vLevel2, buf);
                    }
                    printf("level range: %d,%d~%d,%d\n", buf1 * up / frac, buf2 *up /frac, buf1 * bot / frac, buf2 * bot / frac);

                    Gia_ManForEachAnd( pMan->pGia, pObj, j ) { 
                        nid = Gia_ObjId(pMan->pGia, pObj);
                        if ( Gia_ObjColors(pMan->pGia, nid) == 1 ) {
                            if ( Vec_IntEntry(pMan->vLevel1, nid) * frac < buf1 * up ) continue;
                            if ( Vec_IntEntry(pMan->vLevel1, nid) * frac >= buf1 * bot ) continue;
                            Cec_ManFdCleanOneUnknown( pMan, nid );
                            Cec_ManFdShrink( pMan, nid );
                        } else if ( Gia_ObjColors(pMan->pGia, nid) == 2) {
                            if ( Vec_IntEntry(pMan->vLevel2, nid) * frac < buf2 * up ) continue;
                            if ( Vec_IntEntry(pMan->vLevel2, nid) * frac >= buf2 * bot ) continue;
                            Cec_ManFdCleanOneUnknown( pMan, nid );
                            Cec_ManFdShrink( pMan, nid );
                        }
                    }
                    Cec_ManFdIncreIter( pMan );
                    if (pPars->fVerbose) Cec_ManFdReport( pMan );
                    
                    printf("UNSAT nodes:\n");
                    Vec_IntPrint( Cec_ManFdUnsatId( pMan, 0, 1 ) );
                    printf("Useful nodes:\n");
                    vIntBuff = Cec_ManFdUnsatId( pMan, 0, 0 );
                    Vec_IntPrint( vIntBuff );

                    Vec_IntForEachEntry( vIntBuff, nid, j ) {
                        if (rwCkt == -1) continue;
                        if (Gia_ObjColors(pMan->pGia, nid) != rwCkt) {
                            Vec_IntWriteEntry( vIntBuff, j, -1 );
                        }
                    }

                    Cec_ManDistEasy( pMan->pGia, pMan->vMerge, Gia_ObjId(pMan->pGia, pMan->pObj1), 1 );
                    vCntInit1 = Cec_ManCountVal( pMan->pGia, pMan->vMerge );
                    vCnt1 = Vec_IntDup( vCntInit1 );
                    Cec_ManDistEasy( pMan->pGia, pMan->vMerge, Gia_ObjId(pMan->pGia, pMan->pObj2), 1 );
                    vCntInit2 = Cec_ManCountVal( pMan->pGia, pMan->vMerge );
                    vCnt2 = Vec_IntDup( vCntInit2 );
                    printf("initial cover counts 1: ");
                    Vec_IntPrint(vCnt1);
                    printf("initial cover counts 2: ");
                    Vec_IntPrint(vCnt2);
                    printf("=========================\n");

                    if (nodeId == -10) {
                        vIntBuff2 = Vec_IntStart( Vec_IntSize(vIntBuff) );
                        Vec_IntFill( vIntBuff2, Vec_IntSize(vIntBuff), -1 );

                        Vec_IntForEachEntry( vIntBuff, nid, j ) {
                            printf("nid = %d\n", nid);
                            if (nid == -1) continue;
                            // rewrite
                            buf1 = pMan->dist_short[Cec_ManFdGetPO( pMan, nid, 0, 1 )][nid];

                            buf5 = -1;
                            Vec_IntForEachEntry( Vec_WecEntry( pMan->vMergeSupport, nid ), buf, k ) {
                                // nid2 might not be in pAbs
                                if (buf != 1) continue;
                                nid2 = Vec_IntEntry( pMan->vMerge, k );
                                buf2 = pMan->dist_short[Cec_ManFdGetPO( pMan, nid, 0, 0 )][nid2];
                                assert(buf2 > 0);
                                buf2 += pMan->dist_short[nid][nid2];
                                if (buf5 == -1 || buf2 < buf5) {
                                    buf5 = buf2;
                                }
                            }
                            printf("min dist of nid %d is %d\n", nid, buf1 + buf5);
                            pTemp = (Gia_Man_t*) Vec_PtrEntry( pMan->vVeryStat, nid );
                            Cec_ManDistEasy( pTemp, 0, Gia_ManCoDriverId(pTemp, 0), 0 );
                            
                            buf = -1;
                            Vec_IntForEachEntry( Vec_WecEntry( pMan->vGSupport, nid ), nid2, k ) {
                                // nid2 might not be in pAbs
                                buf2 = pMan->dist_short[Cec_ManFdGetPO( pMan, nid, 0, 0 )][nid2];
                                assert(buf2 > 0);
                                buf2 += Gia_ObjValue(Gia_ManCi(pTemp, k));
                                if (buf2 < buf5) {
                                    buf5 = buf2;
                                    buf = nid2;
                                    printf("decrease to %d, %d, %d by nid2 = %d\n", buf1, pMan->dist_short[Cec_ManFdGetPO( pMan, nid, 0, 0 )][nid2], Gia_ObjValue(Gia_ManCi(pTemp, k)), nid2);
                                }
                            }
                            if (buf == -1) continue;
                            else Vec_IntWriteEntry( vIntBuff2, j, buf1 + buf5 );
                            
                        }
                        Vec_IntPrint( vIntBuff2 );
                        buf = Vec_IntFindMax( vIntBuff2 ) + 1;
                        Vec_IntReplace( vIntBuff2, -1, buf );

                        vIntBuff1 = Vec_IntAlloc( 10 );
                        while (Vec_IntSize(vIntBuff1) < Abc_MinInt(Vec_IntSize(vIntBuff), 5)) {
                            if (Vec_IntFindMin( vIntBuff2 ) == buf) break;
                            buf1 = Vec_IntArgMin( vIntBuff2 );
                            Vec_IntWriteEntry( vIntBuff2, buf1, buf );
                            
                            while (Vec_IntSize(vIntBuff1) != 0 && 
                                   Gia_ObjColors(pMan->pGia, Vec_IntEntry(vIntBuff1, 0)) != Gia_ObjColors(pMan->pGia, Vec_IntEntry( vIntBuff, buf1 ))) {
                                    continue;
                                }
                            Vec_IntPush( vIntBuff1, Vec_IntEntry( vIntBuff, buf1 ) );
                        }
                    } else if (nodeId == -11) {
                        vIntBuff1 = Vec_IntAlloc( 10 );
                        vCnt3 = Vec_IntAlloc( 10 );
                        vCnt4 = Vec_IntAlloc( 10 );
                        buf1 = Vec_IntSize( vCnt1 );
                        buf2 = Vec_IntSize( vCnt2 );
                        cost_buf1 = Cec_ManMergeUniformality( vCntInit1, buf1 );
                        cost_buf2 = Cec_ManMergeUniformality( vCntInit2, buf2 );
                        printf("initial: %f, %f\n", cost_buf1, cost_buf2);
                        while (Vec_IntSize(vIntBuff1) < Abc_MinInt(Vec_IntSize(vIntBuff), 5)) {
                            // cost_buf1 = Cec_ManMergeUniformality( vCnt1, buf1 );
                            // cost_buf2 = Cec_ManMergeUniformality( vCnt2, buf2 );
                            buf = -1;
                            printf("==================== current: %f, %f with vIntBuff1 = ", cost_buf1, cost_buf2);
                            Vec_IntPrint(vIntBuff1);
                            vIntBuff2 = Vec_IntDup( vIntBuff1 );
                            Vec_IntPush( vIntBuff2, -1 );
                            Vec_IntForEachEntry( vIntBuff, nid, j ) {
                                Vec_IntWriteEntry( vIntBuff2, Vec_IntSize(vIntBuff2) - 1, nid );
                                printf("........nid = %d\n", nid);
                                if (nid == -1) continue;
                                if (Vec_IntFind( vIntBuff1, nid ) != -1) {
                                    continue;
                                }
                                // pTemp = Cec_ManFdReplaceOne( pMan, nid );
                                pTemp = Cec_ManFdReplaceMulti( pMan, vIntBuff2 );
                                Vec_IntClear( vCnt3 );
                                Vec_IntClear( vCnt4 );
                                Cec_ManMergeAnal( pTemp, vCnt3, vCnt4, 1 );
                                cost_buf3 = Cec_ManMergeUniformality( vCnt3, buf1 );
                                cost_buf4 = Cec_ManMergeUniformality( vCnt4, buf2 );
                                if (cost_buf1 + cost_buf2 > cost_buf3 + cost_buf4) {
                                    if (Vec_IntSize(vIntBuff1) != 0 && 
                                         Gia_ObjColors(pMan->pGia, Vec_IntEntry(vIntBuff1, 0)) != Gia_ObjColors(pMan->pGia, nid)) {
                                            break;
                                        }
                                    // vCnt1 = Vec_IntDup( vCnt3 );
                                    // vCnt2 = Vec_IntDup( vCnt4 );
                                    cost_buf1 = cost_buf3;
                                    cost_buf2 = cost_buf4;
                                    buf = nid;
                                    printf("decrease to %f, %f with nid = %d\n", cost_buf1, cost_buf2, nid);
                                }
                            }
                            if ( buf != -1 ) {
                                Vec_IntPush( vIntBuff1, buf );
                            }
                            else break;
                            Vec_IntFree( vIntBuff2 );
                        }
                           
                        // while (Vec_IntSize(vIntBuff1) < Abc_MinInt(10, Vec_IntSize(vIntBuff))) {
                        //     buf1 = Cec_ManMergeUniformality( vCnt1, buf3 );
                        //     buf2 = Cec_ManMergeUniformality( vCnt2, buf4 );  
                        //     Vec_IntForEachEntry( vIntBuff, nid, j ) {
                        //         if (nid == -1) continue;
                        //     }
                        // }
                    }
                    Vec_IntDump( "cover_nodes.log", vIntBuff1, 0 );
                    Cec_ManFdStop( pMan, 2 );
                    return 0;
                }

            } else {
                nid = Cec_ManFdMapIdSingle( pMan, nodeId, 1 );
                if (nid < 0) assert(0);
                Cec_ManFdCleanOneUnknown( pMan, nodeId );
                // Cec_ManFdShrink( pMan, nodeId );
                // Cec_ManFdIncreIter( pMan );
                printf("afterall status:%d\n", Vec_IntEntry(pMan->vStat, nodeId));
            }
        }
        Cec_ManFdIncreIter( pMan );
        if (pPars->fVerbose) Cec_ManFdReport( pMan );

        flag = 0;
        if (pPars->fGetCkts) {
            Vec_IntForEachEntry( vRwNd, nodeId, i ) {
                if (Vec_IntEntry(pMan->vStat, nodeId) > 0 || Vec_IntEntry(pMan->vStat, nodeId) == CEC_FD_HUGE) {
                    flag |= Gia_ObjColors(pMan->pGia, nodeId);
                    nid = Cec_ManFdMapIdSingle( pMan, nodeId, 1 );
                    if (pMan->pAbs->pRefs == NULL) 
                        Gia_ManCreateRefs(pMan->pAbs);

                    sprintf(str, "%d(%d)_patch.dot", nodeId, nid);
                    Gia_SelfDefShow( (Gia_Man_t*) Vec_PtrEntry(pMan->vVeryStat, nodeId), str, 0, 0, 0 );

                    pTemp = Cec_ManFdVisSupport( pMan, nodeId, 0 );
                    sprintf(str, "%d(%d)_support.dot", nodeId, nid);
                    vIntBuff = Vec_IntDup( Vec_WecEntry(pMan->vGSupport, nodeId) );
                    Cec_ManGiaMapId( pMan->pGia, vIntBuff );
                    vIntBuff2 = Vec_IntDup( pMan->vMerge );
                    Cec_ManGiaMapId( pMan->pGia, vIntBuff2 );
                    Vec_IntRemoveAll( vIntBuff2, -1, 0 );
                    Gia_SelfDefShow( pTemp, str, vIntBuff, vIntBuff2, 0 );
                    Gia_ManStop( pTemp );
                    Vec_IntFree( vIntBuff );
                    Vec_IntFree( vIntBuff2 );

                    pTemp = Cec_ManFdVisSupport( pMan, nodeId, 1 );
                    sprintf(str, "%d(%d)_rewrite.dot", nodeId, nid);
                    vIntBuff = Vec_IntDup( Vec_WecEntry(pMan->vGSupport, nodeId) );
                    Cec_ManGiaMapId( pMan->pGia, vIntBuff );
                    vIntBuff2 = Vec_IntDup( pMan->vMerge );
                    Cec_ManGiaMapId( pMan->pGia, vIntBuff2 );
                    Vec_IntRemoveAll( vIntBuff2, -1, 0 );
                    Gia_SelfDefShow( pTemp, str, vIntBuff, vIntBuff2, 0 );
                    Gia_ManStop( pTemp );
                    Vec_IntFree( vIntBuff );
                    Vec_IntFree( vIntBuff2 );
                } else {
                    flag = 3;
                }
            }
        }

        if (flag == 3) {
            printf("Not available to rewrite all the nodes.\n");
            Cec_ManFdStop( pMan, 2 );
            return 0;
        }

        if (pPars->fGetCkts) {
            vIntBuff2 = Vec_IntDup( vRwNd );
            Cec_ManFdMapId( pMan, vIntBuff2, 1 );
            Vec_IntRemoveAll( vIntBuff2, -1, 0 );
            Gia_ManFillValue( pMan->pAbs );
            Cec_ManSubcircuit( pMan->pAbs, Gia_ManCo(pMan->pAbs, 0), vIntBuff2, 0, 1, 0 );
            Cec_ManSubcircuit( pMan->pAbs, Gia_ManCo(pMan->pAbs, 1), vIntBuff2, 0, 1, 0 );
            Gia_NodeMffcSizeMark( pMan->pAbs, Gia_ManObj(pMan->pAbs, nid) );
            Vec_IntFree( vIntBuff2 );
            vIntBuff2 = Vec_IntAlloc( 1 );
            Gia_ManForEachObj( pMan->pAbs, pObj, j ) {
                if (Gia_ObjValue(pObj) == -1) Vec_IntPush( vIntBuff2, j );
            }
            sprintf(str, "Mffc.dot");
            Gia_SelfDefShow( pMan->pAbs, str, vIntBuff2, 0, 0 );
            // Vec_IntFree( vIntBuff );
            Vec_IntFree( vIntBuff2 );

            sprintf(str, "beforeRw.dot");
            Gia_SelfDefShow( pMan->pGia, str, vRwNd, pMan->vNodeInit, 0 );
        }
        Vec_IntPrint(vRwNd);
        pTemp = Cec_ManFdReplaceMulti(pMan, Vec_IntDup(vRwNd));
        printf("after replace\n");
        
        if (pPars->fGetCkts) {
            vIntBuff = Vec_IntAlloc( 1 );
            Cec_ManGiaMapId( pTemp, vIntBuff ); // pTemp -> pGia
            vIntBuff2 = Vec_IntInvert( vIntBuff, -1 ); // pGia -> pTemp
            Vec_IntClear( vIntBuff );
            Vec_IntForEachEntry( vRwNd, nid, i ) {
                Vec_IntPush( vIntBuff, Vec_IntEntry( vIntBuff2, nid ) );
            }
            Vec_IntRemoveAll( vIntBuff, -1, 0 );
            Vec_IntPrint( vRwNd );
            Vec_IntPrint( vIntBuff );
        }
        Cec_ManFdUpdate( pMan, pTemp );
        sprintf(str, "Rewrited.aig");
        Gia_AigerWriteSimple( pTemp, str );
        sprintf(str, "runstat.log");
        Cec_ManFdDumpStat( pMan, str );

        if (pMan->pPars->fGetCkts) {
            sprintf(str, "afterRw.dot");
            Gia_SelfDefShow( pMan->pGia, str, vIntBuff, pMan->vNodeInit, 0 );
            // sprintf(str, "afterRw_abs.dot");
            // Gia_SelfDefShow( pMan->pAbs, str, vIntBuff, 0, 0 );
        }
        
    } else {
        printf("check RW at once\n");
        vIntBuff2 = Vec_IntAlloc( 1 );
        Gia_ManForEachObj( pMan->pGia, pObj, i ) {
            nid = Gia_ObjId(pMan->pGia, pObj);
            if (Gia_ObjColors(pMan->pGia, nid) == 1 || Gia_ObjColors(pMan->pGia, nid) == 2) {
                if (pMan->pPars->GType & 1) {
                    printf("check nid: %d\n", nid);
                    vIntBuff = Vec_IntDup( Vec_WecEntry( pMan->vGSupport, nid ) );
                    Vec_IntForEachEntry( Vec_WecEntry( pMan->vGSupport, nid ), nid2, j) {
                        if (Vec_IntFind( pMan->vMerge, nid2 ) == -1) continue;
                        Vec_IntRemove( vIntBuff, nid2 );
                        Vec_IntClear( vIntBuff2 );
                        pTemp = Cec_ManFdGetFd( pMan, nid, vIntBuff, vIntBuff2 );
                        buf = Vec_IntEntry(vIntBuff2, 0);
                        if (pTemp == NULL) {
                            printf("remove nid: %d______SAT (conf: %d)\n", nid2, buf);
                        }
                        else if (pTemp == 1) printf("remove nid: %d______UNSOLVE\n", nid2);
                        else {
                            printf("remove nid: %d______UNSAT (conf: %d)\n", nid2, Vec_IntEntry(vIntBuff2, 0));
                            if (buf < Vec_IntEntry(pMan->vConf, nid) || Vec_IntEntry(pMan->vStat, nid) <= 0) {
                                Cec_ManFdUpdateStat( pMan, nid, vIntBuff, pTemp, 0 );
                                assert(Vec_IntEntry(pMan->vStat, nid) > 0);
                                Vec_IntWriteEntry( pMan->vConf, nid, buf );
                            } 
                            Gia_ManStop( pTemp );
                        }
                        Vec_IntPush( vIntBuff, nid2 );
                    }
                } else {
                    Cec_ManFdCleanOneUnknown( pMan, nid );
                }
                // Cec_ManFdShrink( pMan, nid );
            }
        }
        Cec_ManFdIncreIter( pMan );
        if (pPars->fVerbose) Cec_ManFdReport( pMan );
        sprintf(str, "runstat.log");
        Cec_ManFdDumpStat( pMan, str );
        printf("UNSAT nodes:\n");
            Vec_IntPrint( Cec_ManFdUnsatId( pMan, 0, 1 ) );
    }

    Cec_ManFdStop( pMan, 2 );
    return 0;
}
void Cec_ManFdStatUpdate( Gia_Man_t * pOld, Gia_Man_t * pNew, char* pStat ) {
    Cec_ParFd_t ParsFd, *pPars = &ParsFd;
    Cec_ManFdSetDefaultParams( pPars );
    Cec_ManFd_t* pMan = Cec_ManFdStart( pOld, pPars, pStat );
    Cec_ManMatch( pNew, pOld );
    Cec_ManFdUpdate( pMan, pNew );
    Cec_ManFdDumpStat( pMan, pStat );
}
int Cec_ManVerifyWithAffine( Gia_Man_t * p, Cec_ParCec_t * pPars ) {
    // Gia_AigerWrite( Cec_ManToMiter( p, 0 ), "miter.aig", 0, 0, 0);
    Gia_Man_t* pTemp;
    // pTemp = Cec_ManToMiter( p, 1 );
    // Gia_AigerWrite( Cec_ManFraigSyn( pTemp, Gia_ObjFanin0(Gia_ManPo(pTemp,0)), Gia_ObjFanin0(Gia_ManPo(pTemp,1)), 
    //                 Gia_ObjFaninC0(Gia_ManPo(pTemp,0)) ^ Gia_ObjFaninC0(Gia_ManPo(pTemp,1)), 1, 10000), "miterFraig.aig", 0, 0, 0);
    Cec_ParFd_t ParsFd, *pParsFd = &ParsFd;
    Cec_ManFdSetDefaultParams( pParsFd );
    Cec_ManFd_t* pMan = Cec_ManFdStart( p, pParsFd, 0 );
    
    // Cec_ManPlayground5(p, pPars);
    return 0;

}

// int Cec_

/**Function*************************************************************

  Synopsis    [Simple SAT run to check equivalence.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec_ManVerifySimple( Gia_Man_t * p )
{
    Cec_ParCec_t ParsCec, * pPars = &ParsCec;
    Cec_ManCecSetDefaultParams( pPars );
    pPars->fSilent = 1;
    assert( Gia_ManCoNum(p) == 2 );
    assert( Gia_ManRegNum(p) == 0 );
    return Cec_ManVerify( p, pPars );
}

/**Function*************************************************************

  Synopsis    [New CEC engine applied to two circuits.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec_ManVerifyTwo( Gia_Man_t * p0, Gia_Man_t * p1, int fVerbose )
{
    Cec_ParCec_t ParsCec, * pPars = &ParsCec;
    Gia_Man_t * pMiter;
    int RetValue;
    Cec_ManCecSetDefaultParams( pPars );
    pPars->fVerbose = fVerbose;
    pMiter = Gia_ManMiter( p0, p1, 0, 1, 0, 0, pPars->fVerbose );
    if ( pMiter == NULL )
        return -1;
    RetValue = Cec_ManVerify( pMiter, pPars );
    p0->pCexComb = pMiter->pCexComb; pMiter->pCexComb = NULL;
    Gia_ManStop( pMiter );
    return RetValue;
}
int Cec_ManVerifyTwoInv( Gia_Man_t * p0, Gia_Man_t * p1, int fVerbose )
{
    Cec_ParCec_t ParsCec, * pPars = &ParsCec;
    Gia_Man_t * pMiter;
    int RetValue;
    Cec_ManCecSetDefaultParams( pPars );
    pPars->fVerbose = fVerbose;
    pMiter = Gia_ManMiterInverse( p0, p1, 1, pPars->fVerbose );
    if ( pMiter == NULL )
        return -1;
    RetValue = Cec_ManVerify( pMiter, pPars );
    p0->pCexComb = pMiter->pCexComb; pMiter->pCexComb = NULL;
    Gia_ManStop( pMiter );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [New CEC engine applied to two circuits.]

  Description [Returns 1 if equivalent, 0 if counter-example, -1 if undecided.
  Counter-example is returned in the first manager as pAig0->pSeqModel.
  The format is given in Abc_Cex_t (file "abc\src\aig\gia\gia.h").]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec_ManVerifyTwoAigs( Aig_Man_t * pAig0, Aig_Man_t * pAig1, int fVerbose )
{
    Gia_Man_t * p0, * p1, * pTemp;
    int RetValue;

    p0 = Gia_ManFromAig( pAig0 );
    p0 = Gia_ManCleanup( pTemp = p0 );
    Gia_ManStop( pTemp );

    p1 = Gia_ManFromAig( pAig1 );
    p1 = Gia_ManCleanup( pTemp = p1 );
    Gia_ManStop( pTemp );

    RetValue = Cec_ManVerifyTwo( p0, p1, fVerbose );
    pAig0->pSeqModel = p0->pCexComb; p0->pCexComb = NULL;
    Gia_ManStop( p0 );
    Gia_ManStop( p1 );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Implementation of new signal correspodence.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Cec_LatchCorrespondence( Aig_Man_t * pAig, int nConfs, int fUseCSat )
{
    Gia_Man_t * pGia;
    Cec_ParCor_t CorPars, * pCorPars = &CorPars;
    Cec_ManCorSetDefaultParams( pCorPars );
    pCorPars->fLatchCorr = 1;
    pCorPars->fUseCSat   = fUseCSat;
    pCorPars->nBTLimit   = nConfs;
    pGia = Gia_ManFromAigSimple( pAig );
    Cec_ManLSCorrespondenceClasses( pGia, pCorPars );
    Gia_ManReprToAigRepr( pAig, pGia );
    Gia_ManStop( pGia );
    return Aig_ManDupSimple( pAig );
}

/**Function*************************************************************

  Synopsis    [Implementation of new signal correspodence.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Cec_SignalCorrespondence( Aig_Man_t * pAig, int nConfs, int fUseCSat )
{
    Gia_Man_t * pGia;
    Cec_ParCor_t CorPars, * pCorPars = &CorPars;
    Cec_ManCorSetDefaultParams( pCorPars );
    pCorPars->fUseCSat  = fUseCSat;
    pCorPars->nBTLimit  = nConfs;
    pGia = Gia_ManFromAigSimple( pAig );
    Cec_ManLSCorrespondenceClasses( pGia, pCorPars );
    Gia_ManReprToAigRepr( pAig, pGia );
    Gia_ManStop( pGia );
    return Aig_ManDupSimple( pAig );
}

/**Function*************************************************************

  Synopsis    [Implementation of fraiging.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Cec_FraigCombinational( Aig_Man_t * pAig, int nConfs, int fVerbose )
{
    Gia_Man_t * pGia;
    Cec_ParFra_t FraPars, * pFraPars = &FraPars;
    Cec_ManFraSetDefaultParams( pFraPars );
    pFraPars->fSatSweeping = 1;
    pFraPars->nBTLimit  = nConfs;
    pFraPars->nItersMax = 20;
    pFraPars->fVerbose  = fVerbose;
    pGia = Gia_ManFromAigSimple( pAig );
    Cec_ManSatSweeping( pGia, pFraPars, 0 );
    Gia_ManReprToAigRepr( pAig, pGia );
    Gia_ManStop( pGia );
    return Aig_ManDupSimple( pAig );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

