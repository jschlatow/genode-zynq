/* Xilinx includes */
#include "ap_axi_sdata.h"
#include "hls_stream.h"

/* crypto algorithms includes */
extern "C" {
#include "aes.h"
void xor_buf(const BYTE in[], BYTE out[], size_t len);
}

/* data types */
struct Block {
	BYTE data[AES_BLOCK_SIZE];
};

struct Key {
	BYTE data[32];
};

typedef hls::axis<Block, 0, 0, 0> trans_pkt;

/*
 * top-level function
 *
 * \param in_stream    input AXI stream of plaintext blocks
 * \param out_stream   output AXI stream of encrypted blocks
 * \param key          key
 * \param iv           initialisation vector
 */
void encrypt(hls::stream<trans_pkt> &in_stream,
             hls::stream<trans_pkt> &out_stream,
             Key                    &key,
             Block                  &iv)
{
	/* make in_stream and out_stream an AXI stream */
	#pragma HLS INTERFACE mode=axis port=in_stream
	#pragma HLS INTERFACE mode=axis port=out_stream

	/* make remaining parameters AXI lite (registers) */
	#pragma HLS INTERFACE mode=s_axilite port=key
	#pragma HLS INTERFACE mode=s_axilite port=iv
	#pragma HLS INTERFACE mode=s_axilite port=return

	WORD key_schedule[60];
	aes_key_setup(key.data, key_schedule, 256);

	trans_pkt in_val;

	do {
		#pragma HLS PIPELINE off

		/* read block from input stream */
		in_val = in_stream.read();
		trans_pkt out_val = in_val;

		/* encrypt */
		xor_buf(iv.data, in_val.data.data, AES_BLOCK_SIZE);
		aes_encrypt(in_val.data.data, out_val.data.data, key_schedule, 256);

		/* write block to output stream */
		out_stream.write(out_val);

		/* update iv */
		for (unsigned i=0; i < AES_BLOCK_SIZE; i++) {
			#pragma HLS UNROLL
			iv.data[i] = out_val.data.data[i];
		}

	} while (!in_val.last);
}
