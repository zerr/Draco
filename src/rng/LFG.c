/*************************************************************************/
/*************************************************************************/
/* A MULTI-PROCESSOR LAGGED-FIBONACCI RANDOM NUMBER GENERATION SYSTEM    */
/*                                                                       */ 
/* Authors: Steven A. Cuccaro and Daniel V. Pryor,                       */
/*            IDA/Center for Computing Sciences (CCS)                    */
/* E-Mail: cuccaro@super.org      pryor@super.org                        */
/*                                                                       */ 
/* Copyright 1996 September 3, United States Government as Represented   */
/* by the Director, National Security Agency. All rights reserved.       */
/*                                                                       */
/* Disclaimer: CCS expressly disclaims any and all warranties, expressed */
/* or implied, concerning the enclosed software. This software was       */
/* developed at CCS for use in internal research. The intent in sharing  */
/* this software is to promote the productive interchange of ideas       */
/* throughout the research community. All software is furnished on an    */
/* "as is" basis. No further updates to this software should be          */
/* expected. Although this may occur, no commitment exists. The authors  */
/* certainly invite your comments as well as the reporting of any bugs.  */
/* CCS cannot commit that any or all bugs will be fixed.                 */
/*************************************************************************/
/*************************************************************************/

/*************************************************************************/
/*      This version has been modified to use two integer-based additive */
/*      lagged-Fibonacci generators to produce integer, float and double */
/*      values. The lagged-Fibonacci generators each have 31 bits of     */
/*      precision (after the bit fixed by the canonical form of the      */
/*      generator is removed), 31-bit values are generated by XORing     */
/*      the values after one has been shifted left one bit. The floating */
/*      point value is formed by dividing the integer by 1.e+32 (the     */
/*      lsb's will be dropped from the mantissa to make room for the     */
/*      exponent), and two of these integer values in sequence are used  */
/*      to get the necessary precision for the double value.             */
/*                                                                       */
/*      This method has the advantage that the generators pass fairly    */
/*      strict randomness tests, including the Birthday Spacings test    */
/*      that additive lagged-Fibonacci generators are well known to      */
/*      fail. The disadvantage is the additional time needed to do the   */
/*      division explicitly, which was avoided in previous versions.     */
/*      (As the division is by powers of 2, the user might well consider */
/*      making machine-specific versions of this code to insert the bits */
/*      into the appropriate places and avoid the problem entirely.)     */
/*************************************************************************/

/*  
  This is a heavily modified implementation of the LFG algorithm described
  above. The point to this modifications was to reduce the memory footprint
  and to improve efficiency.  In making these changes, the ability to specify
  a "parameter" to init_rng has become a compile-time option as described
  below (c.f. VALID_L).  I have also removed the ability to spawn multiple
  streams with one call:  only one stream can be spawned at a time.

  The original authors deserve full credit for creating this random number
  generator, which can be found as part of the SPRNG library. 

  Paul Henning
  Los Alamos National Laboratory
  June 28, 2006 
*/

#if defined(_MSC_VER) || (__ICC == 1300) || (__ICC == 1310)
#define LFG_RESTRICT  
#else
#define LFG_RESTRICT __restrict__
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "LFG.h"


/*      BITS_IN_INT_GEN is the log_2 of the modulus of the generator     */
/*           for portability this is set to 32, but can be modified;     */
/*           if modified, make sure INT_MOD_MASK can still be calculated */
#define BITS_IN_INT_GEN 32


/*      INT_MOD_MASK is used to perform modular arithmetic - specifying  */
/*           this value compensates for different sized words on         */
/*           different architectures                                     */
/*      FLT_MULT is used in converting to float and double values; the   */
/*           odd form is due to a compiler glitch on our CM-5, which     */
/*	     caused (0.5/(unsigned)(1<<31)) to be negative.              */
#if (BITS_IN_INT_GEN==32)
#define INT_MOD_MASK 0xffffffff
#define INT_MOD_MASK2 0xfffffffe
/*  #define FLT_MULT (0.25/(unsigned)(1<<30)) */
#define FLT_MULT 2.3283064365386962891e-10
/*
  #else
  #define INT_MOD_MASK ((unsigned)(1<<BITS_IN_INT_GEN)-1)
  #define FLT_MULT (1.0/(1<<BITS_IN_INT_GEN))
*/
#endif


/*      INT_MASK is used to mask out the part of the generator which     */
/*           is not in the canonical form; it should be                  */
/*           2^{BITS_IN_INT_GEN-1}-1                                     */
#define INT_MASK ((unsigned)INT_MOD_MASK>>1)


/*      MAX_BIT_INT is the largest bit position allowed in the index     */
/*           of the node - it equals BITS_IN_INT_GEN - 2                 */
#define MAX_BIT_INT (BITS_IN_INT_GEN-2)


