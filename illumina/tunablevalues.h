#ifndef ILLUMINA_TUNABLEVALUES_H
#define ILLUMINA_TUNABLEVALUES_H

namespace illumina {

//#define TUNING_BUILD

#ifdef TUNING_BUILD
#define TUNABLE_VALUE(name, type, value, ...) extern type name;
#else
#define TUNABLE_VALUE(name, type, value, ...) static constexpr type name = value;
#endif

#include "tunablevalues.def"

#undef TUNABLE_VALUE

} // illumina

#endif //ILLUMINA_TUNABLEVALUES_H
