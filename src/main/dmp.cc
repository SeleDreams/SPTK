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
//                1996-2018  Nagoya Institute of Technology          //
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
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "SPTK/utils/int24_t.h"
#include "SPTK/utils/sptk_utils.h"
#include "SPTK/utils/uint24_t.h"

namespace {

const int kBufferSize(128);
const int kMagicNumberForEndOfFile(-1);
const char* kDefaultDataType("d");

void PrintUsage(std::ostream* stream) {
  // clang-format off
  *stream << std::endl;
  *stream << " dmp - binary file dump" << std::endl;
  *stream << std::endl;
  *stream << "  usage:" << std::endl;
  *stream << "       dmp [ options ] [ infile ] > stdout" << std::endl;
  *stream << "  options:" << std::endl;
  *stream << "       -l l  : block length       (   int)[" << std::setw(5) << std::right << "EOS"            << "][ 1 <= l <=   ]" << std::endl;  // NOLINT
  *stream << "       -m m  : block order        (   int)[" << std::setw(5) << std::right << "EOS"            << "][ 0 <= m <=   ]" << std::endl;  // NOLINT
  *stream << "       -f f  : print format       (string)[" << std::setw(5) << std::right << "N/A"            << "]" << std::endl;  // NOLINT
  *stream << "       +type : data type                  [" << std::setw(5) << std::right << kDefaultDataType << "]" << std::endl;  // NOLINT
  *stream << "                 "; sptk::PrintDataType("c", stream); sptk::PrintDataType("C", stream); *stream << std::endl;  // NOLINT
  *stream << "                 "; sptk::PrintDataType("s", stream); sptk::PrintDataType("S", stream); *stream << std::endl;  // NOLINT
  *stream << "                 "; sptk::PrintDataType("h", stream); sptk::PrintDataType("H", stream); *stream << std::endl;  // NOLINT
  *stream << "                 "; sptk::PrintDataType("i", stream); sptk::PrintDataType("I", stream); *stream << std::endl;  // NOLINT
  *stream << "                 "; sptk::PrintDataType("l", stream); sptk::PrintDataType("L", stream); *stream << std::endl;  // NOLINT
  *stream << "                 "; sptk::PrintDataType("f", stream); sptk::PrintDataType("d", stream); *stream << std::endl;  // NOLINT
  *stream << "                 "; sptk::PrintDataType("e", stream);                                   *stream << std::endl;  // NOLINT
  *stream << "       -h    : print this message" << std::endl;
  *stream << "  infile:" << std::endl;
  *stream << "       data sequence                      [stdin]" << std::endl;
  *stream << "  stdout:" << std::endl;
  *stream << "       dumped data sequence" << std::endl;
  *stream << std::endl;
  *stream << " SPTK: version " << sptk::kVersion << std::endl;
  *stream << std::endl;
  // clang-format on
}

class DataDumpInterface {
 public:
  virtual ~DataDumpInterface() {
  }
  virtual bool Run(std::istream* input_stream) const = 0;
};

template <typename T>
class DataDump : public DataDumpInterface {
 public:
  DataDump(const std::string& print_format, int minimum_index,
           int maximum_index)
      : print_format_(print_format),
        minimum_index_(minimum_index),
        maximum_index_(maximum_index) {
  }

  ~DataDump() {
  }

  virtual bool Run(std::istream* input_stream) const {
    char buffer[kBufferSize];
    T data;
    for (int index(minimum_index_); sptk::ReadStream(&data, input_stream);
         ++index) {
      if (!sptk::SnPrintf(data, print_format_, sizeof(buffer), buffer)) {
        return false;
      }
      std::cout << index << '\t' << buffer << std::endl;

      if (maximum_index_ != kMagicNumberForEndOfFile &&
          maximum_index_ == index) {
        index = minimum_index_ - 1;
      }
    }

    return true;
  }

 private:
  const std::string print_format_;
  const int minimum_index_;
  const int maximum_index_;

