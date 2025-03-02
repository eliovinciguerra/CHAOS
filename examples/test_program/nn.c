#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#define INPUT_SIZE 17
#define HIDDEN1_SIZE 256
#define HIDDEN2_SIZE 128
#define OUTPUT_SIZE 2

// Activation functions
double relu(double x) {
    return x > 0 ? x : 0;
}

double tanh_activation(double x) {
    return tanhf(x);
}

// Dot product of matrices
void dot(double* a, double* b, double* c, int m, int n, int p) {
    for(int i = 0; i < m; i++) {
        for(int j = 0; j < p; j++) {
            c[i * p + j] = 0;
            for(int k = 0; k < n; k++) {
                c[i * p + j] += a[i * n + k] * b[k * p + j];
            }
        }
    }
}

// Adding bias
void add_bias(double* matrix, double* bias, int rows, int cols) {
    for(int i = 0; i < rows; i++) {
        for(int j = 0; j < cols; j++) {
            matrix[i * cols + j] += bias[j];
        }
    }
}

// Apply activation function
void apply_activation(double* matrix, int size, double (*activation)(double)) {
    for(int i = 0; i < size; i++) {
        matrix[i] = activation(matrix[i]);
    }
}

// Define MLP structure
typedef struct {
    double *input_weights;
    double *input_bias;
    double *hidden1_weights;
    double *hidden1_bias;
    double *hidden2_weights;
    double *hidden2_bias;
    double *output_weights;
    double *output_bias;
} MLP;

void forward(MLP *mlp, double *input, double *output) {
    double input_layer_output[HIDDEN1_SIZE];
    double hidden1_layer_output[HIDDEN1_SIZE];
    double hidden2_layer_output[HIDDEN2_SIZE];

    // Input to first hidden layer
    dot(input, mlp->input_weights, input_layer_output, 1, INPUT_SIZE, HIDDEN1_SIZE);
    add_bias(input_layer_output, mlp->input_bias, 1, HIDDEN1_SIZE);
    apply_activation(input_layer_output, HIDDEN1_SIZE, relu);

    // First hidden layer to second hidden layer
    dot(input_layer_output, mlp->hidden1_weights, hidden1_layer_output, 1, HIDDEN1_SIZE, HIDDEN1_SIZE);
    add_bias(hidden1_layer_output, mlp->hidden1_bias, 1, HIDDEN1_SIZE);
    apply_activation(hidden1_layer_output, HIDDEN1_SIZE, relu);

    // second hidden layer to third hidden layer
    dot(hidden1_layer_output, mlp->hidden2_weights, hidden2_layer_output, 1, HIDDEN1_SIZE, HIDDEN2_SIZE);
    add_bias(hidden2_layer_output, mlp->hidden2_bias, 1, HIDDEN2_SIZE);
    apply_activation(hidden2_layer_output, HIDDEN2_SIZE, relu);

    // Third hidden layer to output
    dot(hidden2_layer_output, mlp->output_weights, output, 1, HIDDEN2_SIZE, OUTPUT_SIZE);
    add_bias(output, mlp->output_bias, 1, OUTPUT_SIZE);
}

int run_mlp() {
    // Allocate memory for weights and biases
    MLP mlp;
    mlp.input_weights = (double*)malloc(INPUT_SIZE * HIDDEN1_SIZE * sizeof(double));
    mlp.input_bias = (double*)malloc(HIDDEN1_SIZE * sizeof(double));

    mlp.hidden1_weights = (double*)malloc(HIDDEN1_SIZE * HIDDEN1_SIZE * sizeof(double));
    mlp.hidden1_bias = (double*)malloc(HIDDEN1_SIZE * sizeof(double));
    
    mlp.hidden2_weights = (double*)malloc(HIDDEN1_SIZE * HIDDEN2_SIZE * sizeof(double));
    mlp.hidden2_bias = (double*)malloc(HIDDEN2_SIZE * sizeof(double));

    mlp.output_weights = (double*)malloc(HIDDEN2_SIZE * OUTPUT_SIZE * sizeof(double));
    mlp.output_bias = (double*)malloc(OUTPUT_SIZE * sizeof(double));

    // Dummy input
    double input[INPUT_SIZE];
    for (int i = 0; i < INPUT_SIZE; i++) {
        input[i] = (double)rand() / RAND_MAX;
    }

    // Output array
    double output[OUTPUT_SIZE];

    // Perform forward pass
    forward(&mlp, input, output);

    // Print the output
    int res = 0;
    for(int i = 0; i < OUTPUT_SIZE; i++) {
        res = res + (int)(output[i]*10000);
    }

    // Free allocated memory
    free(mlp.input_weights);
    free(mlp.input_bias);
    free(mlp.hidden1_weights);
    free(mlp.hidden1_bias);
    free(mlp.hidden2_weights);
    free(mlp.hidden2_bias);
    free(mlp.output_weights);
    free(mlp.output_bias);

    return res;
}

int main(void){
    run_mlp();
}