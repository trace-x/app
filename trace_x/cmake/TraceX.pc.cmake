libdir=@LIB_INSTALL_DIR@
includedir=@INCLUDE_INSTALL_ROOT_DIR@

Name: Yaml-cpp
Description: TraceX library
Version: @TRACE_X_VERSION@
Requires:
Libs: -L${libdir} -ltrace_x
Cflags: -I${includedir}
