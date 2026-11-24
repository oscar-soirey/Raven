/**
 * Raven fuzzers - main() of a target built without libFuzzer: runs each
 * file given on the command line once (the corpus of the target), as
 * `fuzzer file...` does with libFuzzer. Exits with 0 when none crashed.
 */

#include "fuzz.h"

int LLVMFuzzerInitialize(int* argc, char*** argv);

int main(int argc, char** argv) {
	int i, count = 0;
	LLVMFuzzerInitialize(&argc, &argv);
	for (i = 1; i < argc; i++) {
		FILE* file = fopen(argv[i], "rb");
		uint8_t* data;
		long length;
		if (!file) {
			fprintf(stderr, "cannot open %s\n", argv[i]);
			return 1;
		}
		fseek(file, 0, SEEK_END);
		length = ftell(file);
		fseek(file, 0, SEEK_SET);
		/* (an exact allocation: AddressSanitizer catches a read one byte too far) */
		data = (uint8_t*)malloc(length > 0 ? (size_t)length : 1);
		if (!data || (length > 0 && fread(data, 1, (size_t)length, file) != (size_t)length)) {
			fprintf(stderr, "cannot read %s\n", argv[i]);
			fclose(file);
			free(data);
			return 1;
		}
		fclose(file);
		LLVMFuzzerTestOneInput(data, (size_t)length);
		free(data);
		count++;
	}
	printf("%d input(s) ran\n", count);
	return 0;
}
