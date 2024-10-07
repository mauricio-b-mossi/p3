# Tips
- Remove leading foward slashes form Dirs.

# Wad format
- **Header**: 12 bytes (FIXED).
    - 4: Magic (ASCII) 
    - 4: Number of file descriptors. 
    - 4: Offset to file descriptor list.

- **Lump Data**: Not fixed, contains contents of files (DYNAMIC).

- **File Descriptor List** # of file descriptors * 16 (DYNAMIC). Each file descriptor is 16 bytes (FIXED).
    - 4: Content offset.
    - 4: Content length.
    - 8: File name (STRICT).

# Types of Files
- Content files.
- Directory files: There are only 2 types of directory files.
    - Map: Formatted "E\dM\d", always contain exactly 10 content files.
    - Namespace: 2 byte length, name is suffixed by "_START", children are enclosed in "name_START" "name_END".

> You can only create namespace dirs!

