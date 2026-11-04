UsbDk API headers from daynix/UsbDk tag v1.00-22, commit 84752c3.
Copyright Red Hat, Inc.; developed by Daynix Computing LTD.
Licensed under Apache-2.0; see LICENSE and the notices retained in each header.
Upstream: https://github.com/daynix/UsbDk/tree/v1.00-22

NimBox modification: the two wcsncpy_s calls in the unused UsbDkFillIDStruct
inline helper use the explicit four-argument C signature with _TRUNCATE so the
header compiles as C17. The ABI structures and exported declarations are unchanged.
No UsbDk binaries are included in this source tree. Use the official signed release.
