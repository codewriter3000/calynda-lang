#include "runtime_internal.h"

#include <errno.h>
#include <stdatomic.h>

typedef struct {
    CalyndaRtWord callable;
} CalyndaRtThreadStartData;

static void *calynda_rt_thread_trampoline(void *raw_data) {
    CalyndaRtThreadStartData *data = (CalyndaRtThreadStartData *)raw_data;
    CalyndaRtWord callable;

    if (!data) {
        abort();
    }

    callable = data->callable;
    free(data);
    (void)calynda_rt_callable_dispatch(callable, NULL, 0);
    return NULL;
}

CalyndaRtWord __calynda_rt_thread_spawn(CalyndaRtWord callable) {
    CalyndaRtThreadStartData *start_data;
    CalyndaRtThread *thread_object;

    start_data = calloc(1, sizeof(*start_data));
    if (!start_data) {
        fprintf(stderr, "runtime: out of memory while creating thread start data\n");
        abort();
    }
    start_data->callable = callable;

    thread_object = rt_new_thread_object();
    if (!thread_object) {
        free(start_data);
        fprintf(stderr, "runtime: out of memory while creating thread object\n");
        abort();
    }

    if (pthread_create(&thread_object->thread,
                       NULL,
                       calynda_rt_thread_trampoline,
                       start_data) != 0) {
        free(start_data);
        fprintf(stderr, "runtime: pthread_create failed\n");
        abort();
    }

    return rt_make_object_word(thread_object);
}

void __calynda_rt_thread_join(CalyndaRtWord thread_handle) {
    CalyndaRtThread *thread_object =
        (CalyndaRtThread *)(void *)calynda_rt_as_object(thread_handle);

    if (!thread_object || thread_object->header.kind != CALYNDA_RT_OBJECT_THREAD) {
        fprintf(stderr, "runtime: attempted join on non-thread value\n");
        abort();
    }
    if (thread_object->joined) {
        abort();
    }
    if (pthread_join(thread_object->thread, NULL) != 0) {
        fprintf(stderr, "runtime: pthread_join failed\n");
        abort();
    }
    thread_object->joined = true;
}

CalyndaRtWord __calynda_rt_mutex_new(void) {
    CalyndaRtMutex *mutex_object = rt_new_mutex_object();

    if (!mutex_object) {
        fprintf(stderr, "runtime: out of memory while creating mutex object\n");
        abort();
    }

    return rt_make_object_word(mutex_object);
}

void __calynda_rt_mutex_lock(CalyndaRtWord mutex_handle) {
    CalyndaRtMutex *mutex_object =
        (CalyndaRtMutex *)(void *)calynda_rt_as_object(mutex_handle);

    if (!mutex_object || mutex_object->header.kind != CALYNDA_RT_OBJECT_MUTEX) {
        fprintf(stderr, "runtime: attempted lock on non-mutex value\n");
        abort();
    }
    if (pthread_mutex_lock(&mutex_object->mutex) != 0) {
        fprintf(stderr, "runtime: pthread_mutex_lock failed\n");
        abort();
    }
}

void __calynda_rt_mutex_unlock(CalyndaRtWord mutex_handle) {
    CalyndaRtMutex *mutex_object =
        (CalyndaRtMutex *)(void *)calynda_rt_as_object(mutex_handle);

    if (!mutex_object || mutex_object->header.kind != CALYNDA_RT_OBJECT_MUTEX) {
        fprintf(stderr, "runtime: attempted unlock on non-mutex value\n");
        abort();
    }
    if (pthread_mutex_unlock(&mutex_object->mutex) != 0) {
        fprintf(stderr, "runtime: pthread_mutex_unlock failed\n");
        abort();
    }
}

/* ------------------------------------------------------------------ */
/*  Thread cancel (alpha.2) — pthread cancellation semantics          */
/* ------------------------------------------------------------------ */

void __calynda_rt_thread_cancel(CalyndaRtWord thread_handle) {
    CalyndaRtThread *thread_object =
        (CalyndaRtThread *)(void *)calynda_rt_as_object(thread_handle);

    if (!thread_object || thread_object->header.kind != CALYNDA_RT_OBJECT_THREAD) {
        fprintf(stderr, "runtime: attempted cancel on non-thread value\n");
        abort();
    }
    if (thread_object->joined) {
        return;
    }
    (void)pthread_cancel(thread_object->thread);
    (void)pthread_join(thread_object->thread, NULL);
    thread_object->joined = true;
}

/* ------------------------------------------------------------------ */
/*  Future<T> (alpha.2)                                               */
/* ------------------------------------------------------------------ */

typedef struct {
    CalyndaRtWord   callable;
    CalyndaRtFuture *future;
} CalyndaRtFutureStartData;

