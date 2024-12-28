#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <climits>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <ranges>
#include <unordered_map>

#include "../src/adt.h"
#include "../src/analysis.h"
#include "../src/bjvm.h"
#include "../src/util.h"
#include "../src/wasm_jit.h"
#include "tests-common.h"

#include <numeric>

using namespace Bjvm::Tests;

double get_time() {
#ifdef EMSCRIPTEN
  return emscripten_get_now();
#else
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
#endif
}

TEST_CASE("Test STR() macro") {
  bjvm_utf8 utf = STR("abc");
  REQUIRE(utf.chars[0] == 'a');
  REQUIRE(utf.chars[1] == 'b');
  REQUIRE(utf.chars[2] == 'c');
  REQUIRE(utf.len == 3);
}

TEST_CASE("Test classfile parsing") {
  bool fuzz = false;

  // list all java files in the jre8 directory
  auto files = ListDirectory("jre8", true);
  double total_millis = 0;

  int count = 0;
  int FILE_COUNT = fuzz ? 64 : INT_MAX;
  for (const auto &file : files) {
    if (!EndsWith(file, ".class")) {
      continue;
    }
    if (count++ > FILE_COUNT) {
      continue;
    }

    auto read = ReadFile(file).value();
    double start = get_time();

    // std::cout << "Reading " << file << "\n";
    bjvm_classdesc cf = {};
    parse_result_t error = bjvm_parse_classfile(read.data(), read.size(), &cf);
    if (error != PARSE_SUCCESS) {
      std::cerr << "Error parsing classfile: " << error << '\n';
      // abort();
    } else {
      bjvm_free_classfile(cf);
    }

    if (fuzz) {
      std::cerr << "Fuzzing classfile: " << file << '\n';

#pragma omp parallel for
      for (size_t i = 0; i < read.size(); ++i) {
        std::cout << "Done with " << i << '\n';
        bjvm_classdesc cf;
        auto copy = read;
        for (int j = 0; j < 256; ++j) {
          copy[i] += 1;
          parse_result_t error =
              bjvm_parse_classfile(copy.data(), copy.size(), &cf);
          if (error != PARSE_SUCCESS) {

          } else {
            bjvm_free_classfile(cf);
          }
        }
      }
    }

    total_millis += get_time() - start;
  }

  std::cout << "Total time: " << total_millis << "ms\n";

  BENCHMARK_ADVANCED("Parse classfile")(Catch::Benchmark::Chronometer meter) {
    auto read =
        ReadFile("./jre8/sun/security/tools/keytool/Main.class").value();
    bjvm_classdesc cf;

    meter.measure([&] {
      parse_result_t error =
          bjvm_parse_classfile(read.data(), read.size(), &cf);
      if (error != PARSE_SUCCESS)
        abort();
      bjvm_free_classfile(cf);
    });
  };
}

TEST_CASE("Class file management") {
  auto vm = CreateTestVM(true);
  auto files = ListDirectory("jre8", true);

  int file_count = std::accumulate(files.begin(), files.end(), 0,
                                   [](int acc, const std::string &file) {
                                     return acc + EndsWith(file, ".class");
                                   });

  size_t len;
  const uint8_t *bytes;
  REQUIRE(bjvm_vm_read_classfile(vm.get(), STR("java/lang/Object.class"),
                                 &bytes, &len) == 0);
  REQUIRE(len > 0);
  REQUIRE(*(uint32_t *)bytes == 0xBEBAFECA);
  REQUIRE(bjvm_vm_read_classfile(vm.get(), STR("java/lang/Object.clas"),
                                 nullptr, &len) != 0);
  REQUIRE(bjvm_vm_read_classfile(vm.get(), STR("java/lang/Object.classe"),
                                 nullptr, &len) != 0);
  bjvm_vm_list_classfiles(vm.get(), nullptr, &len);
  REQUIRE((int)len == file_count);
  std::vector<heap_string> strings(len);

  bjvm_vm_list_classfiles(vm.get(), strings.data(), &len);
  bool found = false;
  for (size_t i = 0; i < len; ++i) {
    found = found ||
            utf8_equals(hslc(strings[i]), "java/lang/ClassLoader.class") == 0;
    free_heap_str(strings[i]);
  }
  printf("Hello");
  REQUIRE(found);
}

