[![Total alerts](https://img.shields.io/lgtm/alerts/g/Azure/sonic-linkmgrd.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/Azure/sonic-linkmgrd/alerts/)

# Project


This repo tracks the source code of linkmgrd for dual-ToR topology.

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
