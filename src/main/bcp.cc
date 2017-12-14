// ----------------------------------------------------------------- //
//             The Speech Signal Processing Toolkit (SPTK)           //
//             developed by SPTK Working Group                       //
//             http://sp-tk.sourceforge.net/                         //
// ----------------------------------------------------------------- //
//                                                                   //
//  Copyright (c) 1984-2007  Tokyo Institute of Technology           //
//                           Interdisciplinary Graduate School of    //
//                           Science and Engineering                 //
//                                                                   //
//                1996-2017  Nagoya Institute of Technology          //
//                           Department of Computer Science          //
//                                                                   //
// All rights reserved.                                              //
//                                                                   //
// Redistribution and use in source and binary forms, with or        //
// without modification, are permitted provided that the following   //
// conditions are met:                                               //
//                                                                   //
// - Redistributions of source code must retain the above copyright  //
//   notice, this list of conditions and the following disclaimer.   //
// - Redistributions in binary form must reproduce the above         //
//   copyright notice, this list of conditions and the following     //
//   disclaimer in the documentation and/or other materials provided //
//   with the distribution.                                          //
// - Neither the name of the SPTK working group nor the names of its //
//   contributors may be used to endorse or promote products derived //
//   from this software without specific prior written permission.   //
//                                                                   //
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            //
// CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       //
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          //
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          //
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS //
// BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          //
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   //
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     //
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON //
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   //
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    //
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           //
// POSSIBILITY OF SUCH DAMAGE.                                       //
// ----------------------------------------------------------------- //

#include <getopt.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "SPTK/utils/int24_t.h"
#include "SPTK/utils/sptk_utils.h"
#include "SPTK/utils/uint24_t.h"

namespace {

const int kDefaultInputStartNumber(0);
const int kDefaultInputBlockLength(512);
const int kDefaultOutputStartNumber(0);
const double kDefaultPadValue(0.0);
const char* kDefaultDataType("d");

void PrintUsage(std::ostream* stream) {
  // clang-format off
  *stream << std::endl;
  *stream << " bcp - block copy" << std::endl;
  *stream << std::endl;
  *stream << "  usage:" << std::endl;
  *stream << "       bcp [ options ] [ infile ] > stdout" << std::endl;
  *stream << "  options:" << std::endl;
  *stream << "       -s s  : start number (input)      (   int)[" << std::setw(5) << std::right << kDefaultInputStartNumber  << "][ 0 <= s <= e ]" << std::endl;  // NOLINT
  *stream << "       -e e  : end number (input)        (   int)[" << std::setw(5) << std::right << "l-1"                     << "][ s <= e <  l ]" << std::endl;  // NOLINT
  *stream << "       -l l  : block length (input)      (   int)[" << std::setw(5) << std::right << kDefaultInputBlockLength  << "][ 1 <= l <=   ]" << std::endl;  // NOLINT
  *stream << "       -m m  : block order (input)       (   int)[" << std::setw(5) << std::right << "l-1"                     << "][ 0 <= m <=   ]" << std::endl;  // NOLINT
  *stream << "       -S S  : start number (output)     (   int)[" << std::setw(5) << std::right << kDefaultOutputStartNumber << "][ 0 <= S <  L ]" << std::endl;  // NOLINT
  *stream << "       -L L  : block length (output)     (   int)[" << std::setw(5) << std::right << "N/A"                     << "][ 1 <= L <=   ]" << std::endl;  // NOLINT
  *stream << "       -M M  : block order (output)      (   int)[" << std::setw(5) << std::right << "N/A"                     << "][ 0 <= M <=   ]" << std::endl;  // NOLINT
  *stream << "       -f f  : pad value for empty slots (double)[" << std::setw(5) << std::right << kDefaultPadValue          << "][   <= f <=   ]" << std::endl;  // NOLINT
  *stream << "       +type : data type                         [" << std::setw(5) << std::right << kDefaultDataType          << "]" << std::endl;  // NOLINT
  *stream << "                 "; sptk::PrintDataType("c", stream); sptk::PrintDataType("C", stream); *stream << std::endl;  // NOLINT
  *stream << "                 "; sptk::PrintDataType("s", stream); sptk::PrintDataType("S", stream); *stream << std::endl;  // NOLINT
  *stream << "                 "; sptk::PrintDataType("h", stream); sptk::PrintDataType("H", stream); *stream << std::endl;  // NOLINT
  *stream << "                 "; sptk::PrintDataType("i", stream); sptk::PrintDataType("I", stream); *stream << std::endl;  // NOLINT
  *stream << "                 "; sptk::PrintDataType("l", stream); sptk::PrintDataType("L", stream); *stream << std::endl;  // NOLINT
  *stream << "                 "; sptk::PrintDataType("f", stream); sptk::PrintDataType("d", stream); *stream << std::endl;  // NOLINT
  *stream << "                 "; sptk::PrintDataType("e", stream); sptk::PrintDataType("a", stream); *stream << std::endl;  // NOLINT
  *stream << "       -h    : print this message" << std::endl;
  *stream << "  infile:" << std::endl;
  *stream << "       data sequence                             [stdin]" << std::endl;  // NOLINT
  *stream << "  stdout:" << std::endl;
  *stream << "       copied data sequence" << std::endl;
  *stream << std::endl;
  *stream << " SPTK: version " << sptk::kVersion << std::endl;
  *stream << std::endl;
  // clang-format on
}

class BlockCopyInterface {
 public:
  virtual ~BlockCopyInterface() {
  }
  virtual bool Run(std::istream* input_stream) const = 0;
};

template <typename T>
class BlockCopy : public BlockCopyInterface {
 public:
  BlockCopy(int input_start_number, int input_end_number,
            int input_block_length, int output_start_number,
            int output_block_length, T pad_value, bool is_ascii = false)
      : input_start_number_(input_start_number),
        input_end_number_(input_end_number),
        input_block_length_(input_block_length),
        output_start_number_(output_start_number),
        output_block_length_(output_block_length),
        pad_value_(pad_value),
        is_ascii_(is_ascii) {
  }

