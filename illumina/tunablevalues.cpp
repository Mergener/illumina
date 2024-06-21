#include "tunablevalues.h"

namespace illumina {

#ifdef TUNING_BUILD
#define TUNABLE_VALUE(name, type, value) type name = value;
#include "tunablevalues.def"
#endif

}