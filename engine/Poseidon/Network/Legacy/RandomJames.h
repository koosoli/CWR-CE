#ifndef _RANDOMJAMES_H
#define _RANDOMJAMES_H

// Marsaglia-Zaman / F. James pseudo-random generator (RANMAR). Period 2^144,
// bit-identical on any platform with >= 24-bit float mantissa. Combines a lagged
// Fibonacci sequence (lags 97 and 33) with an arithmetic sequence.

class RandomJames {

protected:

    double u[97], c, cd, cm;
    int i97, j97;
    double prepMean, prepVar, norMul, norAdd;
    int prepRep;

public:

    bool ok;                                // false == error

    RandomJames ( int ij =1802, int kl =9373 );

    void reset ( int ij =1802, int kl =9373 );
        // deterministic sequence restart
    void randomize ();
        // undeterministic sequence (uses system time)
    bool test ();
        // test the random generator

    double uniformNumber ();
        // uniform random number from [0,1]
    void uniformNumbers ( double *vec, int len );
        // vector of uniform random numbers from [0,1]

    double normalNumber ( double mean, double variance, int rep =4 );
        // random number from normal (Gaussian) distribution
    void normalNumbers ( double mean, double variance, int rep, double *vec, int len );
        // vector of random numbers from normal (Gaussian) distribution

protected:

    inline void checkNormal ( double mean, double variance, int rep );

    };

#endif

