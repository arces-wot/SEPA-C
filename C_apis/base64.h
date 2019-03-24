/* The MIT License (MIT)

Copyright (c) 2014 Little Star Media, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

/*
 This is part of the libb64 project, and has been placed in the public domain.
For details, see http://sourceforge.net/projects/libb64
*/

#ifndef BASE64_H_INCLUDED
#define BASE64_H_INCLUDED

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
	step_A, step_B, step_C
} base64_encodestep;

typedef struct {
	base64_encodestep step;
	char result;
	int stepcount;
} base64_encodestate;

typedef enum {
	step_a, step_b, step_c, step_d
} base64_decodestep;

typedef struct {
	base64_decodestep step;
	char plainchar;
} base64_decodestate;

void base64_init_encodestate(base64_encodestate* state_in);

char base64_encode_value(char value_in);

int base64_encode_block(const char* plaintext_in,
						int length_in,
						char* code_out,
						base64_encodestate* state_in);

int base64_encode_blockend(char* code_out, base64_encodestate* state_in);

void base64_init_decodestate(base64_decodestate* state_in);

int base64_decode_value(char value_in);

int base64_decode_block(const char* code_in,
						const int length_in,
						char* plaintext_out,
						base64_decodestate* state_in);

char* b64_encode(const char* input);

char* b64_decode(const char* input);

#ifdef __cplusplus
}
#endif

#endif