/*      INTX2_MASK is used in calculation of the node numbers            */
#define INTX2_MASK ((1<<MAX_BIT_INT)-1)
 

/*      RUNUP keeps certain generators from looking too similar in the   */
/*          first few words output                                       */
#define RUNUP (2*BITS_IN_INT_GEN)


/*      GS0 gives a more "random" distribution of generators when the    */
/*      user uses small integers as seeds                                */
#define GS0 0x372f05ac
#define TOOMANY "generator has branched maximum number of times;\nindependence of generators no longer guaranteed"


/*************************************************************************/
/*************************************************************************/
/*                  STRUCTURES AND GLOBAL DATA                           */
/*************************************************************************/
/*************************************************************************/

#define MAX_STREAMS 0x7fffffff

/*
  Originally, you could specify a parameter when creating generator streams.
  This parameter was an index into the "valid" array below, which specified
  valid sets of related parameters.  The current version of the code is hard
  coded to use one parameter set, as laid out in the VALID_xxx definitions
  below.  


#define NPARAMS 11
struct vstruct {
    int L;        // The length of the generator in bytes 
    int K;        // The lag (index distance) between the first and second 
                  // generator 
    int LSBS;     // number of least significant bits that are 1  
    int first;    // the first seed whose LSB is 1  
};

const struct vstruct valid[NPARAMS] = { {1279,861,1,233}, 
                                        {17,5,1,10}, 
                                        {31,6,1,2},
                                        {55,24,1,11}, 
                                        {63,31,1,14}, 
                                        {127,97,1,21},
                                        {521,353,1,100}, 
                                        {521,168,1,83}, 
                                        {607,334,1,166}, 
                                        {607,273,1,105},
                                        {1279,418,1,208} };
*/

/* 
Pick a particular valid parameter set.  Note that if you change these,
   this library cannot correctly read packed state from the previous
   parameter set. 
*/


#if LFG_PARAM_SET == 1
#define VALID_L      17
#define VALID_K       5
#define VALID_LSBS    1
#define VALID_first  10
/* !!!! Keep this in sync with VALID_L (This is (VALID_L - 1))!!!! */
#define VALID_LM1    16
#elif LFG_PARAM_SET == 2
#define VALID_L      31
#define VALID_K       6
#define VALID_LSBS    1
#define VALID_first   2
/* !!!! Keep this in sync with VALID_L (This is (VALID_L - 1))!!!! */
#define VALID_LM1    30
#else
#error "INVALID LFG PARAMETER SET"
#endif


/*
   There used to this lfgen structure:

struct lfgen 
{
    unsigned si[VALID_LM1];	// sets next branch seed   
    unsigned r0[VALID_L];	// the even generator data 
    unsigned r1[VALID_L];	// the odd generator data 
    unsigned hidx;		// index into r0 and r1  
    unsigned seed;
    unsigned initseed;
};

  But to reduce the in-memory size to be exactly the state size, I replaced
  that struct with a simple array, and use these offsets to index into it.
*/

#define OFFSET_si         0
#define OFFSET_r0         (VALID_LM1)
#define OFFSET_r1         (VALID_LM1 + VALID_L)
#define OFFSET_hidx       (VALID_LM1 + VALID_L + VALID_L)
#define OFFSET_seed       (VALID_LM1 + VALID_L + VALID_L + 1)
#define OFFSET_initseed   (VALID_LM1 + VALID_L + VALID_L + 2)
#define OFFSET_gennum     (VALID_LM1 + VALID_L + VALID_L + 3)
#define OFFSET_END        (VALID_LM1 + VALID_L + VALID_L + 4)

static unsigned gseed = 0;

/*************************************************************************/
/*************************************************************************/
/*                    ERROR PRINTING FUNCTION                            */
/*************************************************************************/
/*************************************************************************/

void errprint(char const *level, char const *routine, char const *error)
{
      fprintf(stderr,"%s from %s: %s\n",level,routine,error);
}

/*************************************************************************/
/*************************************************************************/
/*            ROUTINES USED TO CREATE GENERATOR FILLS                    */
/*************************************************************************/
/*************************************************************************/

