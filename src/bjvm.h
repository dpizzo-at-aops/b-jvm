//
// Created by Cowpox on 12/10/24.
//

#ifndef BJVM_H
#define BJVM_H

#include <stdint.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Instruction code. Similar instructions like aload_0 are canonicalised to
 * aload with an argument of 0.
 *
 * List of canonicalisations:
 *   aload_<n> -> aload, dload_<n> -> dload, fload_<n> -> fload, iload_<n> ->
 * iload, lload_<n> -> lload, astore_<n> -> astore, dstore_<n> -> dstore,
 * fstore_<n> -> fstore, istore_<n> -> istore, lstore_<n> -> lstore, bipush,
 * sipush, iconst_<n>, iconst_<n> -> iconst, dconst_<d> -> dconst, fconst_<f> ->
 * fconst
 */
typedef enum {
  /** No operands */
  bjvm_bc_insn_nop,

  bjvm_bc_insn_aaload,
  bjvm_bc_insn_aastore,
  bjvm_bc_insn_aconst_null,
  bjvm_bc_insn_areturn,
  bjvm_bc_insn_arraylength,
  bjvm_bc_insn_athrow,
  bjvm_bc_insn_baload,
  bjvm_bc_insn_bastore,
  bjvm_bc_insn_caload,
  bjvm_bc_insn_castore,
  bjvm_bc_insn_d2f,
  bjvm_bc_insn_d2i,
  bjvm_bc_insn_d2l,
  bjvm_bc_insn_dadd,
  bjvm_bc_insn_daload,
  bjvm_bc_insn_dastore,
  bjvm_bc_insn_dcmpg,
  bjvm_bc_insn_dcmpl,
  bjvm_bc_insn_ddiv,
  bjvm_bc_insn_dmul,
  bjvm_bc_insn_dneg,
  bjvm_bc_insn_drem,
  bjvm_bc_insn_dreturn,
  bjvm_bc_insn_dsub,
  bjvm_bc_insn_dup,
  bjvm_bc_insn_dup_x1,
  bjvm_bc_insn_dup_x2,
  bjvm_bc_insn_dup2,
  bjvm_bc_insn_dup2_x1,
  bjvm_bc_insn_dup2_x2,
  bjvm_bc_insn_f2d,
  bjvm_bc_insn_f2i,
  bjvm_bc_insn_f2l,
  bjvm_bc_insn_fadd,
  bjvm_bc_insn_faload,
  bjvm_bc_insn_fastore,
  bjvm_bc_insn_fcmpg,
  bjvm_bc_insn_fcmpl,
  bjvm_bc_insn_fdiv,
  bjvm_bc_insn_fmul,
  bjvm_bc_insn_fneg,
  bjvm_bc_insn_frem,
  bjvm_bc_insn_freturn,
  bjvm_bc_insn_fsub,
  bjvm_bc_insn_i2b,
  bjvm_bc_insn_i2c,
  bjvm_bc_insn_i2d,
  bjvm_bc_insn_i2f,
  bjvm_bc_insn_i2l,
  bjvm_bc_insn_i2s,
  bjvm_bc_insn_iadd,
  bjvm_bc_insn_iaload,
  bjvm_bc_insn_iand,
  bjvm_bc_insn_iastore,
  bjvm_bc_insn_idiv,
  bjvm_bc_insn_imul,
  bjvm_bc_insn_ineg,
  bjvm_bc_insn_ior,
  bjvm_bc_insn_irem,
  bjvm_bc_insn_ireturn,
  bjvm_bc_insn_ishl,
  bjvm_bc_insn_ishr,
  bjvm_bc_insn_isub,
  bjvm_bc_insn_iushr,
  bjvm_bc_insn_ixor,
  bjvm_bc_insn_l2d,
  bjvm_bc_insn_l2f,
  bjvm_bc_insn_l2i,
  bjvm_bc_insn_ladd,
  bjvm_bc_insn_laload,
  bjvm_bc_insn_land,
  bjvm_bc_insn_lastore,
  bjvm_bc_insn_lcmp,
  bjvm_bc_insn_ldiv,
  bjvm_bc_insn_lmul,
  bjvm_bc_insn_lneg,
  bjvm_bc_insn_lor,
  bjvm_bc_insn_lrem,
  bjvm_bc_insn_lreturn,
  bjvm_bc_insn_lshl,
  bjvm_bc_insn_lshr,
  bjvm_bc_insn_lsub,
  bjvm_bc_insn_lushr,
  bjvm_bc_insn_lxor,
  bjvm_bc_insn_monitorenter,
  bjvm_bc_insn_monitorexit,
  bjvm_bc_insn_pop,
  bjvm_bc_insn_pop2,
  bjvm_bc_insn_return,
  bjvm_bc_insn_saload,
  bjvm_bc_insn_sastore,
  bjvm_bc_insn_swap,

  /** Indexes into constant pool */
  bjvm_bc_insn_anewarray,
  bjvm_bc_insn_checkcast,
  bjvm_bc_insn_getfield,
  bjvm_bc_insn_getstatic,
  bjvm_bc_insn_instanceof,
  bjvm_bc_insn_invokedynamic,
  bjvm_bc_insn_new,
  bjvm_bc_insn_putfield,
  bjvm_bc_insn_putstatic,
  bjvm_bc_insn_invokevirtual,
  bjvm_bc_insn_invokespecial,
  bjvm_bc_insn_invokestatic,
  bjvm_bc_insn_ldc,
  bjvm_bc_insn_ldc2_w,

  /** Indexes into local variable table */
  bjvm_bc_insn_dload,
  bjvm_bc_insn_fload,
  bjvm_bc_insn_iload,
  bjvm_bc_insn_lload,
  bjvm_bc_insn_dstore,
  bjvm_bc_insn_fstore,
  bjvm_bc_insn_istore,
  bjvm_bc_insn_lstore,
  bjvm_bc_insn_aload,
  bjvm_bc_insn_astore,

  /** Indexes into instruction table */
  bjvm_bc_insn_goto,
  bjvm_bc_insn_jsr,

  bjvm_bc_insn_if_acmpeq,
  bjvm_bc_insn_if_acmpne,
  bjvm_bc_insn_if_icmpeq,
  bjvm_bc_insn_if_icmpne,
  bjvm_bc_insn_if_icmplt,
  bjvm_bc_insn_if_icmpge,
  bjvm_bc_insn_if_icmpgt,
  bjvm_bc_insn_if_icmple,
  bjvm_bc_insn_ifeq,
  bjvm_bc_insn_ifne,
  bjvm_bc_insn_iflt,
  bjvm_bc_insn_ifge,
  bjvm_bc_insn_ifgt,
  bjvm_bc_insn_ifle,
  bjvm_bc_insn_ifnonnull,
  bjvm_bc_insn_ifnull,

  /** Has some numerical immediate */
  bjvm_bc_insn_iconst,
  bjvm_bc_insn_dconst,
  bjvm_bc_insn_fconst,
  bjvm_bc_insn_lconst,

  /** Cursed */
  bjvm_bc_insn_iinc,
  bjvm_bc_insn_invokeinterface,
  bjvm_bc_insn_multianewarray,
  bjvm_bc_insn_newarray,
  bjvm_bc_insn_tableswitch,
  bjvm_bc_insn_lookupswitch,
  bjvm_bc_insn_ret
} bjvm_insn_code_kind;