static void *calynda_rt_future_trampoline(void *raw_data) {
    CalyndaRtFutureStartData *data = (CalyndaRtFutureStartData *)raw_data;
    CalyndaRtWord callable;
    CalyndaRtFuture *future;

    if (!data) {
        abort();
    }

    callable = data->callable;
    future   = data->future;
    free(data);

    /*
     * pthread_join in future_get provides the happens-before edge that
     * makes the store to future->result visible to the joining thread.
     */
    future->result = calynda_rt_callable_dispatch(callable, NULL, 0);
    return NULL;
}

CalyndaRtWord __calynda_rt_future_spawn(CalyndaRtWord callable) {
    CalyndaRtFutureStartData *start_data;
    CalyndaRtFuture *future_object;

    start_data = calloc(1, sizeof(*start_data));
    if (!start_data) {
        fprintf(stderr, "runtime: out of memory while creating future start data\n");
        abort();
    }

    future_object = rt_new_future_object(callable);
    if (!future_object) {
        free(start_data);
        fprintf(stderr, "runtime: out of memory while creating future object\n");
        abort();
    }

    start_data->callable = callable;
    start_data->future   = future_object;

    if (pthread_create(&future_object->thread,
                       NULL,
                       calynda_rt_future_trampoline,
                       start_data) != 0) {
        free(start_data);
        fprintf(stderr, "runtime: pthread_create failed for future\n");
        abort();
    }

    return rt_make_object_word(future_object);
}

CalyndaRtWord __calynda_rt_future_get(CalyndaRtWord future_handle) {
    CalyndaRtFuture *future_object =
        (CalyndaRtFuture *)(void *)calynda_rt_as_object(future_handle);

    if (!future_object || future_object->header.kind != CALYNDA_RT_OBJECT_FUTURE) {
        fprintf(stderr, "runtime: attempted get on non-future value\n");
        abort();
    }
    if (!future_object->joined) {
        if (pthread_join(future_object->thread, NULL) != 0) {
            fprintf(stderr, "runtime: pthread_join failed for future get\n");
            abort();
        }
        future_object->joined = true;
    }
    return future_object->result;
}

void __calynda_rt_future_cancel(CalyndaRtWord future_handle) {
    CalyndaRtFuture *future_object =
        (CalyndaRtFuture *)(void *)calynda_rt_as_object(future_handle);

    if (!future_object || future_object->header.kind != CALYNDA_RT_OBJECT_FUTURE) {
        fprintf(stderr, "runtime: attempted cancel on non-future value\n");
        abort();
    }
    if (future_object->joined) {
        return;
    }
    (void)pthread_cancel(future_object->thread);
    (void)pthread_join(future_object->thread, NULL);
    future_object->joined = true;
}

/* ------------------------------------------------------------------ */
/*  Atomic<T> (alpha.2) — sequentially-consistent single-word ops    */
/* ------------------------------------------------------------------ */

CalyndaRtWord __calynda_rt_atomic_new(CalyndaRtWord initial_value) {
    CalyndaRtAtomic *atomic_object = rt_new_atomic_object();

    if (!atomic_object) {
        fprintf(stderr, "runtime: out of memory while creating atomic object\n");
        abort();
    }
    atomic_store_explicit(&atomic_object->value,
                          initial_value,
                          memory_order_seq_cst);
    return rt_make_object_word(atomic_object);
}

CalyndaRtWord __calynda_rt_atomic_load(CalyndaRtWord atomic_handle) {
    CalyndaRtAtomic *atomic_object =
        (CalyndaRtAtomic *)(void *)calynda_rt_as_object(atomic_handle);

    if (!atomic_object || atomic_object->header.kind != CALYNDA_RT_OBJECT_ATOMIC) {
        fprintf(stderr, "runtime: attempted load on non-atomic value\n");
        abort();
    }
    return atomic_load_explicit(&atomic_object->value, memory_order_seq_cst);
}

void __calynda_rt_atomic_store(CalyndaRtWord atomic_handle,
                               CalyndaRtWord value) {
    CalyndaRtAtomic *atomic_object =
        (CalyndaRtAtomic *)(void *)calynda_rt_as_object(atomic_handle);

    if (!atomic_object || atomic_object->header.kind != CALYNDA_RT_OBJECT_ATOMIC) {
        fprintf(stderr, "runtime: attempted store on non-atomic value\n");
        abort();
    }
    atomic_store_explicit(&atomic_object->value, value, memory_order_seq_cst);
}

CalyndaRtWord __calynda_rt_atomic_exchange(CalyndaRtWord atomic_handle,
                                           CalyndaRtWord new_value) {
    CalyndaRtAtomic *atomic_object =
        (CalyndaRtAtomic *)(void *)calynda_rt_as_object(atomic_handle);

    if (!atomic_object || atomic_object->header.kind != CALYNDA_RT_OBJECT_ATOMIC) {
        fprintf(stderr, "runtime: attempted exchange on non-atomic value\n");
        abort();
    }
    return atomic_exchange_explicit(&atomic_object->value,
                                    new_value,
                                    memory_order_seq_cst);
}
