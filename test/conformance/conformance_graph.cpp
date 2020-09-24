/*
    Copyright (c) 2020 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/


#include "common/test.h"

#include "common/utils.h"
#include "common/graph_utils.h"

#include "tbb/flow_graph.h"
#include "tbb/task_arena.h"
#include "tbb/global_control.h"

#include "conformance_flowgraph.h"

//! \file conformance_graph.cpp
//! \brief Test for [flow_graph.graph] specification

using namespace tbb::flow;
using namespace std;

//! const graph
//! \brief \ref error_guessing
TEST_CASE("const graph"){
    const graph g;
    CHECK_MESSAGE((g.cbegin() == g.cend()), "Starting graph is empty");

    graph g2;
    CHECK_MESSAGE((g2.begin() == g2.end()), "Starting graph is empty");
}

//! Graph reset
//! \brief \ref requirement
TEST_CASE("graph reset") {
    graph g;
    size_t concurrency_limit = 1;
    tbb::global_control control(tbb::global_control::max_allowed_parallelism, concurrency_limit);

    // Functional nodes
    // TODO: Check input_node, multifunction_node, async_node similarly

    // continue_node
    bool flag = false;
    continue_node<int> source(g, 2, [&](const continue_msg&){ flag = true; return 1;});

    source.try_put(continue_msg());
    g.wait_for_all();

    CHECK_MESSAGE( (flag == false), "Should still be false");

    g.reset(rf_reset_protocol);

    source.try_put(continue_msg());
    g.wait_for_all();
    CHECK_MESSAGE( (flag == false), "Should still be false");

    source.try_put(continue_msg());
    g.wait_for_all();
    CHECK_MESSAGE( (flag == true), "Should be true");

    // functional_node
    int flag_fun = 0;
    function_node<int, int, queueing> f(g, serial, [&](const int& v){ flag_fun++; return v;});

    f.try_put(0);
    f.try_put(0);

    CHECK_MESSAGE( (flag_fun == 0), "Should not be updated");
    g.reset(rf_reset_protocol);

    g.wait_for_all();
    CHECK_MESSAGE( (flag_fun == 1), "Should be updated");

    // Buffering nodes
    // TODO: Check overwrite_node write_once_node priority_queue_node sequencer_node similarly

    // buffer_node
    buffer_node<int> buff(g);

    int tmp = -1;
    CHECK_MESSAGE( (buff.try_get(tmp) == false), "try_get should not succeed");
    CHECK_MESSAGE( (tmp == -1), "Value should not be updated");

    buff.try_put(1);

    g.reset();

    tmp = -1;
    CHECK_MESSAGE( (buff.try_get(tmp) == false), "try_get should not succeed");
    CHECK_MESSAGE( (tmp == -1), "Value should not be updated");
    g.wait_for_all();

    // queue_node
    queue_node<int> q(g);

    tmp = -1;
    CHECK_MESSAGE( (q.try_get(tmp) == false), "try_get should not succeed");
    CHECK_MESSAGE( (tmp == -1), "Value should not be updated");

    q.try_put(1);

    g.reset();

    tmp = -1;
    CHECK_MESSAGE( (q.try_get(tmp) == false), "try_get should not succeed");
    CHECK_MESSAGE( (tmp == -1), "Value should not be updated");
    g.wait_for_all();

    // Check rf_clear_edges
    continue_node<int> src(g, [&](const continue_msg&){ return 1;});
    queue_node<int> dest(g);
    make_edge(src, dest);

    src.try_put(continue_msg());
    g.wait_for_all();

    tmp = -1;
    CHECK_MESSAGE( (dest.try_get(tmp)== true), "Message should pass when edge exists");
    CHECK_MESSAGE( (tmp == 1 ), "Message should pass when edge exists");
    CHECK_MESSAGE( (dest.try_get(tmp)== false), "Message should not pass after item is consumed");

    g.reset(rf_clear_edges);

    tmp = -1;
    src.try_put(continue_msg());
    g.wait_for_all();
    
    CHECK_MESSAGE( (dest.try_get(tmp)== false), "Message should not pass when edge doesn't exist");
    CHECK_MESSAGE( (tmp == -1), "Value should not be altered");

    // TODO: Add check that default invocaiton is the same as with rf_reset_protocol
    // TODO: See if specification for broadcast_node and other service nodes is sufficient for reset checks
}

