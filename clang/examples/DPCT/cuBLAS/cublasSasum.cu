#include "cublas_v2.h"

void test(cublasHandle_t handle, int n, const float *x, int incx, float *res) {
  // Start
  cublasSasum(handle /*cublasHandle_t*/, n /*int*/, x /*const float **/,
              incx /*int*/, res /*float **/);
  // End
}
