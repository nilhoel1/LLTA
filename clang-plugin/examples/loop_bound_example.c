// Example C code showing how to use the loop_bound pragma

#include <stdint.h>

#define ARRAY_SIZE 30
#define INNER_SIZE 40

int16_t array[ARRAY_SIZE][INNER_SIZE];

void Initialize(int16_t ArrayA[ARRAY_SIZE][INNER_SIZE]) {
    #pragma loop_bound(1, 30)
    for (int OuterIndex = 0; OuterIndex < ARRAY_SIZE; OuterIndex++) {
        #pragma loop_bound(1, 40)
        for (int InnerIndex = 0; InnerIndex < INNER_SIZE; InnerIndex++) {
            ArrayA[OuterIndex][InnerIndex] = (int16_t)OuterIndex;
        }
    }
}

int16_t Sum(int16_t ArrayA[ARRAY_SIZE][INNER_SIZE]) {
    int16_t total = 0;

    #pragma loop_bound(1, 30)
    for (int OuterIndex = 0; OuterIndex < ARRAY_SIZE; OuterIndex++) {
        #pragma loop_bound(1, 40)
        for (int InnerIndex = 0; InnerIndex < INNER_SIZE; InnerIndex++) {
            if (ArrayA[OuterIndex][InnerIndex] > 10) {
                total += ArrayA[OuterIndex][InnerIndex];
            } else {
                total -= ArrayA[OuterIndex][InnerIndex];
            }
        }
    }

    return total;
}int main(void) {
    Initialize(array);
    int16_t result = Sum(array);
    return result;
}
