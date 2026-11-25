"""Expose the installed Common and libnl interfaces for the Linkmgrd daemon."""

load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")

def _linkmgrd_swsscommon_impl(ctx):
    # Common 0.0.9's public headers also carry static libnl archives. Use its
    # compile context for includes and its native shared provider for linking.
    return [CcInfo(
        compilation_context = ctx.attr.headers[CcInfo].compilation_context,
        linking_context = ctx.attr.library[CcInfo].linking_context,
    )]

linkmgrd_swsscommon = rule(
    implementation = _linkmgrd_swsscommon_impl,
    attrs = {
        "headers": attr.label(mandatory = True, providers = [CcInfo]),
        "library": attr.label(mandatory = True, providers = [CcInfo]),
    },
)
