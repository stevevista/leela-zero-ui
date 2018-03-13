#include "gtp_choice.h"

string GtpChoice::version() const {
    return switch_ == 0 ? gtp_blt.version() : gtp_proc.version();
}