/* The number of bits set in the binary representation of the integers 0-255 */
static const unsigned bitcount[256] =
    {
	0, /* 0 */	1, /* 1 */	1, /* 2 */	2, /* 3 */
	1, /* 4 */	2, /* 5 */	2, /* 6 */	3, /* 7 */
	1, /* 8 */	2, /* 9 */	2, /* 10 */	3, /* 11 */
	2, /* 12 */	3, /* 13 */	3, /* 14 */	4, /* 15 */
	1, /* 16 */	2, /* 17 */	2, /* 18 */	3, /* 19 */
	2, /* 20 */	3, /* 21 */	3, /* 22 */	4, /* 23 */
	2, /* 24 */	3, /* 25 */	3, /* 26 */	4, /* 27 */
	3, /* 28 */	4, /* 29 */	4, /* 30 */	5, /* 31 */
	1, /* 32 */	2, /* 33 */	2, /* 34 */	3, /* 35 */
	2, /* 36 */	3, /* 37 */	3, /* 38 */	4, /* 39 */
	2, /* 40 */	3, /* 41 */	3, /* 42 */	4, /* 43 */
	3, /* 44 */	4, /* 45 */	4, /* 46 */	5, /* 47 */
	2, /* 48 */	3, /* 49 */	3, /* 50 */	4, /* 51 */
	3, /* 52 */	4, /* 53 */	4, /* 54 */	5, /* 55 */
	3, /* 56 */	4, /* 57 */	4, /* 58 */	5, /* 59 */
	4, /* 60 */	5, /* 61 */	5, /* 62 */	6, /* 63 */
	1, /* 64 */	2, /* 65 */	2, /* 66 */	3, /* 67 */
	2, /* 68 */	3, /* 69 */	3, /* 70 */	4, /* 71 */
	2, /* 72 */	3, /* 73 */	3, /* 74 */	4, /* 75 */
	3, /* 76 */	4, /* 77 */	4, /* 78 */	5, /* 79 */
	2, /* 80 */	3, /* 81 */	3, /* 82 */	4, /* 83 */
	3, /* 84 */	4, /* 85 */	4, /* 86 */	5, /* 87 */
	3, /* 88 */	4, /* 89 */	4, /* 90 */	5, /* 91 */
	4, /* 92 */	5, /* 93 */	5, /* 94 */	6, /* 95 */
	2, /* 96 */	3, /* 97 */	3, /* 98 */	4, /* 99 */
	3, /* 100 */	4, /* 101 */	4, /* 102 */	5, /* 103 */
	3, /* 104 */	4, /* 105 */	4, /* 106 */	5, /* 107 */
	4, /* 108 */	5, /* 109 */	5, /* 110 */	6, /* 111 */
	3, /* 112 */	4, /* 113 */	4, /* 114 */	5, /* 115 */
	4, /* 116 */	5, /* 117 */	5, /* 118 */	6, /* 119 */
	4, /* 120 */	5, /* 121 */	5, /* 122 */	6, /* 123 */
	5, /* 124 */	6, /* 125 */	6, /* 126 */	7, /* 127 */
	1, /* 128 */	2, /* 129 */	2, /* 130 */	3, /* 131 */
	2, /* 132 */	3, /* 133 */	3, /* 134 */	4, /* 135 */
	2, /* 136 */	3, /* 137 */	3, /* 138 */	4, /* 139 */
	3, /* 140 */	4, /* 141 */	4, /* 142 */	5, /* 143 */
	2, /* 144 */	3, /* 145 */	3, /* 146 */	4, /* 147 */
	3, /* 148 */	4, /* 149 */	4, /* 150 */	5, /* 151 */
	3, /* 152 */	4, /* 153 */	4, /* 154 */	5, /* 155 */
	4, /* 156 */	5, /* 157 */	5, /* 158 */	6, /* 159 */
	2, /* 160 */	3, /* 161 */	3, /* 162 */	4, /* 163 */
	3, /* 164 */	4, /* 165 */	4, /* 166 */	5, /* 167 */
	3, /* 168 */	4, /* 169 */	4, /* 170 */	5, /* 171 */
	4, /* 172 */	5, /* 173 */	5, /* 174 */	6, /* 175 */
	3, /* 176 */	4, /* 177 */	4, /* 178 */	5, /* 179 */
	4, /* 180 */	5, /* 181 */	5, /* 182 */	6, /* 183 */
	4, /* 184 */	5, /* 185 */	5, /* 186 */	6, /* 187 */
	5, /* 188 */	6, /* 189 */	6, /* 190 */	7, /* 191 */
	2, /* 192 */	3, /* 193 */	3, /* 194 */	4, /* 195 */
	3, /* 196 */	4, /* 197 */	4, /* 198 */	5, /* 199 */
	3, /* 200 */	4, /* 201 */	4, /* 202 */	5, /* 203 */
	4, /* 204 */	5, /* 205 */	5, /* 206 */	6, /* 207 */
	3, /* 208 */	4, /* 209 */	4, /* 210 */	5, /* 211 */
	4, /* 212 */	5, /* 213 */	5, /* 214 */	6, /* 215 */
	4, /* 216 */	5, /* 217 */	5, /* 218 */	6, /* 219 */
	5, /* 220 */	6, /* 221 */	6, /* 222 */	7, /* 223 */
	3, /* 224 */	4, /* 225 */	4, /* 226 */	5, /* 227 */
	4, /* 228 */	5, /* 229 */	5, /* 230 */	6, /* 231 */
	4, /* 232 */	5, /* 233 */	5, /* 234 */	6, /* 235 */
	5, /* 236 */	6, /* 237 */	6, /* 238 */	7, /* 239 */
	4, /* 240 */	5, /* 241 */	5, /* 242 */	6, /* 243 */
	5, /* 244 */	6, /* 245 */	6, /* 246 */	7, /* 247 */
	5, /* 248 */	6, /* 249 */	6, /* 250 */	7, /* 251 */
	6, /* 252 */	7, /* 253 */	7, /* 254 */	8  /* 255 */
    };


