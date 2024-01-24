#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <lwip/sys.h> // open
#include <errno.h> // errno

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"

#include "brotli/decode.h"
#include "brotli/encode.h"
#include "../common/constants.h"

#include "parameter.h"
#include "brotli.h"

static const char* PrintablePath(const char* path) {
	return path ? path : "con";
}

static int CopyTimeStat(const struct stat* statbuf, const char* output_path) {
#if HAVE_UTIMENSAT
	struct timespec times[2];
	times[0].tv_sec = statbuf->st_atime;
	times[0].tv_nsec = ATIME_NSEC(statbuf);
	times[1].tv_sec = statbuf->st_mtime;
	times[1].tv_nsec = MTIME_NSEC(statbuf);
	return utimensat(AT_FDCWD, output_path, times, AT_SYMLINK_NOFOLLOW);
#else
	struct utimbuf times;
	times.actime = statbuf->st_atime;
	times.modtime = statbuf->st_mtime;
	return utime(output_path, &times);
#endif
}

/*	Copy file times and permissions.
	TODO(eustas): this is a "best effort" implementation; honest cross-platform
	fully featured implementation is way too hacky; add more hacks by request. */
static void CopyStat(const char* input_path, const char* output_path) {
	struct stat statbuf;
	int res;
	if (input_path == 0 || output_path == 0) {
		return;
	}
	if (stat(input_path, &statbuf) != 0) {
		return;
	}
	res = CopyTimeStat(&statbuf, output_path);
	res = chmod(output_path, statbuf.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
	if (res != 0) {
		fprintf(stderr, "setting access bits failed for [%s]: %s\n",
			PrintablePath(output_path), strerror(errno));
	}
	res = chown(output_path, (uid_t)-1, statbuf.st_gid);
	if (res != 0) {
		fprintf(stderr, "setting group failed for [%s]: %s\n",
			PrintablePath(output_path), strerror(errno));
	}
	res = chown(output_path, statbuf.st_uid, (gid_t)-1);
	if (res != 0) {
		fprintf(stderr, "setting user failed for [%s]: %s\n",
			PrintablePath(output_path), strerror(errno));
	}
}

static BROTLI_BOOL OpenInputFile(const char* input_path, FILE** f) {
	*f = NULL;
#if 0
	if (!input_path) {
		*f = fdopen(MAKE_BINARY(STDIN_FILENO), "rb");
		return BROTLI_TRUE;
	}
#endif
	//printf("%s input_path=[%s]\n", __FUNCTION__, input_path);
	*f = fopen(input_path, "rb");
	if (!*f) {
		fprintf(stderr, "failed to open input file [%s]: %s\n",
			PrintablePath(input_path), strerror(errno));
		return BROTLI_FALSE;
	}
	return BROTLI_TRUE;
}

static BROTLI_BOOL OpenOutputFile(const char* output_path, FILE** f,
								  BROTLI_BOOL force) {
#if 0
	int fd;
#endif
	*f = NULL;
#if 0
	if (!output_path) {
		*f = fdopen(MAKE_BINARY(STDOUT_FILENO), "wb");
		return BROTLI_TRUE;
	}
#endif
	//printf("%s output_path=[%s]\n", __FUNCTION__, output_path);
	//printf("%s force=[%d]\n", __FUNCTION__, force);
#if 0
	fd = open(output_path, O_CREAT | (force ? 0 : O_EXCL) | O_WRONLY | O_TRUNC,
			S_IRUSR | S_IWUSR);
	if (fd < 0) {
		fprintf(stderr, "failed to open output file [%s]: %s\n",
			PrintablePath(output_path), strerror(errno));
		return BROTLI_FALSE;
	}
	*f = fdopen(fd, "wb");
	if (!*f) {
		fprintf(stderr, "failed to open output file [%s]: %s\n",
			PrintablePath(output_path), strerror(errno));
		return BROTLI_FALSE;
	}
#endif
	*f = fopen(output_path, "wb");
	if (!*f) {
		fprintf(stderr, "failed to open output file [%s]: %s\n",
			PrintablePath(output_path), strerror(errno));
		return BROTLI_FALSE;
	}
	//printf("%s done\n", __FUNCTION__);
	return BROTLI_TRUE;
}

static BROTLI_BOOL OpenFiles(Context* context) {
	BROTLI_BOOL is_ok = OpenInputFile(context->current_input_path, &context->fin);
	//printf("%s context->test_integrity=%d\n", __FUNCTION__, context->test_integrity);
	if (!context->test_integrity && is_ok) {
		is_ok = OpenOutputFile(
			context->current_output_path, &context->fout, context->force_overwrite);
	}
	//printf("%s done\n", __FUNCTION__);
	return is_ok;
}

static BROTLI_BOOL CloseFiles(Context* context, BROTLI_BOOL rm_input,
							  BROTLI_BOOL rm_output) {
	BROTLI_BOOL is_ok = BROTLI_TRUE;
	if (!context->test_integrity && context->fout) {
		if (fclose(context->fout) != 0) {
			if (is_ok) {
				fprintf(stderr, "fclose failed [%s]: %s\n",
					PrintablePath(context->current_output_path), strerror(errno));
			}
			is_ok = BROTLI_FALSE;
		}
		//printf("%s fclose(context->fout)\n", __FUNCTION__);
		//printf("%s rm_output=%d\n", __FUNCTION__, rm_output);
		if (rm_output && context->current_output_path) {
			unlink(context->current_output_path);
		}

		/* TOCTOU violation, but otherwise it is impossible to set file times. */
		//printf("%s context->copy_stat=%d\n", __FUNCTION__, context->copy_stat);
		if (!rm_output && is_ok && context->copy_stat) {
			CopyStat(context->current_input_path, context->current_output_path);
		}
	}

	if (context->fin) {
		if (fclose(context->fin) != 0) {
			if (is_ok) {
				fprintf(stderr, "fclose failed [%s]: %s\n",
					PrintablePath(context->current_input_path), strerror(errno));
		  }
		  is_ok = BROTLI_FALSE;
		}
	}
	if (rm_input && context->current_input_path) {
		unlink(context->current_input_path);
	}

	context->fin = NULL;
	context->fout = NULL;

	return is_ok;
}

//static const size_t kFileBufferSize = 1 << 19;
static const size_t kFileBufferSize = 1 << 10;

static void InitializeBuffers(Context* context) {
	context->available_in = 0;
	context->next_in = NULL;
	context->available_out = kFileBufferSize;
	context->next_out = context->output;
	context->total_in = 0;
	context->total_out = 0;
	if (context->verbosity > 0) {
		context->start_time = clock();
	}
}

/*	This method might give the false-negative result.
	However, after an empty / incomplete read it should tell the truth. */
static BROTLI_BOOL HasMoreInput(Context* context) {
	return feof(context->fin) ? BROTLI_FALSE : BROTLI_TRUE;
}

static BROTLI_BOOL ProvideInput(Context* context) {
	//printf("%s start\n", __FUNCTION__);
	context->available_in =
	  fread(context->input, 1, kFileBufferSize, context->fin);
	context->total_in += context->available_in;
	context->next_in = context->input;
	//printf("%s context->available_in=%d\n", __FUNCTION__, context->available_in);
	//printf("%s context->total_in=%d\n", __FUNCTION__, context->total_in);
	//printf("%s context->next=%p\n", __FUNCTION__, context->next_in);
	if (ferror(context->fin)) {
		fprintf(stderr, "failed to read input [%s]: %s\n",
			PrintablePath(context->current_input_path), strerror(errno));
		return BROTLI_FALSE;
	}
	//printf("%s done\n", __FUNCTION__);
	return BROTLI_TRUE;
}

/* Internal: should be used only in Provide-/Flush-Output. */
static BROTLI_BOOL WriteOutput(Context* context) {
	size_t out_size = (size_t)(context->next_out - context->output);
	context->total_out += out_size;
	if (out_size == 0) return BROTLI_TRUE;
	if (context->test_integrity) return BROTLI_TRUE;

	fwrite(context->output, 1, out_size, context->fout);
	if (ferror(context->fout)) {
		fprintf(stderr, "failed to write output [%s]: %s\n",
			PrintablePath(context->current_output_path), strerror(errno));
		return BROTLI_FALSE;
	}
	return BROTLI_TRUE;
}

static BROTLI_BOOL ProvideOutput(Context* context) {
	if (!WriteOutput(context)) return BROTLI_FALSE;
	context->available_out = kFileBufferSize;
	context->next_out = context->output;
	return BROTLI_TRUE;
}

static BROTLI_BOOL FlushOutput(Context* context) {
	if (!WriteOutput(context)) return BROTLI_FALSE;
	context->available_out = 0;
	context->next_out = context->output;
	return BROTLI_TRUE;
}

static void PrintBytes(size_t value) {
	if (value < 1024) {
		fprintf(stderr, "%d B", (int)value);
	} else if (value < 1048576) {
		fprintf(stderr, "%0.3f KiB", (double)value / 1024.0);
	} else if (value < 1073741824) {
		fprintf(stderr, "%0.3f MiB", (double)value / 1048576.0);
	} else {
		fprintf(stderr, "%0.3f GiB", (double)value / 1073741824.0);
	}
}

static void PrintFileProcessingProgress(Context* context) {
	fprintf(stderr, "[%s]: ", PrintablePath(context->current_input_path));
	PrintBytes(context->total_in);
	fprintf(stderr, " -> ");
	PrintBytes(context->total_out);
	fprintf(stderr, " in %1.2f sec", (double)(context->end_time - context->start_time) / CLOCKS_PER_SEC);
}

static BROTLI_BOOL CompressFile(Context* context, BrotliEncoderState* s) {
	//printf("%s start\n", __FUNCTION__);
	BROTLI_BOOL is_eof = BROTLI_FALSE;
	BROTLI_BOOL prologue = !!context->comment_len;
	InitializeBuffers(context);
	//printf("%s InitializeBuffers\n", __FUNCTION__);
	for (;;) {
		if (context->available_in == 0 && !is_eof) {
			if (!ProvideInput(context)) return BROTLI_FALSE;
			is_eof = !HasMoreInput(context);
			//printf("%s is_eof=%d\n", __FUNCTION__, is_eof);
		}

		//printf("%s prologue=%d\n", __FUNCTION__, prologue);
		if (prologue) {
			prologue = BROTLI_FALSE;
			const uint8_t* next_meta = context->comment;
			size_t available_meta = context->comment_len;
			//printf("%s start BrotliEncoderCompressStream\n", __FUNCTION__);
			if (!BrotliEncoderCompressStream(s,
				BROTLI_OPERATION_EMIT_METADATA,
				&available_meta, &next_meta,
				&context->available_out, &context->next_out, NULL)) {
					/* Should detect OOM? */
					fprintf(stderr, "failed to emit metadata [%s]\n",
						PrintablePath(context->current_input_path));
					return BROTLI_FALSE;
			}
			//printf("%s done BrotliEncoderCompressStream\n", __FUNCTION__);
			if (available_meta != 0) {
				fprintf(stderr, "failed to emit metadata [%s]\n",
					PrintablePath(context->current_input_path));
				return BROTLI_FALSE;
			}
		} else {
			//printf("%s start BrotliEncoderCompressStream\n", __FUNCTION__);
			//printf("%s BrotliEncoderCompressStream context->available_in=%d\n", __FUNCTION__, context->available_in);
			//printf("%s BrotliEncoderCompressStream context->available_out=%d\n", __FUNCTION__, context->available_out);
			//printf("%s BrotliEncoderCompressStream context->next_in=%p\n", __FUNCTION__, context->next_in);
			//printf("%s BrotliEncoderCompressStream context->next_out=%p\n", __FUNCTION__, context->next_out);
			if (!BrotliEncoderCompressStream(s,
				is_eof ? BROTLI_OPERATION_FINISH : BROTLI_OPERATION_PROCESS,
				&context->available_in, &context->next_in,
				&context->available_out, &context->next_out, NULL)) {
					/* Should detect OOM? */
					fprintf(stderr, "failed to compress data [%s]\n",
						PrintablePath(context->current_input_path));
					return BROTLI_FALSE;
			}
			//printf("%s done BrotliEncoderCompressStream\n", __FUNCTION__);
		}

		if (context->available_out == 0) {
			if (!ProvideOutput(context)) return BROTLI_FALSE;
		}

		if (BrotliEncoderIsFinished(s)) {
			if (!FlushOutput(context)) return BROTLI_FALSE;
			if (context->verbosity > 0) {
				context->end_time = clock();
				fprintf(stderr, "Compressed ");
				PrintFileProcessingProgress(context);
				fprintf(stderr, "\n");
			}
			return BROTLI_TRUE;
		}
	} // end for
}

static BROTLI_BOOL CompressFiles(Context* context) {
	BROTLI_BOOL is_ok = BROTLI_TRUE;
	BROTLI_BOOL rm_input = BROTLI_FALSE;
	BROTLI_BOOL rm_output = BROTLI_TRUE;
	BrotliEncoderState* s = BrotliEncoderCreateInstance(NULL, NULL, NULL);
	if (!s) {
		fprintf(stderr, "out of memory\n");
		return BROTLI_FALSE;
	}
	BrotliEncoderSetParameter(s,
		BROTLI_PARAM_QUALITY, (uint32_t)context->quality);
	if (context->lgwin > 0) {
		/* Specified by user. */
		/* Do not enable "large-window" extension, if not required. */
		if (context->lgwin > BROTLI_MAX_WINDOW_BITS) {
			BrotliEncoderSetParameter(s, BROTLI_PARAM_LARGE_WINDOW, 1u);
		}
		BrotliEncoderSetParameter(s,
			BROTLI_PARAM_LGWIN, (uint32_t)context->lgwin);
	} else {
		/* 0, or not specified by user; could be chosen by compressor. */
		uint32_t lgwin = DEFAULT_LGWIN;
		/* Use file size to limit lgwin. */
		if (context->input_file_length >= 0) {
			lgwin = BROTLI_MIN_WINDOW_BITS;
			while (BROTLI_MAX_BACKWARD_LIMIT(lgwin) <
				   (uint64_t)context->input_file_length) {
				lgwin++;
				if (lgwin == BROTLI_MAX_WINDOW_BITS) break;
			}
		}
		BrotliEncoderSetParameter(s, BROTLI_PARAM_LGWIN, lgwin);
	}
	if (context->input_file_length > 0) {
		uint32_t size_hint = context->input_file_length < (1 << 30) ?
			(uint32_t)context->input_file_length : (1u << 30);
		BrotliEncoderSetParameter(s, BROTLI_PARAM_SIZE_HINT, size_hint);
	}
	if (context->dictionary) {
		BrotliEncoderAttachPreparedDictionary(s, context->prepared_dictionary);
	}
	is_ok = OpenFiles(context);
	if (is_ok && !context->current_output_path &&
		!context->force_overwrite && isatty(STDOUT_FILENO)) {
		fprintf(stderr, "Use -h help. Use -f to force output to a terminal.\n");
		is_ok = BROTLI_FALSE;
	}
	//printf("%s is_ok=%d\n", __FUNCTION__, is_ok);
	if (is_ok) is_ok = CompressFile(context, s);
	BrotliEncoderDestroyInstance(s);
	rm_output = !is_ok;
	if (is_ok && context->reject_uncompressible) {
		if (context->total_out >= context->total_in) {
			rm_output = BROTLI_TRUE;
			if (context->verbosity > 0) {
				fprintf(stderr, "Output is larger than input\n");
			}
		}
	}
	rm_input = !rm_output && context->junk_source;
	if (!CloseFiles(context, rm_input, rm_output)) is_ok = BROTLI_FALSE;
	if (!is_ok) return BROTLI_FALSE;

	return BROTLI_TRUE;
}

static const char* PrettyDecoderErrorString(BrotliDecoderErrorCode code) {
	/* "_ERROR_domain_" is in only added in newer versions. If CLI is linked
	 against older shared library, return error string as is; result might be
	 a bit confusing, e.g. "RESERVED" instead of "FORMAT_RESERVED" */
	const char* prefix = "_ERROR_";
	size_t prefix_len = strlen(prefix);
	const char* error_str = BrotliDecoderErrorString(code);
	size_t error_len = strlen(error_str);
	if (error_len > prefix_len) {
		if (strncmp(error_str, prefix, prefix_len) == 0) {
			error_str += prefix_len;
		}
	}
	return error_str;
}

static void OnMetadataStart(void* opaque, size_t size) {
	Context* context = (Context*) opaque;
	if (context->comment_state == COMMENT_INIT) {
	if (context->comment_len != size) {
		context->comment_state = COMMENT_BAD;
		return;
	}
	context->comment_pos = 0;
	context->comment_state = COMMENT_READ;
	}
}

static void OnMetadataChunk(void* opaque, const uint8_t* data, size_t size) {
	Context* context = (Context*) opaque;
	if (context->comment_state == COMMENT_READ) {
		size_t i;
		for (i = 0; i < size; ++i) {
			if (context->comment_pos >= context->comment_len) {
				context->comment_state = COMMENT_BAD;
				return;
			}
			if (context->comment[context->comment_pos++] != data[i]) {
				context->comment_state = COMMENT_BAD;
				return;
			}
		} // end for
		if (context->comment_pos == context->comment_len) {
			context->comment_state = COMMENT_OK;
		}
	} // end if
}

static BROTLI_BOOL InitDecoder(Context* context) {
	context->decoder = BrotliDecoderCreateInstance(NULL, NULL, NULL);
	if (!context->decoder) {
		fprintf(stderr, "out of memory\n");
		return BROTLI_FALSE;
	}
	/* This allows decoding "large-window" streams. Though it creates
	  fragmentation (new builds decode streams that old builds don't),
	  it is better from used experience perspective. */
	BrotliDecoderSetParameter(
		context->decoder, BROTLI_DECODER_PARAM_LARGE_WINDOW, 1u);
	if (context->dictionary) {
		BrotliDecoderAttachDictionary(context->decoder,
			BROTLI_SHARED_DICTIONARY_RAW, context->dictionary_size,
			context->dictionary);
	}
	return BROTLI_TRUE;
}

static BROTLI_BOOL DecompressFile(Context* context) {
	BrotliDecoderState* s = context->decoder;
	BrotliDecoderResult result = BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT;
	if (context->comment_len) {
		context->comment_state = COMMENT_INIT;
		BrotliDecoderSetMetadataCallbacks(s, &OnMetadataStart, &OnMetadataChunk,
			(void*)context);
	} else {
		context->comment_state = COMMENT_OK;
	}

	InitializeBuffers(context);
	for (;;) {
		/* Early check */
		if (context->comment_state == COMMENT_BAD) {
			fprintf(stderr, "corrupt input [%s]\n",
				PrintablePath(context->current_input_path));
			if (context->verbosity > 0) {
				fprintf(stderr, "reason: comment mismatch\n");
			}
			return BROTLI_FALSE;
		}
		if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) {
			if (!HasMoreInput(context)) {
				fprintf(stderr, "corrupt input [%s]\n",
					PrintablePath(context->current_input_path));
				if (context->verbosity > 0) {
					fprintf(stderr, "reason: truncated input\n");
				}
			return BROTLI_FALSE;
			}
			if (!ProvideInput(context)) return BROTLI_FALSE;
		} else if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
			if (!ProvideOutput(context)) return BROTLI_FALSE;
		} else if (result == BROTLI_DECODER_RESULT_SUCCESS) {
			if (!FlushOutput(context)) return BROTLI_FALSE;
			BROTLI_BOOL has_more_input = (context->available_in != 0);
			int extra_char = EOF;
			if (!has_more_input) {
				extra_char = fgetc(context->fin);
				if (extra_char != EOF) {
					has_more_input = BROTLI_TRUE;
					context->input[0] = (uint8_t)extra_char;
					context->next_in = context->input;
					context->available_in = 1;
				}
			}
			if (has_more_input) {
				if (context->allow_concatenated) {
					if (context->verbosity > 0) {
						fprintf(stderr, "extra input\n");
					}
					if (!ProvideOutput(context)) return BROTLI_FALSE;
					BrotliDecoderDestroyInstance(context->decoder);
					context->decoder = NULL;
					if (!InitDecoder(context)) return BROTLI_FALSE;
					s = context->decoder;
				} else {
					fprintf(stderr, "corrupt input [%s]\n",
						PrintablePath(context->current_input_path));
					if (context->verbosity > 0) {
						fprintf(stderr, "reason: extra input\n");
					}
					return BROTLI_FALSE;
				}
			} else {
				if (context->verbosity > 0) {
					context->end_time = clock();
					fprintf(stderr, "Decompressed ");
					PrintFileProcessingProgress(context);
					fprintf(stderr, "\n");
				}
				/* Final check */
				if (context->comment_state != COMMENT_OK) {
					fprintf(stderr, "corrupt input [%s]\n",
						PrintablePath(context->current_input_path));
					if (context->verbosity > 0) {
						fprintf(stderr, "reason: comment mismatch\n");
					}
				}
				return BROTLI_TRUE;
			}
		} else {  /* result == BROTLI_DECODER_RESULT_ERROR */
			fprintf(stderr, "corrupt input [%s]\n",
				PrintablePath(context->current_input_path));
			if (context->verbosity > 0) {
				BrotliDecoderErrorCode error = BrotliDecoderGetErrorCode(s);
				const char* error_str = PrettyDecoderErrorString(error);
				fprintf(stderr, "reason: %s (%d)\n", error_str, error);
			}
			return BROTLI_FALSE;
		}

		result = BrotliDecoderDecompressStream(s, &context->available_in,
			&context->next_in, &context->available_out, &context->next_out, 0);
	} // end for
}