typedef enum {
  BJVM_TYPE_KIND_BOOLEAN = 4,
  BJVM_TYPE_KIND_CHAR = 5,
  BJVM_TYPE_KIND_FLOAT = 6,
  BJVM_TYPE_KIND_DOUBLE = 7,
  BJVM_TYPE_KIND_BYTE = 8,
  BJVM_TYPE_KIND_SHORT = 9,
  BJVM_TYPE_KIND_INT = 10,
  BJVM_TYPE_KIND_LONG = 11,
  BJVM_TYPE_KIND_VOID = 12,

  BJVM_TYPE_KIND_REFERENCE = 13,
  BJVM_TYPE_KIND_RETURN_ADDRESS = 14 // used by jsr/jsr_w
} bjvm_type_kind;

struct bjvm_bc_tableswitch_data {
  int default_target;
  int *targets;
  int targets_count;

  int low, high;
};

struct bjvm_bc_lookupswitch_data {
  int default_target;
  int *targets;
  int targets_count;

  int *keys;
  int keys_count;
};

struct bjvm_bc_iinc_data {
  uint16_t index;
  int16_t const_;
};

struct bjvm_bc_invokeinterface_data {
  uint16_t index;
  uint8_t count;
};

typedef struct {
  wchar_t *chars;
  int len;
} bjvm_utf8;

