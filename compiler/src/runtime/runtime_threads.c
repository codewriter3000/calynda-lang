#include "runtime_internal.h"

#include <errno.h>

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
