cleanup:
    free((void *)elements);
    rt_gc_shutdown();
    rt_failure_context_pop(&failure);

    process_failure = rt_process_failure_code();
    rt_reset_process_failure();
    if (exit_code == 0 && process_failure != 0) {
        return process_failure;
    }
    return exit_code;
}
