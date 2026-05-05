#include "runtime.h"
#include "runtime_internal.h"

static __thread CalyndaRtNlrSlot calynda_nlr_slots[CALYNDA_RT_NLR_MAX_DEPTH];
static __thread int calynda_nlr_depth = -1;

CalyndaRtNlrSlot *__calynda_rt_nlr_push(void) {
    ++calynda_nlr_depth;
    calynda_nlr_slots[calynda_nlr_depth].pending = false;
    calynda_nlr_slots[calynda_nlr_depth].value = 0;
    return &calynda_nlr_slots[calynda_nlr_depth];
}

void __calynda_rt_nlr_invoke(CalyndaRtNlrSlot *slot, CalyndaRtWord value) {
    if (slot) {
        slot->pending = true;
        slot->value = value;
    }
}

CalyndaRtWord __calynda_rt_nlr_check_pop(CalyndaRtNlrSlot *slot) {
    bool pending = slot ? slot->pending : false;
    --calynda_nlr_depth;
    return pending ? (CalyndaRtWord)1 : (CalyndaRtWord)0;
}

CalyndaRtWord __calynda_rt_nlr_get_value(CalyndaRtNlrSlot *slot) {
    return slot ? slot->value : (CalyndaRtWord)0;
}
