/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{   
    int a, b, c, d, e, f, g, h;
    int i, j, s, k;
    if (M == 32) { // 32 x 32
        for (i = 0; i < N; i += 8) {
            for (j = 0; j < M; j += 8) {
                // 1.Copy
                for (s = i, k = j; s < i + 8; s++, k++) {
                    a = A[s][j];
                    b = A[s][j+1];
                    c = A[s][j+2];
                    d = A[s][j+3];
                    e = A[s][j+4];
                    f = A[s][j+5];
                    g = A[s][j+6];
                    h = A[s][j+7];
                    B[k][i] = a;
                    B[k][i+1] = b;
                    B[k][i+2] = c;
                    B[k][i+3] = d;
                    B[k][i+4] = e;
                    B[k][i+5] = f;
                    B[k][i+6] = g;
                    B[k][i+7] = h;
                }
                // 2.transpose
                for (s = 0; s < 8; s++) {
                    for (k = s + 1; k < 8; k++) {
                        a = B[j+s][i+k];
                        B[j+s][i+k] = B[j+k][i+s];
                        B[j+k][i+s] = a;
                    }
                }
            }
        }
    } else if (M == 64) { // 64 x 64
        for (i = 0; i < N; i += 8) {
            for (j = 0; j < M; j += 8) {
                for (k = 0; k < 4; k++) { //First 4 rows of A
                    // A top right
                    a = A[i+k][j];
                    b = A[i+k][j+1];
                    c = A[i+k][j+2];
                    d = A[i+k][j+3];
                    // A top left
                    e = A[i+k][j+4];
                    f = A[i+k][j+5];
                    g = A[i+k][j+6];
                    h = A[i+k][j+7];
                    // B top left
                    B[j][i+k] = a;
                    B[j+1][i+k] = b;
                    B[j+2][i+k] = c;
                    B[j+3][i+k] = d;
                    // B top right
                    B[j][i+k+4] = e;
                    B[j+1][i+k+4] = f;
                    B[j+2][i+k+4] = g;
                    B[j+3][i+k+4] = h;
                }
                for (k = 0; k < 4; k++) { //Last 4 rows of A
                    //copy 2 columns of A;
                    a = A[i+4][j+k], e = A[i+4][j+k+4];
                    b = A[i+5][j+k], f = A[i+5][j+k+4];
                    c = A[i+6][j+k], g = A[i+6][j+k+4];
                    d = A[i+7][j+k], h = A[i+7][j+k+4];
                    // swap "B top right" with first column
                    s = B[j+k][i+4], B[j+k][i+4] = a, a = s;
                    s = B[j+k][i+5], B[j+k][i+5] = b, b = s;
                    s = B[j+k][i+6], B[j+k][i+6] = c, c = s;
                    s = B[j+k][i+7], B[j+k][i+7] = d, d = s;
                    // Add "2 columns of A" to Bottom of B;
                    B[j+k+4][i] = a, B[j+k+4][i+4] = e;
                    B[j+k+4][i+1] = b, B[j+k+4][i+1+4] = f;
                    B[j+k+4][i+2] = c, B[j+k+4][i+2+4] = g;
                    B[j+k+4][i+3] = d, B[j+k+4][i+3+4] = h;
                }
                
            }
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

