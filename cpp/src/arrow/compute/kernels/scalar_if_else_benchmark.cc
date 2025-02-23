// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include <benchmark/benchmark.h>

#include "arrow/array/concatenate.h"
#include "arrow/compute/api_scalar.h"
#include "arrow/testing/gtest_util.h"
#include "arrow/testing/random.h"
#include "arrow/util/key_value_metadata.h"

namespace arrow {
namespace compute {

const int64_t kNumItems = 1024 * 1024;
const int64_t kFewItems = 64 * 1024;

template <typename Type, typename Enable = void>
struct GetBytesProcessed {};

template <>
struct GetBytesProcessed<BooleanType> {
  static int64_t Get(const std::shared_ptr<Array>& arr) { return arr->length() / 8; }
};

template <typename Type>
struct GetBytesProcessed<Type, enable_if_number<Type>> {
  static int64_t Get(const std::shared_ptr<Array>& arr) {
    using CType = typename Type::c_type;
    return arr->length() * sizeof(CType);
  }
};

template <typename Type>
struct GetBytesProcessed<Type, enable_if_base_binary<Type>> {
  static int64_t Get(const std::shared_ptr<Array>& arr) {
    using ArrayType = typename TypeTraits<Type>::ArrayType;
    using OffsetType = typename TypeTraits<Type>::OffsetType::c_type;
    return arr->length() * sizeof(OffsetType) +
           std::static_pointer_cast<ArrayType>(arr)->total_values_length();
  }
};

template <typename Type>
static void IfElseBench(benchmark::State& state) {
  auto type = TypeTraits<Type>::type_singleton();
  using ArrayType = typename TypeTraits<Type>::ArrayType;

  int64_t len = state.range(0);
  int64_t offset = state.range(1);

  random::RandomArrayGenerator rand(/*seed=*/0);

  auto cond = std::static_pointer_cast<BooleanArray>(
                  rand.ArrayOf(boolean(), len, /*null_probability=*/0.01))
                  ->Slice(offset);
  auto left = std::static_pointer_cast<ArrayType>(
                  rand.ArrayOf(type, len, /*null_probability=*/0.01))
                  ->Slice(offset);
  auto right = std::static_pointer_cast<ArrayType>(
                   rand.ArrayOf(type, len, /*null_probability=*/0.01))
                   ->Slice(offset);

  for (auto _ : state) {
    ABORT_NOT_OK(IfElse(cond, left, right));
  }

  state.SetBytesProcessed(state.iterations() *
                          (GetBytesProcessed<BooleanType>::Get(cond) +
                           GetBytesProcessed<Type>::Get(left) +
                           GetBytesProcessed<Type>::Get(right)));
}

template <typename Type>
static void IfElseBenchContiguous(benchmark::State& state) {
  auto type = TypeTraits<Type>::type_singleton();
  using ArrayType = typename TypeTraits<Type>::ArrayType;

  int64_t len = state.range(0);
  int64_t offset = state.range(1);

  ASSERT_OK_AND_ASSIGN(auto temp1, MakeArrayFromScalar(BooleanScalar(true), len / 2));
  ASSERT_OK_AND_ASSIGN(auto temp2,
                       MakeArrayFromScalar(BooleanScalar(false), len - len / 2));
  ASSERT_OK_AND_ASSIGN(auto concat, Concatenate({temp1, temp2}));
  auto cond = std::static_pointer_cast<BooleanArray>(concat)->Slice(offset);

  random::RandomArrayGenerator rand(/*seed=*/0);
  auto left = std::static_pointer_cast<ArrayType>(
                  rand.ArrayOf(type, len, /*null_probability=*/0.01))
                  ->Slice(offset);
  auto right = std::static_pointer_cast<ArrayType>(
                   rand.ArrayOf(type, len, /*null_probability=*/0.01))
                   ->Slice(offset);

  for (auto _ : state) {
    ABORT_NOT_OK(IfElse(cond, left, right));
  }

  state.SetBytesProcessed(state.iterations() *
                          (GetBytesProcessed<BooleanType>::Get(cond) +
                           GetBytesProcessed<Type>::Get(left) +
                           GetBytesProcessed<Type>::Get(right)));
}

static void IfElseBench64(benchmark::State& state) {
  return IfElseBench<UInt64Type>(state);
}

static void IfElseBench32(benchmark::State& state) {
  return IfElseBench<UInt32Type>(state);
}

static void IfElseBenchString32(benchmark::State& state) {
  return IfElseBench<StringType>(state);
}

static void IfElseBenchString64(benchmark::State& state) {
  return IfElseBench<LargeStringType>(state);
}

static void IfElseBench64Contiguous(benchmark::State& state) {
  return IfElseBenchContiguous<UInt64Type>(state);
}

static void IfElseBench32Contiguous(benchmark::State& state) {
  return IfElseBenchContiguous<UInt32Type>(state);
}

static void IfElseBenchString64Contiguous(benchmark::State& state) {
  return IfElseBenchContiguous<UInt64Type>(state);
}

static void IfElseBenchString32Contiguous(benchmark::State& state) {
  return IfElseBenchContiguous<UInt32Type>(state);
}

template <typename Type>
static void CaseWhenBench(benchmark::State& state) {
  auto type = TypeTraits<Type>::type_singleton();
  using ArrayType = typename TypeTraits<Type>::ArrayType;

  int64_t len = state.range(0);
  int64_t offset = state.range(1);

  random::RandomArrayGenerator rand(/*seed=*/0);

  auto cond_field =
      field("cond", boolean(), key_value_metadata({{"null_probability", "0.01"}}));
  auto cond = rand.ArrayOf(*field("", struct_({cond_field, cond_field, cond_field}),
                                  key_value_metadata({{"null_probability", "0.0"}})),
                           len);
  auto val1 = std::static_pointer_cast<ArrayType>(
      rand.ArrayOf(type, len, /*null_probability=*/0.01));
  auto val2 = std::static_pointer_cast<ArrayType>(
      rand.ArrayOf(type, len, /*null_probability=*/0.01));
  auto val3 = std::static_pointer_cast<ArrayType>(
      rand.ArrayOf(type, len, /*null_probability=*/0.01));
  auto val4 = std::static_pointer_cast<ArrayType>(
      rand.ArrayOf(type, len, /*null_probability=*/0.01));
  for (auto _ : state) {
    ABORT_NOT_OK(
        CaseWhen(cond->Slice(offset), {val1->Slice(offset), val2->Slice(offset),
                                       val3->Slice(offset), val4->Slice(offset)}));
  }

  // Set bytes processed to ~length of output
  state.SetBytesProcessed(state.iterations() * GetBytesProcessed<Type>::Get(val1));
  state.SetItemsProcessed(state.iterations() * (len - offset));
}

static void CaseWhenBenchList(benchmark::State& state) {
  auto type = list(int64());
  auto fld = field("", type);

  int64_t len = state.range(0);
  int64_t offset = state.range(1);

  random::RandomArrayGenerator rand(/*seed=*/0);

  auto cond_field =
      field("cond", boolean(), key_value_metadata({{"null_probability", "0.01"}}));
  auto cond = rand.ArrayOf(*field("", struct_({cond_field, cond_field, cond_field}),
                                  key_value_metadata({{"null_probability", "0.0"}})),
                           len);
  auto val1 = rand.ArrayOf(*fld, len);
  auto val2 = rand.ArrayOf(*fld, len);
  auto val3 = rand.ArrayOf(*fld, len);
  auto val4 = rand.ArrayOf(*fld, len);
  for (auto _ : state) {
    ABORT_NOT_OK(
        CaseWhen(cond->Slice(offset), {val1->Slice(offset), val2->Slice(offset),
                                       val3->Slice(offset), val4->Slice(offset)}));
  }

  // Set bytes processed to ~length of output
  state.SetBytesProcessed(state.iterations() *
                          GetBytesProcessed<Int64Type>::Get(
                              std::static_pointer_cast<ListArray>(val1)->values()));
  state.SetItemsProcessed(state.iterations() * (len - offset));
}

template <typename Type>
static void CaseWhenBenchContiguous(benchmark::State& state) {
  auto type = TypeTraits<Type>::type_singleton();
  using ArrayType = typename TypeTraits<Type>::ArrayType;

  int64_t len = state.range(0);
  int64_t offset = state.range(1);

  ASSERT_OK_AND_ASSIGN(auto trues, MakeArrayFromScalar(BooleanScalar(true), len / 3));
  ASSERT_OK_AND_ASSIGN(auto falses, MakeArrayFromScalar(BooleanScalar(false), len / 3));
  ASSERT_OK_AND_ASSIGN(auto nulls, MakeArrayOfNull(boolean(), len - 2 * (len / 3)));
  ASSERT_OK_AND_ASSIGN(auto concat, Concatenate({trues, falses, nulls}));
  auto cond1 = std::static_pointer_cast<BooleanArray>(concat);

  random::RandomArrayGenerator rand(/*seed=*/0);
  auto cond2 = std::static_pointer_cast<BooleanArray>(
      rand.ArrayOf(boolean(), len, /*null_probability=*/0.01));
  auto val1 = std::static_pointer_cast<ArrayType>(
      rand.ArrayOf(type, len, /*null_probability=*/0.01));
  auto val2 = std::static_pointer_cast<ArrayType>(
      rand.ArrayOf(type, len, /*null_probability=*/0.01));
  auto val3 = std::static_pointer_cast<ArrayType>(
      rand.ArrayOf(type, len, /*null_probability=*/0.01));
  ASSERT_OK_AND_ASSIGN(
      auto cond, StructArray::Make({cond1, cond2}, std::vector<std::string>{"a", "b"},
                                   nullptr, /*null_count=*/0));

  for (auto _ : state) {
    ABORT_NOT_OK(CaseWhen(cond->Slice(offset), {val1->Slice(offset), val2->Slice(offset),
                                                val3->Slice(offset)}));
  }

  // Set bytes processed to ~length of output
  state.SetBytesProcessed(state.iterations() * GetBytesProcessed<Type>::Get(val1));
  state.SetItemsProcessed(state.iterations() * (len - offset));
}

static void CaseWhenBench64(benchmark::State& state) {
  return CaseWhenBench<UInt64Type>(state);
}

static void CaseWhenBench64Contiguous(benchmark::State& state) {
  return CaseWhenBenchContiguous<UInt64Type>(state);
}

static void CaseWhenBenchString(benchmark::State& state) {
  return CaseWhenBench<StringType>(state);
}

static void CaseWhenBenchStringContiguous(benchmark::State& state) {
  return CaseWhenBenchContiguous<StringType>(state);
}

template <typename Type>
static void CoalesceBench(benchmark::State& state) {
  using CType = typename Type::c_type;
  auto type = TypeTraits<Type>::type_singleton();

  int64_t len = state.range(0);
  int64_t offset = state.range(1);

  random::RandomArrayGenerator rand(/*seed=*/0);

  std::vector<Datum> arguments;
  for (int i = 0; i < 4; i++) {
    arguments.emplace_back(
        rand.ArrayOf(type, len, /*null_probability=*/0.25)->Slice(offset));
  }

  for (auto _ : state) {
    ABORT_NOT_OK(CallFunction("coalesce", arguments));
  }

  state.SetBytesProcessed(state.iterations() * arguments.size() * (len - offset) *
                          sizeof(CType));
}

template <typename Type>
static void CoalesceNonNullBench(benchmark::State& state) {
  using CType = typename Type::c_type;
  auto type = TypeTraits<Type>::type_singleton();

  int64_t len = state.range(0);
  int64_t offset = state.range(1);

  random::RandomArrayGenerator rand(/*seed=*/0);

  std::vector<Datum> arguments;
  arguments.emplace_back(
      rand.ArrayOf(type, len, /*null_probability=*/0.25)->Slice(offset));
  arguments.emplace_back(rand.ArrayOf(type, len, /*null_probability=*/0)->Slice(offset));

  for (auto _ : state) {
    ABORT_NOT_OK(CallFunction("coalesce", arguments));
  }

  state.SetBytesProcessed(state.iterations() * arguments.size() * (len - offset) *
                          sizeof(CType));
}

static void CoalesceBench64(benchmark::State& state) {
  return CoalesceBench<Int64Type>(state);
}

static void CoalesceNonNullBench64(benchmark::State& state) {
  return CoalesceBench<Int64Type>(state);
}

template <typename Type>
static void ChooseBench(benchmark::State& state) {
  constexpr int kNumChoices = 5;
  using CType = typename Type::c_type;
  auto type = TypeTraits<Type>::type_singleton();

  int64_t len = state.range(0);
  int64_t offset = state.range(1);

  random::RandomArrayGenerator rand(/*seed=*/0);

  std::vector<Datum> arguments;
  arguments.emplace_back(
      rand.Int64(len, /*min=*/0, /*max=*/kNumChoices - 1, /*null_probability=*/0.1)
          ->Slice(offset));
  for (int i = 0; i < kNumChoices; i++) {
    arguments.emplace_back(
        rand.ArrayOf(type, len, /*null_probability=*/0.25)->Slice(offset));
  }

  for (auto _ : state) {
    ABORT_NOT_OK(CallFunction("choose", arguments));
  }

  state.SetBytesProcessed(state.iterations() * (len - offset) * sizeof(CType));
}

static void ChooseBench64(benchmark::State& state) {
  return ChooseBench<Int64Type>(state);
}

BENCHMARK(IfElseBench32)->Args({kNumItems, 0});
BENCHMARK(IfElseBench64)->Args({kNumItems, 0});

BENCHMARK(IfElseBench32)->Args({kNumItems, 99});
BENCHMARK(IfElseBench64)->Args({kNumItems, 99});

BENCHMARK(IfElseBench32Contiguous)->Args({kNumItems, 0});
BENCHMARK(IfElseBench64Contiguous)->Args({kNumItems, 0});

BENCHMARK(IfElseBench32Contiguous)->Args({kNumItems, 99});
BENCHMARK(IfElseBench64Contiguous)->Args({kNumItems, 99});

BENCHMARK(IfElseBenchString32)->Args({kNumItems, 0});
BENCHMARK(IfElseBenchString64)->Args({kNumItems, 0});

BENCHMARK(IfElseBenchString32Contiguous)->Args({kNumItems, 99});
BENCHMARK(IfElseBenchString64Contiguous)->Args({kNumItems, 99});

BENCHMARK(CaseWhenBench64)->Args({kNumItems, 0});
BENCHMARK(CaseWhenBench64)->Args({kNumItems, 99});

BENCHMARK(CaseWhenBench64Contiguous)->Args({kNumItems, 0});
BENCHMARK(CaseWhenBench64Contiguous)->Args({kNumItems, 99});

BENCHMARK(CaseWhenBenchList)->Args({kFewItems, 0});
BENCHMARK(CaseWhenBenchList)->Args({kFewItems, 99});

BENCHMARK(CaseWhenBenchString)->Args({kFewItems, 0});
BENCHMARK(CaseWhenBenchString)->Args({kFewItems, 99});

BENCHMARK(CaseWhenBenchStringContiguous)->Args({kFewItems, 0});
BENCHMARK(CaseWhenBenchStringContiguous)->Args({kFewItems, 99});

BENCHMARK(CoalesceBench64)->Args({kNumItems, 0});
BENCHMARK(CoalesceBench64)->Args({kNumItems, 99});

BENCHMARK(CoalesceNonNullBench64)->Args({kNumItems, 0});
BENCHMARK(CoalesceNonNullBench64)->Args({kNumItems, 99});

BENCHMARK(ChooseBench64)->Args({kNumItems, 0});
BENCHMARK(ChooseBench64)->Args({kNumItems, 99});

}  // namespace compute
}  // namespace arrow