static BROTLI_BOOL DecompressFiles(Context* context) {
	BROTLI_BOOL is_ok = BROTLI_TRUE;
	BROTLI_BOOL rm_input = BROTLI_FALSE;
	BROTLI_BOOL rm_output = BROTLI_TRUE;
	if (!InitDecoder(context)) return BROTLI_FALSE;
	is_ok = OpenFiles(context);
	if (is_ok && !context->current_input_path &&
		!context->force_overwrite && isatty(STDIN_FILENO)) {
		fprintf(stderr, "Use -h help. Use -f to force input from a terminal.\n");
		is_ok = BROTLI_FALSE;
	}
	if (is_ok) is_ok = DecompressFile(context);
	if (context->decoder) BrotliDecoderDestroyInstance(context->decoder);
	context->decoder = NULL;
	rm_output = !is_ok;
	rm_input = !rm_output && context->junk_source;
	if (!CloseFiles(context, rm_input, rm_output)) is_ok = BROTLI_FALSE;
	if (!is_ok) return BROTLI_FALSE;

	return BROTLI_TRUE;
}

void init_context(Context *context) {
    //context->quality = 11;
    context->quality = 0;
    context->lgwin = -1;
    context->verbosity = 0;
    context->comment_len = 0;
    //context->force_overwrite = BROTLI_FALSE;
    context->force_overwrite = BROTLI_TRUE;
    context->junk_source = BROTLI_FALSE;
    context->reject_uncompressible = BROTLI_FALSE;
    context->copy_stat = BROTLI_TRUE;
    context->test_integrity = BROTLI_FALSE;
    context->write_to_stdout = BROTLI_FALSE;
    context->decompress = BROTLI_FALSE;
    context->large_window = BROTLI_FALSE;
    context->allow_concatenated = BROTLI_FALSE;
    context->output_path = NULL;
    context->dictionary_path = NULL;
    context->suffix = DEFAULT_SUFFIX;
    for (int i = 0; i < MAX_OPTIONS; ++i) context->not_input_indices[i] = 0;
    context->longest_path_len = 1;
    context->input_count = 0;

    context->dictionary = NULL;
    context->dictionary_size = 0;
    context->decoder = NULL;
    context->prepared_dictionary = NULL;
    context->modified_path = NULL;
    context->iterator = 0;
    context->ignore = 0;
    context->iterator_error = BROTLI_FALSE;
    context->buffer = NULL;
    context->current_input_path = NULL;
    context->current_output_path = NULL;
    context->fin = NULL;
    context->fout = NULL;
}


