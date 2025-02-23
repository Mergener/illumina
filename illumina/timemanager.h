#ifndef ILLUMINA_TIMEMANAGER_H
#define ILLUMINA_TIMEMANAGER_H

#include "clock.h"
#include "searchdefs.h"
#include "tunablevalues.h"
#include "types.h"

namespace illumina {

struct PVResults;

class TimeManager {
public:
    void start_movetime(i64 movetime_ms);
    void start_normal(i64 our_time_ms,
                      i64 our_inc_ms,
                      i64 their_time_ms,
                      i64 their_inc_ms,
                      int moves_to_go = 0);
    void start_infinite();
    bool time_up_hard() const;
    bool time_up_soft() const;
    void on_pv_results(const PVResults& pv_results);
    TimeManager() = default;

private:
    TimePoint m_time_start {};
    enum {
        NORMAL,
        MOVETIME,
        INFINITE
    } m_mode;
    i64 m_hard;
    i64 m_soft;
};

} // illumina

#endif // ILLUMINA_TIMEMANAGER_H