  ~BlockCopy() {
  }

  virtual bool Run(std::istream* input_stream) const {
    const int copy_length(input_end_number_ - input_start_number_ + 1);
    const int left_pad_length(output_start_number_);
    const int right_pad_length(output_block_length_ - output_start_number_ -
                               copy_length);

    std::vector<T> pad_data(std::max(left_pad_length, right_pad_length),
                            pad_value_);
    std::vector<T> input_data(input_block_length_);

    if (is_ascii_) {
      T* pads(&(pad_data[0]));
      T* inputs(&(input_data[0]));
      bool halt(false);
      while (!halt) {
        // read data
        for (int i(0); i < input_block_length_; ++i) {
          std::string word;
          *input_stream >> word;
          if (word.empty()) {
            halt = true;
            break;
          }
          if (input_start_number_ <= i && i <= input_end_number_) {
            try {
              inputs[i] = std::stold(word);
            } catch (std::invalid_argument) {
              return false;
            }
          }
        }
        if (halt) break;

        // write data
        for (int i(0); i < left_pad_length; ++i) {
          std::cout << pads[i] << " ";
        }
        for (int i(input_start_number_); i <= input_end_number_; ++i) {
          std::cout << std::setprecision(
                           std::numeric_limits<long double>::digits10 + 1)
                    << inputs[i] << " ";
        }
        for (int i(0); i < right_pad_length; ++i) {
          std::cout << pads[i] << " ";
        }
        std::cout << std::endl;
      }
    } else {
      while (sptk::ReadStream(false, 0, 0, input_block_length_, &input_data,
                              input_stream)) {
        if (0 < left_pad_length &&
            !sptk::WriteStream(0, left_pad_length, pad_data, &std::cout)) {
          return false;
        }
        if (!sptk::WriteStream(input_start_number_, copy_length, input_data,
                               &std::cout)) {
          return false;
        }
        if (0 < right_pad_length &&
            !sptk::WriteStream(0, right_pad_length, pad_data, &std::cout)) {
          return false;
        }
      }
    }

    return true;
  }

 private:
  const int input_start_number_;
  const int input_end_number_;
  const int input_block_length_;
  const int output_start_number_;
  const int output_block_length_;
  const T pad_value_;
  const bool is_ascii_;

