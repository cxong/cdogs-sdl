/*
 * Copyright (c) 2011 Tony Bai <bigwhite.cn@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <rlutil/rlutil.h>

#include "cbehave.h"


cbehave_scope_e cbehave_scope;

static cbehave_symbol_head_t _symbol_list;

static cbehave_symbol_t* lookup_symbol(const char *symbol_name, int obj_type);
static void add_value(cbehave_symbol_t *s, void *value, int count);
static cbehave_value_t* get_value(cbehave_symbol_t *s);

#ifdef __APPLE__
#define DEFAULT_COLOR BLACK
#else
#define DEFAULT_COLOR GREY
#endif

void should_int_equal(int actual, int expected, 
                      void *state, const char *file, 
                      int line) {
    int *_scenario_state = (int*)state;
    if ((expected) != (actual)) { 
        (*_scenario_state) = 1;
        setColor(RED);
        printf("\t\t\t%s:%d: Failed: expected[%d], but actual[%d].\n",
                file,
                line,
                expected,
                actual);
        setColor(DEFAULT_COLOR);
    }
}

void should_int_gt(int val1, int val2,
                   void *state,
                   const char *file, int line) {
    int *_scenario_state = (int*)state;
    if ((val1) <= (val2)) {
        (*_scenario_state) = 1;
        setColor(RED);
        printf("\t\t\t%s:%d: Failed: [%d] not greater than [%d].\n",
                file,
                line,
                val1,
                val2);
        setColor(DEFAULT_COLOR);
    }
}

void should_int_lt(int val1, int val2,
                   void *state,
                   const char *file, int line) {
    int *_scenario_state = (int*)state;
    if ((val1) >= (val2)) {
        (*_scenario_state) = 1;
        setColor(RED);
        printf("\t\t\t%s:%d: Failed: [%d] not less than [%d].\n",
                file,
                line,
                val1,
                val2);
        setColor(DEFAULT_COLOR);
    }
}

void should_int_ge(int val1, int val2,
                   void *state,
                   const char *file, int line) {
    int *_scenario_state = (int*)state;
    if ((val1) < (val2)) {
        (*_scenario_state) = 1;
        setColor(RED);
        printf("\t\t\t%s:%d: Failed: [%d] not greater than or equal to [%d].\n",
                file,
                line,
                val1,
                val2);
        setColor(DEFAULT_COLOR);
    }
}

void should_int_le(int val1, int val2,
                   void *state,
                   const char *file, int line) {
    int *_scenario_state = (int*)state;
    if ((val1) > (val2)) {
        (*_scenario_state) = 1;
        setColor(RED);
        printf("\t\t\t%s:%d: Failed: [%d] not less than or equal to [%d].\n",
                file,
                line,
                val1,
                val2);
        setColor(DEFAULT_COLOR);
    }
}

void should_str_equal(const char *actual, const char *expected, void *state,
                      const char *file, int line) {

    int *_scenario_state = (int*)state;
    /* 
     * both pointers are NULL or pointing to the same memory 
     */
    if (expected == actual) return;
    
    if (expected && actual) {
        if (!strcmp(expected, actual)) {
            return;
        }
    }

    (*_scenario_state) = 1;
    setColor(RED);
    printf("\t\t\t%s:%d: Failed: expected[%s], but actual[%s].\n", 
            file, line,
            expected ? expected : "NULL",
            actual ? actual : "NULL");
    setColor(DEFAULT_COLOR);
}

void should_mem_equal(const void *actual, const void *expected, size_t size, void *state,
                      const char *file, int line) {

    int *_scenario_state = (int*)state;
    /* 
     * both pointers are NULL or pointing to the same memory 
     */
    if (expected == actual) return;
    
    if (expected && actual) {
        if (!memcmp(expected, actual, size)) {
            return;
        }
    }

    (*_scenario_state) = 1;
    setColor(RED);
    printf("\t\t\t%s:%d: Failed: memory does not equal.\n", file, line);
    setColor(DEFAULT_COLOR);
}

void should_be_bool(bool actual, bool expected, void *state, const char *file, int line) {
    int *_scenario_state = (int*)state;
    if (actual != expected) {
        (*_scenario_state) = 1;
        setColor(RED);
        printf("\t\t\t%s:%d: Failed: actual[%d] is not a %s value.\n",
                file,
                line,
                actual,
                expected ? "true" : "false");
        setColor(DEFAULT_COLOR);
    }
}

void cbehave_given_entry(const char *prompt, const char *str, void *state) {
    (void)(state);
    printf("\t\t%s %s\n", prompt, str);
}

void cbehave_when_entry(const char *prompt, const char *str, void *state) {
    (void)(state);
	printf("\t\t%s %s\n", prompt, str);
}

void cbehave_then_entry(const char *prompt, const char *str, void *state) {
    (void)(state);
	printf("\t\t%s %s\n", prompt, str);
}

void cbehave_scenario_entry(const char *str, void *state) {
    cbehave_state *cur = (cbehave_state*)state;
    cur->total_scenarios++;

    printf("\tScenario: %s\n", str);
}

void cbehave_feature_entry(const char *str, void *old_state, void *state) {
    cbehave_state *cur = (cbehave_state*)state;
    cbehave_state *old = (cbehave_state*)old_state;

    cur->total_features++;
    memcpy(old, state, sizeof(*cur));

    printf("\nFeature: %s\n", str);
}

void cbehave_given_exit(void *state) {
    (void)(state);
}

void cbehave_when_exit(void *state) {
    (void)(state);
}

void cbehave_then_exit(void *state) {
    (void)(state);
}

