#include <math.h>
#include <natives.h>
#include <util.h>

// TODO read the properties from the VM instead of hardcoding them
DECLARE_NATIVE("java/lang", System, initProperties,
               "(Ljava/util/Properties;)Ljava/util/Properties;") {
  bjvm_obj_header *props_obj = args[0].obj;
  const wchar_t *const props[][2] = {
      {L"file.encoding", L"UTF-8"},   {L"stdout.encoding", L"UTF-8"},
      {L"native.encoding", L"UTF-8"}, {L"stderr.encoding", L"UTF-8"},
      {L"line.separator", L"\n"},     {L"path.separator", L":"},
      {L"file.separator", L"/"}};
  bjvm_cp_method *put = bjvm_easy_method_lookup(
      props_obj->descriptor, "put",
      "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;", true, false);
  for (size_t i = 0; i < sizeof(props) / sizeof(props[0]); ++i) {
    bjvm_stack_value put_args[3] = {
        {.obj = props_obj},
        {.obj = bjvm_intern_string(thread, props[i][0], wcslen(props[i][0]))},
        {.obj = bjvm_intern_string(thread, props[i][1], wcslen(props[i][1]))}};
    bjvm_stack_value result;
    // call put() with String key and value
    bjvm_thread_run(thread, put, put_args, &result);
  }
  return value_null();
}

DECLARE_NATIVE("java/lang", System, mapLibraryName,
               "(Ljava/lang/String;)Ljava/lang/String;") {
  return args[0];
}

DECLARE_NATIVE("java/lang", System, arraycopy,
               "(Ljava/lang/Object;ILjava/lang/Object;II)V") {
  assert(argc == 5);
  bjvm_obj_header *src = args[0].obj;
  bjvm_obj_header *dest = args[2].obj;
  if (src == nullptr || dest == nullptr) {
    ThrowLangException(NullPointerException);
    return value_null();
  }
  bool src_not_array = src->descriptor->kind == BJVM_CD_KIND_ORDINARY;
  if (src_not_array || dest->descriptor->kind == BJVM_CD_KIND_ORDINARY) {
    // Can't copy non-array objects to each other
    ThrowLangException(ArrayStoreException);
    return value_null();
  }
  bool src_is_1d_primitive = Is1DPrimitiveArray(src),
       dst_is_1d_primitive = Is1DPrimitiveArray(dest);
  if (src_is_1d_primitive != dst_is_1d_primitive ||
      (src_is_1d_primitive && src->descriptor->primitive_component !=
                                  dest->descriptor->primitive_component)) {
    ThrowLangException(ArrayStoreException);
    return value_null();
  }

  int src_pos = args[1].i;
  int dest_pos = args[3].i;
  int length = args[4].i;
  int src_length = *array_length(src);
  int dest_length = *array_length(dest);
  // Verify that everything is in bounds
  // TODO add more descriptive error messages
  if (src_pos < 0 || dest_pos < 0 || length < 0 ||
      (int64_t)src_pos + length > src_length ||
      (int64_t)dest_pos + length > dest_length) {
    ThrowLangException(ArrayIndexOutOfBoundsException);
    return value_null();
  }

  // We can copy primitive arrays directly.
  // For reference arrays, if the component type of the src class is an
  // instanceof the destination class, then we don't need to perform any checks.
  // Otherwise, we need to perform an instanceof check on each element and raise
  // an ArrayStoreException as appropriate.
  if (src_is_1d_primitive || bjvm_instanceof(src->descriptor->one_fewer_dim,
                                             dest->descriptor->one_fewer_dim)) {
    size_t element_size;

    switch (src->descriptor->primitive_component) {
#define CASE(type, underlying)                                                 \
  case BJVM_TYPE_KIND_##type:                                                  \
    element_size = sizeof(underlying);                                         \
    break;
      CASE(BYTE, int8_t)
      CASE(CHAR, uint16_t)
      CASE(DOUBLE, double)
      CASE(FLOAT, float)
      CASE(INT, int32_t)
      CASE(LONG, int64_t)
      CASE(SHORT, int16_t)
      CASE(BOOLEAN, uint8_t)
      CASE(REFERENCE, bjvm_obj_header *)
#undef CASE

    default:
      UNREACHABLE();
    }

    memmove((char *)array_data(dest) + dest_pos * element_size,
            (char *)array_data(src) + src_pos * element_size,
            length * element_size);

    return value_null();
  }

  for (int i = 0; i < length; ++i) {
    // may-alias case handled above
    bjvm_obj_header *src_elem =
        ((bjvm_obj_header **)array_data(src))[src_pos + i];
    if (src_elem && !bjvm_instanceof(src_elem->descriptor,
                                     dest->descriptor->one_fewer_dim)) {
      ThrowLangException(ArrayStoreException);
      return value_null();
    }
    ((bjvm_obj_header **)array_data(dest))[dest_pos + i] = src_elem;
  }

  return value_null();
}