TEST_CASE("Compressed bitset") {
  for (int size = 1; size < 256; ++size) {
    std::vector<uint8_t> reference(size);
    auto bitset = bjvm_init_compressed_bitset(size);

    for (int i = 0; i < 1000; ++i) {
      int index = rand() % size;
      switch (rand() % 4) {
      case 0: {
        int *set_bits = nullptr, length = 0, capacity = 0;
        set_bits = bjvm_list_compressed_bitset_bits(bitset, set_bits, &length,
                                                    &capacity);
        for (int i = 0; i < length; ++i) {
          if (!reference[set_bits[i]])
            REQUIRE(false);
        }
        free(set_bits);
        break;
      }
      case 1: {
        bool test = bjvm_test_set_compressed_bitset(&bitset, index);
        if (test != reference[index])
          REQUIRE(test == reference[index]);
        reference[index] = true;
        break;
      }
      case 2: {
        bool test = bjvm_test_reset_compressed_bitset(&bitset, index);
        if (test != reference[index])
          REQUIRE(test == reference[index]);
        reference[index] = false;
        break;
      }
      case 3: {
        bool test = bjvm_test_compressed_bitset(bitset, index);
        if (test != reference[index])
          REQUIRE(test == reference[index]);
        break;
      }
      }
    }

    bjvm_free_compressed_bitset(bitset);
  }
}

TEST_CASE("parse_field_descriptor valid cases") {
  const char *fields =
      "Lcom/example/Example;[I[[[JLjava/lang/String;[[Ljava/lang/Object;BVCZ";
  bjvm_field_descriptor com_example_Example, Iaaa, Jaa, java_lang_String,
      java_lang_Object, B, V, C, Z;
  REQUIRE(
      !parse_field_descriptor(&fields, strlen(fields), &com_example_Example));
  REQUIRE(!parse_field_descriptor(&fields, strlen(fields), &Iaaa));
  REQUIRE(!parse_field_descriptor(&fields, strlen(fields), &Jaa));
  REQUIRE(!parse_field_descriptor(&fields, strlen(fields), &java_lang_String));
  REQUIRE(!parse_field_descriptor(&fields, strlen(fields), &java_lang_Object));
  REQUIRE(!parse_field_descriptor(&fields, strlen(fields), &B));
  REQUIRE(!parse_field_descriptor(&fields, strlen(fields), &V));
  REQUIRE(!parse_field_descriptor(&fields, strlen(fields), &C));
  REQUIRE(!parse_field_descriptor(&fields, strlen(fields), &Z));

  REQUIRE(
      utf8_equals(hslc(com_example_Example.class_name), "com/example/Example"));
  REQUIRE(com_example_Example.dimensions == 0);
  REQUIRE(com_example_Example.base_kind == BJVM_TYPE_KIND_REFERENCE);

  REQUIRE(Iaaa.base_kind == BJVM_TYPE_KIND_INT);
  REQUIRE(Iaaa.dimensions == 1);

  REQUIRE(Jaa.base_kind == BJVM_TYPE_KIND_LONG);
  REQUIRE(Jaa.dimensions == 3);

  REQUIRE(utf8_equals(hslc(java_lang_String.class_name), "java/lang/String"));
  REQUIRE(java_lang_String.dimensions == 0);

  REQUIRE(utf8_equals(hslc(java_lang_Object.class_name), "java/lang/Object"));
  REQUIRE(java_lang_Object.dimensions == 2);

  REQUIRE(B.base_kind == BJVM_TYPE_KIND_BYTE);
  REQUIRE(C.base_kind == BJVM_TYPE_KIND_CHAR);
  REQUIRE(V.base_kind == BJVM_TYPE_KIND_VOID);
  REQUIRE(Z.base_kind == BJVM_TYPE_KIND_BOOLEAN);

  free_field_descriptor(com_example_Example);
  free_field_descriptor(java_lang_Object);
  free_field_descriptor(java_lang_String);
}

TEST_CASE("VM initialization") { CreateTestVM(true); }

struct TestCaseResult {
  std::string stdout_;
  std::string stderr_;
};