typedef struct {
  bjvm_utf8 *name;
} bjvm_cp_class_info;

typedef struct bjvm_cp_name_and_type {
  bjvm_utf8 *name;
  bjvm_utf8 *descriptor;
} bjvm_cp_name_and_type;

typedef struct {
  bjvm_type_kind kind;
  // Can be nonzero for any kind
  int dimensions;
  bjvm_utf8 class_name; // For reference and array types only
} bjvm_field_descriptor;

bool bjvm_is_field_wide(bjvm_field_descriptor desc);

typedef struct {
  bjvm_cp_class_info *class_info;
  bjvm_cp_name_and_type *name_and_type;

  bjvm_field_descriptor *parsed_descriptor;
} bjvm_cp_field_info;

typedef struct bjvm_method_descriptor bjvm_method_descriptor;

// Used by both methodref and interface methodref
typedef struct {
  bjvm_cp_class_info *class_info;
  bjvm_cp_name_and_type *name_and_type;
  bjvm_method_descriptor *method_descriptor;
} bjvm_cp_method_info;

typedef struct {
  bjvm_utf8 *chars;
} bjvm_cp_string_info;

typedef struct {
  // Sign extended if original entry was an Integer
  int64_t value;
} bjvm_cp_integral_info;

typedef struct {
  // Extended if original entry was a Float
  double value;
} bjvm_cp_floating_info;

typedef enum {
  BJVM_MH_KIND_GET_FIELD = 1,
  BJVM_MH_KIND_GET_STATIC = 2,
  BJVM_MH_KIND_PUT_FIELD = 3,
  BJVM_MH_KIND_PUT_STATIC = 4,
  BJVM_MH_KIND_INVOKE_VIRTUAL = 5,
  BJVM_MH_KIND_INVOKE_STATIC = 6,
  BJVM_MH_KIND_INVOKE_SPECIAL = 7,
  BJVM_MH_KIND_NEW_INVOKE_SPECIAL = 8,
  BJVM_MH_KIND_INVOKE_INTERFACE = 9
} bjvm_method_handle_kind;

typedef struct {
  bjvm_method_handle_kind handle_kind;
  uint16_t reference_index; // TODO
} bjvm_cp_method_handle_info;

typedef struct {
  bjvm_utf8 *descriptor;
} bjvm_cp_method_type_info;

typedef struct {
  uint16_t bootstrap_method_attr_index; // TODO convert to pointer
  bjvm_cp_name_and_type *name_and_type;
  bjvm_method_descriptor *method_descriptor;
} bjvm_cp_invoke_dynamic_info;

typedef enum {
  BJVM_CP_KIND_INVALID = 0,
  BJVM_CP_KIND_UTF8 = 1 << 0,
  BJVM_CP_KIND_INTEGER = 1 << 1,
  BJVM_CP_KIND_FLOAT = 1 << 2,
  BJVM_CP_KIND_LONG = 1 << 3,
  BJVM_CP_KIND_DOUBLE = 1 << 4,
  BJVM_CP_KIND_CLASS = 1 << 5,
  BJVM_CP_KIND_STRING = 1 << 6,
  BJVM_CP_KIND_FIELD_REF = 1 << 7,
  BJVM_CP_KIND_METHOD_REF = 1 << 8,
  BJVM_CP_KIND_INTERFACE_METHOD_REF = 1 << 9,
  BJVM_CP_KIND_NAME_AND_TYPE = 1 << 10,
  BJVM_CP_KIND_METHOD_HANDLE = 1 << 11,
  BJVM_CP_KIND_METHOD_TYPE = 1 << 12,
  BJVM_CP_KIND_INVOKE_DYNAMIC = 1 << 13,
} bjvm_cp_entry_kind;

