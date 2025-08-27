
#include "t1_c1_decoder.h"

// #include <complex.h>
#include <stddef.h>
#include <stdlib.h>

#include "buffers.h"
#include "fifo.h"
#include "fir.h"
#include "iir.h"
#include "rtl_wmbus_util.h"
#include "t1_c1_decimator.h"
#include "t1_c1_packet_decoder.h"
#include "t1_c1_polar_discriminator.h"
#include "logging.h"

struct runlength_algorithm_t1_c1 {
  int run_length;
  int bit_length;
  int cum_run_length_error;
  unsigned state;
  uint32_t raw_bitstream;
  uint32_t bitstream;
  struct t1_c1_packet_decoder_work decoder;
};

static void runlength_algorithm_reset_t1_c1(
    struct runlength_algorithm_t1_c1* algo) {
  algo->run_length = 0;
  algo->bit_length = 8 * 256;
  algo->cum_run_length_error = 0;
  algo->state = 0u;
  algo->raw_bitstream = 0;
  algo->bitstream = 0;
  reset_t1_c1_packet_decoder(&algo->decoder);
}

static inline float lp_fir_butter_800kHz_100kHz_160kHz(float sample) {
#define COEFFS 11
  static float b[COEFFS] = {
      -0.00456638213, -0.002571450348, 0.02689425925,  0.1141330398,
      0.2264456422,   0.2793297826,    0.2264456422,   0.1141330398,
      0.02689425925,  -0.002571450348, -0.00456638213,
  };
  static float hist[COEFFS];

  static FIRF_FILTER filter = {.length = COEFFS, .b = b, .hist = hist};
#undef COEFFS

  return firf(sample, &filter);
}

