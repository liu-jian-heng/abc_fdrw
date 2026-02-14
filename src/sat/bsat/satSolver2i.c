/**CFile****************************************************************

  FileName    [satSolver2i.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT solver.]

  Synopsis    [Records the trace of SAT solving in the CNF form.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 2, 2013.]

  Revision    [$Id: satSolver2i.c,v 1.4 2013/09/02 00:00:00 casem Exp $]

***********************************************************************/

#include "satSolver2.h"
// #include "aig/gia/gia.h"
#include "aig/gia/giaAig.h"
#include "sat/cnf/cnf.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
extern void Gia_SelfDefShow( Gia_Man_t * p, char * pFileName, Vec_Int_t * vLabel0, Vec_Int_t * vLabel1, Vec_Wec_t* vRelate);

struct Int2_Man_t_ 
{
    sat_solver2 *   pSat;      // user's SAT solver
    Vec_Int_t *     vGloVars;  // IDs of global variables
    Vec_Int_t *     vVar2Glo;  // mapping of SAT variables into their global IDs
    Gia_Man_t *     pGia;      // AIG manager to store the interpolant
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Managing interpolation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Int2_Man_t * Int2_ManStart( sat_solver2 * pSat, int * pGloVars, int nGloVars )
{
    Int2_Man_t * p;
    int i;
    p = ABC_CALLOC( Int2_Man_t, 1 );
    p->pSat     = pSat;
    p->vGloVars = Vec_IntAllocArrayCopy( pGloVars, nGloVars );
    p->vVar2Glo = Vec_IntInvert( p->vGloVars, -1 );
    p->pGia     = Gia_ManStart( 10 * Vec_IntSize(p->vGloVars) );
    // printf( "pGia alloc size: %d\n", 10 * Vec_IntSize(p->vGloVars) );
    p->pGia->pName = Abc_UtilStrsav( "interpolant" );
    for ( i = 0; i < nGloVars; i++ )
        Gia_ManAppendCi( p->pGia );
    Gia_ManHashStart( p->pGia );
    return p;
}
void Int2_ManStop( Int2_Man_t * p )
{
    if ( p == NULL )
        return;
    Gia_ManStopP( &p->pGia );
    Vec_IntFree( p->vGloVars );
    Vec_IntFree( p->vVar2Glo );
    ABC_FREE( p );
}
void * Int2_ManReadInterpolant( sat_solver2 * pSat )
{
    Int2_Man_t * p = pSat->pInt2;
    Gia_Man_t * pTemp, * pGia = p->pGia; p->pGia = NULL;
    // return NULL, if the interpolant is not ready (for example, when the solver returned 'sat')
    if ( pSat->hProofLast == -1 )
        return NULL;
    // create AIG with one primary output
    assert( Gia_ManPoNum(pGia) == 0 );
    Gia_ManAppendCo( pGia, pSat->hProofLast );  
    pSat->hProofLast = -1;
    // cleanup the resulting AIG
    pGia = Gia_ManCleanup( pTemp = pGia );
    Gia_ManStop( pTemp );
    return (void *)pGia;
}

/**Function*************************************************************

  Synopsis    [Computing interpolant for a clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Int2_ManChainStart( Int2_Man_t * p, clause * c )
{   
    if ( c->lrn )
        return veci_begin(&p->pSat->claProofs)[clause_id(c)];
    if ( !c->partA ) // the clause is not a learnt clause, nor partA, must be true
        return 1;
    if ( c->lits[c->size] < 0 )
    {
        int i, Var, CiId, Res = 0;
        for ( i = 0; i < (int)c->size; i++ )
        {
            // get ID of the global variable
            if ( Abc_Lit2Var(c->lits[i]) >= Vec_IntSize(p->vVar2Glo) )
                continue;
            Var = Vec_IntEntry( p->vVar2Glo, Abc_Lit2Var(c->lits[i]) );
            if ( Var < 0 )
                continue;
            // get literal of the AIG node
            CiId = Gia_ObjId( p->pGia, Gia_ManCi(p->pGia, Var) );
            // compute interpolant of the clause
            Res = Gia_ManHashOr( p->pGia, Res, Abc_Var2Lit(CiId, Abc_LitIsCompl(c->lits[i])) );
        }
        c->lits[c->size] = Res;
    }
    return c->lits[c->size];
}
int Int2_ManChainResolve( Int2_Man_t * p, clause * c, int iLit, int varA )
{   // here
    extern void Gia_SelfDefShow( Gia_Man_t * p, char * pFileName, Vec_Int_t * vLabel0, Vec_Int_t * vLabel1, Vec_Wec_t* vRelate);
    // printf("----------\n");
    // for ( int i = 0; i <= c->size; i++ ) {
    //     if (c->lits[i] < 0) printf("* ");
    //     else printf("%c%d ", Abc_LitIsCompl(c->lits[i]) ? '-' : ' ', Abc_Lit2Var(c->lits[i]));
    // }
    // printf("\n");
    int iLit2 = Int2_ManChainStart( p, c );
    // printf("iLit = %d, iLit2 = %d, varA = %d, clauseA = %d, clauseLearn = %d, clauseId = %d, veciSize = %d\n", iLit, iLit2, varA, c->partA, c->lrn, clause_id(c), veci_size(&p->pSat->claProofs));
    // for ( int i = 0; i < veci_size(&p->pSat->claProofs); i++ ) {
    //     printf("%d ", veci_begin(&p->pSat->claProofs)[i]);
    // }
    
    assert( iLit >= 0 );
    assert( c->size > 0 );
    if ( iLit2 < 0 || Abc_Lit2Var(iLit2) >= Gia_ManObjNum(p->pGia) ) {
        // printf("iLit = %d, iLit2 = %d, varA = %d, clauseA = %d, clauseLearn = %d, cLast = %d\n", iLit, iLit2, varA, c->partA, c->lrn, c->lits[c->size]);
        // Gia_SelfDefShow( p->pGia, "current_patch.dot", 0, 0, NULL );
        Gia_ManStop(p->pGia);
        return -1;
    } 
    
    if ( varA )
        iLit = Gia_ManHashOr( p->pGia, iLit, iLit2 );
    else
        iLit = Gia_ManHashAnd( p->pGia, iLit, iLit2 );
    // printf("iLit = %d\n", iLit);
    // Gia_SelfDefShow( p->pGia, "selfdef.dot", 0, 0, NULL );
    return iLit;
}

/**Function*************************************************************

  Synopsis    [Test for the interpolation procedure.]

  Description [The input AIG can be any n-input comb circuit with one PO 
  (not necessarily a comb miter).  The interpolant depends on n+1 variables
  and equal to the relation f = F(x0,x1,...,xn).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManInterTest( Gia_Man_t * p )
{   
    Sat_Mem_t * pSatMem;
    clause* c;
    sat_solver2 * pSat;
    Gia_Man_t * pInter;
    Aig_Man_t * pMan;
    Vec_Int_t * vGVars;
    Cnf_Dat_t * pCnf;
    Aig_Obj_t * pObj;
    int Lit, Cid, Var, status, i, j, k;
    abctime clk = Abc_Clock();
    assert( Gia_ManRegNum(p) == 0 );
    /// assert( Gia_ManCoNum(p) == 1 );

    // derive CNFs
    pMan = Gia_ManToAigSimple( p );
    /// pCnf = Cnf_Derive( pMan, 1 );
    pCnf = Cnf_Derive( pMan, Gia_ManCoNum(p) );

    // start the solver
    pSat = sat_solver2_new();
    pSat->fVerbose = 1;
    sat_solver2_setnvars( pSat, 2*pCnf->nVars+1 );

    // set A-variables (all used except PI/PO, which will be global variables)
    Aig_ManForEachObj( pMan, pObj, i )
        if ( pCnf->pVarNums[pObj->Id] >= 0 && !Aig_ObjIsCi(pObj) && !Aig_ObjIsCo(pObj) )
            var_set_partA( pSat, pCnf->pVarNums[pObj->Id], 1 );

    // add clauses of A
    for ( i = 0; i < pCnf->nClauses; i++ )
    {
        Cid = sat_solver2_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1], -1 );
        clause2_set_partA( pSat, Cid, 1 ); // this API should be called for each clause of A
    }
    Sat_Solver2WriteDimacs( pSat, "dimac1.txt", 0, 0, 0 );

    // add clauses of B (after shifting all CNF variables by pCnf->nVars)
    Cnf_DataLift( pCnf, pCnf->nVars );
    for ( i = 0; i < pCnf->nClauses; i++ )
        sat_solver2_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1], -1 );
    Cnf_DataLift( pCnf, -pCnf->nVars );

    // add PI equality clauses
    vGVars = Vec_IntAlloc( Aig_ManCoNum(pMan)+1 );
    Aig_ManForEachCi( pMan, pObj, i )
    {
        Var = pCnf->pVarNums[pObj->Id];
        sat_solver2_add_buffer( pSat, Var, pCnf->nVars + Var, 0, 0, -1 );
        Vec_IntPush( vGVars, Var );
    }

    // add an XOR clause in the end
    Var = pCnf->pVarNums[Aig_ManCo(pMan,0)->Id];
    sat_solver2_add_xor( pSat, Var, pCnf->nVars + Var, 2*pCnf->nVars, 0, 0, -1 );
    Vec_IntPush( vGVars, Var );

    // pSatMem = &pSat->Mem;
    // Aig_ManForEachObj( pMan, pObj, i ) {
    //     // printf("%d, %d\n", pObj->Id, pCnf->pVarNums[pObj->Id]);
    //     if ( pCnf->pVarNums[pObj->Id] >= 0 ) {
    //         printf("%d: %d\n", pCnf->pVarNums[pObj->Id], var_is_partA( pSat, pCnf->pVarNums[pObj->Id])  ); // use pCnf->pVarNums[pObj->Id] to get var of circuit nd
    //         // printf("-->%d\n", pObj->Id);
    //     }
    // }
    // Sat_MemForEachClause2( pSatMem, c, k, j ) {
    //     for (i = 0; i < c->size; i++)
    //     {
    //         printf("%c%d ", Abc_LitIsCompl(c->lits[i]) ? '-' : ' ', Abc_Lit2Var(c->lits[i]));
    //     }
    //     printf("\n");
    //     printf("clauseA = %d, clauseLearn = %d, mark = %d\n", c->partA, c->lrn, c->mark);
    // }
    
    // start the interpolation manager
    pSat->pInt2 = Int2_ManStart( pSat, Vec_IntArray(vGVars), Vec_IntSize(vGVars) );

    // solve the problem
    Lit = toLitCond( 2*pCnf->nVars, 0 );
    // Vec_IntPrint( vGVars );
    // printf("-->%d\n", Lit);
    status = sat_solver2_solve( pSat, &Lit, &Lit + 1, 0, 0, 0, 0 );
    assert( status == l_False );
    Sat_Solver2PrintStats( stdout, pSat );

    // derive interpolant
    pInter = (Gia_Man_t *)Int2_ManReadInterpolant( pSat );
    Gia_ManPrintStats( pInter, NULL );
    Abc_PrintTime( 1, "Total interpolation time", Abc_Clock() - clk );

    // clean up
    Vec_IntFree( vGVars );
    Cnf_DataFree( pCnf );
    Aig_ManStop( pMan );
    sat_solver2_delete( pSat );

    // return interpolant
    return pInter;
}
sat_solver2 * Int2_ManSolver( Gia_Man_t *p, Vec_Int_t *vVarMap ) {
    clause* c;
    sat_solver2 * pSat;
    Gia_Obj_t * pObj_gia;
    Aig_Man_t * pMan;
    Cnf_Dat_t * pCnf;
    Aig_Obj_t * pObj;
    int Lit, Cid, Var, status, const_1, debug = 0;
    int nid, i, j, k;
    assert( Gia_ManRegNum(p) == 0 );
    assert( vVarMap == NULL || Vec_IntSize(vVarMap) == Gia_ManObjNum(p) );
    // derive CNFs
    pMan = Gia_ManToAigSimple( p );
    pCnf = Cnf_Derive( pMan, Gia_ManCoNum(p) );
    pSat = sat_solver2_new();
    sat_solver2_setnvars( pSat, 3 * pCnf->nVars + 1 );
    
    Aig_ManForEachObj( pMan, pObj, i ) {
        assert( Abc_Lit2Var(Gia_ObjValue(Gia_ManObj(p, pObj->Id))) == pObj->Id );
        if ( vVarMap ) {
            Vec_IntWriteEntry( vVarMap, pObj->Id, pCnf->pVarNums[pObj->Id] );
            // Vec_IntWriteEntry( vVarMap, Gia_ManObjNum(p) + pObj->Id, pCnf->pVarNums[pObj->Id] + pCnf->nVars );
            // Vec_IntWriteEntry( vVarMap, 2 * Gia_ManObjNum(p) + pObj->Id, pCnf->pVarNums[pObj->Id] + 2 * pCnf->nVars );
        }
        if ( pCnf->pVarNums[pObj->Id] >= 0 ) { // Possibly faulty
            var_set_partA( pSat, pCnf->pVarNums[pObj->Id], 1 ); // use pCnf->pVarNums[pObj->Id] to get var of circuit nd
        }
    }
    
    for ( i = 0; i < pCnf->nClauses; i++ )
    {
        Cid = sat_solver2_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1], -1 );
        clause2_set_partA( pSat, Cid, 1 ); // this API should be called for each clause of A
    }

    Cnf_DataLift( pCnf, pCnf->nVars );
    for ( i = 0; i < pCnf->nClauses; i++ )
        sat_solver2_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1], -1 );
    Cnf_DataLift( pCnf, -pCnf->nVars );

    for ( i = 0; i < pCnf->nVars; i++ )
        sat_solver2_add_controlBuffer( pSat, i, pCnf->nVars + i, 2 * pCnf->nVars + i, 0, 0, -1 );
    // Sat_Solver2WriteDimacs( pSat, "patch_new.dimac", 0, 0, 0 ); // bound equiv vars

    Cnf_DataFree( pCnf );
    Aig_ManStop( pMan );
    return pSat;
}
Gia_Man_t * Int2_ManFd( sat_solver2 * pSat, Vec_Int_t* vG, int varF, int nConflicts, int fVerbose ) {
    Gia_Man_t * pInter;
    Vec_Int_t* assumptions;
    assert( pSat->size % 3 == 1 );
    int nVars = (pSat->size - 1) / 3;
    int i, var, status;
    assumptions = Vec_IntAlloc( 2 + Vec_IntSize(vG) ); // or all var should be set
    Vec_IntForEachEntry( vG, var, i ) {
        assert(var_is_partA( pSat, var ) == 1);
        var_set_partA( pSat, var, 0 );
        Vec_IntPush( assumptions, toLitCond( 2 * nVars + var, 1 ) ); // all G should be 0
        // Vec_IntWriteEntry( vG, i, 2 * nVars + var );
    }
    // printf("pre:\n");
    // Sat_Solver2WriteDimacs( pSat, "patch.dimac", 0, 0, 0 );
    Vec_IntPush( assumptions, toLitCond( varF, 0 ) );
    Vec_IntPush( assumptions, toLitCond( varF + nVars, 1 ) ); // F should be 1
    pSat->pInt2 = Int2_ManStart( pSat, Vec_IntArray(vG), Vec_IntSize(vG) );
    status = sat_solver2_solve( pSat, Vec_IntArray(assumptions), Vec_IntLimit(assumptions), nConflicts, 0, 0, 0 );
    Vec_IntForEachEntry( vG, var, i ) {
        var_set_partA( pSat, var, 1 );
    }
    // Sat_Solver2KeepLearnt( pSat );

    Vec_IntFree( assumptions );
    if ( status == l_False ) {
        if (pSat->pInt2) {
            pInter = (Gia_Man_t*) Int2_ManReadInterpolant( pSat );
        } else {
            printf("problemetic!\n");
            // Gia_SelfDefShow( pSat->pInt2->pGia, "problemetic.dot", 0, 0, NULL );
            pInter = -1;
        }
    }
    if ( status == l_False ) {
        return pInter;
    } else if ( status == l_Undef ) {
        return -1;
    } else {
        // int * Sat_Solver2GetModel( sat_solver2 * p, int * pVars, int nVars )
        return 0;
    }
}

Gia_Man_t * Int2_ManFdSimp( Gia_Man_t * p, int nConflim, int* nConf ) {
    // extern void Sat_SolverClauseWriteDimacs( FILE * pFile, clause * pC, int fIncrement );
    Sat_Mem_t * pSatMem;
    clause* c;
    sat_solver2 * pSat, *pSat2;
    Gia_Man_t * pInter;
    Gia_Obj_t * pObj_gia;
    Aig_Man_t * pMan;
    Vec_Int_t * vGVars, *vFs;
    Cnf_Dat_t * pCnf;
    Aig_Obj_t * pObj;
    int varF, Lit, Cid, Var, status, const_1, debug = 0;
    int nid, i, j, k;
    abctime clk = Abc_Clock();
    assert( Gia_ManRegNum(p) == 0 );

    // derive CNFs
    vGVars = Vec_IntAlloc( Gia_ManCoNum(p) );
    Gia_ManForEachCo( p, pObj_gia, i ) {
        // if (i != Gia_ManCoNum(p) - 1) Vec_IntPush( vGVars, Gia_ObjFaninId0(p, ) );
        // else varF = Gia_ObjFaninId0(p, ); // the last one is the output variable
        if (i != Gia_ManCoNum(p) - 1) Vec_IntPush( vGVars, Gia_ObjFaninId0(p, Gia_ObjId(p, pObj_gia)) );
        else varF = Gia_ObjFaninId0(p, Gia_ObjId(p, pObj_gia));
    }

    pMan = Gia_ManToAigSimple( p );
    pCnf = Cnf_Derive( pMan, Gia_ManCoNum(p) );

    Vec_IntForEachEntry( vGVars, nid, i ) {
        Vec_IntWriteEntry( vGVars, i, Abc_Lit2Var(Gia_ObjValue(Gia_ManObj(p, nid)))); // the last one is the output variable
    }
    varF = Abc_Lit2Var(Gia_ObjValue(Gia_ManObj(p, varF)));

    pSat = sat_solver2_new();
    pSat->verbosity = debug;

    // org: sat_solver2_setnvars( pSat, 2*pCnf->nVars + 1 );
    sat_solver2_setnvars( pSat, 2*pCnf->nVars + 1 + Vec_IntSize(vGVars) );
    if (pSat->verbosity) {
        printf("SAT solver nVars = %d\n", pCnf->nVars);
        printf("AIG has %d nodes\n", Aig_ManObjNum(pMan));
        printf("node in ckt ref (G):");
        Vec_IntPrint( vGVars );
        printf("var F = %d\n", varF);
    }

    // set A-variables (all used except PI/PO, which will be global variables)
    Aig_ManForEachObj( pMan, pObj, i ) {
        if (pSat->verbosity) printf("AIG to CNF: %d, %d\n", pObj->Id, pCnf->pVarNums[pObj->Id]);
        if ( pCnf->pVarNums[pObj->Id] >= 0 && Vec_IntFind( vGVars, pObj->Id ) == -1 ) {
            var_set_partA( pSat, pCnf->pVarNums[pObj->Id], 1 ); // use pCnf->pVarNums[pObj->Id] to get var of circuit nd
        } 
        // else {
        //     if (Vec_IntFind( vGVars, pObj->Id )) printf("%d(%d) is %d\n", pObj->Id, pCnf->pVarNums[pObj->Id], var_is_partA( pSat, pCnf->pVarNums[pObj->Id] ) );
        // }
    }
    
    Vec_IntForEachEntry( vGVars, nid, i ) {
        pObj = (Aig_Obj_t*) Vec_PtrGetEntry(pMan->vObjs, nid);
        assert( pCnf->pVarNums[pObj->Id] >= 0 );
        Vec_IntWriteEntry( vGVars, i, pCnf->pVarNums[pObj->Id] );
    }
    
    pObj = (Aig_Obj_t*) Vec_PtrGetEntry(pMan->vObjs, varF);
    assert( pCnf->pVarNums[pObj->Id] >= 0 );
    varF =  pCnf->pVarNums[pObj->Id];
    var_set_partA( pSat, pCnf->pVarNums[pObj->Id], 1 );

    // add clauses of A
    for ( i = 0; i < pCnf->nClauses; i++ )
    {
        Cid = sat_solver2_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1], -1 );
        clause2_set_partA( pSat, Cid, 1 ); // this API should be called for each clause of A
    }
    
    // add clauses of B (after shifting all CNF variables by pCnf->nVars)
    Cnf_DataLift( pCnf, pCnf->nVars );
    for ( i = 0; i < pCnf->nClauses; i++ )
        sat_solver2_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1], -1 );
    Cnf_DataLift( pCnf, -pCnf->nVars );

    // add PI equality clauses
    Aig_ManForEachObj( pMan, pObj, i ) {
        // org: sat_solver2_add_buffer( pSat, Var, pCnf->nVars + Var, 0, 0, -1 );
        if ( pCnf->pVarNums[pObj->Id] == -1 ) continue;
        Var = pCnf->pVarNums[pObj->Id];
        lit Lits[3];
        int Cid;

        Lits[0] = toLitCond( Var, 0 );
        Lits[1] = toLitCond( pCnf->nVars + Var, 1 );
        Lits[2] = toLitCond( 2*pCnf->nVars + Var, 0 ); // new var for equivalence // + i
        Cid = sat_solver2_addclause( pSat, Lits, Lits + 3, -1 );

        Lits[0] = toLitCond( Var, 1 );
        Lits[1] = toLitCond( pCnf->nVars + Var, 0 );
        Lits[2] = toLitCond( 2*pCnf->nVars + Var, 0 ); // + i
        Cid = sat_solver2_addclause( pSat, Lits, Lits + 3, -1 );

    }
    // printf("7\n");

    pSatMem = &pSat->Mem;
    // Aig_ManForEachObj( pMan, pObj, i ) { 
    //     if ( pCnf->pVarNums[pObj->Id] >= 0 ) {
    //         printf("%d: %d\n", pCnf->pVarNums[pObj->Id], var_is_partA( pSat, pCnf->pVarNums[pObj->Id])  ); // use pCnf->pVarNums[pObj->Id] to get var of circuit nd
    //     }
    // }
    // Sat_MemForEachClause2( pSatMem, c, k, j ) {
    //     for (i = 0; i < c->size; i++)
    //     {
    //         printf("%c%d ", Abc_LitIsCompl(c->lits[i]) ? '-' : ' ', Abc_Lit2Var(c->lits[i]));
    //     }
    //     printf("\n");
    //     printf("clauseA = %d, clauseLearn = %d, mark = %d\n", c->partA, c->lrn, c->mark);
    // }
    // Aig_ManForEachObj( pMan, pObj, i ) {
    //     // printf("%d, %d\n", pObj->Id, pCnf->pVarNums[pObj->Id]);
    //     if ( pCnf->pVarNums[pObj->Id] >= 0 ) {
    //         printf("%d: %d\n", pCnf->pVarNums[pObj->Id], var_is_partA( pSat, pCnf->pVarNums[pObj->Id])  ); // use pCnf->pVarNums[pObj->Id] to get var of circuit nd
    //         // printf("-->%d\n", pObj->Id);
    //     }
    // }
        
    if (pSat->verbosity) Sat_Solver2WriteDimacs( pSat, "patch.dimac", 0, 0, 0 ); // bound equiv vars

    pSat->pInt2 = Int2_ManStart( pSat, Vec_IntArray(vGVars), Vec_IntSize(vGVars) );
    // printf("8\n");

    // solve the problem

    // org: vFs = Vec_IntAlloc(2);
    vFs = Vec_IntAlloc(2+Vec_IntSize(vGVars));
    for (int i = 0; i < Vec_IntSize(vGVars); i++) {
        Vec_IntPush( vFs, toLitCond(2*pCnf->nVars + Vec_IntEntry(vGVars, i), 1) ); // set all equiv vars to 0
    }
    Vec_IntPush( vFs, toLitCond(varF, 0) );
    Vec_IntPush( vFs, toLitCond(varF + pCnf->nVars, 1) );

    status = sat_solver2_solve( pSat, Vec_IntArray(vFs), Vec_IntLimit(vFs), nConflim, 0, 0, 0 );
    *nConf = pSat->stats.conflicts;
    // printf("conflicts = %d\n", pSat->stats.conflicts);

    if (pSat->verbosity) Sat_Solver2WriteDimacs( pSat, "learnt.dimac", 0, 0, 0 );

    if ( status == l_False ) {
        if (pSat->pInt2) {
            pInter = (Gia_Man_t *)Int2_ManReadInterpolant( pSat );
        } else {
            Gia_SelfDefShow( p, "problemetic.dot", 0, 0, NULL );
            pInter = 1;
        }
    }

    // clean up
    Vec_IntFree( vGVars );
    Cnf_DataFree( pCnf );
    Aig_ManStop( pMan );
    sat_solver2_delete( pSat );
    if ( status == l_False ) { 
        return pInter;
    } else if ( status == l_Undef ) {
        return 1;
    } else {
        return 0;
    }

}
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