typedef struct bjvm_cp_entry {
  bjvm_cp_entry_kind kind;
  // Index of this entry within the constant pool
  int my_index;

  union {
    bjvm_utf8 utf8;
    bjvm_cp_string_info string;

    bjvm_cp_floating_info floating;
    bjvm_cp_integral_info integral;

    bjvm_cp_name_and_type name_and_type;
    bjvm_cp_class_info class_info;

    bjvm_cp_field_info fieldref_info;
    bjvm_cp_method_info methodref;
    bjvm_cp_method_handle_info method_handle_info;
    bjvm_cp_method_type_info method_type_info;
    bjvm_cp_invoke_dynamic_info indy_info;
  };
} bjvm_cp_entry;

struct bjvm_multianewarray_data {
  bjvm_cp_class_info *entry;
  uint8_t dimensions;
};

typedef struct {
  bjvm_insn_code_kind kind;
  int program_counter;

  union {
    // for newarray
    bjvm_type_kind array_type;
    // constant pool index or local variable index or branch target (instruction
    // index)
    uint32_t index;
    // Integer or long immediate
    int64_t integer_imm;
    // Float immediate
    float f_imm;
    // Double immediate
    double d_imm;
    // lookupswitch
    struct bjvm_bc_lookupswitch_data lookupswitch;
    // tableswitch
    struct bjvm_bc_tableswitch_data tableswitch;
    // iinc
    struct bjvm_bc_iinc_data iinc;
    // invoke interface
    struct bjvm_bc_invokeinterface_data invokeinterface;
    // multianewarray
    struct bjvm_multianewarray_data multianewarray;
    // non-owned pointer into the constant pool
    bjvm_cp_entry *cp;
  };
} bjvm_bytecode_insn;

typedef struct bjvm_constant_pool {
  int entries_len;
  bjvm_cp_entry entries[];
} bjvm_constant_pool;

typedef enum {
  BJVM_ACCESS_PUBLIC = 0x0001,
  BJVM_ACCESS_PRIVATE = 0x0002,
  BJVM_ACCESS_PROTECTED = 0x0004,
  BJVM_ACCESS_STATIC = 0x0008,
  BJVM_ACCESS_FINAL = 0x0010,
  BJVM_ACCESS_SYNCHRONIZED = 0x0020,
  BJVM_ACCESS_BRIDGE = 0x0040,
  BJVM_ACCESS_VARARGS = 0x0080,
  BJVM_ACCESS_NATIVE = 0x0100,
  BJVM_ACCESS_INTERFACE = 0x0200,
  BJVM_ACCESS_ABSTRACT = 0x0400,
  BJVM_ACCESS_STRICT = 0x0800,
  BJVM_ACCESS_SYNTHETIC = 0x1000,
  BJVM_ACCESS_ANNOTATION = 0x2000,
  BJVM_ACCESS_ENUM = 0x4000
} bjvm_access_flags;

typedef enum {
  BJVM_ATTRIBUTE_KIND_CODE = 0,
  BJVM_ATTRIBUTE_KIND_CONSTANT_VALUE = 1,
  BJVM_ATTRIBUTE_KIND_UNKNOWN = 2,
} bjvm_attribute_kind;

typedef struct bjvm_method_descriptor {
  bjvm_field_descriptor *args;
  int args_count;
  int args_cap;
  bjvm_field_descriptor return_type;
} bjvm_method_descriptor;

typedef struct {
  int start_insn;
  int end_insn;
  int handler_pc;
  bjvm_cp_class_info *catch_type;
} bjvm_exception_table_entry;

typedef struct {
  bjvm_exception_table_entry *entries;
  int entries_count;
} bjvm_attribute_exception_table;

typedef struct bjvm_attribute bjvm_attribute;

typedef struct {
  uint16_t max_stack;
  uint16_t max_locals;
  int insn_count;
  int max_formal_pc;

  bjvm_bytecode_insn *code;
  bjvm_attribute_exception_table *exception_table;

  bjvm_attribute *attributes;
  int attributes_count;
} bjvm_attribute_code;

typedef struct bjvm_attribute {
  bjvm_attribute_kind kind;
  bjvm_utf8 *name;
  uint32_t length;

  union {
    bjvm_attribute_code code;
    bjvm_cp_entry *constant_value;
  };
} bjvm_attribute;

typedef struct {
  union {
    // Used if the number of bits in the local variable table/stack is less than
    // 64
    uint64_t bits_inl;
    struct {
      uint64_t *bits;
      uint32_t size_words;
    } ptr;
  };
} bjvm_compressed_bitset;