  DISALLOW_COPY_AND_ASSIGN(DataDump<T>);
};

class DataDumpWrapper {
 public:
  DataDumpWrapper(const std::string& data_type,
                  const std::string& given_print_format, int minimum_index,
                  int maximum_index)
      : data_dump_(NULL) {
    std::string print_format(given_print_format);
    if ("c" == data_type) {
      if (print_format.empty()) print_format = "%d";
      data_dump_ =
          new DataDump<int8_t>(print_format, minimum_index, maximum_index);
    } else if ("s" == data_type) {
      if (print_format.empty()) print_format = "%d";
      data_dump_ =
          new DataDump<int16_t>(print_format, minimum_index, maximum_index);
    } else if ("h" == data_type) {
      if (print_format.empty()) print_format = "%d";
      data_dump_ = new DataDump<sptk::int24_t>(print_format, minimum_index,
                                               maximum_index);
    } else if ("i" == data_type) {
      if (print_format.empty()) print_format = "%d";
      data_dump_ =
          new DataDump<int32_t>(print_format, minimum_index, maximum_index);
    } else if ("l" == data_type) {
      if (print_format.empty()) print_format = "%lld";
      data_dump_ =
          new DataDump<int64_t>(print_format, minimum_index, maximum_index);
    } else if ("C" == data_type) {
      if (print_format.empty()) print_format = "%u";
      data_dump_ =
          new DataDump<uint8_t>(print_format, minimum_index, maximum_index);
    } else if ("S" == data_type) {
      if (print_format.empty()) print_format = "%u";
      data_dump_ =
          new DataDump<uint16_t>(print_format, minimum_index, maximum_index);
    } else if ("H" == data_type) {
      if (print_format.empty()) print_format = "%u";
      data_dump_ = new DataDump<sptk::uint24_t>(print_format, minimum_index,
                                                maximum_index);
    } else if ("I" == data_type) {
      if (print_format.empty()) print_format = "%u";
      data_dump_ =
          new DataDump<uint32_t>(print_format, minimum_index, maximum_index);
    } else if ("L" == data_type) {
      if (print_format.empty()) print_format = "%llu";
      data_dump_ =
          new DataDump<uint64_t>(print_format, minimum_index, maximum_index);
    } else if ("f" == data_type) {
      if (print_format.empty()) print_format = "%g";
      data_dump_ =
          new DataDump<float>(print_format, minimum_index, maximum_index);
    } else if ("d" == data_type) {
      if (print_format.empty()) print_format = "%g";
      data_dump_ =
          new DataDump<double>(print_format, minimum_index, maximum_index);
    } else if ("e" == data_type) {
      if (print_format.empty()) print_format = "%Lg";
      data_dump_ =
          new DataDump<long double>(print_format, minimum_index, maximum_index);
    }
  }

  ~DataDumpWrapper() {
    delete data_dump_;
  }

  bool IsValid() const {
    return NULL != data_dump_;
  }

  bool Run(std::istream* input_stream) const {
    return IsValid() && data_dump_->Run(input_stream);
  }

 private:
  DataDumpInterface* data_dump_;

  DISALLOW_COPY_AND_ASSIGN(DataDumpWrapper);
};

}  // namespace

int main(int argc, char* argv[]) {
  int minimum_index(0);
  int maximum_index(kMagicNumberForEndOfFile);
  std::string print_format("");
  std::string data_type(kDefaultDataType);

  for (;;) {
    const int option_char(getopt_long(argc, argv, "l:m:f:h", NULL, NULL));
    if (-1 == option_char) break;

    switch (option_char) {
      case 'l': {
        if (!sptk::ConvertStringToInteger(optarg, &maximum_index) ||
            maximum_index <= 0) {
          std::ostringstream error_message;
          error_message
              << "The argument for the -l option must be a positive integer";
          sptk::PrintErrorMessage("dmp", error_message);
          return 1;
        }
        minimum_index = 1;
        break;
      }
      case 'm': {
        if (!sptk::ConvertStringToInteger(optarg, &maximum_index) ||
            maximum_index < 0) {
          std::ostringstream error_message;
          error_message << "The argument for the -m option must be a "
                        << "non-negative integer";
          sptk::PrintErrorMessage("dmp", error_message);
          return 1;
        }
        minimum_index = 0;
        break;
      }
      case 'f': {
        print_format = optarg;
        if ("%" != print_format.substr(0, 1)) {
          std::ostringstream error_message;
          error_message << "The argument for the -f option must be begin with "
                        << "%";
          sptk::PrintErrorMessage("dmp", error_message);
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
      sptk::PrintErrorMessage("dmp", error_message);
      return 1;
    }
  }

  // open stream
  std::ifstream ifs;
  ifs.open(input_file, std::ios::in | std::ios::binary);
  if (ifs.fail() && NULL != input_file) {
    std::ostringstream error_message;
    error_message << "Cannot open file " << input_file;
    sptk::PrintErrorMessage("dmp", error_message);
    return 1;
  }
  std::istream& input_stream(ifs.fail() ? std::cin : ifs);

  DataDumpWrapper data_dump(data_type, print_format, minimum_index,
                            maximum_index);

  if (!data_dump.IsValid()) {
    std::ostringstream error_message;
    error_message << "Unexpected argument for the +type option";
    sptk::PrintErrorMessage("dmp", error_message);
    return 1;
  }

  if (!data_dump.Run(&input_stream)) {
    std::ostringstream error_message;
    error_message << "Failed to dump";
    sptk::PrintErrorMessage("dmp", error_message);
    return 1;
  }

  return 0;
}