TestCaseResult run_test_case(std::string folder, bool capture_stdio = true) {
  const char *classpath[2] = {folder.c_str(), nullptr};
  bjvm_vm_options options = bjvm_default_vm_options();

  TestCaseResult result{};

  options.load_classfile = load_classfile;
  options.load_classfile_param = classpath;
  options.write_stdout = capture_stdio ? +[](int ch, void *param) {
    auto *result = (TestCaseResult *)param;
  result->stdout_ += (char)ch;
  } : nullptr;
  options.write_stderr = capture_stdio ? +[](int ch, void *param) {
    auto *result = (TestCaseResult *)param;
    result->stderr_ += (char)ch;
  } : nullptr;
  options.write_byte_param = &result;

  bjvm_vm *vm = bjvm_create_vm(options);
  bjvm_thread *thr = bjvm_create_thread(vm, bjvm_default_thread_options());

  bjvm_classdesc *desc = bootstrap_class_create(thr, STR("Main"));
  bjvm_stack_value args[1] = {{.obj = nullptr}};

  bjvm_cp_method *method;
  bjvm_initialize_class(thr, desc);

  method = bjvm_easy_method_lookup(desc, STR("main"),
                                   STR("([Ljava/lang/String;)V"), false, false);

  bjvm_thread_run(thr, method, args, nullptr);

  if (thr->current_exception) {
    method = bjvm_easy_method_lookup(thr->current_exception->descriptor,
                                     STR("toString"),
                                     STR("()Ljava/lang/String;"), true, false);
    bjvm_stack_value args[1] = {{.obj = thr->current_exception}}, result;
    thr->current_exception = nullptr;
    bjvm_thread_run(thr, method, args, &result);
    heap_string read = read_string_to_utf8(result.obj);
    std::cout << "Exception thrown!\n" << read.chars << '\n' << '\n';
    free_heap_str(read);

    // Then call printStackTrace ()V
    method =
        bjvm_easy_method_lookup(args[0].obj->descriptor, STR("printStackTrace"),
                                STR("()V"), true, false);
    bjvm_thread_run(thr, method, args, nullptr);
  }

  bjvm_free_thread(thr);
  bjvm_free_vm(vm);

  return result;
}

TEST_CASE("String hash table") {
  bjvm_string_hash_table tbl = bjvm_make_hash_table(free, 0.75, 48);
  REQUIRE(tbl.load_factor == 0.75);
  REQUIRE(tbl.entries_cap == 48);

  std::unordered_map<std::string, std::string> reference;
  for (int i = 0; i < 5000; ++i) {
    std::string key = std::to_string(i * 5201);
    std::string value = std::to_string(i);
    reference[key] = value;
    free(bjvm_hash_table_insert(&tbl, key.c_str(), -1,
                                (void *)strdup(value.c_str())));
    free(bjvm_hash_table_insert(&tbl, key.c_str(), -1,
                                (void *)strdup(value.c_str())));
  }
  for (int i = 1; i <= 4999; i += 2) {
    std::string key = std::to_string(i * 5201);
    free(bjvm_hash_table_delete(&tbl, key.c_str(), -1));
  }
  REQUIRE(tbl.entries_count == 2500);
  for (int i = 1; i <= 4999; i += 2) {
    std::string key = std::to_string(i * 5201);
    void *lookup = bjvm_hash_table_lookup(&tbl, key.c_str(), -1);
    REQUIRE(lookup == nullptr);
    std::string value = std::to_string(i);
    free(bjvm_hash_table_insert(&tbl, key.c_str(), -1, strdup(value.c_str())));
  }

  for (int i = 0; i < 5000; i += 2) {
    std::string key = std::to_string(i * 5201);
    void *value = bjvm_hash_table_lookup(&tbl, key.c_str(), -1);
    REQUIRE(value != nullptr);
    REQUIRE(reference[key] == (const char *)value);
  }

  bjvm_free_hash_table(tbl);
}

TEST_CASE("SignaturePolymorphic methods found") {
  // TODO
}

TEST_CASE("Malformed classfiles") {
  // TODO
}

TEST_CASE("Interning") {
  auto result = run_test_case("test_files/interning/");
  REQUIRE(result.stdout_ ==
          "false\nfalse\ntrue\ntrue\ntrue\nfalse\ntrue\nfalse\n");
}

TEST_CASE("NegativeArraySizeException") {
  auto result = run_test_case("test_files/negative_array_size/");
  std::string expected = "abcdefghij";
  REQUIRE(result.stdout_ == expected);
}

TEST_CASE("System.arraycopy") {
  auto result = run_test_case("test_files/arraycopy/");
  std::string expected = "abcdefghijkljklm";
  REQUIRE(result.stdout_ == expected);
}