void cbehave_scenario_exit(void *scenario_state, void *state) {
    int *_scenario_state = (int*)scenario_state;
    cbehave_state *cur = (cbehave_state*)state;

    if ((*_scenario_state) == 1) {
        cur->failed_scenarios++;
    }
}

void cbehave_feature_exit(void *old_state, void *state) {
    cbehave_state *cur = (cbehave_state*)state;
    cbehave_state *old = (cbehave_state*)old_state;

    if (cur->failed_scenarios > old->failed_scenarios) {
        cur->failed_features++;
    }
}

void cbehave_feature_return(const char *file, int line, int ret, void *state) {
    cbehave_state *cur = (cbehave_state*)state;
    
    cur->failed_scenarios++;

    setColor(RED);
    printf("\t\t\t%s:%d: Exception occurred, error code: %d.\n",
            file,
            line,
            ret);
    setColor(DEFAULT_COLOR);
}


int _cbehave_runner(const char *description, const cbehave_feature *features, int count) {
    cbehave_state *state = NULL;
    int i;
    int ret;

    printf("%s\n", CBEHAVE_LOGO);
    printf("%s\n", description);

    state = (cbehave_state*)malloc(sizeof(*state));
    if (!state) {
        setColor(RED);
        printf("\t%s:%d: Failed to alloc memory, error code: %d.\n", 
                __FILE__, __LINE__, errno);
        setColor(DEFAULT_COLOR);
        return -1;
    }
    memset(state, 0, sizeof(*state));

    APR_RING_INIT(&_symbol_list, cbehave_symbol_t, link);

    for (i = 0; i < count; i++) {
        features[i].func(state);        
    }

    printf("\nSummary: \n");
    if (state->failed_features) {
        setColor(RED);
    } else {
        setColor(GREEN);
    }
    printf("\tfeatures: [%d/%d]\n",
           state->total_features - state->failed_features, state->total_features);
    setColor(DEFAULT_COLOR);

    if (state->failed_scenarios) {
        setColor(RED);
    } else {
        setColor(GREEN);
    }
    printf("\tscenarios: [%d/%d]\n", state->total_scenarios - state->failed_scenarios, state->total_scenarios);
    setColor(DEFAULT_COLOR);

    ret = (state->failed_features == 0) ? 0 : 1;

    if (state) {
        free(state);
    }
    return ret;
}

void* cbehave_mock_obj(const char *fcname,
                       int lineno,
                       const char *fname,
                       int obj_type) {
    cbehave_symbol_t   *s  = NULL;
    cbehave_value_t    *v  = NULL;
    void            *p;

    s = lookup_symbol(fcname, obj_type);
    if (!s) {
        printf("\t[CBEHAVE]: can't find the symbol: <%s> which is being mocked!, %d line in file %s\n",
                fcname, lineno, fname);
        exit(EXIT_FAILURE);
    }

    if (s->always_return_flag) return s->value;

    v = get_value(s);
    if (!v) {
        printf("\t[CBEHAVE]: you have not set the value of mock obj <%s>!, %d line in file %s\n",
                fcname, lineno, fname);
        exit(EXIT_FAILURE);
    }

    p = v->value;

    APR_RING_REMOVE(v, link);
    free(v);

    return p;
}


void cbehave_mock_obj_return(const char *symbol_name,
                             void *value,
                             const char *fcname,
                             int lineno,
                             const char *fname,
                             int obj_type,
                             int count) {

    cbehave_symbol_t   *s  = lookup_symbol(symbol_name, obj_type);
    (void)(fcname);
    (void)(lineno);
    (void)(fname);
    if (!s) {
        errno = 0;
        s = (cbehave_symbol_t*)malloc(sizeof(*s));
        if (!s) {
            printf("\t[CBEHAVE]: malloc error!, errcode[%d]\n", errno);
            exit(EXIT_FAILURE);
        }
        memset(s, 0, sizeof(*s));
        strcpy(s->desc, symbol_name);
        s->obj_type = obj_type;
        APR_RING_INIT(&(s->value_list), cbehave_value_t, link);
        APR_RING_INSERT_TAIL(&_symbol_list, s, cbehave_symbol_t, link);
    }

    add_value(s, value, count);
}

static cbehave_symbol_t* lookup_symbol(const char *symbol_name, int obj_type) {
    cbehave_symbol_t   *s  = NULL;

    APR_RING_FOREACH(s, &_symbol_list, cbehave_symbol_t, link) {
        if (s != NULL) {
            if ((s->obj_type == obj_type)
                && (!strcmp(s->desc, symbol_name))) {
                return s;
            }
        }
    }

    return NULL;
}

static void add_value(cbehave_symbol_t *s, void *value, int count) {
    cbehave_value_t    *v  = NULL;
    int i;

    /* 
     * make the obj always to return one same value
     * until another cbehave_mock_obj_return invoking
     */
    if (count == -1) {
        s->always_return_flag = 1;
        s->value = value;
        return;
    }

    s->always_return_flag = 0;

    for (i = 0; i < count; i++) {
        errno = 0;
        v = (cbehave_value_t*)malloc(sizeof(*v));
        if (!v) {
            printf("\t[CBEHAVE]: malloc error!, errcode[%d]\n", errno);
            exit(EXIT_FAILURE);
        }
        memset(v, 0, sizeof(*v));
        v->value    = value;

        APR_RING_INSERT_TAIL(&(s->value_list), v, cbehave_value_t, link);
    }
}

static cbehave_value_t* get_value(cbehave_symbol_t *s) {
    cbehave_value_t    *v  = NULL;

    v = APR_RING_FIRST(&(s->value_list));
    return v;
}