// Result of the analysis of a code segment. During analysis, stack operations
// on longs/doubles are simplified as if they only took up one stack slot (e.g.,
// pop2 on a double becomes a pop, while pop2 on two ints stays as a pop2).
// Also, we progressively resolve the state of the stack and local variable
// table at each program counter, and store a bitset of which stack/local
// variables are references, so that the GC can follow them.
typedef struct {
  bjvm_compressed_bitset *insn_index_to_references;
  int insn_count;
} bjvm_code_analysis;

typedef struct bjvm_cp_method {
  bjvm_access_flags access_flags;

  bjvm_utf8 *name;
  bjvm_utf8 *descriptor;

  bjvm_method_descriptor *parsed_descriptor;
  bjvm_code_analysis *code_analysis;

  int attributes_count;
  bjvm_attribute *attributes;
  bjvm_attribute_code *code;

  bool is_signature_polymorphic;
} bjvm_cp_method;

typedef struct {
  bjvm_access_flags access_flags;
  bjvm_utf8 *name;
  bjvm_utf8 *descriptor;

  int attributes_count;
  bjvm_attribute *attributes;
  // Offset of the field in the static or instance data area
  int byte_offset;
} bjvm_cp_field;

typedef enum {
  BJVM_CLASSDESC_KIND_ORDINARY_ARRAY = 0,
  BJVM_CLASSDESC_KIND_BYTE_ARRAY = 1,
  BJVM_CLASSDESC_KIND_CHAR_ARRAY = 2,
  BJVM_CLASSDESC_KIND_DOUBLE_ARRAY = 3,
  BJVM_CLASSDESC_KIND_FLOAT_ARRAY = 4,
  BJVM_CLASSDESC_KIND_INT_ARRAY = 5,
  BJVM_CLASSDESC_KIND_LONG_ARRAY = 6,
  BJVM_CLASSDESC_KIND_SHORT_ARRAY = 7,
  BJVM_CLASSDESC_KIND_BOOLEAN_ARRAY = 8,
  BJVM_CLASSDESC_KIND_ORDINARY = 10,
} bjvm_classdesc_kind;

typedef struct bjvm_array_classdesc bjvm_array_classdesc;

typedef struct {
  bjvm_classdesc_kind kind;
  bjvm_constant_pool *pool;

  bjvm_access_flags access_flags;
  bjvm_cp_class_info *this_class;
  bjvm_cp_class_info *super_class;

  int interfaces_count;
  bjvm_cp_class_info **interfaces;

  int fields_count;
  bjvm_cp_field *fields;

  int methods_count;
  bjvm_cp_method *methods;

  int attributes_count;
  bjvm_attribute *attributes;

  bjvm_array_classdesc *array_type;

  // Whether this class corresponds to the primordial object class
  bool is_primordial_object;
  uint16_t minor_version;
  uint16_t major_version;
} bjvm_classdesc;

typedef uint64_t bjvm_mark_word_t;

typedef struct bjvm_ordinary_class bjvm_ordinary_class;

typedef struct bjvm_array_classdesc {
  bjvm_classdesc base;
  int dimensions;
  bjvm_classdesc *base_component;
} bjvm_array_classdesc;

typedef struct bjvm_primitive_array_classdesc {
  bjvm_classdesc base;
  int dimensions;
  bjvm_type_kind element_type;
} bjvm_primitive_array_classdesc;

// Equivalent to HotSpot's InstanceKlass
typedef struct bjvm_ordinary_class {
  bjvm_classdesc base;
  bjvm_classdesc classfile;
} bjvm_ordinary_classdesc;

// Appears at the top of every object -- corresponds to HotSpot's oopDesc
typedef struct {
  volatile bjvm_mark_word_t mark_word;
  bjvm_classdesc *descriptor;
} bjvm_obj_header;

struct bjvm_class_loader;
typedef struct bjvm_vm bjvm_vm;

typedef struct bjvm_hash_table_entry {
  struct bjvm_hash_table_entry *next;
  wchar_t *key;
  uint32_t key_len;
  void *data;
} bjvm_hash_table_entry;