/* deglitch_filter_t1_c1 has been calculated by a Python script as follows.
   The filter is counting "1" among 7 bits and saying "1" if count("1") >= 3
else "0". Notice here count("1") >= 3. (More intuitive in that case would be
count("1") >= 3.5.) That forces the filter to put more "1" than "0" on the
output, because RTL-SDR streams more "0" than "1" - i don't know why RTL-SDR do
this. x = 'static const uint8_t deglitch_filter_t1_c1[128] = {' mod8 = 8

for i in range(2**7):
    s = '{0:07b};'.format(i)
    val = '1' if bin(i).count("1") >= 3 else '0'
    print(s[0] + ";" + s[1] + ";" + s[2] + ";" + s[3] + ";" + s[4] + ";" + s[5]
+ ";" + s[6] + ";;%d;;%s" % (bin(i).count("1"), val))

    if i % 8 == 0: x += '\n\t'
    x += val + ','

x += '};\n'

print(x)
*/
static const uint8_t deglitch_filter_t1_c1[128] = {
    0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1,
    1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1,
    1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

static const uint32_t ACCESS_CODE_T1_C1 = 0x543d;
static const uint32_t ACCESS_CODE_T1_C1_BITMASK = 0xFFFFu;
static const unsigned ACCESS_CODE_T1_C1_ERRORS = 0u;  // 0 if no errors allowed

/** @brief Sparse Ones runs in time proportional to the number
 *         of 1 bits.
 *
 *   From: http://gurmeet.net/puzzles/fast-bit-counting-routines
 */
static inline unsigned count_set_bits_sparse_one(uint32_t n) {
  unsigned count = 0;

  while (n) {
    count++;
    n &= (n - 1);  // set rightmost 1 bit in n to 0
  }

  return count;
}

static inline float bp_iir_cheb1_800kHz_90kHz_98kHz_102kHz_110kHz(
    float sample) {
#define GAIN 1.874981046e-06
#define SECTIONS 3
  static const float b[3 * SECTIONS] = {
      1, 1.999994649,     0.9999946492, 1, -1.99999482, 0.9999948196,
      1, 1.703868036e-07, -1.000010531,
  };
  static const float a[3 * SECTIONS] = {
      1, -1.387139203, 0.9921518712, 1, -1.403492665, 0.9845934971,
      1, -1.430055639, 0.9923856172,
  };
  static float hist[3 * SECTIONS] = {};

  static IIRF_FILTER filter = {
      .sections = SECTIONS, .b = b, .a = a, .gain = GAIN, .hist = hist};
#undef SECTIONS
#undef GAIN

  return iirf(sample, &filter);
}

static inline unsigned count_set_bits(uint32_t n) {
#if defined(__i386__) || defined(__arm__)
  return __builtin_popcount(n);
#else
  return count_set_bits_sparse_one(n);
#endif
}

static void runlength_algorithm_t1_c1(unsigned raw_bit, unsigned rssi,
                                      struct runlength_algorithm_t1_c1* algo) {
  algo->raw_bitstream = (algo->raw_bitstream << 1) | raw_bit;

  const unsigned state = deglitch_filter_t1_c1[algo->raw_bitstream & 0x3Fu];

  // Edge detector.
  if (algo->state == state) {
    algo->run_length++;
  } else {
    if (algo->run_length < 5) {
      runlength_algorithm_reset_t1_c1(algo);
      algo->state = state;
      algo->run_length = 1;
      return;
    }

    // const int unscaled_run_length = algo->run_length;

    algo->run_length *=
        256;  // resolution scaling up for fixed point calculation

    const int half_bit_length = algo->bit_length / 2;

    if (algo->run_length <= half_bit_length) {
      runlength_algorithm_reset_t1_c1(algo);
      algo->state = state;
      algo->run_length = 1;
      return;
    }

    int num_of_bits_rx;
    for (num_of_bits_rx = 0; algo->run_length > half_bit_length;
         num_of_bits_rx++) {
      algo->run_length -= algo->bit_length;

      unsigned bit = algo->state;

      algo->bitstream = (algo->bitstream << 1) | bit;

      if (count_set_bits((algo->bitstream & ACCESS_CODE_T1_C1_BITMASK) ^
                         ACCESS_CODE_T1_C1) <= ACCESS_CODE_T1_C1_ERRORS) {
        bit |=
            (1u
             << PACKET_PREAMBLE_DETECTED_SHIFT);  // packet detected; mark the
                                                  // bit similar to "Access
                                                  // Code"-Block in GNU Radio
      }

      t1_c1_packet_decoder(bit, rssi, &algo->decoder, "rla;");
    }

#if 0
        const int bit_error_length = algo->run_length / num_of_bits_rx;
        if (in_rx_t1_c1_packet_decoder(&algo->decoder))
        {
            fprintf(stdout, "rl = %d, num_of_bits_rx = %d, bit_length = %d, old_bit_error_length = %d, new_bit_error_length = %d\n",
                    unscaled_run_length, num_of_bits_rx, algo->bit_length, algo->bit_error_length, bit_error_length);
        }
#endif

    // Some kind of PI controller is implemented below: u[n] = u[n-1] + Kp *
    // e[n] + Ki * sum(e[0..n]). Kp and Ki were found by experiment; e[n] :=
    // algo->run_length; u[[n] is the new bit length; u[n-1] is the last known
    // bit length
    algo->cum_run_length_error += algo->run_length;  // sum(e[0..n])
#define PI_KP 32
#define PI_KI 16
    // algo->bit_length += (algo->run_length / PI_KP +
    // algo->cum_run_length_error / PI_KI) / num_of_bits_rx;
    algo->bit_length +=
        (algo->run_length + algo->cum_run_length_error / PI_KI) /
        (PI_KP * num_of_bits_rx);
#undef PI_KI
#undef PI_KP

    algo->state = state;
    algo->run_length = 1;
  }
}

struct time2_algorithm_t1_c1 {
  uint32_t bitstream;
  struct t1_c1_packet_decoder_work t1_c1_decoder;
};

static void time2_algorithm_t1_c1_reset(struct time2_algorithm_t1_c1* algo) {
  algo->bitstream = 0;
  reset_t1_c1_packet_decoder(&algo->t1_c1_decoder);
}

static void time2_algorithm_t1_c1(unsigned bit, unsigned rssi,
                                  struct time2_algorithm_t1_c1* algo) {
  algo->bitstream = (algo->bitstream << 1) | bit;

  if (count_set_bits((algo->bitstream & ACCESS_CODE_T1_C1_BITMASK) ^
                     ACCESS_CODE_T1_C1) <= ACCESS_CODE_T1_C1_ERRORS) {
    bit |= (1u << PACKET_PREAMBLE_DETECTED_SHIFT);  // packet detected; mark the
                                                    // bit similar to "Access
                                                    // Code"-Block in GNU Radio
  }

  t1_c1_packet_decoder(bit, rssi, &algo->t1_c1_decoder, "t2a;");
}

static const unsigned opts_CLOCK_LOCK_THRESHOLD_T1_C1 =
    2;  // Is not implemented as option yet.

void t1_c1_signal_chain(float _delta_phi_t1_c1, float rssi_t1_c1,
                        struct time2_algorithm_t1_c1* t2_algo_t1_c1,
                        struct runlength_algorithm_t1_c1* rl_algo_t1_c1) {
  static int16_t old_clock_t1_c1 = INT16_MIN;
  static unsigned clock_lock_t1_c1 = 0;

  // Post-filtering to prevent bit errors because of signal jitter.
  float delta_phi_t1_c1 = lp_fir_butter_800kHz_100kHz_160kHz(_delta_phi_t1_c1);

  // Get the bit!
  unsigned bit_t1_c1 = (delta_phi_t1_c1 >= 0) ? (1u << PACKET_DATABIT_SHIFT)
                                              : (0u << PACKET_DATABIT_SHIFT);

  // --- runlength algorithm section begin ---
  runlength_algorithm_t1_c1(bit_t1_c1, rssi_t1_c1, rl_algo_t1_c1);

  // --- runlength algorithm section end ---

  // --- time2 algorithm section begin ---
  // --- clock recovery section begin ---
  // The time-2 method is implemented: push squared signal through a bandpass
  // tuned close to the symbol rate. Saturating band-pass output produces a
  // rectangular pulses with the required timing information.
  // Clock-Signal is crossing zero in half period.
  const int16_t clock_t1_c1 = (bp_iir_cheb1_800kHz_90kHz_98kHz_102kHz_110kHz(
                                   delta_phi_t1_c1 * delta_phi_t1_c1) >= 0)
                                  ? INT16_MAX
                                  : INT16_MIN;
  // fwrite(&clock_t1_c1, sizeof(clock_t1_c1), 1, clock_out);

  if (clock_t1_c1 > old_clock_t1_c1) {  // Clock signal rising edge detected.
    clock_lock_t1_c1 = 1;
  } else if (clock_t1_c1 == INT16_MAX) {  // Clock signal is still high.
    if (clock_lock_t1_c1 <
        opts_CLOCK_LOCK_THRESHOLD_T1_C1) {  // Skip up to
                                            // (opts_CLOCK_LOCK_THRESHOLD_T1_C1
                                            // - 1) clock bits
      // to get closer to the middle of the data bit.
      clock_lock_t1_c1++;
    } else if (clock_lock_t1_c1 ==
               opts_CLOCK_LOCK_THRESHOLD_T1_C1) {  // Sample data bit at
                                                   // CLOCK_LOCK_THRESHOLD_T1_C1
                                                   // clock bit position.
      clock_lock_t1_c1++;
      time2_algorithm_t1_c1(bit_t1_c1, rssi_t1_c1, t2_algo_t1_c1);
    }
  }
  old_clock_t1_c1 = clock_t1_c1;
  // --- clock recovery section end ---
  // --- time2 algorithm section end ---
}

void* t1_c1_decoder(void* args) {
  // receive raw samples from the fifo_raw_sample (in args)
  // run decimation in a subthread -> decimated I/Q samples
  // Perform Polar Discrimination on the I/Q samples -> delta_phi and rssi
  // feed into the t1_c1_signal_chain

  set_thread_name("T1 C1 decoder");

  // extract arguments
  t_fifo* p_fifo_raw_sample = ((t_t1_c1_decoder_args*)args)->fifo_raw_sample;

  // setup the fifo for the decimated I/Q samples
  t_fifo fifo_decimated_sample;
  fifo_reset(  //
      &fifo_decimated_sample,
      sizeof(decimated_sample_buffers.buffers) /
          sizeof(decimated_sample_buffers.buffers[0]),
      NULL);

  // setup the fifo for the Polar Discriminator output
  t_fifo fifo_phi_rssi;
  fifo_reset(
      &fifo_phi_rssi,
      sizeof(phi_rssi_buffers.buffers) / sizeof(phi_rssi_buffers.buffers[0]),
      NULL);

  // initalization frame deconding algorithms

  struct time2_algorithm_t1_c1 t2_algo_t1_c1;
  time2_algorithm_t1_c1_reset(&t2_algo_t1_c1);

  struct runlength_algorithm_t1_c1 rl_algo_t1_c1;
  runlength_algorithm_reset_t1_c1(&rl_algo_t1_c1);

  // start the decimator thread
  // consumes raw I/Q samples
  // produces decimated I/Q samples
  pthread_t thread_decimator;
  t_decimation_t1_c1_args t1_c1_decimation_args = {
      .fifo_raw_sample = p_fifo_raw_sample,
      .fifo_decimated_sample = &fifo_decimated_sample};
  if (pthread_create(&thread_decimator, NULL, t1_c1_decimator,
                     &t1_c1_decimation_args) != 0) {
    perror("pthread_create");
    exit(EXIT_FAILURE);
  }

  // start the polar discriminator thread for C1 and T1 frames
  // consumes decimated I/Q samples
  // produces delta_phi and rssi stream
  pthread_t thread_polar_discriminator;
  t_t1_c1_polar_discriminator_args t1_c1_polar_discriminator_args = {
      .fifo_iq_sample = &fifo_decimated_sample,
      .fifo_phi_rssi = &fifo_phi_rssi};
  if (pthread_create(&thread_polar_discriminator, NULL,
                     t1_c1_polar_discriminator,
                     &t1_c1_polar_discriminator_args) != 0) {
    perror("pthread_create");
    exit(EXIT_FAILURE);
  }

  int stop = 0;

  while (!stop) {
    // Get phi_rssi buffer (fed by the Polar Discriminator)
    t_phi_rssi_buffer* phi_rssi_buffer =
        &phi_rssi_buffers.buffers[fifo_get_read_idx(&fifo_phi_rssi)];

    // stop if we are told so
    if (phi_rssi_buffer->stop) {
      stop = 1;
      break;
    }
    // process the full buffer
    for (size_t k = 0;
         k < sizeof(phi_rssi_buffer->data) / sizeof(phi_rssi_buffer->data[0]);
         k++) {

      // feed the delpta_phi and into the signal chain
      // TODO: run the low-pass filter (on delta_phi) in a separate thread
      // TODO: run the clock recovery and time2 algorithm an a separate thread
      t1_c1_signal_chain(phi_rssi_buffer->data[k].phi,   //
                         phi_rssi_buffer->data[k].rssi,  //
                         &t2_algo_t1_c1,                 //
                         &rl_algo_t1_c1);
    }

    // after processing the raw sample we releases it to be filled again
    fifo_release_read_idx(&fifo_phi_rssi);
  }

  pthread_join(thread_decimator, NULL);
  pthread_join(thread_polar_discriminator, NULL);
  return NULL;
}
