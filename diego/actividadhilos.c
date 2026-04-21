#include <stdio.h>
#include <omp.h>
static long num_pasos=1.9e9;
double paso;
#define NUM_THREADS 100
void main(){
int i, nthreads;
double pi, sum[NUM_THREADS];
paso=1.0/(double) num_pasos;
omp_set_num_threads(NUM_THREADS);
const double ST = omp_get_wtime();
#pragma omp parallel
{
int i, id, nthrds;
double x;
id = omp_get_thread_num();
nthrds = omp_get_num_threads();
if (id==0) nthreads=nthrds;
for (i=id, sum[id]=0.0;i<num_pasos;i=i+nthrds)
{
x=(i+0.5)*paso;
sum[id]+=4.0/(1.0+x*x);
}
}
for (i=0,pi=0.0;i<nthreads;i++)
{
pi+=sum[i]*paso;
}
printf("pi = (%lf)\n",pi);
const double STOP = omp_get_wtime();
printf("Tiempo = %lf \n", (STOP - ST));
}

// 10 hilos 2.187214
// 20 hilos 1.967170
// 30 hilos 1.850708
// 40 hilos 1.758026
// 50 hilos 1.647009
// 60 hilos 1.675763 
// 70 hilos 1.632386
// 80 hilos 1.644252
// 90 hilos 1.611866
// 100 hilos 1.543033