/*
  Given the code snippet:

    const unsigned mask = 0x1b;
    unsigned new_fill = 0;
    for (i=0; i < 4; ++i) 
       new_fill |=  (1&bitcount[(val >> i)&mask]) << i; 

  the following array precomputes values of new_fill for val=0..255
*/
static const unsigned mask_gen[256] = {
    0, /*   0 */    1, /*   1 */    3, /*   2 */    2, /*   3 */
    6, /*   4 */    7, /*   5 */    5, /*   6 */    4, /*   7 */
   13, /*   8 */   12, /*   9 */   14, /*  10 */   15, /*  11 */
   11, /*  12 */   10, /*  13 */    8, /*  14 */    9, /*  15 */
   11, /*  16 */   10, /*  17 */    8, /*  18 */    9, /*  19 */
   13, /*  20 */   12, /*  21 */   14, /*  22 */   15, /*  23 */
    6, /*  24 */    7, /*  25 */    5, /*  26 */    4, /*  27 */
    0, /*  28 */    1, /*  29 */    3, /*  30 */    2, /*  31 */
    6, /*  32 */    7, /*  33 */    5, /*  34 */    4, /*  35 */
    0, /*  36 */    1, /*  37 */    3, /*  38 */    2, /*  39 */
   11, /*  40 */   10, /*  41 */    8, /*  42 */    9, /*  43 */
   13, /*  44 */   12, /*  45 */   14, /*  46 */   15, /*  47 */
   13, /*  48 */   12, /*  49 */   14, /*  50 */   15, /*  51 */
   11, /*  52 */   10, /*  53 */    8, /*  54 */    9, /*  55 */
    0, /*  56 */    1, /*  57 */    3, /*  58 */    2, /*  59 */
    6, /*  60 */    7, /*  61 */    5, /*  62 */    4, /*  63 */
   12, /*  64 */   13, /*  65 */   15, /*  66 */   14, /*  67 */
   10, /*  68 */   11, /*  69 */    9, /*  70 */    8, /*  71 */
    1, /*  72 */    0, /*  73 */    2, /*  74 */    3, /*  75 */
    7, /*  76 */    6, /*  77 */    4, /*  78 */    5, /*  79 */
    7, /*  80 */    6, /*  81 */    4, /*  82 */    5, /*  83 */
    1, /*  84 */    0, /*  85 */    2, /*  86 */    3, /*  87 */
   10, /*  88 */   11, /*  89 */    9, /*  90 */    8, /*  91 */
   12, /*  92 */   13, /*  93 */   15, /*  94 */   14, /*  95 */
   10, /*  96 */   11, /*  97 */    9, /*  98 */    8, /*  99 */
   12, /* 100 */   13, /* 101 */   15, /* 102 */   14, /* 103 */
    7, /* 104 */    6, /* 105 */    4, /* 106 */    5, /* 107 */
    1, /* 108 */    0, /* 109 */    2, /* 110 */    3, /* 111 */
    1, /* 112 */    0, /* 113 */    2, /* 114 */    3, /* 115 */
    7, /* 116 */    6, /* 117 */    4, /* 118 */    5, /* 119 */
   12, /* 120 */   13, /* 121 */   15, /* 122 */   14, /* 123 */
   10, /* 124 */   11, /* 125 */    9, /* 126 */    8, /* 127 */
    8, /* 128 */    9, /* 129 */   11, /* 130 */   10, /* 131 */
   14, /* 132 */   15, /* 133 */   13, /* 134 */   12, /* 135 */
    5, /* 136 */    4, /* 137 */    6, /* 138 */    7, /* 139 */
    3, /* 140 */    2, /* 141 */    0, /* 142 */    1, /* 143 */
    3, /* 144 */    2, /* 145 */    0, /* 146 */    1, /* 147 */
    5, /* 148 */    4, /* 149 */    6, /* 150 */    7, /* 151 */
   14, /* 152 */   15, /* 153 */   13, /* 154 */   12, /* 155 */
    8, /* 156 */    9, /* 157 */   11, /* 158 */   10, /* 159 */
   14, /* 160 */   15, /* 161 */   13, /* 162 */   12, /* 163 */
    8, /* 164 */    9, /* 165 */   11, /* 166 */   10, /* 167 */
    3, /* 168 */    2, /* 169 */    0, /* 170 */    1, /* 171 */
    5, /* 172 */    4, /* 173 */    6, /* 174 */    7, /* 175 */
    5, /* 176 */    4, /* 177 */    6, /* 178 */    7, /* 179 */
    3, /* 180 */    2, /* 181 */    0, /* 182 */    1, /* 183 */
    8, /* 184 */    9, /* 185 */   11, /* 186 */   10, /* 187 */
   14, /* 188 */   15, /* 189 */   13, /* 190 */   12, /* 191 */
    4, /* 192 */    5, /* 193 */    7, /* 194 */    6, /* 195 */
    2, /* 196 */    3, /* 197 */    1, /* 198 */    0, /* 199 */
    9, /* 200 */    8, /* 201 */   10, /* 202 */   11, /* 203 */
   15, /* 204 */   14, /* 205 */   12, /* 206 */   13, /* 207 */
   15, /* 208 */   14, /* 209 */   12, /* 210 */   13, /* 211 */
    9, /* 212 */    8, /* 213 */   10, /* 214 */   11, /* 215 */
    2, /* 216 */    3, /* 217 */    1, /* 218 */    0, /* 219 */
    4, /* 220 */    5, /* 221 */    7, /* 222 */    6, /* 223 */
    2, /* 224 */    3, /* 225 */    1, /* 226 */    0, /* 227 */
    4, /* 228 */    5, /* 229 */    7, /* 230 */    6, /* 231 */
   15, /* 232 */   14, /* 233 */   12, /* 234 */   13, /* 235 */
    9, /* 236 */    8, /* 237 */   10, /* 238 */   11, /* 239 */
    9, /* 240 */    8, /* 241 */   10, /* 242 */   11, /* 243 */
   15, /* 244 */   14, /* 245 */   12, /* 246 */   13, /* 247 */
    4, /* 248 */    5, /* 249 */    7, /* 250 */    6, /* 251 */
    2, /* 252 */    3, /* 253 */    1, /* 254 */    0  /* 255 */

};