TEST_CASE("Passing long to method calls") {
  auto result = run_test_case("test_files/long_calls/");
  std::string expected = "abcdabcd";
  REQUIRE(result.stdout_ == expected);
}

TEST_CASE("Big decimal #1") {
  std::string expected = R"(10.5 + 2.3 = 12.8
10.5 - 2.3 = 8.2
10.5 * 2.3 = 24.15
10 / 3 (rounded to 2 decimals) = 3.33
10.5 equals 10.500: false
10.5 compareTo 10.500: 0
pi * pi is 9.869604401089358618834490999876151135199537704046847772071781337378379488504041
)";

  for (int i = 0; i < 10; ++i)
    expected += expected; // repeat 1024 times

  auto result = run_test_case("test_files/big_decimal/");
  REQUIRE(result.stdout_ == expected);

  BENCHMARK("Big decimal #2") {
    auto result = run_test_case("test_files/big_decimal/");
  };
}

TEST_CASE("Math natives") {
  auto result = run_test_case("test_files/math/", true);
  REQUIRE(result.stdout_ == "abcdefghijklmnopqrstu");
}

TEST_CASE("Stack traces") {
  BENCHMARK("Stack traces") {
    auto result = run_test_case("test_files/stack_trace_bench/", false);
  };
}

/*
TEST_CASE("Signature polymorphism") {
  auto result = run_test_case("test_files/signature_polymorphism/", false);
}
*/

TEST_CASE("Basic lambda") {
  auto result = run_test_case("test_files/basic_lambda/", true);

  REQUIRE(result.stdout_ == R"(Hello from lambda!
variable passed in
2eggs
Hello from lambda!
variable passed in
2eggs
)");
}

TEST_CASE("Advanced lambda") {
  std::string expected = R"(10 + 5 = 15
Sum of numbers: 15
1
2
3
4
5
Incremented: 2
Incremented: 4
Incremented: 6
Incremented: 8
Incremented: 10
Sum of lots of numbers: 499500
Cow value: 15
)";
  for (int i = 0; i < 10; ++i)
    expected += expected;
  auto result = run_test_case("test_files/advanced_lambda/", true);
  // REQUIRE(result.stdout_.length() == expected.length());
  REQUIRE(result.stdout_ == expected);

  BENCHMARK("Advanced lambda benchmark") {
    auto result = run_test_case("test_files/advanced_lambda/", true);
  };
}

