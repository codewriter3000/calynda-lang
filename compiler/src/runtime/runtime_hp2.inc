CalyndaRtWord __calynda_rt_hetero_array_new(const CalyndaRtTypeDescriptor *type_desc,
                                            size_t element_count,
                                            const CalyndaRtWord *elements);
CalyndaRtTypeTag __calynda_rt_hetero_array_get_tag(CalyndaRtWord target, CalyndaRtWord index);
CalyndaRtWord __calynda_rt_hetero_array_append(CalyndaRtWord arr, CalyndaRtWord element);
CalyndaRtWord __calynda_rt_hetero_array_prepend(CalyndaRtWord element, CalyndaRtWord arr);
CalyndaRtWord __calynda_rt_hetero_array_cdr(CalyndaRtWord arr);

CalyndaRtWord __calynda_rt_thread_spawn(CalyndaRtWord callable);
void          __calynda_rt_thread_join(CalyndaRtWord thread_handle);
void          __calynda_rt_thread_cancel(CalyndaRtWord thread_handle);
CalyndaRtWord __calynda_rt_mutex_new(void);
void          __calynda_rt_mutex_lock(CalyndaRtWord mutex_handle);
void          __calynda_rt_mutex_unlock(CalyndaRtWord mutex_handle);

/* Future helpers (alpha.2): spawn returns Future object, get joins and
   returns the stored result, cancel calls pthread_cancel + join. */
CalyndaRtWord __calynda_rt_future_spawn(CalyndaRtWord callable);
CalyndaRtWord __calynda_rt_future_get(CalyndaRtWord future_handle);
void          __calynda_rt_future_cancel(CalyndaRtWord future_handle);

/* Atomic helpers (alpha.2): single-word SC atomic operations. */
CalyndaRtWord __calynda_rt_atomic_new(CalyndaRtWord initial_value);
CalyndaRtWord __calynda_rt_atomic_load(CalyndaRtWord atomic_handle);
void          __calynda_rt_atomic_store(CalyndaRtWord atomic_handle,
                                        CalyndaRtWord value);
CalyndaRtWord __calynda_rt_atomic_exchange(CalyndaRtWord atomic_handle,
                                           CalyndaRtWord new_value);

CalyndaRtWord __calynda_typeof(CalyndaRtWord value, CalyndaRtWord type_text);
CalyndaRtWord __calynda_isint(CalyndaRtWord value, CalyndaRtWord type_text);
CalyndaRtWord __calynda_isfloat(CalyndaRtWord value, CalyndaRtWord type_text);
CalyndaRtWord __calynda_isbool(CalyndaRtWord value, CalyndaRtWord type_text);
CalyndaRtWord __calynda_isstring(CalyndaRtWord value, CalyndaRtWord type_text);
CalyndaRtWord __calynda_isarray(CalyndaRtWord value, CalyndaRtWord type_text);
CalyndaRtWord __calynda_issametype(CalyndaRtWord left_value,
                                   CalyndaRtWord left_type_text,
                                   CalyndaRtWord right_value,
                                   CalyndaRtWord right_type_text);

/* Manual memory pointer operations */
CalyndaRtWord __calynda_deref(CalyndaRtWord ptr);
CalyndaRtWord __calynda_deref_sized(CalyndaRtWord ptr, CalyndaRtWord size);
CalyndaRtWord __calynda_addr(CalyndaRtWord value);
CalyndaRtWord __calynda_offset(CalyndaRtWord ptr, CalyndaRtWord count);
CalyndaRtWord __calynda_offset_stride(CalyndaRtWord ptr, CalyndaRtWord count,
                                      CalyndaRtWord stride);
void          __calynda_store(CalyndaRtWord ptr, CalyndaRtWord value);
void          __calynda_store_sized(CalyndaRtWord ptr, CalyndaRtWord value,
                                    CalyndaRtWord size);
/* stackalloc() returns scratch storage whose lifetime ends with the enclosing
    manual scope once compiler-inserted cleanup runs. */
CalyndaRtWord __calynda_stackalloc(CalyndaRtWord size);

/* Bounds-checked memory operations (manual checked {} mode). */
CalyndaRtWord __calynda_bc_malloc(CalyndaRtWord size);
CalyndaRtWord __calynda_bc_calloc(CalyndaRtWord n, CalyndaRtWord size);
CalyndaRtWord __calynda_bc_realloc(CalyndaRtWord ptr, CalyndaRtWord new_size);
void          __calynda_bc_free(CalyndaRtWord ptr);
CalyndaRtWord __calynda_bc_stackalloc(CalyndaRtWord size);
CalyndaRtWord __calynda_bc_deref(CalyndaRtWord ptr);
void          __calynda_bc_store(CalyndaRtWord ptr, CalyndaRtWord value);
CalyndaRtWord __calynda_bc_offset(CalyndaRtWord ptr, CalyndaRtWord idx,
                                  CalyndaRtWord elem_size);

/* Non-local return (NLR) slot — used by |var block parameters */
typedef struct {
    bool           pending;
    CalyndaRtWord  value;
} CalyndaRtNlrSlot;

#define CALYNDA_RT_NLR_MAX_DEPTH 64

/* Push a fresh NLR slot onto the thread-local stack and return a pointer to it. */
CalyndaRtNlrSlot *__calynda_rt_nlr_push(void);
/* Mark the slot as triggered with the given return value. */
void              __calynda_rt_nlr_invoke(CalyndaRtNlrSlot *slot, CalyndaRtWord value);
/* Pop the slot; returns 1 if NLR was triggered, 0 otherwise. */
CalyndaRtWord     __calynda_rt_nlr_check_pop(CalyndaRtNlrSlot *slot);
/* Retrieve the stored return value from the slot. */
CalyndaRtWord     __calynda_rt_nlr_get_value(CalyndaRtNlrSlot *slot);

