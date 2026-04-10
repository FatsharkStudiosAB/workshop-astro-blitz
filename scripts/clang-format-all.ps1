# Run clang-format on all C source and header files in src/ and tests/.
Get-ChildItem -Recurse -Include '*.c','*.h' -Path src,tests | ForEach-Object {
    clang-format -i $_.FullName
}