  DISALLOW_COPY_AND_ASSIGN(BlockCopy<T>);
};

class BlockCopyWrapper {
 public:
  BlockCopyWrapper(const std::string& data_type, int input_start_number,
                   int input_end_number, int input_block_length,
                   int output_start_number, int output_block_length,
                   double pad_value)
      : block_copy_(NULL) {
    if ("c" == data_type) {
      block_copy_ = new BlockCopy<int8_t>(
          input_start_number, input_end_number, input_block_length,
          output_start_number, output_block_length, pad_value);
    } else if ("s" == data_type) {
      block_copy_ = new BlockCopy<int16_t>(
          input_start_number, input_end_number, input_block_length,
          output_start_number, output_block_length, pad_value);
    } else if ("h" == data_type) {
      block_copy_ = new BlockCopy<sptk::int24_t>(
          input_start_number, input_end_number, input_block_length,
          output_start_number, output_block_length, sptk::int24_t(pad_value));
    } else if ("i" == data_type) {
      block_copy_ = new BlockCopy<int32_t>(
          input_start_number, input_end_number, input_block_length,
          output_start_number, output_block_length, pad_value);
    } else if ("l" == data_type) {
      block_copy_ = new BlockCopy<int64_t>(
          input_start_number, input_end_number, input_block_length,
          output_start_number, output_block_length, pad_value);
    } else if ("C" == data_type) {
      block_copy_ = new BlockCopy<uint8_t>(
          input_start_number, input_end_number, input_block_length,
          output_start_number, output_block_length, pad_value);
    } else if ("S" == data_type) {
      block_copy_ = new BlockCopy<uint16_t>(
          input_start_number, input_end_number, input_block_length,
          output_start_number, output_block_length, pad_value);
    } else if ("H" == data_type) {
      block_copy_ = new BlockCopy<sptk::uint24_t>(
          input_start_number, input_end_number, input_block_length,
          output_start_number, output_block_length, sptk::uint24_t(pad_value));
    } else if ("I" == data_type) {
      block_copy_ = new BlockCopy<uint32_t>(
          input_start_number, input_end_number, input_block_length,
          output_start_number, output_block_length, pad_value);
    } else if ("L" == data_type) {
      block_copy_ = new BlockCopy<uint64_t>(
          input_start_number, input_end_number, input_block_length,
          output_start_number, output_block_length, pad_value);
    } else if ("f" == data_type) {
      block_copy_ = new BlockCopy<float>(
          input_start_number, input_end_number, input_block_length,
          output_start_number, output_block_length, pad_value);
    } else if ("d" == data_type) {
      block_copy_ = new BlockCopy<double>(
          input_start_number, input_end_number, input_block_length,
          output_start_number, output_block_length, pad_value);
    } else if ("e" == data_type) {
      block_copy_ = new BlockCopy<long double>(
          input_start_number, input_end_number, input_block_length,
          output_start_number, output_block_length, pad_value);
    } else if ("a" == data_type) {
      block_copy_ = new BlockCopy<long double>(
          input_start_number, input_end_number, input_block_length,
          output_start_number, output_block_length, pad_value, true);
    }
  }

  ~BlockCopyWrapper() {
    delete block_copy_;
  }

  bool IsValid() const {
    return NULL != block_copy_;
  }

  bool Run(std::istream* input_stream) const {
    return IsValid() && block_copy_->Run(input_stream);
  }

 private:
  BlockCopyInterface* block_copy_;

  DISALLOW_COPY_AND_ASSIGN(BlockCopyWrapper);
};

}  // namespace