TEST_CASE("Class<?> implementation") {
  auto result = run_test_case("test_files/reflection_class/", true);
  REQUIRE(result.stdout_ ==
          R"(int1041nullfalsefalsefalsefalsefalsefalse00nulltruenull
boolean1041nullfalsefalsefalsefalsefalsefalse00nulltruenull
byte1041nullfalsefalsefalsefalsefalsefalse00nulltruenull
char1041nullfalsefalsefalsefalsefalsefalse00nulltruenull
void1041nullfalsefalsefalsefalsefalsefalse00nulltruenull
short1041nullfalsefalsefalsefalsefalsefalse00nulltruenull
long1041nullfalsefalsefalsefalsefalsefalse00nulltruenull
float1041nullfalsefalsefalsefalsefalsefalse00nulltruenull
double1041nullfalsefalsefalsefalsefalsefalse00nulltruenull
java.lang.Integer17nullfalsefalsetruefalsefalsefalse1153class java.lang.Numberfalsenull
[Ljava.lang.Integer;1041class java.lang.Integertruefalsefalsetruefalsefalse00class java.lang.Objectfalsenull
[[Ljava.lang.Integer;1041class [Ljava.lang.Integer;truefalsefalsefalsefalsefalse00class java.lang.Objectfalsenull
[I1041inttruefalsefalsefalsetruefalse00class java.lang.Objectfalsenull
[[I1041class [Itruefalsefalsefalsefalsetrue00class java.lang.Objectfalsenull
[C1041chartruefalsefalsefalsefalsefalse00class java.lang.Objectfalsenull
[[C1041class [Ctruefalsefalsefalsefalsefalse00class java.lang.Objectfalsenull
java.io.Serializable1537nullfalsetruetruetruetruetrue00nullfalsenull
)");
}

TEST_CASE("NPE") {
  auto result = run_test_case("test_files/npe/", true);
  REQUIRE(result.stdout_ == "abcdefghijklmnopqr");
}

TEST_CASE("ArithmeticException") {
  auto result = run_test_case("test_files/arithmetic_exception/", true);
  REQUIRE(result.stdout_ == "abcd");
}

TEST_CASE("Numeric conversions") {
  auto result = run_test_case("test_files/numerics/", true);
  REQUIRE(result.stdout_ == R"(3.4028235E38 -> 2147483647
1.4E-45 -> 0
Infinity -> 2147483647
-Infinity -> -2147483648
NaN -> 0
3.4028235E38 -> 9223372036854775807L
1.4E-45 -> 0L
Infinity -> 9223372036854775807L
-Infinity -> -9223372036854775808L
NaN -> 0L
1.7976931348623157E308 -> 2147483647
4.9E-324 -> 0
Infinity -> 2147483647
-Infinity -> -2147483648
NaN -> 0
1.7976931348623157E308 -> 9223372036854775807L
4.9E-324 -> 0L
Infinity -> 9223372036854775807L
-Infinity -> -9223372036854775808L
NaN -> 0L
)");
}

TEST_CASE("ClassCircularityError") {
  auto result = run_test_case("test_files/circularity/", true);
  REQUIRE(result.stdout_ == "abc");
}

TEST_CASE("Array creation doesn't induce <clinit>") {
  auto result = run_test_case("test_files/array_clinit/", true);
  REQUIRE(result.stdout_ == "Hey :)\n");
}

TEST_CASE("Simple OutOfMemoryError") {
  auto result = run_test_case("test_files/out_of_memory/", true);
  REQUIRE(result.stdout_.find("OutOfMemoryError") != std::string::npos);
}

TEST_CASE("Exceptions in <clinit>") {
  auto result = run_test_case("test_files/eiie/", true);
  REQUIRE(result.stdout_ == R"(Egg
Chicken
)");
}

TEST_CASE("ConstantValue initialisation") {
  auto result = run_test_case("test_files/constant_value/", true);
  REQUIRE(result.stdout_ == "2147483647\n");
}

TEST_CASE("Deranged CFG") {
  auto result = run_test_case("test_files/cfg_fuck/", true);
  auto expected = ReadFile("test_files/cfg_fuck/reference.txt").value();
  std::string as_string;
  for (int i = 0; i < expected.size(); ++i)
    as_string.push_back(expected.at(i));
  REQUIRE(result.stdout_ == as_string);
}

TEST_CASE("Analysis") {
  // TODO check that all code analysis functions succeed across all JDK
  // functions
}


TEST_CASE("Immediate dominators computation on cursed CFG") {
  bjvm_classdesc desc;
  auto cursed_file = ReadFile("test_files/cfg_fuck/Main.class").value();
  bjvm_parse_classfile(cursed_file.data(), cursed_file.size(), &desc);
  REQUIRE(utf8_equals(hslc(desc.name), "Main"));

  bjvm_cp_method *m = desc.methods + 4;
  REQUIRE(utf8_equals(m->name, "main"));

  bjvm_analyze_method_code(m, nullptr);
  auto *analy = (bjvm_code_analysis *)m->code_analysis;
  bjvm_scan_basic_blocks(m->code, analy);
  bjvm_compute_dominator_tree(analy);

  BENCHMARK("analyze method code") { bjvm_analyze_method_code(m, nullptr); };

  BENCHMARK("scan basic blocks") {
    free(analy->blocks);
    analy->blocks = nullptr;
    bjvm_scan_basic_blocks(m->code, analy);
  };

  BENCHMARK("compute dominator tree") {
    analy->dominator_tree_computed = false;
    bjvm_compute_dominator_tree(analy);
  };

  std::vector<std::pair<int, int>> doms = {
      {1, 0},  {2, 1},  {3, 2},   {4, 3},   {5, 4},   {6, 5},
      {7, 6},  {8, 6},  {9, 6},   {10, 6},  {11, 6},  {12, 6},
      {13, 6}, {14, 5}, {15, 14}, {16, 14}, {17, 16}, {18, 5},
      {19, 4}, {20, 4}, {21, 20}, {22, 1}};

  for (auto [a, b] : doms) {
    REQUIRE(analy->blocks[a].idom == b);
  }

  int result = bjvm_attempt_reduce_cfg(analy);
  REQUIRE(result == 0);

  bjvm_cp_method *m2 = desc.methods + 5;
  bjvm_analyze_method_code(m2, nullptr);

  bjvm_free_classfile(desc);
}

TEST_CASE("Playground") {
  auto result = run_test_case("test_files/playground/", false);
}
