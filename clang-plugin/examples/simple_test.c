// Simple test for loop_bound pragma

#pragma loop_bound(5, 10)
for (int i = 0; i < 10; i++) {
    int x = i * 2;
}
