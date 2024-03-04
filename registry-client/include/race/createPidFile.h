#ifndef __SOURCE_CREATE_PID_FILE_H__
#define __SOURCE_CREATE_PID_FILE_H__

#include <cstdint>

namespace rta {

/**
 * @brief Create a PID file to guarantee only one instance of the application is running.
 *
 * @return std::int32_t The file descriptor of the created PID file, or -1 on error.
 */
std::int32_t createPidFile();

}  // namespace rta

#endif