int main(int argc, char* argv[]) {
  int input_start_number(kDefaultInputStartNumber);
  int input_end_number(kDefaultInputBlockLength - 1);
  int input_block_length(kDefaultInputBlockLength);
  int output_start_number(kDefaultOutputStartNumber);
  int output_block_length(kDefaultInputBlockLength);
  double pad_value(kDefaultPadValue);
  std::string data_type(kDefaultDataType);
  bool is_input_end_number_specified(false);
  bool is_output_block_length_specified(false);

  for (;;) {
    const int option_char(
        getopt_long(argc, argv, "s:e:l:m:S:L:M:f:h", NULL, NULL));
    if (-1 == option_char) break;

    switch (option_char) {
      case 's': {
        if (!sptk::ConvertStringToInteger(optarg, &input_start_number) ||
            input_start_number < 0) {
          std::ostringstream error_message;
          error_message << "The argument for the -s option must be a "
                        << "non-negative integer";
          sptk::PrintErrorMessage("bcp", error_message);
          return 1;
        }
        break;
      }
      case 'e': {
        if (!sptk::ConvertStringToInteger(optarg, &input_end_number) ||
            input_end_number < 0) {
          std::ostringstream error_message;
          error_message << "The argument for the -e option must be a "
                        << "non-negative integer";
          sptk::PrintErrorMessage("bcp", error_message);
          return 1;
        }
        is_input_end_number_specified = true;
        break;
      }
      case 'l': {
        if (!sptk::ConvertStringToInteger(optarg, &input_block_length) ||
            input_block_length <= 0) {
          std::ostringstream error_message;
          error_message
              << "The argument for the -l option must be a positive integer";
          sptk::PrintErrorMessage("bcp", error_message);
          return 1;
        }
        break;
      }
      case 'm': {
        if (!sptk::ConvertStringToInteger(optarg, &input_block_length) ||
            input_block_length < 0) {
          std::ostringstream error_message;
          error_message << "The argument for the -m option must be a "
                        << "non-negative integer";
          sptk::PrintErrorMessage("bcp", error_message);
          return 1;
        }
        ++input_block_length;
        break;
      }
      case 'S': {
        if (!sptk::ConvertStringToInteger(optarg, &output_start_number) ||
            output_start_number < 0) {
          std::ostringstream error_message;
          error_message << "The argument for the -S option must be a "
                        << "non-negative integer";
          sptk::PrintErrorMessage("bcp", error_message);
          return 1;
        }
        break;
      }
      case 'L': {
        if (!sptk::ConvertStringToInteger(optarg, &output_block_length) ||
            output_block_length <= 0) {
          std::ostringstream error_message;
          error_message
              << "The argument for the -L option must be a positive integer";
          sptk::PrintErrorMessage("bcp", error_message);
          return 1;
        }
        is_output_block_length_specified = true;
        break;
      }
      case 'M': {
        if (!sptk::ConvertStringToInteger(optarg, &output_block_length) ||
            output_block_length < 0) {
          std::ostringstream error_message;
          error_message << "The argument for the -M option must be a "
                        << "non-negative integer";
          sptk::PrintErrorMessage("bcp", error_message);
          return 1;
        }
        ++output_block_length;
        is_output_block_length_specified = true;
        break;
      }
      case 'f': {
        if (!sptk::ConvertStringToDouble(optarg, &pad_value)) {
          std::ostringstream error_message;
          error_message << "The argument for the -f option must be numeric";
          sptk::PrintErrorMessage("bcp", error_message);
          return 1;
        }
        break;
      }
      case 'h': {
        PrintUsage(&std::cout);
        return 0;
      }
      default: {
        PrintUsage(&std::cerr);
        return 1;
      }
    }
  }

  if (!is_input_end_number_specified) {
    input_end_number = input_block_length - 1;
  } else if (input_block_length <= input_end_number) {
    std::ostringstream error_message;
    error_message << "End number " << input_end_number
                  << " must be less than block length " << input_block_length;
    sptk::PrintErrorMessage("bcp", error_message);
    return 1;
  } else if (input_end_number < input_start_number) {
    std::ostringstream error_message;
    error_message << "End number " << input_end_number
                  << " must be equal to or greater than start number "
                  << input_start_number;
    sptk::PrintErrorMessage("bcp", error_message);
    return 1;
  }

  if (input_block_length <= input_start_number) {
    std::ostringstream error_message;
    error_message << "Start number " << input_start_number
                  << " must be less than block length " << input_block_length;
    sptk::PrintErrorMessage("bcp", error_message);
    return 1;
  }

  const int copy_length(input_end_number - input_start_number + 1);
  if (!is_output_block_length_specified) {
    output_block_length = output_start_number + copy_length;
  } else if (output_block_length < output_start_number + copy_length) {
    std::ostringstream error_message;
    error_message << "Output block length is too short";
    sptk::PrintErrorMessage("bcp", error_message);
    return 1;
  }

  // get input file
  const char* input_file(NULL);
  for (int i(argc - optind); 1 <= i; --i) {
    const char* arg(argv[argc - i]);
    if (0 == std::strncmp(arg, "+", 1)) {
      const std::string str(arg);
      data_type = str.substr(1, std::string::npos);
    } else if (NULL == input_file) {
      input_file = arg;
    } else {
      std::ostringstream error_message;
      error_message << "Too many input files";
      sptk::PrintErrorMessage("bcp", error_message);
      return 1;
    }
  }

  // open stream
  std::ifstream ifs;
  ifs.open(input_file, std::ios::in | std::ios::binary);
  if (ifs.fail() && NULL != input_file) {
    std::ostringstream error_message;
    error_message << "Cannot open file " << input_file;
    sptk::PrintErrorMessage("bcp", error_message);
    return 1;
  }
  std::istream& input_stream(ifs.fail() ? std::cin : ifs);

  BlockCopyWrapper block_copy(data_type, input_start_number, input_end_number,
                              input_block_length, output_start_number,
                              output_block_length, pad_value);

  if (!block_copy.IsValid()) {
    std::ostringstream error_message;
    error_message << "Unexpected argument for the +type option";
    sptk::PrintErrorMessage("bcp", error_message);
    return 1;
  }

  if (!block_copy.Run(&input_stream)) {
    std::ostringstream error_message;
    error_message << "Failed to copy";
    sptk::PrintErrorMessage("bcp", error_message);
    return 1;
  }

  return 0;
}