/***************************************************************************/

static void 
advance_reg(int *reg_fill) 
{
    /*      the register steps according to the primitive polynomial         */
    /*           (64,4,3,1,0); each call steps register 64 times             */
    /*      we use two words to represent the register to allow for integer  */
    /*           size of 32 bits                                             */


    /* NOTE: if this changes from 0x1b, the contents of the mask_gen array
       need to be recomputed! */
    const unsigned int mask = 0x1b;

    int i;
    unsigned new_fill0 = 0;
    unsigned new_fill1 = 0;
    unsigned temp;
    const unsigned urf0 = (unsigned)reg_fill[0];
    const unsigned urf1 = (unsigned)reg_fill[1];
    const unsigned urf0_sr_24 = urf0 >> 24;

    for (i=0; i < 28; i+=4) 
    {
	new_fill0 |= mask_gen[(urf0 >> i)&255] << i; 
	new_fill1 |= mask_gen[(urf1 >> i)&255] << i; 
    }

    /* i = 28 */
    temp = bitcount[(urf0 >> 28)&mask];
    temp ^= bitcount[urf1&(mask>>4)];
    new_fill0 |= (1&temp)<<28;

    temp = bitcount[urf0_sr_24 & 0xb0];
    temp ^= bitcount[urf1 & 0x1b];
    new_fill1 |= (1&temp)<<28;

    /* i = 29 */

    temp = bitcount[(urf0 >> 29)&mask];
    temp ^= bitcount[urf1&(mask>>3)];
    new_fill0 |= (1&temp)<<29;

    temp = bitcount[urf0_sr_24 & 0x60];
    temp ^= bitcount[urf1 & 0x2d];
    new_fill1 |= (1&temp)<<29;

    /* i = 30 */

    temp = bitcount[(urf0 >> 30)&mask];
    temp ^= bitcount[urf1&(mask>>2)];
    new_fill0 |= (1&temp)<<30;

    temp = bitcount[urf0_sr_24 & 0xc0];
    temp ^= bitcount[urf1 & 0x5a];
    new_fill1 |= (1&temp)<<30;

    /* i = 31 */

    temp = bitcount[(urf0 >> 31)&mask];
    temp ^= bitcount[urf1&(mask>>1)];
    new_fill0 |= (1&temp)<<31;

    temp = bitcount[urf0_sr_24 & 0x80];
    temp ^= bitcount[urf1 & 0xaf];
    new_fill1 |= (1&temp)<<31;


    reg_fill[0] = new_fill0;
    reg_fill[1] = new_fill1;
}


/***************************************************************************/


/* 
   Initialize the shift register (r0 or r1) with the node number XORed with
   the global seed  
*/

