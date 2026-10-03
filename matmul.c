// Matrix multiplication - loop optimizations
// C = A * B, square matrices of size n x n, stored row major

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int n = 1024;
int bs = 64;    // block size for tiling

double get_time()
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

// basic version, i j k order
void matmul_naive(double *A, double *B, double *C)
{
    int i, j, k;
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            double sum = 0;
            for (k = 0; k < n; k++)
                sum += A[i*n + k] * B[k*n + j];   // B is accessed column wise here
            C[i*n + j] = sum;
        }
    }
}

// loop interchange - i k j order, now B and C are accessed row wise
void matmul_interchange(double *A, double *B, double *C)
{
    int i, j, k;
    memset(C, 0, n * n * sizeof(double));
    for (i = 0; i < n; i++) {
        for (k = 0; k < n; k++) {
            double a = A[i*n + k];
            for (j = 0; j < n; j++)
                C[i*n + j] += a * B[k*n + j];
        }
    }
}

// loop tiling - work on bs x bs blocks so they stay in cache
void matmul_tiling(double *A, double *B, double *C)
{
    int i, j, k, ii, jj, kk;
    memset(C, 0, n * n * sizeof(double));
    for (ii = 0; ii < n; ii += bs) {
        for (kk = 0; kk < n; kk += bs) {
            for (jj = 0; jj < n; jj += bs) {
                int iend = ii + bs < n ? ii + bs : n;
                int kend = kk + bs < n ? kk + bs : n;
                int jend = jj + bs < n ? jj + bs : n;
                for (i = ii; i < iend; i++) {
                    for (k = kk; k < kend; k++) {
                        double a = A[i*n + k];
                        for (j = jj; j < jend; j++)
                            C[i*n + j] += a * B[k*n + j];
                    }
                }
            }
        }
    }
}

// loop unrolling - unroll the j loop by 4
void matmul_unroll(double *A, double *B, double *C)
{
    int i, j, k;
    memset(C, 0, n * n * sizeof(double));
    for (i = 0; i < n; i++) {
        for (k = 0; k < n; k++) {
            double a = A[i*n + k];
            for (j = 0; j + 3 < n; j += 4) {
                C[i*n + j]     += a * B[k*n + j];
                C[i*n + j + 1] += a * B[k*n + j + 1];
                C[i*n + j + 2] += a * B[k*n + j + 2];
                C[i*n + j + 3] += a * B[k*n + j + 3];
            }
            // leftover when n is not a multiple of 4
            for (; j < n; j++)
                C[i*n + j] += a * B[k*n + j];
        }
    }
}

// all three together: tiling + ikj order + unroll k by 4
// unrolling k means each C[i][j] is loaded/stored once for 4 multiplies
void matmul_all(double *A, double *B, double *C)
{
    int i, j, k, ii, jj, kk;
    memset(C, 0, n * n * sizeof(double));
    for (ii = 0; ii < n; ii += bs) {
        for (kk = 0; kk < n; kk += bs) {
            for (jj = 0; jj < n; jj += bs) {
                int iend = ii + bs < n ? ii + bs : n;
                int kend = kk + bs < n ? kk + bs : n;
                int jend = jj + bs < n ? jj + bs : n;
                for (i = ii; i < iend; i++) {
                    for (k = kk; k + 3 < kend; k += 4) {
                        double a0 = A[i*n + k];
                        double a1 = A[i*n + k + 1];
                        double a2 = A[i*n + k + 2];
                        double a3 = A[i*n + k + 3];
                        for (j = jj; j < jend; j++) {
                            C[i*n + j] += a0 * B[k*n + j] + a1 * B[(k+1)*n + j]
                                        + a2 * B[(k+2)*n + j] + a3 * B[(k+3)*n + j];
                        }
                    }
                    for (; k < kend; k++) {
                        double a = A[i*n + k];
                        for (j = jj; j < jend; j++)
                            C[i*n + j] += a * B[k*n + j];
                    }
                }
            }
        }
    }
}

// run a version a few times and keep the best time
double run(void (*f)(double *, double *, double *), double *A, double *B, double *C, int runs)
{
    double best = 0;
    for (int r = 0; r < runs; r++) {
        double t1 = get_time();
        f(A, B, C);
        double t2 = get_time();
        if (r == 0 || t2 - t1 < best)
            best = t2 - t1;
    }
    return best;
}

int main(int argc, char *argv[])
{
    int runs = 3;
    if (argc > 1) n = atoi(argv[1]);
    if (argc > 2) bs = atoi(argv[2]);
    if (argc > 3) runs = atoi(argv[3]);
    if (n <= 0 || bs <= 0 || runs <= 0) {
        printf("usage: %s [n] [block size] [runs]\n", argv[0]);
        return 1;
    }

    double *A = malloc(n * n * sizeof(double));
    double *B = malloc(n * n * sizeof(double));
    double *C = malloc(n * n * sizeof(double));
    if (!A || !B || !C) {
        printf("malloc failed\n");
        return 1;
    }

    srand(1);
    for (int i = 0; i < n * n; i++) {
        A[i] = rand() % 100 / 10.0;
        B[i] = rand() % 100 / 10.0;
    }

    printf("n = %d, block size = %d, runs = %d\n\n", n, bs, runs);
    printf("%-20s %10s %10s\n", "version", "time(s)", "speedup");

    double t0 = run(matmul_naive, A, B, C, runs);
    printf("%-20s %10.4f %10.2f\n", "naive", t0, 1.0);

    double t;
    t = run(matmul_interchange, A, B, C, runs);
    printf("%-20s %10.4f %10.2f\n", "interchange", t, t0 / t);

    t = run(matmul_tiling, A, B, C, runs);
    printf("%-20s %10.4f %10.2f\n", "tiling", t, t0 / t);

    t = run(matmul_unroll, A, B, C, runs);
    printf("%-20s %10.4f %10.2f\n", "unrolling", t, t0 / t);

    t = run(matmul_all, A, B, C, runs);
    printf("%-20s %10.4f %10.2f\n", "all combined", t, t0 / t);

    free(A);
    free(B);
    free(C);
    return 0;
}
