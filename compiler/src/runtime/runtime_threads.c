#include "runtime_internal.h"

#include <errno.h>
#include <stdatomic.h>

typedef struct {
    CalyndaRtWord callable;
} CalyndaRtThreadStartData;

static void *calynda_rt_thread_trampoline(void *raw_data) {
    CalyndaRtThreadStartData *data = (CalyndaRtThreadStartData *)raw_data;
    CalyndaRtWord callable;
    RtFailureContext failure;

    if (!data) {
        rt_fatalf(CALYNDA_RT_EXIT_RUNTIME_ERROR,
                  "runtime: missing thread start data\n");
    }

    callable = data->callable;
    free(data);
    rt_failure_context_push(&failure);
    if (setjmp(failure.jump) == 0) {
        (void)calynda_rt_callable_dispatch(callable, NULL, 0);
    }
    rt_failure_context_pop(&failure);
    return NULL;
}

CalyndaRtWord __calynda_rt_thread_spawn(CalyndaRtWord callable) {
    CalyndaRtThreadStartData *start_data;
    CalyndaRtThread *thread_object;

    start_data = calloc(1, sizeof(*start_data));
    if (!start_data) {
        fprintf(stderr, "runtime: out of memory while creating thread start data\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }
    start_data->callable = callable;

    thread_object = rt_new_thread_object();
    if (!thread_object) {
        free(start_data);
        fprintf(stderr, "runtime: out of memory while creating thread object\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }

    if (pthread_create(&thread_object->thread,
                       NULL,
                       calynda_rt_thread_trampoline,
                       start_data) != 0) {
        free(start_data);
        fprintf(stderr, "runtime: pthread_create failed\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }

    return rt_make_object_word(thread_object);
}

void __calynda_rt_thread_join(CalyndaRtWord thread_handle) {
    CalyndaRtThread *thread_object =
        (CalyndaRtThread *)(void *)calynda_rt_as_object(thread_handle);

    if (!thread_object || thread_object->header.kind != CALYNDA_RT_OBJECT_THREAD) {
        fprintf(stderr, "runtime: attempted join on non-thread value\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }
    if (thread_object->joined) {
        rt_fatalf(CALYNDA_RT_EXIT_RUNTIME_ERROR,
                  "runtime: attempted to join a thread twice\n");
    }
    if (pthread_join(thread_object->thread, NULL) != 0) {
        fprintf(stderr, "runtime: pthread_join failed\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }
    thread_object->joined = true;
}

CalyndaRtWord __calynda_rt_mutex_new(void) {
    CalyndaRtMutex *mutex_object = rt_new_mutex_object();

    if (!mutex_object) {
        fprintf(stderr, "runtime: out of memory while creating mutex object\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }

    return rt_make_object_word(mutex_object);
}

void __calynda_rt_mutex_lock(CalyndaRtWord mutex_handle) {
    CalyndaRtMutex *mutex_object =
        (CalyndaRtMutex *)(void *)calynda_rt_as_object(mutex_handle);

    if (!mutex_object || mutex_object->header.kind != CALYNDA_RT_OBJECT_MUTEX) {
        fprintf(stderr, "runtime: attempted lock on non-mutex value\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }
    if (pthread_mutex_lock(&mutex_object->mutex) != 0) {
        fprintf(stderr, "runtime: pthread_mutex_lock failed\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }
}

void __calynda_rt_mutex_unlock(CalyndaRtWord mutex_handle) {
    CalyndaRtMutex *mutex_object =
        (CalyndaRtMutex *)(void *)calynda_rt_as_object(mutex_handle);

    if (!mutex_object || mutex_object->header.kind != CALYNDA_RT_OBJECT_MUTEX) {
        fprintf(stderr, "runtime: attempted unlock on non-mutex value\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }
    if (pthread_mutex_unlock(&mutex_object->mutex) != 0) {
        fprintf(stderr, "runtime: pthread_mutex_unlock failed\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
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
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
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
    RtFailureContext failure;

    if (!data) {
        rt_fatalf(CALYNDA_RT_EXIT_RUNTIME_ERROR,
                  "runtime: missing future start data\n");
    }

    callable = data->callable;
    future   = data->future;
    free(data);

    /*
     * pthread_join in future_get provides the happens-before edge that
     * makes the store to future->result visible to the joining thread.
     */
    rt_failure_context_push(&failure);
    if (setjmp(failure.jump) == 0) {
        future->result = calynda_rt_callable_dispatch(callable, NULL, 0);
    }
    rt_failure_context_pop(&failure);
    return NULL;
}

CalyndaRtWord __calynda_rt_future_spawn(CalyndaRtWord callable) {
    CalyndaRtFutureStartData *start_data;
    CalyndaRtFuture *future_object;

    start_data = calloc(1, sizeof(*start_data));
    if (!start_data) {
        fprintf(stderr, "runtime: out of memory while creating future start data\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }

    future_object = rt_new_future_object(callable);
    if (!future_object) {
        free(start_data);
        fprintf(stderr, "runtime: out of memory while creating future object\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }

    start_data->callable = callable;
    start_data->future   = future_object;

    if (pthread_create(&future_object->thread,
                       NULL,
                       calynda_rt_future_trampoline,
                       start_data) != 0) {
        free(start_data);
        fprintf(stderr, "runtime: pthread_create failed for future\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }

    return rt_make_object_word(future_object);
}

CalyndaRtWord __calynda_rt_future_get(CalyndaRtWord future_handle) {
    CalyndaRtFuture *future_object =
        (CalyndaRtFuture *)(void *)calynda_rt_as_object(future_handle);

    if (!future_object || future_object->header.kind != CALYNDA_RT_OBJECT_FUTURE) {
        fprintf(stderr, "runtime: attempted get on non-future value\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }
    if (!future_object->joined) {
        if (pthread_join(future_object->thread, NULL) != 0) {
            fprintf(stderr, "runtime: pthread_join failed for future get\n");
            rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
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
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
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

#include "runtime_threads_p2.inc"
