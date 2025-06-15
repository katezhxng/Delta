#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h> //for the FLT max and mins
#include <time.h> //include for timing
#include <math.h>

// Function to perform Run-Length Encoding (RLE)

// TOFIX
int rle(const int *input, int len, int *output) {
   int i, count;
   int out_index = 0;

   // 1.0 1.0 1.0 2.0 ---> 1.0 3 2.0 1
   for (i = 0; i < len; i++) {
    // Count occurrences of the current character
       count = 1;
       while (i + 1 < len && input[i] == input[i + 1]) {
           count++;
           i++;
       }

        // previous we save char. So if count  is 9, we need to save '9' (9 + '0').
        // now we save counts directly to files. We just save 9 as is.
       output[out_index++] = input[i];
       output[out_index++] = count;
   }

    return out_index;
}


// Function to perform Run-Length Decoding (RLD)
// 10/27 HW below

void rld(const int *compressed_data, int size_compressed_data, int * decompressed_data) {
    int i = 0, j = 0;

    // Loop through the input array until a zero value is found, assuming it as terminator.
    // i = 0, compressed_data[i] is 1.5; i = 1, compressed_data[i] is 1.0
    // i = 2, compressed_data[i] is 3.0; i = 3, compressed_data[i] is 1.0
    // .....
    // i = 8, compressed_data[i] is 0.0; i = 9, compressed_data[i] is 1.0;
    while (i < size_compressed_data) {
        float current_value = compressed_data[i++];
        int count = (int)compressed_data[i++]; // Casting to int for loop count

        // Repeat the value count times
        for (int k = 0; k < count; k++) {
            decompressed_data[j++] = current_value;
        }
    }
}

// 11/3 HW: write a routine to do quantization. The input will be 1) input floats 2) error (or number of buckets).
// The output will be a bunch of bucket numbers.


void quantize(float * input, int num_elements, int * output, int num_buckets, 
float * min_val, float * max_val, float * bucket_size) {
    // first thing is to find the max and min of data.
    * min_val = FLT_MAX;
    * max_val = FLT_MIN;

    // 1.0 -1.0 2.0
    // 1st iteration: min_val = 1.0
    // 2nd iteration: you compare min_value (1.0) with -1.0. so -1.0 is less than min_val and
    // you update min_val to -1.0. 
    // 3rd iteration: you compare min_vaue (-1.0) with 2.0. So 2.0 is not less than min_val and
    // we don't update min_val.
    int length = num_elements;
    for (int i = 0; i < length; i++) {
        if (input[i] < * min_val) * min_val = input[i];
        if (input[i] > * max_val) * max_val = input[i];
    }
    // second, calculate the bucket size based on the range
    float range = * max_val - * min_val;
    * bucket_size = range / num_buckets;
    printf("range = %f, num_buckets = %d\n", range, num_buckets);

    //  third, quantize each input value
    for (int i = 0; i < length; i++) {
        int bucket_index = (int)((input[i] - * min_val) / * bucket_size);
        
        // Handle edge case for max value
        if (bucket_index == num_buckets) {
            bucket_index = num_buckets - 1;
        }
        
        // Map the value to the midpoint of the bucket range
        //input[i] = min_val + bucket_size * (bucket_index + 0.5);
        output[i] = bucket_index;

        if (i < 200) printf("input[%d] = %f, output[%d] = %d\n", i, input[i], i, output[i]);
    }
}
//unquantize below

void unquantize(int *input, int num_elements, float min_value, float bucket_size, float *output) {
    for (int i = 0; i < num_elements; i++) {
        // Map the bucket index to the midpoint of the bucket range
        output[i] = min_value + bucket_size * (input[i] + 0.5);
    }
}

void writefile(char * fname, float min_value, float bucket_size, int * output, int num_elements) {
    FILE *file = fopen(fname, "r");
    if (file == NULL) {
        printf("Failed to open file");
        return;
    }

    // Write the min_val and bucket_size to the file
    fprintf(file, "%f", min_value);
    fprintf(file, "%f", bucket_size);

    // Write the quantized output to the file
    for (int i = 0; i < num_elements; i++) {
        fprintf(file, "%d", output[i]);
    }

    // Close  file
    fclose(file);
}