typedef struct bjvm_string_hash_table_iterator {
  bjvm_hash_table_entry *current_base;
  bjvm_hash_table_entry *current;
  bjvm_hash_table_entry *end;
} bjvm_hash_table_iterator;

// Separate chaining hash map from (wchar_t) strings to void* (arbitrary)
// entries. Sigh.
typedef struct bjvm_string_hash_table {
  void (*free_fn)(void *entry);
  bjvm_hash_table_entry *entries;

  size_t entries_count;
  size_t entries_cap;

  double load_factor;
} bjvm_string_hash_table;

bjvm_string_hash_table bjvm_make_hash_table(void (*free_fn)(void *),
                                            double load_factor,
                                            size_t initial_capacity);

bjvm_hash_table_iterator
bjvm_hash_table_get_iterator(bjvm_string_hash_table *tbl);

bool bjvm_hash_table_iterator_has_next(bjvm_hash_table_iterator iter,
                                       wchar_t **key, size_t *key_len,
                                       void **value);

bool bjvm_hash_table_iterator_next(bjvm_hash_table_iterator *iter);

uint32_t bjvm_hash_string(const wchar_t *key, size_t len);

void bjvm_hash_table_rehash(bjvm_string_hash_table *tbl, size_t new_capacity);

/**
 * Insert the key/value pair into the hash table and return the old value, if
 * any. Ownership of the key is passed into the function.
 */
void *bjvm_hash_table_insert_impl(bjvm_string_hash_table *tbl, wchar_t *key,
                                  int len, void *value, bool copy_key);

/**
 * Insert the key/value pair into the hash table and return the old value, if
 * any. If len = -1, the key is treated as a null-terminated string literal.
 */
[[nodiscard]] void *bjvm_hash_table_insert(bjvm_string_hash_table *tbl,
                                           const wchar_t *key, int len,
                                           void *value);

/**
 * Delete the key from the hash table and return the old value, if any. If len =
 * -1, the key is treated as a null- terminated string literal. Pass the result
 * of this function to the free function, as long as it accepts NULL pointers.
 */
[[nodiscard]] void *bjvm_hash_table_delete(bjvm_string_hash_table *tbl,
                                           const wchar_t *key, int len);

/**
 * Look up the value in the hash table.
 */
void *bjvm_hash_table_lookup(bjvm_string_hash_table *tbl, const wchar_t *key,
                             int len);

void bjvm_free_hash_table(bjvm_string_hash_table tbl);

typedef struct bjvm_vm {
  void *classpath_manager;
  bjvm_string_hash_table classes;
} bjvm_vm;

typedef struct {
  bool unused;
} bjvm_vm_options;

/**
 * For simplicity, we always store local variables/stack variables as 64 bits,
 * and only use part of them in the case of integer or float (or, in 32-bit
 * mode, reference) values.
 */
typedef union {
  int64_t l;
  int i; // used for all integer types except long, plus boolean
  float f;
  double d;
  bjvm_obj_header *obj; // reference type
} bjvm_stack_value;

typedef struct {
  int program_counter; // in instruction indices
  int max_stack;
  int max_locals;
  int stack_depth;

  // First max_stack bjvm_stack_values, then max_locals more
  bjvm_stack_value values[];
} bjvm_stack_frame;

typedef struct bjvm_thread {
  // Global VM corresponding to this thread
  bjvm_vm *vm;

  // Essentially the stack of the thread -- a contiguous buffer which stores
  // stack_frames
  uint8_t *frame_buffer;
  uint32_t frame_buffer_capacity;
  // Also pointer one past the end of the last stack frame
  uint32_t frame_buffer_used;

  // Pointers into the frame_buffer
  bjvm_stack_frame **frames;
  uint32_t frames_count;
  uint32_t frames_cap;

  // Currently propagating exception
  bjvm_obj_header *current_exception;

  bool js_jit_enabled;
} bjvm_thread;

bjvm_array_classdesc *
bjvm_checked_to_array_classdesc(bjvm_classdesc *classdesc);
bjvm_classdesc *
bjvm_checked_to_primitive_array_classdesc(bjvm_classdesc *classdesc);

/**
 * Create an uninitialized frame with space sufficient for the given method.
 * Raises a StackOverflowError if the frames are exhausted.
 */
