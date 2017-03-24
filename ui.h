
//void ui();

//begin tv code


#include "TVoutSPI.h"
void ui() {
  char *videoptr = usdc.TVBuffer;
  TVsetup();
  TVloop();
}
