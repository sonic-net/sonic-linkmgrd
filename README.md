[![Total alerts](https://img.shields.io/lgtm/alerts/g/Azure/sonic-linkmgrd.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/Azure/sonic-linkmgrd/alerts/)

# Project


This repo tracks the source code of linkmgrd for dual-ToR topology.

# IPv6 Probing

- IPv4 remains preferred. Active-active ports use `soc_ipv6` only when
    `soc_ipv4` is absent. The shared endpoint selection also allows active-standby
    ports to use `server_ipv6` when `server_ipv4` is absent. IPv6 receive and
    switch-command unit tests cover active-standby; live validation is scoped to
    active-active ports.
- Configure non-unspecified Loopback2 and Loopback3 addresses in the selected
    family before creating probers. Missing selected-family loopbacks raise a
    configuration exception with an ERROR log before GUID generation, socket
    initialization, or hardware session creation.
- Restart linkmgrd after changing the selected address family, including
    removing `soc_ipv4` to select IPv6. Existing probers do not rebuild their
    socket filters, GUIDs, and hardware sessions for a family change. Correct
    incomplete loopback configuration before restarting linkmgrd as well.
- Normal probing intervals are family-specific. The decreased interval used
    to measure switchover overhead is intentionally shared by IPv4 and IPv6
    (10 ms by default), despite the legacy IPv4 name of its accessor.

# Coding style guide

Please follow the simple rules laid out below (will update as needed):

## Spacing
- Please configure your editor to enable expand white spaces. Do not use tab for spacing in source code.

- Please configure your editor to set tab stop at 4 spaces. And use 4 white spaces for each indentation level.

- Please leave one white space after following keywords:

```
    if (...)

    while (...)

    for (...)
```

## Curly braces
- Function definition: start a new line
```
void function(...)
{
    // code block
}
```

- if/for/while condition when condition is one liner: same line
```
if (...) {
    // code block
}
```

- if/for/while condition expands multiple lines: start a new line with ending ')' and '{'
```
if (...
    ...
) {
    // code block
}
```

## Indentations
- Please use 4 white spaces for each indentation level.

- Multiple line condition indentation for (if/while/for) and/or function calls: indent one level and the ')' indent at the same level of the keyword or function name
```
if (condition line 1 ..
    condition line 2 (indent to '(' above)
    condition line 3
) {
    // code block
}

foo(
    parameter1,
    parameter2,
    parameter3
);

```
