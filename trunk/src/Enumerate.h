#pragma once

#include "MMConfig.h"

///////////////////////////////////////////////////////////////////////////
// Arithmatic routines

int NPermute(int n, int r);
int NPower(int n, int r);
int NComb(int n, int r) ;

///////////////////////////////////////////////////////////////////////////
// Codeword enumeration routines

int Enumerate_Rep(
	int length, 
	int ndigits, 
	codeword_t* results);

int Enumerate_NoRep(
	int length, 
	int ndigits, 
	codeword_t* results);

///////////////////////////////////////////////////////////////////////////
// Codeword list filtering routines

int FilterByEquivalence_NoRep(
	const codeword_t *src,
	unsigned int nsrc,
	const unsigned char eqclass[16],
	codeword_t *dest);

int FilterByEquivalence_Rep(
	const codeword_t *src,
	unsigned int nsrc,
	const unsigned char eqclass[16],
	codeword_t *dest);
