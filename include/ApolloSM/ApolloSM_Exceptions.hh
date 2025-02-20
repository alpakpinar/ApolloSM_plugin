
#ifndef __EXCEPTIONHANDLER_HPP__
#define __EXCEPTIONHANDLER_HPP__

#include <BUException/ExceptionBase.hh>

namespace BUException {
  ExceptionClassGenerator(IO_ERROR, "Read/write error,\n");
  ExceptionClassGenerator(FILE_ERROR, "File error.\n")
  ExceptionClassGenerator(JTAG_ERROR, "JTAG error.\n");
  ExceptionClassGenerator(EYESCAN_ERROR, "Eyescan error.\n");
  ExceptionClassGenerator(BAD_PARAMETER, "Bad Parameter.\n");
}

#endif