void comp_task(void *pvParameters) {
	PARAMETER_t *task_parameter = pvParameters;
	PARAMETER_t param;
	memcpy((char *)&param, task_parameter, sizeof(PARAMETER_t));
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	ESP_LOGI(pcTaskGetName(NULL), "param.srcPath=[%s]", param.srcPath);
	ESP_LOGI(pcTaskGetName(NULL), "param.dstPath=[%s]", param.dstPath);

	Context context;
	init_context(&context);

	int result = 0;
	context.current_input_path=param.srcPath;
	context.current_output_path=param.dstPath;
	ESP_LOGI(pcTaskGetName(NULL), "kFileBufferSize=%d", kFileBufferSize);
	context.buffer = (uint8_t*)malloc(kFileBufferSize * 2);
	if (context.buffer == NULL) {
		ESP_LOGE(pcTaskGetName(NULL), "malloc fail");
		result = -1;
	} else {
		context.input = context.buffer;
		context.output = context.buffer + kFileBufferSize;
		BROTLI_BOOL ret = CompressFiles(&context);
		ESP_LOGI(pcTaskGetName(NULL), "CompressFiles ret=%d", ret);
		free(context.buffer);
		if (ret == BROTLI_TRUE) {
			ESP_LOGI(pcTaskGetName(NULL), "CompressFiles success");
		} else {
			ESP_LOGE(pcTaskGetName(NULL), "CompressFiles fail");
			result = -2;
		}
	}
	xTaskNotify( param.ParentTaskHandle, result, eSetValueWithOverwrite );
	ESP_LOGI(pcTaskGetName(NULL), "Finish");
	vTaskDelete(NULL);
}