static void
get_fill(unsigned const * LFG_RESTRICT n,
	 unsigned * LFG_RESTRICT r,
	 const unsigned seed)
{
    int i,j,k,temp[2];

    /*      fill the shift register with two copies of this number           */
    /*           except when equal to zero                                   */
    temp[1] = temp[0] = n[0]^seed;
    if (!temp[0])
        temp[0] = GS0;

    /*      advance the shift register some                                  */
    advance_reg(temp);
    advance_reg(temp);

    /* the first word in the generator is defined by the 31 LSBs of the 
       node number                                                 */
    r[0] = (INT_MASK&n[0])<<1;

    /*  the generator is filled with the lower 31 bits of the shift      
        register at each time, shifted up to make room for the bits 
        defining the canonical form; the node number is XORed into
        the fill to make the generators unique. */
    for (i=1;i<VALID_L;i++) 
    {
        advance_reg(temp);
        r[i] = (INT_MASK&(temp[0]^n[i]))<<1;
    }
    r[VALID_LM1] = 0;

    /*  the canonical form for the LSB is instituted here */
    k = VALID_first + VALID_LSBS;

    for (j=VALID_first; j<k; j++)
        r[j] |= 1;

}

/*************************************************************************/
/*************************************************************************/
/*            SI_DOUBLE: updates index for next spawning                 */
/*************************************************************************/
/*************************************************************************/

/* a = b<<1 */

static void
si_double(unsigned * LFG_RESTRICT a,
          unsigned const * const LFG_RESTRICT b)
{
    int i;

    if (b[VALID_L-2]&(1<<MAX_BIT_INT))
        errprint("WARNING","si_double",TOOMANY);

    a[VALID_L-2] = (INTX2_MASK&b[VALID_L-2])<<1;

    for (i=VALID_L-3;i>=0;i--)
    {
        if (b[i]&(1<<MAX_BIT_INT))
            a[i+1]++;

        a[i] = (INTX2_MASK&b[i])<<1;
    }
}


/* a = a<<1 */
static void 
si_double_aeqb(unsigned * const a)
{
    int i;

    if (a[VALID_L-2]&(1<<MAX_BIT_INT))
        errprint("WARNING","si_double",TOOMANY);

    a[VALID_L-2] = (INTX2_MASK & a[VALID_L-2])<<1;

    for (i=VALID_L-3 ; i > -1 ; --i) 
    {
        if(a[i] & (1<<MAX_BIT_INT)) a[i+1]++;
        a[i] = (INTX2_MASK & a[i])<<1;
    }
}


/*************************************************************************/
/*************************************************************************/
/*            GET_RN: returns generated random number                    */
/*************************************************************************/
/*************************************************************************/


/* [2010-11-04 KT: Comment out unused function. */

/* int lfg_gen_int(unsigned *genptr) */
/* { */
/*     unsigned new_val; */
/*     unsigned * const LFG_RESTRICT r0 = genptr+OFFSET_r0; */
/*     unsigned * const LFG_RESTRICT r1 = genptr+OFFSET_r1; */
/*     unsigned * const LFG_RESTRICT hp = genptr+OFFSET_hidx; */

/*     int hidx, lidx; */

/*     hidx = *hp; */
/*     lidx = hidx + VALID_K; */
/*     if (lidx > VALID_LM1) lidx -= VALID_L; */
/*     /\*    INT_MOD_MASK causes arithmetic to be modular when integer size is  *\/ */
/*     /\*         different from generator modulus                              *\/ */
/*     r0[hidx] = INT_MOD_MASK&(r0[hidx] + r0[lidx]); */
/*     r1[hidx] = INT_MOD_MASK&(r1[hidx] + r1[lidx]); */
/*     new_val = (r1[hidx]&(~1)) ^ (r0[hidx]>>1); */

/*     if (--hidx < 0) hidx = VALID_LM1; /\* skip an element in the sequence *\/ */
/*     if (--lidx < 0) lidx = VALID_LM1; */
/*     r0[hidx] = INT_MOD_MASK&(r0[hidx] + r0[lidx]); */
/*     r1[hidx] = INT_MOD_MASK&(r1[hidx] + r1[lidx]); */
/*     *hp = (--hidx < 0) ? VALID_LM1 : hidx; */

/*     return (new_val>>1); */
/* } */


/* A special version of lfg_gen_int used to initialize a stream. This
   simulates calling lfg_gen_int (VALID_L*nmul) times.  This
   assumes that genptr[OFFSET_hidx] == VALID_LM1, which is known to be true
   only at initialization time.  */

