#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE 0x1000
#define ROUND_UP(x, y) ((x) % (y) == 0 ? (x) : (x) + ((y) - (x) % (y)))

/// The number of times we've seen each letter in the input, initially zero
size_t letter_counts[26] = {0};

// storing characteristics of inputs

typedef struct input {
  char* data_file;
  off_t size_file;
} input_t;

// setting up lock variable
pthread_mutex_t lock;

// function that will run in threads
void* counter(void* var) {
  input_t inputs = *(input_t*)var;

  for (size_t i = 0; i < inputs.size_file; i++) {
    char c = inputs.data_file[i];
    if (c >= 'a' && c <= 'z') {
      pthread_mutex_lock(&lock);  // locking counts

      letter_counts[c - 'a']++;

      pthread_mutex_unlock(&lock);  // unlocking counts

    } else if (c >= 'A' && c <= 'Z') {
      pthread_mutex_lock(&lock);  // locking counts

      letter_counts[c - 'A']++;

      pthread_mutex_unlock(&lock);  // unlocking counts
    }
  }

  return NULL;
}

/**
 * This function should divide up the file_data between the specified number of
 * threads, then have each thread count the number of occurrences of each letter
 * in the input data. Counts should be written to the letter_counts array. Make
 * sure you count only the 26 different letters, treating upper- and lower-case
 * letters as the same. Skip all other characters.
 *
 * \param num_threads   The number of threads your program must use to count
 *                      letters. Divide work evenly between threads
 * \param file_data     A pointer to the beginning of the file data array
 * \param file_size     The number of bytes in the file_data array
 */
void count_letters(int num_threads, char* file_data, off_t file_size) {
  pthread_mutex_init(&lock, NULL);  // initialize lock
  // dividing file into num_threads chunks and storing chunk size
  off_t chunk = (off_t)floor(file_size / num_threads);

  int num_threads_starting = num_threads;  // storing inputted number of threads

  off_t file_size_remaining = file_size % num_threads;  // storing remaining size

  // tracking if file has not been completely divided and if remainder exists

  if (file_size_remaining > 0) {
    num_threads--;  // decrement num_thread if there is a remainder
  }

  // array to store threads
  pthread_t id[num_threads_starting];
  // array to store file characteristics
  input_t characteristics[num_threads_starting];

  int i;  // variable to help traverse arrays above
  for (i = 0; i < num_threads; i++) {
    characteristics[i].data_file = &file_data[chunk * i];                 // storing file data
    characteristics[i].size_file = chunk;                                 // storing size
    int rc = pthread_create(&id[i], NULL, counter, &characteristics[i]);  // creating thread

    if (rc) {
      perror("pthread create failed");
      exit(2);
    }
  }

  // case: remainder exists
  if (file_size_remaining > 0) {
    characteristics[i].data_file = &file_data[chunk * i];  // storing remaining data

    // storing size of remaining chunk and remainder
    characteristics[i].size_file = chunk + file_size_remaining;
    int rc = pthread_create(&id[i], NULL, counter, &characteristics[i]);  // creating thread
    if (rc) {
      perror("pthread create failed");
      exit(2);
    }
  }

  for (int j = 0; j < num_threads_starting; j++) {
    int rc = pthread_join(id[j], NULL);  // joining threads
    if (rc) {
      perror("pthread create failed");
      exit(2);
    }
  }
}

/**
 * Show instructions on how to run the program.
 * \param program_name  The name of the command to print in the usage info
 */
void show_usage(char* program_name) {
  fprintf(stderr, "Usage: %s <N> <input file>\n", program_name);
  fprintf(stderr, "    where <N> is the number of threads (1, 2, 4, or 8)\n");
  fprintf(stderr, "    and <input file> is a path to an input text file.\n");
}

int main(int argc, char** argv) {
  // Check parameter count
  if (argc != 3) {
    show_usage(argv[0]);
    exit(1);
  }

  // Read thread count
  int num_threads = atoi(argv[1]);
  if (num_threads != 1 && num_threads != 2 && num_threads != 4 && num_threads != 8) {
    fprintf(stderr, "Invalid number of threads: %s\n", argv[1]);
    show_usage(argv[0]);
    exit(1);
  }

  // Open the input file
  int fd = open(argv[2], O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "Unable to open input file: %s\n", argv[2]);
    show_usage(argv[0]);
    exit(1);
  }

  // Get the file size
  off_t file_size = lseek(fd, 0, SEEK_END);
  if (file_size == -1) {
    fprintf(stderr, "Unable to seek to end of file\n");
    exit(2);
  }

  // Seek back to the start of the file
  if (lseek(fd, 0, SEEK_SET)) {
    fprintf(stderr, "Unable to seek to the beginning of the file\n");
    exit(2);
  }

  // Load the file with mmap
  char* file_data = mmap(NULL, ROUND_UP(file_size, PAGE_SIZE), PROT_READ, MAP_PRIVATE, fd, 0);
  if (file_data == MAP_FAILED) {
    fprintf(stderr, "Failed to map file\n");
    exit(2);
  }

  // Call the function to count letter frequencies
  count_letters(num_threads, file_data, file_size);

  // Print the letter counts
  for (int i = 0; i < 26; i++) {
    printf("%c: %lu\n", 'a' + i, letter_counts[i]);
  }

  return 0;
}
