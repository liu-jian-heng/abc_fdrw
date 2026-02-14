/**CFile****************************************************************

  FileName    [vec.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resizable arrays.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: vec.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__misc__vec__vec_h
#define ABC__misc__vec__vec_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "misc/util/abc_global.h"

#include <stdio.h>
#include <math.h>
#include "vecInt.h"
#include "vecFlt.h"
#include "vecStr.h"
#include "vecPtr.h"
#include "vecVec.h"
#include "vecAtt.h"
#include "vecWrd.h"
#include "vecBit.h"
#include "vecMem.h"
#include "vecWec.h"

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////
static inline float    Vec_IntMean( Vec_Int_t* vec ) {
    return Vec_IntSize( vec ) == 0 ? -1.0 : (1.0 * Vec_IntSum( vec )) / Vec_IntSize( vec );
}

static inline float    Vec_IntSqMean( Vec_Int_t* vec ) {
    float sq_sum = 0.0;
    int i, entry;
    Vec_IntForEachEntry( vec, entry, i ) {
        sq_sum += entry * entry * 1.0;
    }
    return Vec_IntSize( vec ) == 0 ? -1.0 : sq_sum / Vec_IntSize( vec );
}
static inline float    Vec_IntVar( Vec_Int_t* vec ) {
    if ( Vec_IntSize( vec ) == 0 ) return -1.0;
    int avg = Vec_IntMean( vec );
    int sq_mean = Vec_IntSqMean( vec );
    return sq_mean - avg * avg;
}

// value that have no log would be -1.0
static inline Vec_Flt_t*      Vec_IntLog( Vec_Int_t* vec ) {
    Vec_Flt_t* vLog = Vec_FltStart( Vec_IntSize( vec ) );
    int i;
    for ( i = 0; i < Vec_IntSize( vec ); i++ ) {
        if ( Vec_IntEntry( vec, i ) <= 0 ) Vec_FltWriteEntry( vLog, i, -1.0 );
        if ( Vec_IntEntry( vec, i ) == 1 ) Vec_FltWriteEntry( vLog, i, 0.0 );
        Vec_FltWriteEntry( vLog, i, log2f( Abc_Int2Float( Vec_IntEntry( vec, i ) ) ));
    }
    return vLog;
}
/*
  fType: 0 --> no return, just print infos
  fType: 1 --> v = (v - vmin) / (vmax - vmin) * 255
  fType: 2 --> v = (v's order) / (total) * 255
  fType: 3 --> v = (v - vavg) / vstddev * 32 + 127.5; v = min(255, max(0, v))
  fType: 4 --> v = (v - vmin) / (vmax - vmin) * 255
  fIgnore: -1 --> ignore vmin, 0 --> do not ignore, 1 --> ignore vmax
*/
static inline Vec_Int_t*    Vec_IntColorize( Vec_Int_t* vInfo, int fType, int fIgnore ) {
    int i, entry;
    int vmin = Vec_IntFindMin( vInfo );
    int vmax = Vec_IntFindMax( vInfo );
    int nmin = Vec_IntCountEntry( vInfo, vmin );
    int nmax = Vec_IntCountEntry( vInfo, vmax );
    int buf_i;
    Vec_Int_t* vTrim = Vec_IntDup( vInfo );
    if (fIgnore < 0) Vec_IntRemoveAll( vTrim, vmin, 0 );
    if (fIgnore > 0) Vec_IntRemoveAll( vTrim, vmax, 0 );
    // Vec_Flt_t* vLog = Vec_IntLog( vTrim );
    buf_i = Vec_IntFindMin( vTrim );
    float vlogmin = buf_i <= 0 ? -1.0 : log2f( Abc_Int2Float( buf_i ));
    buf_i = Vec_IntFindMax( vTrim );
    float vlogmax = buf_i <= 0 ? -1.0 : log2f( Abc_Int2Float( buf_i ));
    float vavg = Vec_IntMean( vTrim );
    float vstddev = Vec_IntVar( vTrim );
    float buf_f;

    vstddev = sqrtf(vstddev);
    Vec_Int_t* vColor = fType == 0 ? NULL : Vec_IntStart( Vec_IntSize(vInfo) );
    Vec_Int_t* vInfoSorted = Vec_IntDup( vTrim );
    Vec_IntSort( vInfoSorted, 0 );
    Vec_IntForEachEntry( vInfo, entry, i ) {
        if (fType > 0 && vmin == entry && fIgnore == -1) {
            Vec_IntWriteEntry( vColor, i, -1 );
            continue;
        }
        if (fType > 0 && vmax == entry && fIgnore ==  1) {
            Vec_IntWriteEntry( vColor, i, -1 );
            continue;
        }
        switch (fType)
        {
          case 0:
            break;
          case 1:
            if (vmax == vmin)
              Vec_IntWriteEntry( vColor, i, 128 );
            else {
                buf_f = 1.0 * (entry - vmin) / (vmax - vmin);
                Vec_IntWriteEntry( vColor, i, (int)roundf(buf_f * 255.0) );
            }
            break;
          case 2:
            buf_i = Vec_IntFind( vInfoSorted, entry );
            // printf("%d ", buf_i);
            buf_i += Vec_IntFindRev( vInfoSorted, entry );
            // printf("%d ", buf_i);
            buf_i /= 2;
            // printf("%d ", buf_i);
            buf_f = 1.0 * buf_i / Vec_IntSize(vTrim);
            assert(buf_f >= 0.0 && buf_f <= 1.0);
            // printf("%.2f ", buf_f);
            Vec_IntWriteEntry( vColor, i, (int)roundf(buf_f * 255.0) );
            // Vec_IntWriteEntry( vColor, i, (i) / (Vec_IntSize(vInfo) - 1) * 256 );
            break;
          case 3:
            buf_f = (1.0 * entry - vavg) / vstddev * 32.0 + 127.5;
            if (buf_f < 0.0) buf_f = 0.0;
            if (buf_f > 255.0) buf_f = 255.0;
            Vec_IntWriteEntry( vColor, i, (int)roundf(buf_f) );
            break;
          case 4:
            if (vlogmax == vlogmin)
              Vec_IntWriteEntry( vColor, i, 128 );
            else {
                buf_f = 1.0 * (log2f( Abc_Int2Float( entry ) ) - vlogmin) / (vlogmax - vlogmin);
                Vec_IntWriteEntry( vColor, i, (int)roundf(buf_f * 255.0) );
            }
          default:
            break;
        }
    }
    if (fType == 0) {
        printf("Num: %d, Min: %d (# = %d), Max: %d (# = %d), Avg: %.2f, StdDev: %.2f\n", Vec_IntSize(vInfo), vmin, nmin, vmax, nmax, vavg, vstddev);
        if (Vec_IntSize(vTrim) > 8) {
          printf("midpoint of eighth places: ");
          buf_i = Vec_IntSize(vTrim) / 8;
          for (i = 0; i < 8; i++) {
              buf_f = 1.0 * (Vec_IntEntry(vInfoSorted, i * buf_i) + Vec_IntEntry(vInfoSorted, (i + 1) * buf_i - 1)) / 2;
              printf("%f ", buf_f);
          }
          printf("\n");
        }
    }
    Vec_IntFree( vInfoSorted );
    Vec_IntFree( vTrim );
    // Vec_IntFree( vLog );
    return vColor;
}
////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

