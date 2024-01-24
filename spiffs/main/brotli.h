#ifndef BROTLI_H_
#define BROTLI_H_

#define chmod(F, P) (0)
#define chown(F, O, G) (0)

#define DEFAULT_LGWIN 24
#define DEFAULT_SUFFIX ".br"
#define MAX_OPTIONS 24
#define MAX_COMMENT_LEN 80

typedef enum {
	COMMENT_INIT,
	COMMENT_READ,
	COMMENT_OK,
	COMMENT_BAD,
} CommentState;

typedef struct {
	/* Parameters */
	int quality;
	int lgwin;
	int verbosity;
	BROTLI_BOOL force_overwrite;
	BROTLI_BOOL junk_source;
	BROTLI_BOOL reject_uncompressible;
	BROTLI_BOOL copy_stat;
	BROTLI_BOOL write_to_stdout;
	BROTLI_BOOL test_integrity;
	BROTLI_BOOL decompress;
	BROTLI_BOOL large_window;
	BROTLI_BOOL allow_concatenated;
	const char* output_path;
	const char* dictionary_path;
	const char* suffix;
	uint8_t comment[MAX_COMMENT_LEN];
	size_t comment_len;
	size_t comment_pos;
	CommentState comment_state;
	int not_input_indices[MAX_OPTIONS];
	size_t longest_path_len;
	size_t input_count;

	/* Inner state */
	int argc;
	char** argv;
	uint8_t* dictionary;
	size_t dictionary_size;
	BrotliEncoderPreparedDictionary* prepared_dictionary;
	BrotliDecoderState* decoder;
	char* modified_path;	/* Storage for path with appended / cut suffix */
	int iterator;
	int ignore;
	BROTLI_BOOL iterator_error;
	uint8_t* buffer;
	uint8_t* input;
	uint8_t* output;
	const char* current_input_path;
	const char* current_output_path;
	int64_t input_file_length;	/* -1, if impossible to calculate */
	FILE* fin;
	FILE* fout;

	/* I/O buffers */
	size_t available_in;
	const uint8_t* next_in;
	size_t available_out;
	uint8_t* next_out;

	/* Reporting */
	/* size_t would be large enough,
	 until 4GiB+ files are compressed / decompressed on 32-bit CPUs. */
	size_t total_in;
	size_t total_out;
	clock_t start_time;
	clock_t end_time;
} Context;


#endif /* BROTLI_H_ */