void decomp_task(void *pvParameters) {
	PARAMETER_t *task_parameter = pvParameters;
	PARAMETER_t param;
	memcpy((char *)&param, task_parameter, sizeof(PARAMETER_t));
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	ESP_LOGI(pcTaskGetName(NULL), "param.srcPath=[%s]", param.srcPath);
	ESP_LOGI(pcTaskGetName(NULL), "param.dstPath=[%s]", param.dstPath);

	Context context;
	//context.quality = 11;
	context.quality = 0;
	context.lgwin = -1;
	context.verbosity = 0;
	context.comment_len = 0;
	//context.force_overwrite = BROTLI_FALSE;
	context.force_overwrite = BROTLI_TRUE;
	context.junk_source = BROTLI_FALSE;
	context.reject_uncompressible = BROTLI_FALSE;
	context.copy_stat = BROTLI_TRUE;
	context.test_integrity = BROTLI_FALSE;
	context.write_to_stdout = BROTLI_FALSE;
	context.decompress = BROTLI_FALSE;
	context.large_window = BROTLI_FALSE;
	context.allow_concatenated = BROTLI_FALSE;
	context.output_path = NULL;
	context.dictionary_path = NULL;
	context.suffix = DEFAULT_SUFFIX;
	for (int i = 0; i < MAX_OPTIONS; ++i) context.not_input_indices[i] = 0;
	context.longest_path_len = 1;
	context.input_count = 0;

	context.dictionary = NULL;
	context.dictionary_size = 0;
	context.decoder = NULL;
	context.prepared_dictionary = NULL;
	context.modified_path = NULL;
	context.iterator = 0;
	context.ignore = 0;
	context.iterator_error = BROTLI_FALSE;
	context.buffer = NULL;
	context.current_input_path = NULL;
	context.current_output_path = NULL;
	context.fin = NULL;
	context.fout = NULL;

	int result = 0;
	context.current_input_path=param.srcPath;
	context.current_output_path=param.dstPath;
	ESP_LOGI(pcTaskGetName(NULL), "kFileBufferSize=%d", kFileBufferSize);
	context.buffer = (uint8_t*)malloc(kFileBufferSize * 2);
	if (context.buffer == NULL) {
		ESP_LOGE(pcTaskGetName(NULL), "malloc fail");
		result = -1;
	} else {
		context.input = context.buffer;
		context.output = context.buffer + kFileBufferSize;
		BROTLI_BOOL ret = DecompressFiles(&context);
		ESP_LOGI(pcTaskGetName(NULL), "CompressFiles ret=%d", ret);
		free(context.buffer);
		if (ret == BROTLI_TRUE) {
			ESP_LOGI(pcTaskGetName(NULL), "CompressFiles success");
		} else {
			ESP_LOGE(pcTaskGetName(NULL), "CompressFiles fail");
			result = -2;
		}
	}
	xTaskNotify( param.ParentTaskHandle, result, eSetValueWithOverwrite );
	ESP_LOGI(pcTaskGetName(NULL), "Finish");
	vTaskDelete(NULL);
}
