cleanup:
    mr_free_manual_cleanup_state(&unit_ctx);
    if (!ok) {
        mr_unit_free(&unit);
    }
    return ok;
}
