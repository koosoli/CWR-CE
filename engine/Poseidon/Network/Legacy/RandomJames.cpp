#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <time.h>
#include <Poseidon/Network/Legacy/RandomJames.h>
#include <cmath>
#include <Poseidon/Foundation/Framework/Log.hpp>

bool RandomJames::test()
{
    reset(1802, 9373);
    if (!ok)
    {
        return false;
    }
    double temp[1000];
    int i;
    for (i = 0; i++ < 20;)
    {
        uniformNumbers(temp, 1000);
        if (!ok)
        {
            return false;
        }
    }
    uniformNumbers(temp, 6);
    for (i = 0; i < 6; i++)
    {
        temp[i] = temp[i] * 4096.0 * 4096.0 + 0.5;
    }
    return ((int)temp[0] == 6533892 && (int)temp[1] == 14220222 && (int)temp[2] == 7275067 && (int)temp[3] == 6172232 &&
            (int)temp[4] == 8354498 && (int)temp[5] == 10633180);
}

// Seed ranges: 0 <= ij <= 31328, 0 <= kl <= 30081. Seeds 1802/9373 are the
// canonical self-test pair (see test()).
void RandomJames::reset(int ij, int kl)
{
    double s, t;
    int i, j, k, l, m;
    int ii, jj;

    if (ij < 0 || ij > 31328 || kl < 0 || kl > 30081)
    {
        LOG_DEBUG(Core, "RandomJames::reset() - the first random number seed must have a "
                        "value between 0 and 31328");
        LOG_DEBUG(Core, "                       the second random number seed must have a "
                        "value between 0 and 30081");
        ok = false;
        return;
    }
    ok = true;

    i = (int)fmod(ij / 177.0, 177.0) + 2;
    j = (int)fmod(ij, 177.0) + 2;
    k = (int)fmod(kl / 169.0, 178.0) + 1;
    l = (int)fmod(kl, 169.0);

    for (ii = 0; ii < 97; ii++)
    {
        s = 0.0;
        t = 0.5;
        for (jj = 0; jj < 24; jj++)
        {
            m = (int)fmod(fmod(i * j, 179.0) * k, 179.0);
            i = j;
            j = k;
            k = m;
            l = (int)fmod(53.0 * l + 1.0, 169.0);
            if (fmod(l * m, 64.0) >= 32)
            {
                s = s + t;
            }
            t *= 0.5;
        }
        u[ii] = s;
    }

    c = 362436.0 / 16777216.0;
    cd = 7654321.0 / 16777216.0;
    cm = 16777213.0 / 16777216.0;

    i97 = 96;
    j97 = 32;
}

RandomJames::RandomJames(int ij, int kl)
{
    reset(ij, kl);
    prepMean = 0.0;
    prepVar = -1.0;
    prepRep = 0;
}

double RandomJames::uniformNumber()
{
    if (!ok)
    {
        return 0.0;
    }
    double uni = u[i97] - u[j97];
    if (uni < 0.0)
    {
        uni += 1.0;
    }
    u[i97] = uni;
    if (--i97 < 0)
    {
        i97 = 96;
    }
    if (--j97 < 0)
    {
        j97 = 96;
    }
    if ((c -= cd) < 0.0)
    {
        c += cm;
    }
    if ((uni -= c) < 0.0)
    {
        uni += 1.0;
    }
    return uni;
}

void RandomJames::uniformNumbers(double* vec, int len)
{
    if (!vec)
    {
        return;
    }
    while (len--)
    {
        *vec++ = uniformNumber();
    }
}

inline void RandomJames::checkNormal(double mean, double variance, int rep)
{
    if (rep != prepRep || variance != prepVar || mean != prepMean)
    {
        prepRep = rep;
        prepMean = mean;
        prepVar = variance;
        norMul = variance * sqrt(12.0 / rep);
        norAdd = mean - rep * 0.5 * norMul;
    }
}

double RandomJames::normalNumber(double mean, double variance, int rep)
{
    checkNormal(mean, variance, rep);
    int i;
    double sum = 0.0;
    for (i = 0; i++ < rep;)
    {
        sum += uniformNumber();
    }
    return (sum * norMul + norAdd);
}

void RandomJames::normalNumbers(double mean, double variance, int rep, double* vec, int len)
{
    if (!vec)
    {
        return;
    }
    while (len--)
    {
        *vec++ = normalNumber(mean, variance, rep);
    }
}

// Derives the two seeds from the system clock: one weights seconds most and
// year-day least, the other in reverse, so the pair varies widely per call.
void RandomJames::randomize()
{
    struct tm* tm_now;
    double s_sig, s_insig, maxs_sig, maxs_insig;
    time_t secs_now;
    int s, m, h, d, s1, s2;

    time(&secs_now);
    tm_now = localtime(&secs_now);

    s = tm_now->tm_sec + 1;
    m = tm_now->tm_min + 1;
    h = tm_now->tm_hour + 1;
    d = tm_now->tm_yday + 1;

    maxs_sig = 60.0 + 60.0 / 60.0 + 24.0 / 60.0 / 60.0 + 366.0 / 24.0 / 60.0 / 60.0;
    maxs_insig = 60.0 + 60.0 * 60.0 + 24.0 * 60.0 * 60.0 + 366.0 * 24.0 * 60.0 * 60.0;

    s_sig = s + m / 60.0 + h / 60.0 / 60.0 + d / 24.0 / 60.0 / 60.0;
    s_insig = s + m * 60.0 + h * 60.0 * 60.0 + d * 24.0 * 60.0 * 60.0;

    s1 = (int)(s_sig / maxs_sig * 31328.0);
    s2 = (int)(s_insig / maxs_insig * 30081.0);

    reset(s1, s2);
}
