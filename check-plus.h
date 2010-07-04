/* Check-Plus: An EDSL for declaring unit tests using Check
 * Copyright (C) 2010 Nelson Elhage
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <check.h>

#define __used __attribute__((used))

Suite *gc_suite();
Suite *vm_suite();

typedef void (*test_func)(int);
typedef void (*test_hook)(void);

struct test_case {
    test_func  *funcs, *end_funcs;
    const char *test_name;
    test_hook *setup_hooks, *end_setup_hooks;
    test_hook *teardown_hooks, *end_teardown_hooks;
};

struct test_suite {
    struct test_case *cases, *end_cases;
    const char *suite_name;
};

#define BEGIN_SUITE(name) _BEGIN_SUITE(SUITE_NAME, name)
#define _BEGIN_SUITE(sym, name) __BEGIN_SUITE(sym, name)
#define __BEGIN_SUITE(sym, name)                                        \
    struct test_case _begin_ ## sym ## _cases[];                        \
    struct test_case _end_ ## sym ## _cases[];                          \
    struct test_suite _suite_ ## sym = {                                \
        .cases       = (struct test_case*)_begin_ ## sym ## _cases,     \
        .end_cases   = (struct test_case*)_end_ ## sym ## _cases,       \
        .suite_name  = name,                                            \
    };                                                                  \
    struct test_case _begin_ ## sym ## _cases[0]                        \
    __attribute__((section(".data." #sym ".cases")))                    \

#define BEGIN_TEST_CASE(name) _BEGIN_TEST_CASE(SUITE_NAME, TEST_CASE, name)
#define _BEGIN_TEST_CASE(sym, test, name) __BEGIN_TEST_CASE(sym, test, name)

#define __BEGIN_TEST_CASE(suite, test, name)                    \
    _BEGIN_TEST_CASE_IMPL(suite, suite ## _ ## test,            \
                ".data." #suite "." #test, name)

#define _BEGIN_TEST_CASE_IMPL(suite, sym, sec, name)            \
    test_func _begin_ ## sym ## _funcs[];                       \
    test_hook _begin_ ## sym ## _setup[];                       \
    test_hook _begin_ ## sym ## _teardown[];                    \
    test_func _end_ ## sym ## _funcs[];                         \
    test_hook _end_ ## sym ## _setup[];                         \
    test_hook _end_ ## sym ## _teardown[];                      \
    struct test_case _test_ ## sym                              \
    __attribute__((section(".data." #suite ".cases"),           \
                   aligned(__alignof__(struct test_case)))) = { \
        .test_name = name,                                      \
        .funcs = _begin_ ## sym ## _funcs,                      \
        .end_funcs = _end_ ## sym ## _funcs,                    \
        .setup_hooks = _begin_ ## sym ## _setup,                \
        .end_setup_hooks = _end_ ## sym ## _setup,              \
        .teardown_hooks  = _begin_ ## sym ## _teardown,         \
        .end_teardown_hooks  = _end_ ## sym ## _teardown,       \
    };                                                          \
    test_func _begin_ ## sym ## _funcs[0]                       \
    __attribute__((section(sec ".funcs")));                     \
    test_hook _begin_ ## sym ## _setup[0]                       \
    __attribute__((section(sec ".setup")));                     \
    test_hook _begin_ ## sym ## _teardown[0]                    \
    __attribute__((section(sec ".teardown")))                   \


#define DEFINE_FIXTURE(setup, teardown)                         \
    _DEFINE_FIXTURE(SUITE_NAME, TEST_CASE, setup, teardown)
#define _DEFINE_FIXTURE(suite, test, setup, teardown)   \
    _SETUP_HOOK(suite, test, setup);                    \
    _TEARDOWN_HOOK(suite, test, teardown)

#define _SETUP_HOOK(suite, test, hook)                                  \
    test_hook _setup_##suite##_##test##_##hook __used                   \
    __attribute__((section(".data." #suite "." #test ".setup"))) =      \
         hook

#define _TEARDOWN_HOOK(suite, test, hook)                               \
    test_hook _teardown_##suite##_##test##_##hook __used                \
    __attribute__((section(".data." #suite "." #test ".teardown"))) =   \
         hook


#define TEST(fn) _TEST(SUITE_NAME, TEST_CASE, fn)
#define _TEST(sym, test, fn) __TEST(sym, test, fn)
#define __TEST(suite, test, fn)                              \
    _TEST_IMPL(suite ## _ ## test,                           \
               ".data." #suite "." #test,                    \
               suite##_##test##_##fn)

#define _TEST_IMPL(sym, sec, fn)                                \
    static void fn(int);                                        \
    test_func _test_##fn __used                                 \
    __attribute__((section(sec ".funcs"))) = fn;                \
    START_TEST(fn)

#define END_TEST_CASE _END_TEST_CASE(SUITE_NAME, TEST_CASE)
#define _END_TEST_CASE(sym, test) __END_TEST_CASE(sym, test)
#define __END_TEST_CASE(suite, test)                     \
    _END_TEST_CASE_IMPL(suite ## _ ## test,              \
                   ".data." #suite "." #test)           \

#define _END_TEST_CASE_IMPL(sym, sec)                   \
    test_func _end_ ## sym ## _funcs[0]                 \
    __attribute__((section(sec ".funcs")));             \
    test_hook _end_ ## sym ## _setup[0]                 \
    __attribute__((section(sec ".setup")));             \
    test_hook _end_ ## sym ## _teardown[0]              \
    __attribute__((section(sec ".teardown")))

#define END_SUITE _END_SUITE(SUITE_NAME)
#define _END_SUITE(sym) __END_SUITE(sym)
#define __END_SUITE(sym)                                \
    struct test_case _end_##sym##_cases[0]              \
    __attribute__((section(".data." #sym ".cases")))

#define construct_test_suite(name)              \
    _construct_test_suite(&(_suite_ ## name))

static inline Suite *_construct_test_suite(struct test_suite* spec) {
    Suite *s = suite_create(spec->suite_name);
    struct test_case *c;

    for(c = spec->cases; c < spec->end_cases; c++) {
        TCase *tc = tcase_create(c->test_name);
        test_hook *setup, *teardown;
        test_func *fn;
        for(setup = c->setup_hooks, teardown = c->teardown_hooks;
            setup < c->end_setup_hooks;
            setup++, teardown++) {
            tcase_add_checked_fixture(tc, *setup, *teardown);
        }
        for(fn = c->funcs; fn < c->end_funcs; fn++) {
            tcase_add_test(tc, *fn);
        }
        suite_add_tcase(s, tc);
    }
    return s;
}