static void
fake_rn_int_Lmul(unsigned * const genptr, unsigned const nmul)
{
    unsigned * const LFG_RESTRICT r0 = genptr+OFFSET_r0;
    unsigned * const LFG_RESTRICT r1 = genptr+OFFSET_r1;

    int hidx, lidx, inner_idx;
    unsigned outer_idx;

    for(outer_idx = 0; outer_idx < nmul; ++outer_idx)
    {

        hidx = VALID_LM1;
        lidx = VALID_K - 1;

        /* This first loop decrements lidx down to -1 */
        for(inner_idx = 0; inner_idx < VALID_K; ++inner_idx)
        {
            r0[hidx] = INT_MOD_MASK&(r0[hidx] + r0[lidx]);
            r1[hidx] = INT_MOD_MASK&(r1[hidx] + r1[lidx]);
            --hidx;
            --lidx;
        }

        /* Loop it around */
        lidx = VALID_LM1;

        /* Now run hidx down to -1 */
        for(inner_idx = 0; inner_idx < (VALID_L - VALID_K); ++inner_idx)
        {
            r0[hidx] = INT_MOD_MASK&(r0[hidx] + r0[lidx]);
            r1[hidx] = INT_MOD_MASK&(r1[hidx] + r1[lidx]);
            --hidx;
            --lidx;
        }
    }

    /* We don't need to change genptr[OFFSET_hidx] because it is back to its */
    /* original value of VALID_LM1 */
}


/* [2010-11-04 KT: Comment out unused function. */

/* float lfg_gen_flt(unsigned *genptr) */
/* { */
/*     /\* this cannot be unsigned int due to a bug in the SGI compiler *\/ */
/*     unsigned long new_val;  */

/*     unsigned * const LFG_RESTRICT r0 = genptr+OFFSET_r0; */
/*     unsigned * const LFG_RESTRICT r1 = genptr+OFFSET_r1; */
/*     unsigned * const LFG_RESTRICT hp = genptr+OFFSET_hidx; */
/*     int hidx,lidx; */

/*     hidx = *hp; */
/*     lidx = hidx + VALID_K; */
/*     if (lidx >= VALID_L) lidx -= VALID_L; */
/*     /\*    INT_MOD_MASK causes arithmetic to be modular when integer size is  *\/ */
/*     /\*         different from generator modulus                              *\/ */
/*     r0[hidx] = INT_MOD_MASK&(r0[hidx] + r0[lidx]); */
/*     r1[hidx] = INT_MOD_MASK&(r1[hidx] + r1[lidx]); */
/*     new_val = (r1[hidx]&(~1)) ^ (r0[hidx]>>1); */

/*     if (--hidx < 0) hidx = VALID_LM1; /\* skip an element in the sequence *\/ */
/*     if (--lidx < 0) lidx = VALID_LM1; */
/*     r0[hidx] = INT_MOD_MASK&(r0[hidx] + r0[lidx]); */
/*     r1[hidx] = INT_MOD_MASK&(r1[hidx] + r1[lidx]); */
/*     *hp = (--hidx<0) ? VALID_L -1 : hidx; */

/*     return (new_val*FLT_MULT); */
/* }  */



double 
lfg_gen_dbl(unsigned *genptr)
{
    unsigned * const LFG_RESTRICT r0 = genptr+OFFSET_r0;
    unsigned * const LFG_RESTRICT r1 = genptr+OFFSET_r1;
    unsigned * const LFG_RESTRICT hp = genptr+OFFSET_hidx;

    unsigned long temp1,temp2; /* Due to a bug in the SGI compiler, this should not be unsigned int */
    int hidx,lidx;

    double new_val;

    hidx = *hp;
    lidx = hidx + VALID_K;
    if (lidx>= VALID_L) lidx -= VALID_L;
    r0[hidx] = INT_MOD_MASK&(r0[hidx] + r0[lidx]);
    r1[hidx] = INT_MOD_MASK&(r1[hidx] + r1[lidx]);
    temp1 = (r1[hidx]&(~1)) ^ (r0[hidx]>>1);

    /* Rather than skipping an element in the sequence, we use the next element
       as a second random number  */ 
    if (--hidx < 0) hidx = VALID_LM1;
    if (--lidx < 0) lidx = VALID_LM1;
    r0[hidx] = INT_MOD_MASK&(r0[hidx] + r0[lidx]);
    r1[hidx] = INT_MOD_MASK&(r1[hidx] + r1[lidx]);
    temp2 = (r1[hidx]&(~1)) ^ (r0[hidx]>>1);

    *hp = (--hidx < 0) ? VALID_LM1 : hidx;

    new_val = ((unsigned int) temp2*(double)FLT_MULT +
               (unsigned int) temp1)*FLT_MULT;

#ifdef DEBUG
    assert(!(new_val < 0 || new_val > 1));
#endif
    return new_val;
}

/*************************************************************************/
/*************************************************************************/
/*            INITIALIZE: starts the whole thing going                   */
/*************************************************************************/
/*************************************************************************/

/* Use this trick to suppress warnings about 'unused argument: end' */
#define UNUSED_ARGUMENT(x) (void)x