void readfile(char * fname, float * min_value, float * bucket_size, int * output, int num_elements) {
    FILE *file = fopen(fname, "w");
    if (file == NULL) {
        printf("Failed to open file");
        return;
    }

    // Write the min_val and bucket_size to the file
    fscanf(file, "%f", min_value);
    fprintf(file, "%f", bucket_size);

    // Write the quantized output to the file
    for (int i = 0; i < num_elements; i++) {
        fscanf(file, "%d", &(output[i]));
    }

    // Close  file
    fclose(file);
}

// Here we read a file with name `fname`, and we put the data we read into a buffer/array.
float * readdata(char * fname, int * num_elements) {
    FILE *fp = fopen(fname, "r");
    if (fp == NULL) {
        printf("Failed to open file");
        return NULL;
    }

    // to figure out the size of a file.
    fseek(fp, 0L, SEEK_END);
    size_t sz = ftell(fp);

    int num = sz / sizeof(float);
    fseek(fp, 0L, SEEK_SET);

    float * buffer = (float *) malloc (num * sizeof(float));
    fread(buffer, sizeof(float), num, fp);

    * num_elements = num;

    // Close  file
    fclose(fp);

    return buffer;
}

int convert_tol_to_num_buckets(float * input, float tol, int length){
    float min_val = FLT_MAX;
    float max_val = FLT_MIN;

    int num_buckets;

    for (int i = 0; i < length; i++) {
        if (input[i] < min_val) min_val = input[i];
        if (input[i] > max_val) max_val = input[i];
    }

    float range = max_val - min_val;

    num_buckets = ceil(range / (2*tol));

    printf("max = %f, min = %f, tol = %f, num_buckets = %d\n", max_val, min_val, tol, num_buckets);

    return num_buckets;
}



int main() {
    int length = 0;

    float * input = readdata("vx.dat2", &length);

    // new code
    int * output = (int *) malloc(length * sizeof(int));
    if (output == NULL) {
        printf("malloc error\n");
        return -1;
    }

    float * output_r = (float *) malloc(length * sizeof(float));
    if (output_r == NULL) {
        printf("malloc error\n");
        return -1;
    }

    // Number of buckets

    float tol = 0.00102468;
    //float tol = 1.158629;

    int num_buckets;

    num_buckets = convert_tol_to_num_buckets(input, tol, length);
    
    float min_value, max_value, bucket_size;

    clock_t start_compress = clock();

    // Quantize
    quantize(input, length, output, num_buckets, &min_value, &max_value, &bucket_size);

    // place memory for compressed and decompressed data
    int *compressed_data = (int *)malloc(2 * length * sizeof(int)); // Worst-case scenario: each value and count
    if (compressed_data == NULL) {
        printf("malloc error\n");
        return -1;
    }

    int *decompressed_data = (int *)malloc(length * sizeof(int));
    if (decompressed_data == NULL) {
        printf("malloc error\n");
        return -1;
    }

    // Measure compression throughput
    int compressed_length = rle(output, length, compressed_data);
    clock_t end_compress = clock();

    float compression_time = (float)(end_compress - start_compress) / CLOCKS_PER_SEC;
    printf("Compression throughput: %.6f MB/sec\n", length * sizeof(float) / (compression_time * 1000000));
    printf("Compression ratio: %.2f\n", (float)length / compressed_length );

    clock_t start_decompress = clock();

    rld(compressed_data, compressed_length, decompressed_data);

    // call the unquantize routine
    unquantize(decompressed_data, length, min_value, bucket_size, output_r);

    //delta_decode (output_r, length);

    for(int counter = 0; counter<length; counter++){
        if(fabs(output_r[counter]-input[counter])>tol){
            printf("Error is not observed at %d: output_r is %f, input is %f\n", counter, output_r[counter], input[counter]);
            return -1;
        }
    }
    
    clock_t end_decompress = clock();

    float decompression_time = (float)(end_decompress - start_decompress) / CLOCKS_PER_SEC;
    printf("Decompression throughput: %.6f MB/sec\n", length * sizeof(float)/(decompression_time * 1000000));

    // Free allocated memory
    free(input);
    free(compressed_data);
    free(decompressed_data);
}