bjvm_stack_frame *bjvm_push_frame(bjvm_thread *thread,
                                  const bjvm_cp_method *method);

/**
 * Pop the topmost frame from the stack, optionally passing a pointer as a debug
 * check.
 */
void bjvm_pop_frame(bjvm_thread *thr, const bjvm_stack_frame *reference);

bjvm_vm *bjvm_create_vm(bjvm_vm_options options);

typedef struct {
  uint32_t stack_space;
  // Whether to enable JavaScript JIT compilation
  bool js_jit_enabled;
} bjvm_thread_options;

void bjvm_fill_default_thread_options(bjvm_thread_options *options);
bjvm_thread *bjvm_create_thread(bjvm_vm *vm, bjvm_thread_options options);
void bjvm_free_thread(bjvm_thread *thread);

/**
 * Add the given classfile as accessible to the VM (but don't necessarily load
 * it yet).
 */
int bjvm_vm_register_classfile(bjvm_vm *vm, const wchar_t *filename,
                               const uint8_t *bytes, size_t len);

/**
 * Read the classfile in the class path. Returns -1 on failure to find the
 * classfile. Writes a pointer to the classfile bytes and the length of the
 * classfile to the given pointers.
 */
int bjvm_vm_read_classfile(bjvm_vm *vm, const wchar_t *filename,
                           const uint8_t **bytes, size_t *len);

void bjvm_vm_list_classfiles(bjvm_vm *vm, wchar_t **strings, size_t *count);

/**
 * Parse a Java class file.
 *
 * The error message corresponds to a ClassFormatError in Java land.
 * (UnsupportedClassVersionErrors and VerifyErrors should be raised elsewhere.)
 *
 * @param bytes Start byte of the classfile.
 * @param len Length of the classfile in bytes.
 * @param result Where to write the result.
 * @return NULL on success, otherwise an error message (which is the caller's
 * responsibility to free).
 */
char *bjvm_parse_classfile(uint8_t *bytes, size_t len, bjvm_classdesc *result);

/**
 * Free the classfile.
 */
void bjvm_free_classfile(bjvm_classdesc cf);

int bjvm_initialize_vm(bjvm_vm *vm);
void bjvm_free_vm(bjvm_vm *vm);

/**
 * Implementation details, but exposed for testing...
 */

typedef bjvm_type_kind bjvm_analy_stack_entry;

// State of the stack (or local variable table) during analysis, indexed by
// formal JVM semantics (i.e., long/double take up two slots, and the second
// slot is unusable).
typedef struct {
  bjvm_analy_stack_entry *entries;
  int entries_count;
  int entries_cap;
  bool from_jump_target;
} bjvm_analy_stack_state;

char *print_analy_stack_state(const bjvm_analy_stack_state *state);

bjvm_compressed_bitset bjvm_init_compressed_bitset(int bits_capacity);
void bjvm_free_compressed_bitset(bjvm_compressed_bitset bits);
bool bjvm_is_bitset_compressed(bjvm_compressed_bitset bits);
int *bjvm_list_compressed_bitset_bits(bjvm_compressed_bitset bits,
                                      int *existing_buf, int *length,
                                      int *capacity);
bool bjvm_test_compressed_bitset(bjvm_compressed_bitset bits, size_t bit_index);
bool bjvm_test_reset_compressed_bitset(bjvm_compressed_bitset *bits,
                                       size_t bit_index);
bool bjvm_test_set_compressed_bitset(bjvm_compressed_bitset *bits,
                                     size_t bit_index);

char *bjvm_locals_on_function_entry(const bjvm_utf8 *descriptor,
                                    bjvm_analy_stack_state *locals);
char *parse_field_descriptor(const wchar_t **chars, size_t len,
                             bjvm_field_descriptor *result);
char *parse_method_descriptor(const bjvm_utf8 *descriptor,
                              bjvm_method_descriptor *result);
bool utf8_equals(bjvm_utf8 *entry, const char *str);
char *lossy_utf8_entry_to_chars(const bjvm_utf8 *utf8);
bjvm_utf8 bjvm_make_utf8(const wchar_t *c_literal);
void free_utf8_entry(bjvm_utf8 entry);
void free_field_descriptor(bjvm_field_descriptor descriptor);

#ifdef __cplusplus
}
#endif

#endif // BJVM_H
