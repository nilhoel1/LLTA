/*
 * Example: Optional Gurobi Integration
 *
 * This demonstrates how to write code that works with or without Gurobi.
 * Compile with -DENABLE_GUROBI to enable Gurobi support.
 */

#include <stdio.h>
#include <stdlib.h>

#ifdef ENABLE_GUROBI
#include "gurobi_c.h"
#endif

/* Solve a simple optimization problem */
int solve_optimization_problem(double *result) {
#ifdef ENABLE_GUROBI
    GRBenv *env = NULL;
    GRBmodel *model = NULL;
    int error = 0;
    double objval;

    /* Create environment */
    error = GRBloadenv(&env, NULL);
    if (error) {
        fprintf(stderr, "Error: Could not create Gurobi environment\n");
        return -1;
    }

    /* Create an empty model */
    error = GRBnewmodel(env, &model, "example", 0, NULL, NULL, NULL, NULL, NULL);
    if (error) {
        fprintf(stderr, "Error: Could not create model\n");
        GRBfreeenv(env);
        return -1;
    }

    /* Add a variable: 0 <= x <= 10 */
    error = GRBaddvar(model, 0, NULL, NULL, 1.0, 0.0, 10.0, GRB_CONTINUOUS, "x");
    if (error) goto cleanup;

    /* Set objective: minimize -x (i.e., maximize x) */
    error = GRBsetintattr(model, GRB_INT_ATTR_MODELSENSE, GRB_MINIMIZE);
    if (error) goto cleanup;

    /* Optimize model */
    error = GRBoptimize(model);
    if (error) goto cleanup;

    /* Get the objective value */
    error = GRBgetdblattr(model, GRB_DBL_ATTR_OBJVAL, &objval);
    if (error) goto cleanup;

    *result = objval;
    printf("Gurobi solver: Optimal objective = %.2f\n", objval);

cleanup:
    GRBfreemodel(model);
    GRBfreeenv(env);
    return error;

#else
    /* Fallback: Simple heuristic solution */
    printf("Gurobi not available - using fallback heuristic\n");
    *result = 0.0;  /* Simple fallback value */
    return 0;
#endif
}

/* Check if Gurobi is available at runtime */
int has_gurobi_support(void) {
#ifdef ENABLE_GUROBI
    return 1;
#else
    return 0;
#endif
}

int main(void) {
    double result = 0.0;

    printf("=== Gurobi Optional Example ===\n");

    if (has_gurobi_support()) {
        printf("✓ Built with Gurobi support\n");
    } else {
        printf("✗ Built without Gurobi support\n");
    }

    int status = solve_optimization_problem(&result);

    if (status == 0) {
        printf("Result: %.2f\n", result);
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "Optimization failed\n");
        return EXIT_FAILURE;
    }
}
