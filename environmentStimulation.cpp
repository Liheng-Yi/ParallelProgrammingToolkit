#include <cmath>
#include <omp.h>
#include <stdlib.h>
#include <iostream>
#include <stdio.h>

using namespace std;

int NowYear = 2024;
int NowMonth = 1;

int NowNumDeer = 2;
int NowHeight = 5.;

float NowPrecip = 1.1;    
float NowTemp = 50;

const float GRAIN_GROWS_PER_MONTH =      30.0;
const float ONE_DEER_EATS_PER_MONTH  =   1;

const float AVG_PRECIP_PER_MONTH =       7.0;    // average
const float AMP_PRECIP_PER_MONTH =       6.0;    // plus or minus
const float RANDOM_PRECIP =              2.0;    // plus or minus noise

const float AVG_TEMP =                  60.0;   // average
const float AMP_TEMP =                  20.0;   // plus or minus
const float RANDOM_TEMP =               10.0;   // plus or minus noise

const float MIDTEMP =                   49.0;
const float MIDPRECIP =                 10.0;

// my own agent
int NowNumAliens = 2;
const int min_army = 2;
const int max_army = 5;

omp_lock_t Lock;
int NumInThreadTeam;
int NumAtBarrier;
int NumGone;
unsigned int seed;

void InitBarrier(int n) {
    NumInThreadTeam = n;
    NumAtBarrier = 0;
    omp_init_lock(&Lock);
}

float Sqr(float x) {
    return x*x;
}

void WaitBarrier() {
    omp_set_lock(&Lock);
    {
        NumAtBarrier++;
        if(NumAtBarrier == NumInThreadTeam) {
            NumGone = 0;
            NumAtBarrier = 0;
            while(NumGone != NumInThreadTeam-1);
            omp_unset_lock(&Lock);
            return;
        }
    }
    omp_unset_lock(&Lock);

    while(NumAtBarrier != 0);

    #pragma omp atomic
    NumGone++;
}

float Ranf(unsigned int *seedp, float low, float high) {
    float r = (float) rand_r(seedp);
    return(low  +  r * (high - low) / (float)RAND_MAX);
}

void Deer() {
    while(NowYear < 2030) {
        int carryingCapacity = (int)(NowHeight);
        int nextNumDeer = NowNumDeer;
        int NextNumAliens = NowNumAliens;

        if(nextNumDeer < carryingCapacity) {
            nextNumDeer += 2;
        } else if(nextNumDeer > carryingCapacity) {
            nextNumDeer -= 1;
        }
        // aligns drop deer
        nextNumDeer += NextNumAliens;

        if (nextNumDeer > 30) {
            nextNumDeer -= NextNumAliens * 5;
        }

        if(nextNumDeer < 0) {
            nextNumDeer = 0;
        }

        if (NowHeight == 0) {
            nextNumDeer /= 2;
        }
        WaitBarrier();
        NowNumDeer = nextNumDeer;
        WaitBarrier();
        WaitBarrier();
    }
}

void Grain() {
    while(NowYear < 2030) {
        float tempFactor = exp(-Sqr((NowTemp - MIDTEMP) / 10.));
        float precipFactor = exp(-Sqr((NowPrecip - MIDPRECIP) / 10.));
        int numAliens = NowNumAliens;
        float nextHeight = NowHeight;    
        nextHeight += tempFactor * precipFactor * GRAIN_GROWS_PER_MONTH;
        nextHeight -= (float)NowNumDeer * ONE_DEER_EATS_PER_MONTH;
        nextHeight = nextHeight + numAliens*3 ;
                
        if(nextHeight < 0.) {
            nextHeight = 0.;
        }
        WaitBarrier();

        NowHeight = nextHeight;
        WaitBarrier();
        WaitBarrier();
    }
}

void Aliens() {
    while(NowYear < 2030) {
        int NextNumAliens = NowNumAliens;

        NextNumAliens = (int)(Ranf(&seed, min_army, max_army));

        WaitBarrier();

        NowNumAliens = NextNumAliens;

        WaitBarrier();

        WaitBarrier();
    }
}

void Watcher() {
    while(NowYear < 2030) {
        WaitBarrier();

        WaitBarrier();
        printf("Year:%d Month:%d Temperature:%.2f Precipitation:%.2f Number_of_Deers:%d Height_of_Grain:%d Number_of_Aliens:%d\n",
               NowYear, NowMonth, (NowTemp - 32) * 5 / 9, NowPrecip, NowNumDeer, NowHeight, NowNumAliens);


        WaitBarrier();

        NowMonth++;
        if (NowMonth > 11) {
            NowYear++;
            NowMonth = 0;
        }
        
        if (NowYear >= 2030){
            exit(0);
        } 
        float ang = (30. * (float)NowMonth + 15.) * (M_PI / 180.);
        float temp = AVG_TEMP - AMP_TEMP * cos(ang);
        NowTemp = temp + Ranf(&seed, -RANDOM_TEMP, RANDOM_TEMP);

        float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin(ang);
        NowPrecip = precip + Ranf(&seed, -RANDOM_PRECIP, RANDOM_PRECIP);
        if(NowPrecip < 0.)
            NowPrecip = 0.;
    }
}

int main(int argc, char **argv) {
    omp_set_num_threads(4);
    InitBarrier(4);

    #pragma omp parallel sections
    {
        #pragma omp section
        {
            Deer();
        }

        #pragma omp section
        {
            Grain();
        }

        #pragma omp section
        {
            Watcher();
        }
        #pragma omp section
        {
            Aliens();
        }
    }
    return 0;
}