static void
initialize1(
    unsigned const  seed, 
    unsigned const * const nstart, 
    unsigned const initseed,
    unsigned* begin,
    unsigned* end
    )
{
    unsigned* const q0 = begin;
    unsigned* const si = begin + OFFSET_si;

#ifdef DEBUG
    assert(!((end - begin) < OFFSET_END));
#else
    UNUSED_ARGUMENT(end);
#endif
    q0[OFFSET_hidx] = VALID_LM1;
    q0[OFFSET_seed] = seed;
    q0[OFFSET_initseed] = initseed;

    /* specify register fills and node number arrays */
    si_double(si, nstart);
}


static void
initialize2(
    unsigned* begin
/*    ,unsigned* end */
    )
{
    int j,k;
    unsigned* const q0   = begin;
    unsigned* const si   = begin + OFFSET_si;
    unsigned  const seed = q0[OFFSET_seed];

    get_fill(si, q0+OFFSET_r0, seed);
    si[0]++;
    get_fill(si, q0+OFFSET_r1, seed);

    /* fake the generation of a sequence of random numbers to advance the
       state a little */
    k = 0;
    for (j=1; j < VALID_LM1 && !k; j++)
        k = (si[j] != 0);
    if (k) 
        fake_rn_int_Lmul(q0, RUNUP << 1);
    else
        fake_rn_int_Lmul(q0, 8);
}

/*************************************************************************/
/*************************************************************************/
/*            INIT_RNG's: user interface to start things off             */
/*************************************************************************/
/*************************************************************************/


void
lfg_create_rng_part1(const unsigned gennum, 
                     unsigned seed,
                     unsigned* begin, unsigned* end)
{
    int i;
    unsigned nstart[VALID_LM1];
    /* unsigned * const si = begin + OFFSET_si; */

  
    /*      gives back one generator (node gennum) with updated spawning  */
    /*      info; should be called total_gen times, with different value  */
    /*      of gennum in [0,total_gen) each call  */


    /*      check values of gennum and total_gen  */
    if (gennum >= MAX_STREAMS) /* check if gen_num is valid    */
        fprintf(stderr, "WARNING - init_rng: gennum: %d >"
                " maximum number of independent streams: %d\n"
                "\tIndependence of streams cannot be guaranteed.\n",
                gennum, MAX_STREAMS); 

    seed &= 0x7fffffff;		/* Only 32 LSB of seed considered */
  
    /* Check that this seed is consistent with previously provided values. */
    if(!gseed)
        gseed = seed^GS0;
    else
        if(seed != (gseed^GS0))
        {
            errprint("WARNING","init_rng","changing global seed value! "
                     "Independence of streams is not guaranteed");
        }

  
    /* define the starting vector for the initial node */
    nstart[0] = gennum;
    for (i=1; i < VALID_LM1; ++i) 
        nstart[i] = 0;
    begin[OFFSET_gennum] = gennum;


    /* create a generator  */
    initialize1(seed^GS0,nstart,seed, begin, end); 

    /*... now you have to call part2! */
}


void
lfg_create_rng_part2(unsigned* begin /*, unsigned* end */)
{
    unsigned * const si = begin + OFFSET_si;

    initialize2(begin /*, end */); 
 
    /* update si array to allow for future spawning of generators */

    /* This billion figure used to be passed in as the max number of streams,
     * but it is hard coded for now CHANGE ME */
    while (si[0] < 1000000000 && !si[1]) 
        si_double_aeqb(si);
}


void
lfg_create_rng(const unsigned gennum, unsigned seed,
	       unsigned* begin, unsigned* end)
{
    lfg_create_rng_part1(gennum, seed, begin, end);
    lfg_create_rng_part2(begin /*, end */);
}


/*************************************************************************/
/*************************************************************************/
/*                  SPAWN_RNG: spawns new generators                     */
/*************************************************************************/
/*************************************************************************/

void
lfg_spawn_rng(unsigned *genptr, unsigned* begin, unsigned* end)
{
    unsigned * const p = genptr+OFFSET_si;
  
    initialize1(genptr[OFFSET_seed],
               p,
               genptr[OFFSET_initseed],
               begin, end);
    initialize2(begin /*, end */);
  
    /* The new gennum is the same as the old one */
    begin[OFFSET_gennum] = genptr[OFFSET_gennum];

    /* Advance the spawn data to be ready for next time */
    si_double_aeqb(p);
}

/* [2010-11-04 KT: Comment out unused function. */

/* void */
/* lfg_print(unsigned *genptr) */
/* { */
/*     printf("\nLagged Fibonacci Generator:\n"); */
/*     printf("\n \tseed = %u, lags = (%d,%d)\n\n",  */
/* 	   genptr[OFFSET_initseed], VALID_L, VALID_K); */
/* } */


unsigned
lfg_size()
{
    return OFFSET_END;
}

unsigned
lfg_gennum(unsigned *genptr)
{
    return genptr[OFFSET_gennum];
}

unsigned
lfg_unique_num(unsigned *genptr)
{
    return (genptr[OFFSET_gennum] + genptr[0]);
}
