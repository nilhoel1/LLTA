/*
 * Example: Optional HiGHS Integration
 *
 * This demonstrates how to write code that works with or without HiGHS.
 * Compile with -DENABLE_HIGHS to enable HiGHS support.
 */

#include <stdio.h>
#include <stdlib.h>

#ifdef ENABLE_HIGHS
#include "interfaces/highs_c_api.h"
#endif

/* Solve a simple linear programming problem */
int solve_lp_problem(double *result) {
#ifdef ENABLE_HIGHS
    /*
     * Solve: maximize x subject to 0 <= x <= 10
     * This is a trivial LP for demonstration
     */

    void *highs = Highs_create();

    if (!highs) {
        fprintf(stderr, "Error: Could not create HiGHS instance\n");
        return -1;
    }

    /* Set up the problem:
     * Maximize: x
     * Subject to: 0 <= x <= 10
     */
    int num_cols = 1;
    int num_rows = 0;

    double col_cost = -1.0;  /* Negative for maximization */
    double col_lower = 0.0;
    double col_upper = 10.0;

    int status = Highs_addCol(highs, col_cost, col_lower, col_upper,
                              0, NULL, NULL);

    if (status != 0) {
        fprintf(stderr, "Error: Could not add column\n");
        Highs_destroy(highs);
        return -1;
    }

    /* Run the optimizer */
    status = Highs_run(highs);

    if (status != 0) {
        fprintf(stderr, "Error: Optimization failed\n");
        Highs_destroy(highs);
        return -1;
    }

    /* Get the solution */
    double solution[1];
    Highs_getSolution(highs, solution, NULL, NULL, NULL);

    *result = solution[0];
    printf("HiGHS solver: Optimal solution x = %.2f\n", *result);

    Highs_destroy(highs);
    return 0;

#else
    /* Fallback: Simple heuristic solution */
    printf("HiGHS not available - using fallback heuristic\n");
    *result = 0.0;  /* Simple fallback value */
    return 0;
#endif
}

/* Check if HiGHS is available at runtime */
int has_highs_support(void) {
#ifdef ENABLE_HIGHS
    return 1;
#else
    return 0;
#endif
}

int main(void) {
    double result = 0.0;

    printf("=== HiGHS Optional Example ===\n");

    if (has_highs_support()) {
        printf("✓ Built with HiGHS support\n");
    } else {
        printf("✗ Built without HiGHS support\n");
    }

    int status = solve_lp_problem(&result);

    if (status == 0) {
        printf("Result: %.2f\n", result);
        return EXIT_SUCCESS;
    }

    fprintf(stderr, "Optimization failed\n");
    return EXIT_FAILURE;
}
