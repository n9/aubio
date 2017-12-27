/*
  Copyright (C) 2007-2013 Paul Brossier <piem@aubio.org>

  This file is part of aubio.

  aubio is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  aubio is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with aubio.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "utils.h"
#define PROG_HAS_PITCH 1
#define PROG_HAS_SILENCE 1
#include "parse_args.h"

aubio_pvoc_t *pv;    // a phase vocoder
cvec_t *fftgrain;    // outputs a spectrum
aubio_mfcc_t * mfcc; // which the mfcc will process
fvec_t * mfcc_out;   // to get the output coefficients

smpl_t level; 		 // current level
smpl_t pitch_confidence;

aubio_pitch_t *o;
fvec_t *pitch;

uint_t n_filters = 40;
uint_t n_coefs = 13;

void process_block (fvec_t *ibuf, fvec_t *obuf)
{
  fvec_zeros(obuf);
  
  //compute mag spectrum
  aubio_pvoc_do (pv, ibuf, fftgrain);
  //compute mfccs
  aubio_mfcc_do(mfcc, fftgrain, mfcc_out);
  
  aubio_pitch_do (o, ibuf, pitch);
  level = aubio_level_lin (ibuf);
  pitch_confidence = aubio_pitch_get_confidence (o);
}

void process_print (void)
{
  /* output times in selected format */
  print_time (blocks * hop_size);

  smpl_t pitch_found = fvec_get_sample(pitch, 0);
  outmsg ("\t%f\t%f\t%f\t", level, pitch_found, pitch_confidence);

  /* output extracted mfcc */
  fvec_print (mfcc_out);
}

int main(int argc, char **argv) {
  int ret = 0;
  // change some default params
  buffer_size  = 512;
  hop_size = 256;

  examples_common_init(argc,argv);

  verbmsg ("using source: %s at %dHz\n", source_uri, samplerate);
  verbmsg ("pitch method: %s, ", pitch_method);
  verbmsg ("pitch unit: %s, ", pitch_unit);
  verbmsg ("buffer_size: %d, ", buffer_size);
  verbmsg ("hop_size: %d, ", hop_size);
  verbmsg ("tolerance: %f\n", pitch_tolerance);

  // <mfcc>
  pv = new_aubio_pvoc (buffer_size, hop_size);
  fftgrain = new_cvec (buffer_size);
  mfcc = new_aubio_mfcc(buffer_size, n_filters, n_coefs, samplerate);
  mfcc_out = new_fvec(n_coefs);
  if (pv == NULL || fftgrain == NULL || mfcc == NULL || mfcc_out == NULL) {
    ret = 1;
    goto beach;
  }
  // </mfcc>
  
  // <pitch>
  o = new_aubio_pitch (pitch_method, buffer_size, hop_size, samplerate);
  if (o == NULL) { ret = 1; goto beach; }
  if (pitch_tolerance != 0.)
    aubio_pitch_set_tolerance (o, pitch_tolerance);
  if (silence_threshold != -90.)
    aubio_pitch_set_silence (o, silence_threshold);
  if (pitch_unit != NULL)
    aubio_pitch_set_unit (o, pitch_unit);

  pitch = new_fvec (1);
  // </pitch>

  examples_common_process((aubio_process_func_t)process_block, process_print);

  // <mfcc>
  del_aubio_pvoc (pv);
  del_cvec (fftgrain);
  del_aubio_mfcc(mfcc);
  del_fvec(mfcc_out);  
  // </mfcc>
  
  // <pitch>  
  del_aubio_pitch (o);
  del_fvec (pitch);  
  // </pitch>

beach:
  examples_common_del();
  return ret;
}
