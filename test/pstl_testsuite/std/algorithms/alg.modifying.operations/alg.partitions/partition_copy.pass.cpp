// -*- C++ -*-
//===-- partition_copy.pass.cpp -------------------------------------------===//
//
// Copyright (C) 2017-2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// This file incorporates work covered by the following copyright and permission
// notice:
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
//
//===----------------------------------------------------------------------===//

// Tests for stable_partition and partition_copy
#include "support/pstl_test_config.h"
#include "support/utils.h"

#include _PSTL_TEST_HEADER(execution)
#include _PSTL_TEST_HEADER(algorithm)

#include <cstdlib>
#include <iterator>

using namespace TestUtils;

template <typename T>
struct test_partition_copy
{
    template <typename Policy, typename InputIterator, typename OutputIterator, typename OutputIterator2,
              typename UnaryOp>
    void
    operator()(Policy&& exec, InputIterator first, InputIterator last, OutputIterator true_first,
               OutputIterator true_last, OutputIterator2 false_first, OutputIterator2 false_last, UnaryOp unary_op)
    {

        auto actual_ret = ::std::partition_copy(exec, first, last, true_first, false_first, unary_op);

        EXPECT_TRUE(::std::distance(true_first, actual_ret.first) == ::std::count_if(first, last, unary_op),
                    "partition_copy has wrong effect from true sequence");
        EXPECT_TRUE(::std::distance(false_first, actual_ret.second) ==
                        ::std::count_if(first, last, oneapi::dpl::__internal::__not_pred<UnaryOp>(unary_op)),
                    "partition_copy has wrong effect from false sequence");
    }

//dummy specialization in case of broken configurations
#if _PSTL_ICC_18_19_TEST_SIMD_MONOTONIC_WINDOWS_RELEASE_BROKEN
template <typename InputIterator, typename OutputIterator, typename OutputIterator2,
          typename UnaryOp>
    void
    operator()(oneapi::dpl::execution::unsequenced_policy, InputIterator first, InputIterator last, OutputIterator true_first,
               OutputIterator true_last, OutputIterator2 false_first, OutputIterator2 false_last, UnaryOp unary_op)
    {
    }

    template <typename InputIterator, typename OutputIterator, typename OutputIterator2,
              typename UnaryOp>
    void
    operator()(oneapi::dpl::execution::parallel_unsequenced_policy, InputIterator first, InputIterator last, OutputIterator true_first,
               OutputIterator true_last, OutputIterator2 false_first, OutputIterator2 false_last, UnaryOp unary_op)
    {
    }
#endif

#if _PSTL_ICC_1800_TEST_MONOTONIC_RELEASE_64_BROKEN
    template <typename InputIterator, typename OutputIterator, typename OutputIterator2, typename UnaryOp>
    void
    operator()(oneapi::dpl::execution::unsequenced_policy, ::std::reverse_iterator<InputIterator> first,
               ::std::reverse_iterator<InputIterator> last, ::std::reverse_iterator<OutputIterator> true_first,
               ::std::reverse_iterator<OutputIterator> true_last, ::std::reverse_iterator<OutputIterator2> false_first,
               OutputIterator2 false_last, UnaryOp unary_op)
    {
    }

    template <typename InputIterator, typename OutputIterator, typename OutputIterator2, typename UnaryOp>
    void
    operator()(oneapi::dpl::execution::parallel_unsequenced_policy, ::std::reverse_iterator<InputIterator> first,
               ::std::reverse_iterator<InputIterator> last, ::std::reverse_iterator<OutputIterator> true_first,
               ::std::reverse_iterator<OutputIterator> true_last, ::std::reverse_iterator<OutputIterator2> false_first,
               OutputIterator2 false_last, UnaryOp unary_op)
    {
    }
#endif
};

template <typename T, typename UnaryPred>
void
test(UnaryPred pred)
{

    const ::std::size_t max_size = 100000;
    Sequence<T> in(max_size, [](::std::size_t v) -> T { return T(v); });
    Sequence<T> actual_true(max_size);
    Sequence<T> actual_false(max_size);
    for (::std::size_t n = 0; n <= max_size; n = n <= 16 ? n + 1 : ::std::size_t(3.1415 * n))
    {

        // for non-const input iterators
        invoke_on_all_policies<0>()(test_partition_copy<T>(), in.begin(), in.begin() + n, actual_true.begin(),
                                    actual_true.begin() + n, actual_false.begin(), actual_false.begin() + n, pred);

        // for const input iterators
        invoke_on_all_policies<1>()(test_partition_copy<T>(), in.cbegin(), in.cbegin() + n, actual_true.begin(),
                                    actual_true.begin() + n, actual_false.begin(), actual_false.begin() + n, pred);
    }
}

struct test_non_const
{
    template <typename Policy, typename InputIterator, typename OutputInterator>
    void
    operator()(Policy&& exec, InputIterator input_iter, OutputInterator out_iter)
    {
        auto is_even = [&](float64_t v) {
            uint32_t i = (uint32_t)v;
            return i % 2 == 0;
        };

        partition_copy(exec, input_iter, input_iter, out_iter, out_iter, non_const(is_even));
    }
};

int
main()
{
    test<int16_t>([](const int32_t value) { return value % 2 == 0; });

#if !_PSTL_ICC_16_17_TEST_REDUCTION_RELEASE_BROKEN
    test<int32_t>([](const int32_t value) { return true; });
#endif

#if !_ONEDPL_FPGA_DEVICE
    test<float64_t>([](const float64_t value) { return value > 2 << 6; });
#endif

#if !_ONEDPL_BACKEND_SYCL
    test<Wrapper<float64_t>>([](const Wrapper<float64_t>& value) -> bool { return value.get_my_field() != nullptr; });
#endif

    test_algo_basic_double<int32_t>(run_for_rnd_bi<test_non_const>());

    ::std::cout << done() << ::std::endl;
    return 0;
}
